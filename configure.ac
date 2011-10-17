# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.64])
AC_INIT([cheese],
        [3.2.1],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=cheese],
        [cheese],
        [http://projects.gnome.org/cheese/])

AC_CONFIG_HEADERS([cheese-config.h])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 -Wall -Wno-portability])
AM_MAINTAINER_MODE([enable])

# Check for programs
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AM_PROG_VALAC([0.11.6])

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

CFLAGS="${CFLAGS} -Wall"
CFLAGS+=" -DGSEAL_ENABLE"

GNOME_MAINTAINER_MODE_DEFINES
GNOME_COMPILE_WARNINGS([maximum])

#*******************************************************************************
# Libraries
#*******************************************************************************
# Before making a release, the CHEESE_LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
LIBCHEESE_LT_VERSION=2:3:1
LIBCHEESE_GTK_LT_VERSION=20:5:0
AC_SUBST(LIBCHEESE_LT_VERSION)
AC_SUBST(LIBCHEESE_GTK_LT_VERSION)

#*******************************************************************************
# Expanded dirs
#*******************************************************************************
# This macro expands DIR and assigns it to RET.
# If DIR is NONE, then it's replaced by DEFAULT.
# Based on AC_DEFINE_DIR
AC_DEFUN([CHEESE_FULLPATH], [
  test "x$prefix" = xNONE && prefix="$ac_default_prefix"
  test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
  ac_expand=[$]$1
  test "x$ac_expand" = xNONE && ac_expand="[$]$2"
  ac_expand=`eval echo [$]ac_expand`
  $3=`eval echo [$]ac_expand`
])

# FULL_LIBEXECDIR is used for X-GNOME-Bugzilla-ExtraInfoScript expansion
# in data/cheese.desktop.in.in
CHEESE_FULLPATH(libexecdir, NONE, FULL_LIBEXECDIR)
AC_SUBST(FULL_LIBEXECDIR)

#*******************************************************************************
# Internationalization
#*******************************************************************************
GETTEXT_PACKAGE=cheese
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

IT_PROG_INTLTOOL([0.40.0])

#*******************************************************************************
# Check required libraries
#*******************************************************************************


GLIB_REQUIRED=2.28.0
GOBJECT_REQUIRED=2.28.0
GIO_REQUIRED=2.28.0
GTK_REQUIRED=2.99.4
GDK_REQUIRED=2.99.4
LIBGNOMEDESKTOP_REQUIRED=2.91.6
GSTREAMER_REQUIRED=0.10.32
CAIRO_REQUIRED=1.10.0
PANGOCAIRO_REQUIRED=1.28.0
LIBRSVG_REQUIRED=2.32.0
CLUTTER_REQUIRED=1.6.1
CLUTTERGTK_REQUIRED=0.91.8
CLUTTERGST_REQUIRED=1.0.0
GEE_REQUIRED=0.6.0
LIBCANBERRA_REQUIRED=0.26

#*******************************************************************************
# Check for udev
#*******************************************************************************

UDEV_PKG=
PKG_CHECK_MODULES(UDEV, gudev-1.0,
                  [AC_DEFINE(HAVE_UDEV, 1, [Define to 1 to enable udev support])
                  UDEV_PKG=gudev-1.0],
                  [UDEV_PKG=])

AC_MSG_CHECKING([operating system])
case $host in
  *-linux*)
    AC_DEFINE(OS_LINUX, [], [Linux backend])
    AC_SUBST(OS_LINUX)
    AC_MSG_RESULT([Linux])
    if test x$UDEV_PKG = "x" ; then
            AC_MSG_ERROR([libgudev is required under Linux and not installed])
    fi
    ;;
  *)
    AC_MSG_RESULT([unsupported operating system])
esac


AC_CHECK_HEADER([sys/videoio.h],
                [AC_DEFINE(USE_SYS_VIDEOIO_H, 1, define to 1 if sys/videoio.h is present)],
                [AC_DEFINE(USE_SYS_VIDEOIO_H, 0, define to 1 if sys/videoio.h is present)], [])

XTEST_LIBS=
AC_CHECK_HEADER(X11/extensions/XTest.h, [
  AC_CHECK_LIB(Xtst, XTestFakeKeyEvent, [
    AC_DEFINE(HAVE_XTEST, 1, [Defined if the XTEST X extension is present])
    XTEST_LIBS="-lX11 -lXtst"])
])
AC_SUBST(XTEST_LIBS)
AM_CONDITIONAL(HAVE_XTEST, test "x$HAVE_TEST" = "xyes")

PKG_CHECK_MODULES(CHEESE, \ 
  glib-2.0 >= $GLIB_REQUIRED \
  gio-2.0 >= $GIO_REQUIRED \
  x11 \
  gobject-2.0 >= $GOBJECT_REQUIRED \
  gdk-pixbuf-2.0 \
  gstreamer-0.10 >= $GSTREAMER_REQUIRED \
  gstreamer-plugins-base-0.10 >= $GSTREAMER_REQUIRED \
  cairo >= $CAIRO_REQUIRED \
  pangocairo >= $PANGOCAIRO_REQUIRED \
  clutter-1.0 >= $CLUTTER_REQUIRED \
  clutter-gst-1.0 >= $CLUTTERGST_REQUIRED \
  mx-1.0 \
  $UDEV_PKG
  )
AC_SUBST(CHEESE_CFLAGS)
AC_SUBST(CHEESE_LIBS)
CHEESE_LIBS="$CHEESE_LIBS -lgstinterfaces-0.10"

PKG_CHECK_MODULES(CHEESE_GTK, \ 
  gio-2.0 >= $GIO_REQUIRED \
  gtk+-3.0 >= $GTK_REQUIRED \
  gdk-3.0 >= $GDK_REQUIRED \
  gnome-desktop-3.0 >= $LIBGNOMEDESKTOP_REQUIRED \
  librsvg-2.0 >= $LIBRSVG_REQUIRED \
  clutter-gtk-1.0 >= $CLUTTERGTK_REQUIRED \
  gee-1.0 >= $GEE_REQUIRED \
  libcanberra-gtk3 >= $LIBCANBERRA_REQUIRED \
  $UDEV_PKG \
  gnome-video-effects
  )
AC_SUBST(CHEESE_GTK_CFLAGS)
AC_SUBST(CHEESE_GTK_LIBS)

GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)
AC_SUBST(GLIB_MKENUMS)

GOBJECT_INTROSPECTION_CHECK([0.6.7])

# API documentation
GTK_DOC_CHECK([1.14],[--flavour no-tmpl])

# User manual
GNOME_DOC_INIT([0.20])

GLIB_GSETTINGS
#*******************************************************************************

AC_CONFIG_FILES([
Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/version.xml
data/Makefile
data/cheese.pc
data/cheese-gtk.pc
data/cheese.desktop.in
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/22x22/Makefile
data/icons/24x24/Makefile
data/icons/32x32/Makefile
data/icons/48x48/Makefile
data/icons/256x256/Makefile
data/icons/scalable/Makefile
data/icons/16x16/actions/Makefile
data/icons/22x22/actions/Makefile
data/icons/24x24/actions/Makefile
data/icons/32x32/actions/Makefile
data/icons/48x48/actions/Makefile
data/icons/scalable/actions/Makefile
data/pixmaps/Makefile
help/Makefile
libcheese/Makefile
src/Makefile
tests/Makefile
po/Makefile.in
])
AC_OUTPUT

#**************************************************************************
