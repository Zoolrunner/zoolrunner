# Configure paths for FreeType2
# Marcelo Magallon 2001-10-26, based on gtk.m4 by Owen Taylor

dnl AM_CHECK_FT2([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for FreeType2, and define FT2_CFLAGS and FT2_LIBS
dnl
AC_DEFUN(AC_CHECK_FT2,
[dnl
dnl Get the cflags and libraries from the freetype-config script
dnl
AC_ARG_WITH(freetype-prefix,
[  --with-ft-prefix=PFX    Prefix where FreeType is installed (optional)],
            ft_config_prefix="$withval", ft_config_prefix="")
AC_ARG_WITH(freetype-exec-prefix,
[  --with-ft-exec-prefix=PFX
                          Exec prefix where FreeType is installed (optional)],
            ft_config_exec_prefix="$withval", ft_config_exec_prefix="")
AC_ARG_ENABLE(freetypetest,
[  --disable-freetypetest
                          Do not try to compile and run a test FreeType program],
            [], enable_fttest=yes)

if test x$ft_config_exec_prefix != x ; then
  ft_config_args="$ft_config_args --exec-prefix=$ft_config_exec_prefix"
  if test x${FT2_CONFIG+set} != xset ; then
    FT2_CONFIG=$ft_config_exec_prefix/bin/freetype-config
  fi
fi
if test x$ft_config_prefix != x ; then
  ft_config_args="$ft_config_args --prefix=$ft_config_prefix"
  if test x${FT2_CONFIG+set} != xset ; then
    FT2_CONFIG=$ft_config_prefix/bin/freetype-config
  fi
fi
AC_PATH_PROG(FT2_CONFIG, freetype-config, no)
if test "$FT2_CONFIG" = "no" ; then
  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  if test "$PKG_CONFIG" != "no" && $PKG_CONFIG --exists freetype2 ; then
    FT2_CONFIG="$PKG_CONFIG"
    ft_config_args="freetype2"
    ft_config_version_args="--modversion freetype2"
  fi
fi

min_ft_version=ifelse([$1], ,6.1.0,$1)
AC_MSG_CHECKING(for FreeType - version >= $min_ft_version)
no_ft=""
if test "$FT2_CONFIG" = "no" ; then
  no_ft=yes
else
  FT2_CFLAGS=`$FT2_CONFIG $ft_config_args --cflags`
  FT2_LIBS=`$FT2_CONFIG $ft_config_args --libs`
  if test -z "$ft_config_version_args" ; then
    ft_config_version_args="$ft_config_args --version"
  fi
  ft_config_major_version=`$FT2_CONFIG $ft_config_version_args | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
  ft_config_minor_version=`$FT2_CONFIG $ft_config_version_args | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
  ft_config_micro_version=`$FT2_CONFIG $ft_config_version_args | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
  ft_min_major_version=`echo $min_ft_version | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
  ft_min_minor_version=`echo $min_ft_version | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
  ft_min_micro_version=`echo $min_ft_version | \
         sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
  if test "x$enable_fttest" = "xyes" ; then
    ft_config_is_lt=no
    if test $ft_config_major_version -lt $ft_min_major_version ; then
      ft_config_is_lt=yes
    else
      if test $ft_config_major_version -eq $ft_min_major_version ; then
        if test $ft_config_minor_version -lt $ft_min_minor_version ; then
          ft_config_is_lt=yes
        else
          if test $ft_config_minor_version -eq $ft_min_minor_version ; then
            if test $ft_config_micro_version -lt $ft_min_micro_version ; then
              ft_config_is_lt=yes
            fi
          fi
        fi
      fi
    fi
    if test "x$ft_config_is_lt" = "xyes" ; then
      ifelse([$3], , :, [$3])
      AC_MSG_RESULT(no)
    else
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $FT2_CFLAGS"
      LIBS="$FT2_LIBS $LIBS"
dnl
dnl Sanity checks for the results of freetype-config to some extent.
dnl Do not require running the probe: FreeType may be detected while
dnl configuring in environments where build-time execution is restricted.
dnl
      AC_TRY_LINK([
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_CACHE_IMAGE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
], [
  FT_Library library;
  FT_Error error;
  FTC_Image_Cache image_cache;
  FTC_Image_Desc image_desc;
  FTC_Font font;
  image_cache = 0;
  memset(&image_desc, 0, sizeof(image_desc));
  memset(&font, 0, sizeof(font));
  error = FT_Init_FreeType(&library);
  if (error)
    return 1;
  FT_Done_FreeType(library);
  return 0;
],, no_ft=yes)
      CFLAGS="$ac_save_CFLAGS"
      LIBS="$ac_save_LIBS"
    fi             # test $ft_config_version -lt $ft_min_version
  fi               # test "x$enable_fttest" = "xyes"
fi                 # test "$FT2_CONFIG" = "no"
if test "x$ft_config_is_lt" != "xyes" ; then
  if test "x$no_ft" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     AC_MSG_RESULT(no)
     if test "$FT2_CONFIG" = "no" ; then
       echo "*** Neither freetype-config nor pkg-config metadata for FreeType 2 could be found."
       echo "*** If FreeType 2 was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the FT2_CONFIG environment variable to the"
       echo "*** full path to freetype-config."
     else
       echo "*** The FreeType test program failed to run.  If your system uses"
       echo "*** shared libraries and they are installed outside the normal"
       echo "*** system library path, make sure the variable LD_LIBRARY_PATH"
       echo "*** (or whatever is appropiate for your system) is correctly set."
     fi
     FT2_CFLAGS=""
     FT2_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(FT2_CFLAGS)
  AC_SUBST(FT2_LIBS)
fi
])
