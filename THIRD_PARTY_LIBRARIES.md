# Third-Party Libraries

This file records third-party code found in the ZoolRunner source tree and the
version currently visible in checked-in source files.

Versions are taken from bundled headers, source files, README files, or release
notes where available. If checked-in metadata disagrees with compiled source,
the source/header version is treated as authoritative and the discrepancy is
noted.

| Library or code | Version found | Location and notes |
| --- | --- | --- |
| zlib | `1.3.2` | `modules/zlib/src/zlib.h` |
| zlib inside NSS | `1.2.5` | `security/nss/lib/zlib/zlib.h` |
| old NSS jar zlib header/API copy | `1.0.4` | `security/nss/lib/jar/jzlib.h` |
| libpng | `1.6.58` | `modules/libimg/png/png.h` |
| JPEG / libjpeg API | libjpeg-turbo `3.2.0`, configured with `JPEG_LIB_VERSION 62` for libjpeg 6b API compatibility | `jpeg/jconfig.h`, `jpeg/jconfigint.h`, `jpeg/jversion.h`; replaced IJG JPEG `6b` |
| SQLite, Mozilla storage copy | `3.53.4` | `db/sqlite3/src/sqlite3.h`; updated from the previous `3.3.5` copy, whose `README.MOZILLA` metadata incorrectly said `3.3.4` |
| SQLite, NSS copy | `3.7.15` | `security/nss/lib/sqlite/sqlite3.h`; `security/nss/lib/sqlite/README` says `3.10.2`, but `sqlite3.h` and `sqlite3.c` define `3.7.15` |
| bzip2 / libbzip2 | `1.0.8`, `13-Jul-2019` | `modules/libbz2/src/bzlib_private.h` |
| Expat | `1.95.7` | `parser/expat/lib/expat.h` |
| cairo | `1.0.2` | `gfx/cairo/cairo/src/cairo-features.h.in` |
| libpixman | snapshot `0.1.4`, `2005-03-07` | `gfx/cairo/libpixman/NEWS` |
| NSS | `3.42` customized beta | `security/nss/lib/nss/nss.h` |
| NSPR | `4.7.7` | `nsprpub/pr/include/prinit.h` |
| libIDL | `0.8.14` | `build/unix/libIDL/configure.in`, `build/unix/libIDL/include/libIDL/IDL.h` |
| Boehm-Demers-Weiser GC | `4.14` | `gc/boehm/version.h`, `gc/boehm/README` |
| Doug Lea malloc / dlmalloc | `2.7.0pre7` | `xpcom/build/malloc.c` |
| GUSI | `2.1.5` | `xpinstall/wizard/libxpnet/GUSI/README`, `xpinstall/wizard/libxpnet/GUSI/get_GUSI_source.txt` |
| Apple MoreFiles | `1.4.9` | `xpcom/MoreFiles/ReadMe.txt` release notes |
| Berkeley DB-derived DBM/hash code | `db.h 8.7`, `6/16/94` | `dbm/include/mcom_db.h`; a similar copy exists under `security/nss/lib/dbm` |
| XFree86 makedepend | no package release found; CVS ids from 2001-2003 | `config/mkdepend`, `security/coreconf/mkdepend`, `security/nss/coreconf/mkdepend` |
| MySpell | no clean version marker found | `extensions/spellcheck/myspell/src`; `README.mozilla` describes it as an OpenOffice MySpell copy |
| fdlibm | no clean package version found; file ids `fdlibm.h 1.5` and `s_lib_version.c 1.3`, `95/01/18` | `js/src/fdlibm` |
| Google Test | `release-1.8.1` | `security/nss/gtests/google_test/VERSION`; used by NSS tests |
| HACL* generated/automation support | pinned commit `1da331f9ef30e13269e45ae73bbe4a4bca679ae6` | `security/nss/automation/taskcluster/docker-hacl/Dockerfile`; generated/freebl verified code is present in the NSS tree |

System-only dependencies are not counted as bundled third-party libraries here.
The tree has wrappers or build hooks for libraries such as GTK, GLib, FreeType,
MySQL, and PostgreSQL, but their library source is not bundled in this
repository.
