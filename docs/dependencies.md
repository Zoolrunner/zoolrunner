# Bundled dependencies

[Documentation index](README.md)

Dependencies are updated individually behind the existing Mozilla interfaces.
The bundled versions below describe this source tree, including private copies
that must not be assumed interchangeable with their main-tree counterparts.

| Dependency | Version | Source / integration notes |
| --- | --- | --- |
| NSS | 3.42, customized beta marker | [Version header](../security/nss/lib/nss/nss.h) |
| NSPR | 4.7.7 | [Version header](../nsprpub/pr/include/prinit.h) |
| SQLite, Mozilla storage | 3.53.4 | [Header](../db/sqlite3/src/sqlite3.h); updated from 3.3.5 |
| SQLite, NSS private copy | 3.7.15 | [Header](../security/nss/lib/sqlite/sqlite3.h) |
| Expat | 2.8.4 | [Gecko pause/replay adapter and regressions](../parser/expat/README.zoolrunner.md) |
| libpng | 1.6.58 | [Header](../modules/libimg/png/png.h) |
| zlib, main tree | 1.3.2 | [Header](../modules/zlib/src/zlib.h) |
| zlib, NSS private copy | 1.2.5 | [Header](../security/nss/lib/zlib/zlib.h) |
| bzip2/libbzip2 | 1.0.8 | [Version header](../modules/libbz2/src/bzlib_private.h); updated from 1.0.3 |
| libjpeg-turbo | 3.2.0 | [Bundled source](../jpeg); replaces IJG 6b while retaining its API compatibility level (`JPEG_LIB_VERSION 62`) |
| Cairo | 1.1.1 in the version header | [Header](../gfx/cairo/cairo/src/cairo-features.h.in); the historical [integration notes](../gfx/cairo/README) still identify 1.0.2 |
| libIDL | 0.8.14 | [Bundled source](../build/unix/libIDL); final upstream release, retained for XPIDL |

Updates must account for historical APIs, local library modifications, MSVC
2005, and minimum operating-system targets. NSPR, NSS, Cairo/pixman, and
SpiderMonkey's fdlibm require particular care because of their platform and
runtime assumptions. Vendored libIDL keeps builds possible where distributions
no longer supply it; it does not replace XPIDL.
