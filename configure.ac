# Process this file with autoconf to produce a configure script.

AC_INIT(cheese, 2.32.0)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])
AM_CONFIG_HEADER(cheese-config.h)
AM_MAINTAINER_MODE

dnl make sure we keep ACLOCAL_FLAGS around for maintainer builds to work
AC_SUBST(ACLOCAL_AMFLAGS, "\${ACLOCAL_FLAGS}")

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AM_PROG_LIBTOOL

CFLAGS="${CFLAGS} -Wall"
CFLAGS+=" -DGSEAL_ENABLE"

GNOME_MAINTAINER_MODE_DEFINES
GNOME_COMPILE_WARNINGS

GTK_DOC_CHECK(1.11)

#*******************************************************************************
# soname
#*******************************************************************************
# Before making a release, the CHEESE_LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
CHEESE_LT_VERSION=18:0:0
AC_SUBST(CHEESE_LT_VERSION)

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
AM_GLIB_GNU_GETTEXT

GNOME_DOC_INIT

#*******************************************************************************
# Check required libraries
#*******************************************************************************

GLIB_REQUIRED=2.16.0
GOBJECT_REQUIRED=2.12.0
GIO_REQUIRED=2.16.0
GTK_REQUIRED=2.19.1
GDK_REQUIRED=2.14.0
LIBGNOMEDESKTOP_REQUIRED=2.26.0
GCONF_REQUIRED=2.16.0
GSTREAMER_REQUIRED=0.10.23
CAIRO_REQUIRED=1.4.0
DBUS_REQUIRED=1.0
DBUS_GLIB_REQUIRED=0.7
PANGOCAIRO_REQUIRED=1.18.0
LIBRSVG_REQUIRED=2.18.0

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
      AC_DEFINE(HAVE_XTEST, 1, [Defined if the XTEST X extension is
present])
      XTEST_LIBS="-lX11 -lXtst"])
  ])
AC_SUBST(XTEST_LIBS)
AM_CONDITIONAL(HAVE_XTEST, test "x$HAVE_TEST" = "xyes")

PKG_CHECK_MODULES(CHEESE, \ 
  glib-2.0 >= $GLIB_REQUIRED \
  gobject-2.0 >= $GOBJECT_REQUIRED \
  gio-2.0 >= $GIO_REQUIRED \
  gtk+-2.0 >= $GTK_REQUIRED \
  gdk-2.0 >= $GDK_REQUIRED \
  gnome-desktop-2.0 >= $LIBGNOMEDESKTOP_REQUIRED \
  gconf-2.0 >= $GCONF_REQUIRED \
  gstreamer-0.10 >= $GSTREAMER_REQUIRED \
  gstreamer-plugins-base-0.10 >= $GSTREAMER_REQUIRED \
  cairo >= $CAIRO_REQUIRED \
  dbus-1 >= $DBUS_REQUIRED \
  dbus-glib-1 >= $DBUS_GLIB_REQUIRED \
  pangocairo >= $PANGOCAIRO_REQUIRED \
  librsvg-2.0 >= $LIBRSVG_REQUIRED
  libcanberra-gtk $UDEV_PKG)
AC_SUBST(CHEESE_CFLAGS)

CHEESE_LIBS="$CHEESE_LIBS -lgstinterfaces-0.10"

AC_SUBST(CHEESE_LIBS)

GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)
AC_SUBST(GLIB_MKENUMS)

AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2

#*******************************************************************************

AC_OUTPUT([
Makefile
cheese-gtk.pc
docs/Makefile
docs/reference/Makefile
docs/reference/version.xml
data/Makefile
data/cheese.desktop.in
data/effects/Makefile
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

#**************************************************************************
