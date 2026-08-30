# ZoolRunner libjpeg-turbo Integration Notes

ZoolRunner replaced the previous IJG libjpeg 6b copy with libjpeg-turbo 3.2.0.
The bundled copy is built through ZoolRunner's existing Mozilla makefiles; the
upstream CMake build is not used by normal ZoolRunner builds.

The integration intentionally preserves the historical libjpeg 6b-facing API:

* `JPEG_LIB_VERSION` remains `62`.
* ZoolRunner uses libjpeg-turbo's traditional libjpeg API, not the TurboJPEG
  API.
* SIMD is not enabled by default in the Mozilla makefile integration. The
  generic C implementation is the portable baseline.

Local compatibility choices preserved or added during the update:

* Mozilla's historical NSPR linkage and callback mappings in `jmorecfg.h`.
* The historical Mozilla `boolean` typedef size.
* The OS/2 `RGB_*` guard in `jpeglib.h`.
* Windows `basetsd.h` include guards compatible with older SDKs.
* VC6-compatible `SNPRINTF` macro definitions in `jinclude.h` and `tjutil.h`.
* Checked-in `jconfig.h` and `jconfigint.h` so configuring ZoolRunner does not
  require libjpeg-turbo's upstream build system.
* `NO_GETENV` and `NO_PUTENV` in `Makefile.in` to avoid optional environment
  helper paths that are not needed by ZoolRunner's JPEG decoder integration.
