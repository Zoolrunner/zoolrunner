# Documentation

Start with the [project README](../readme.md). Choose a guide below for the task
at hand; detailed validation records are linked from the short testing guides.
Commands in these guides run from the repository root unless stated otherwise.

## Build

| Platform | Start here |
| --- | --- |
| All targets | [Application profiles](../mozconfigs/README.md) |
| Linux | [Build guide](../build/linux/README.md) |
| Modern macOS | [Build guide](../mozconfigs/macos/README.md) |
| Mac OS X i386 | [Cross-build guide](../mozconfigs/macos/i386/README.md) |
| Mac OS X PowerPC | [Cross-build guide](../mozconfigs/macos/powerpc/README.md) |
| Windows | [MSVC 2005 / Wine guide](../build/win32/msvc8-cross/README.md) |

## Test and debug

| Area | Guide |
| --- | --- |
| JavaScript | [ES5.1](../js/tests/es5/README.md) · [ES2015](../js/tests/es6/README.md) |
| Benchmark compatibility | [Speedometer 2.1](../layout/html/tests/speedometer21/README.md) |
| HTML/CSS rendering and known gaps | [Layout probes](../layout/html/tests/style/README-probes.md) |
| Calendar | [Compatibility tests](../calendar/test/README-compatibility.md) |
| Composer and application lifecycle | [Lifecycle tests](../editor/composer/tests/README.md) |
| Linux packages | [Runtime checks and results](../build/linux/NOTES.md) |
| macOS packages | [Runtime tests](../build/macosx/tests/README.md) |
| Windows packages | [Runtime tests](../build/win32/msvc8-cross/tests/README.md) |

## Platform records

- [Linux implementation and validation](../build/linux/NOTES.md)
- [Modern macOS workflows and validation](../mozconfigs/macos/NOTES.md)
- Architecture notes: [arm64](../mozconfigs/macos/arm64/NOTES.md),
  [x86_64](../mozconfigs/macos/x86_64/NOTES.md),
  [i386](../mozconfigs/macos/i386/NOTES.md),
  [PowerPC](../mozconfigs/macos/powerpc/NOTES.md)
- PowerPC: [10.0 deployment results](../mozconfigs/macos/powerpc/10.0-status.md) ·
  [Interactive guest setup](../mozconfigs/macos/powerpc/utm.md)
- Windows: [Compatibility](../build/win32/msvc8-cross/COMPATIBILITY.md) ·
  [Implementation and workflow history](../build/win32/msvc8-cross/NOTES.md)

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

Build guides contain build instructions. Keep runtime procedures, result tables
and implementation history in the linked test guides and platform records.
