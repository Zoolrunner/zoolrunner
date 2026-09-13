# Contributing to ZoolRunner
## Git Branches
* `master` - Current stable release
* `beta` - Most of the time a mirror of `master`, this branch is used in the days leading up to a release.
* `nightly` - The working branch of ZoolRunner, this should be used as the target for pull requests
* `oldmaster` - The original master branch, obsolete and provided for historical reasons only. Does not build.

## Contributing:
Pull requests must be submitted to the nightly branch, and all messages/titles for PRs/commits must be descriptive. All code should be well commented and easy to read, and retain the style and indentation of the source document. Please try to keep commits self-contained as much as possible. Pull requests can be submitted to fix existing bugs, or to add new features.

***Please check back to this document from time to time for updated standards***

## Documentation and Windows compatibility

Update the relevant README, build/test notes, and `AGENTS.md` alongside changes
to features, requirements, or development workflows. Distinguish implemented
features and minimum targets from actually verified runtime support.

Windows 95 and Windows NT 4.0 are the minimum Windows targets. Windows builds
are made from Linux or macOS hosts using genuine Microsoft Visual C++ 2005
(MSVC 8.0 / VC8) through Wine; CrossOver is supported on macOS. Follow the
[Windows build guide](build/win32/msvc8-cross/README.md) and preserve its static
CRT and legacy packaging policies. Track target-runtime verification in
[COMPATIBILITY.md](build/win32/msvc8-cross/COMPATIBILITY.md).
