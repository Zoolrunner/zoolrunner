#!/usr/bin/perl

# Run one MSVC 8 target tool under Wine/CrossOver, translating only arguments
# which are known to contain paths.  Compiler switches beginning with '/' are
# deliberately not subjected to blanket path conversion.

use strict;
use warnings;
use Cwd qw(abs_path getcwd);
use File::Basename qw(dirname);
use File::Spec;
use File::Temp qw(tempfile);
use Text::ParseWords qw(shellwords);

sub fail {
    print STDERR "msvc8-cross: @_.\n";
    exit 1;
}

my $tool = shift @ARGV;
fail('missing tool name') unless defined $tool;
fail("unsupported tool '$tool'") unless $tool =~ /\A(?:cl|link|lib|rc|mt|ml|midl)\z/;

my $root = $ENV{MSVC8_ROOT};
fail('MSVC8_ROOT is not set') unless defined $root && length $root;
$root = abs_path($root);
fail('MSVC8_ROOT does not name a directory') unless defined $root && -d $root;

my $script_dir = dirname(abs_path($0));
my $wine_run = File::Spec->catfile($script_dir, 'wine-run.sh');
my $program = File::Spec->catfile($root, 'bin', "$tool.exe");
if ($tool eq 'midl' && !-f $program) {
    my $sdk_midl = File::Spec->catfile($root, 'PlatformSDK', 'Bin', 'midl.exe');
    $program = $sdk_midl if -f $sdk_midl;
}
fail("cannot find $tool.exe below '$root'") unless -f $program;

sub wine_paths {
    my (@paths) = @_;
    return () unless @paths;
    open(my $pipe, '-|', $wine_run, '--winepath', @paths)
        or fail("cannot execute $wine_run");
    my @converted = <$pipe>;
    close($pipe) or fail('winepath failed');
    # Do not rely on Perl's current input record separator here. Response-file
    # conversion temporarily slurps a file with an undefined separator.
    s/[\r\n]+\z// for @converted;
    fail('winepath returned an unexpected number of paths')
        unless @converted == @paths;
    return @converted;
}

sub looks_like_path {
    my ($value) = @_;
    # A leading slash is ambiguous: it is also MSVC's normal option prefix.
    # Treat it as a native absolute path only when that input already exists;
    # output paths are handled by their /Fo, /OUT:, and related option names.
    return -e $value ? 1 : 0 if $value =~ m{\A/};
    return 1 if $value =~ m{\A\.\.?[\\/]};
    # Unknown dash-prefixed arguments are compiler/resource options, even when
    # their values contain slashes (for example -DICON="path/to/icon.ico").
    # Only the explicitly recognized path-bearing options are converted.
    return 0 if $value =~ m{\A-};
    return 1 if $value =~ m{[\\/]};
    return 1 if $value =~ /\.(?:c|cc|cpp|cxx|asm|s|obj|res|rc|lib|dll|exe|def|manifest|pdb|pch|tlb)\z/i;
    return 0;
}

sub native_absolute {
    my ($path) = @_;
    return undef unless $path =~ m{\A/};
    return $path;
}

sub validate_runtime_argument {
    my ($arg) = @_;
    return unless $tool eq 'cl' && $ENV{MSVC8_REQUIRE_STATIC_RTL};
    if ($arg =~ /\A[\/-]MDd?\z/i) {
        fail("dynamic MSVC runtime option '$arg' is forbidden by MSVC8_REQUIRE_STATIC_RTL");
    }
}

# First collect absolute native paths.  Converting them in one winepath process
# avoids adding one Wine startup for every include or object argument.
my @path_refs;
my @args;
my $compile_only = grep { /\A[\/-]c\z/i } @ARGV;
my ($requested_output, $produced_output);
while (@ARGV) {
    my $arg = shift @ARGV;
    validate_runtime_argument($arg);
    if ($arg eq '-o') {
        fail("$tool: -o requires an output path") unless @ARGV;
        my $output = shift @ARGV;
        if ($tool eq 'cl') {
            push @args, ($compile_only ? '/Fo' : '/Fe') . $output;
            if (!$compile_only && $output !~ /\.exe\z/i) {
                $requested_output = $output;
                $produced_output = "$output.exe";
            }
        } elsif ($tool eq 'link' || $tool eq 'lib') {
            push @args, '/OUT:' . $output;
            if ($tool eq 'link' && $output !~ /\.exe\z/i) {
                $requested_output = $output;
                $produced_output = "$output.exe";
            }
        } elsif ($tool eq 'ml' || $tool eq 'rc') {
            push @args, '/Fo' . $output;
        } else {
            push @args, $arg, $output;
        }
        next;
    }
    push @args, $arg;
}
for my $index (0 .. $#args) {
    my $arg = $args[$index];
    my $path;
    if ($arg =~ /\A\@(.+)\z/s) {
        $path = $1;
    } elsif ($arg =~ /\A[\/-](?:I|Fo|Fe|Fd|Fp|Fa|FI|FR)(.+)\z/is) {
        $path = $1;
    } elsif ($arg =~ /\A[\/-](?:LIBPATH|OUT|PDB|DEF|MANIFESTFILE|IMPLIB|TLB|DLLDATA):(.+)\z/is) {
        $path = $1;
    } elsif (looks_like_path($arg)) {
        $path = $arg;
    }
    my $absolute = defined $path ? native_absolute($path) : undef;
    push @path_refs, [$index, $path, $absolute] if defined $absolute;
}

my @windows = wine_paths(map { $_->[2] } @path_refs);
my %converted;
for my $i (0 .. $#path_refs) {
    $converted{$path_refs[$i]->[1]} = $windows[$i];
}

sub convert_path {
    my ($path) = @_;
    return $converted{$path} if exists $converted{$path};
    if ($path =~ m{\A/}) {
        my ($windows_path) = wine_paths($path);
        return $windows_path;
    }
    $path =~ s{/}{\\}g;
    return $path;
}

sub quote_response_arg {
    my ($arg) = @_;
    $arg =~ s/(\\*)"/$1$1\\"/g;
    $arg =~ s/(\\+)\z/$1$1/;
    return qq{"$arg"};
}

my @temporary_files;
sub convert_response_file {
    my ($path) = @_;
    my $native = $path;
    if ($native =~ /\A[A-Za-z]:[\\\/]/) {
        return convert_path($path);
    }
    open(my $input, '<', $native) or fail("cannot read response file '$native'");
    local $/;
    my $contents = <$input>;
    close($input);
    my @response_args = shellwords($contents);
    validate_runtime_argument($_) for @response_args;
    my @rewritten = map { convert_argument($_) } @response_args;
    my ($output, $output_name) = tempfile('msvc8-response-XXXXXX', SUFFIX => '.rsp', TMPDIR => 1, UNLINK => 0);
    print $output join("\n", map { quote_response_arg($_) } @rewritten), "\n";
    close($output) or fail("cannot write temporary response file '$output_name'");
    push @temporary_files, $output_name;
    my ($output_win) = wine_paths($output_name);
    return $output_win;
}

sub convert_argument {
    my ($arg) = @_;
    if ($arg =~ /\A\@(.+)\z/s) {
        return '@' . convert_response_file($1);
    }
    if ($arg =~ /\A([\/-](?:I|Fo|Fe|Fd|Fp|Fa|FI|FR))(.+)\z/is) {
        return $1 . convert_path($2);
    }
    if ($arg =~ /\A([\/-](?:LIBPATH|OUT|PDB|DEF|MANIFESTFILE|IMPLIB|TLB|DLLDATA):)(.+)\z/is) {
        return $1 . convert_path($2);
    }
    return convert_path($arg) if looks_like_path($arg);
    return $arg;
}

my @tool_args = map { convert_argument($_) } @args;
my $link_output;
if ($tool eq 'link') {
    for my $arg (@args) {
        if ($arg =~ /\A[\/-]OUT:(.+)\z/is) {
            $link_output = $1;
        }
    }
}

my @environment_paths = (
    File::Spec->catdir($root, 'include'),
    File::Spec->catdir($root, 'atlmfc', 'include'),
    File::Spec->catdir($root, 'PlatformSDK', 'Include'),
    File::Spec->catdir($root, 'lib'),
    File::Spec->catdir($root, 'atlmfc', 'lib'),
    File::Spec->catdir($root, 'PlatformSDK', 'Lib'),
    File::Spec->catdir($root, 'bin'),
    File::Spec->catdir($root, 'PlatformSDK', 'Bin'),
);
for my $directory (@environment_paths) {
    fail("required toolchain directory is missing: '$directory'") unless -d $directory;
}
my @environment_windows = wine_paths(@environment_paths);
$ENV{INCLUDE} = join(';', @environment_windows[0 .. 2]);
$ENV{LIB} = join(';', @environment_windows[3 .. 5]);
my $tool_path = join(';', @environment_windows[6 .. 7]);
$ENV{WINEPATH} = defined $ENV{WINEPATH} && length $ENV{WINEPATH}
    ? "$tool_path;$ENV{WINEPATH}" : $tool_path;

# LINK is both a common make variable and an MSVC linker environment
# variable containing implicit command-line arguments.  Never let a parent
# make's tool name become an input file such as link.obj.
delete $ENV{LINK} if $tool eq 'link';

if ($tool eq 'link' && $ENV{MSVC8_USE_PROCESS_HEAP}) {
    my $heap_source = File::Spec->catfile($script_dir, 'process-heap.c');
    my @source_stat = stat($heap_source);
    fail("cannot stat process heap source '$heap_source'") unless @source_stat;
    my $cache_dir = $ENV{TMPDIR} || '/tmp';
    my $heap_object = File::Spec->catfile($cache_dir,
        "zoolrunner-msvc8-process-heap-$<-$source_stat[7]-$source_stat[9].obj");
    my $lock_dir = "$heap_object.lock";

    if (!-f $heap_object) {
        my $have_lock;
        for (1 .. 600) {
            if (mkdir($lock_dir, 0700)) {
                $have_lock = 1;
                last;
            }
            last if -f $heap_object;
            select(undef, undef, undef, 0.1);
        }
        if ($have_lock) {
            if (!-f $heap_object) {
                my ($object_file, $object_name) = tempfile(
                    'msvc8-process-heap-XXXXXX', SUFFIX => '.obj',
                    DIR => $cache_dir, UNLINK => 0);
                close($object_file);
                unlink($object_name);
                my ($source_win, $object_win) =
                    wine_paths($heap_source, $object_name);
                my $compiler = File::Spec->catfile($root, 'bin', 'cl.exe');
                my $compile_status = system($wine_run, '--', $compiler,
                    '/nologo', '/c', '/TC', '/O1', '/GS-', '/MT',
                    "/Fo$object_win", $source_win);
                if ($compile_status != 0 || !-f $object_name) {
                    unlink($object_name);
                    rmdir($lock_dir);
                    fail('could not build the shared process-heap object');
                }
                rename($object_name, $heap_object) or do {
                    unlink($object_name);
                    rmdir($lock_dir);
                    fail("cannot install process heap object '$heap_object': $!");
                };
            }
            rmdir($lock_dir);
        }
        fail('timed out waiting for the shared process-heap object')
            unless -f $heap_object;
    }
    my ($heap_object_win) = wine_paths($heap_object);
    unshift @tool_args, $heap_object_win;
}

if ($ENV{MSVC8_CROSS_VERBOSE}) {
    print STDERR "msvc8-cross: $program\n";
    print STDERR "  [$_]\n" for @tool_args;
}

my $status = system($wine_run, '--', $program, @tool_args);
unlink @temporary_files if @temporary_files;
if ($status == -1) {
    fail("could not execute $tool.exe: $!");
}
my $embed_manifest = exists $ENV{MSVC8_EMBED_MANIFEST}
    ? $ENV{MSVC8_EMBED_MANIFEST} : 1;
if ($status == 0 && $tool eq 'link' && defined $link_output &&
    $embed_manifest ne '0') {
    my $manifest = "$link_output.manifest";
    if (-f $manifest && $link_output !~ /\A[A-Za-z]:[\\\/]/) {
        my $output_dir = dirname(File::Spec->rel2abs($link_output));
        my ($rc_file, $rc_name) = tempfile('msvc8-manifest-XXXXXX',
            SUFFIX => '.rc', DIR => $output_dir, UNLINK => 0);
        close($rc_file);
        my ($res_file, $res_name) = tempfile('msvc8-manifest-XXXXXX',
            SUFFIX => '.res', DIR => $output_dir, UNLINK => 0);
        close($res_file);
        unlink($res_name);
        my ($manifest_win, $rc_win, $res_win) =
            wine_paths($manifest, $rc_name, $res_name);
        (my $manifest_rc_path = $manifest_win) =~ s{\\}{/}g;
        my $resource_id = grep { /\A[\/-]DLL\z/i } @args ? 2 : 1;
        open($rc_file, '>', $rc_name)
            or fail("cannot write manifest resource script '$rc_name'");
        print $rc_file "$resource_id 24 \"$manifest_rc_path\"\n";
        close($rc_file)
            or fail("cannot close manifest resource script '$rc_name'");

        my $rc_program = File::Spec->catfile($root, 'bin', 'rc.exe');
        my $rc_status = system($wine_run, '--', $rc_program,
            "/fo$res_win", $rc_win);
        if ($rc_status == 0) {
            $status = system($wine_run, '--', $program, @tool_args,
                '/MANIFEST:NO', $res_win);
        } else {
            $status = $rc_status;
        }
        if ($status == 0) {
            unlink($manifest, $rc_name, $res_name);
        } else {
            print STDERR "msvc8-cross: manifest fallback files retained: $rc_name $res_name\n";
        }
    }
}
if ($status == 0 && defined $requested_output &&
    !-e $requested_output && -e $produced_output) {
    rename($produced_output, $requested_output)
        or fail("cannot rename '$produced_output' to '$requested_output': $!");
}
exit($status >> 8);
