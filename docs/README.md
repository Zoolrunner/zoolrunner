# Documentation

Start with the [project README](../readme.md). Choose a guide below for the task
at hand; detailed validation records are linked from the short testing guides.
Commands in these guides run from the repository root unless stated otherwise.

## Build and run

| Platform | Start here |
| --- | --- |
| All targets | [Application profiles](../mozconfigs/README.md) |
| Linux | [Build, package and runtime guide](../build/linux/README.md) |
| Modern macOS | [Build guide](../mozconfigs/macos/README.md) |
| Mac OS X i386 | [Cross-build guide](../mozconfigs/macos/i386/README.md) |
| Mac OS X PowerPC | [10.0 status](../mozconfigs/macos/powerpc/10.0-status.md) · [Interactive UTM setup](../mozconfigs/macos/powerpc/utm.md) |
| Windows | [MSVC 2005 / Wine guide](../build/win32/msvc8-cross/README.md) · [Compatibility record](../build/win32/msvc8-cross/COMPATIBILITY.md) |

## Test and debug

| Area | Guide |
| --- | --- |
| JavaScript | [ES5.1](../js/tests/es5/README.md) · [ES2015](../js/tests/es6/README.md) |
| Benchmark compatibility | [Speedometer 2.1](../layout/html/tests/speedometer21/README.md) |
| HTML/CSS rendering and known gaps | [Layout probes](../layout/html/tests/style/README-probes.md) |
| Calendar | [Compatibility tests](../calendar/test/README-compatibility.md) |
| Composer and application lifecycle | [Lifecycle tests](../editor/composer/tests/README.md) |
| macOS packages | [Runtime tests](../build/macosx/tests/README.md) |
| Windows packages | [Runtime tests](../build/win32/msvc8-cross/tests/README.md) |

## Reference

- [Bundled dependency versions](dependencies.md)
- [Development requirements](../AGENTS.md)
- [Previous project overview and milestone record](project-record.md)
- Detailed test records: [ES5.1](../js/tests/es5/DETAILS.md),
  [ES2015](../js/tests/es6/DETAILS.md),
  [Speedometer](../layout/html/tests/speedometer21/DETAILS.md)

## Keeping docs readable

Keep entry pages focused on purpose, essential commands, current limits and
links. Put feature-specific details and dated results in the relevant reference
record. Update an existing status summary instead of appending another one.
Preserve revision, platform, test scope and failures in the detailed record;
a successful historical run does not validate a later checkout.
