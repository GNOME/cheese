# Process this file with autoconf to produce a configure script.

AC_INIT(cheese, 2.28.1)

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

GNOME_MAINTAINER_MODE_DEFINES
GNOME_COMPILE_WARNINGS

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
GTK_REQUIRED=2.17.4
GDK_REQUIRED=2.14.0
LIBGNOMEDESKTOP_REQUIRED=2.26.0
GCONF_REQUIRED=2.16.0
GSTREAMER_REQUIRED=0.10.20
LIBEBOOK_REQUIRED=1.12.0
CAIRO_REQUIRED=1.4.0
DBUS_REQUIRED=1.0
DBUS_GLIB_REQUIRED=0.7
HAL_REQUIRED=0.5.9
PANGOCAIRO_REQUIRED=1.18.0
LIBRSVG_REQUIRED=2.18.0

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
  libebook-1.2 >= $LIBEBOOK_REQUIRED \
  cairo >= $CAIRO_REQUIRED \
  dbus-1 >= $DBUS_REQUIRED \
  dbus-glib-1 >= $DBUS_GLIB_REQUIRED \
  hal >= $HAL_REQUIRED \
  pangocairo >= $PANGOCAIRO_REQUIRED \
  librsvg-2.0 >= $LIBRSVG_REQUIRED)
AC_SUBST(CHEESE_CFLAGS)

CHEESE_LIBS="$CHEESE_LIBS -lgstinterfaces-0.10"

AC_SUBST(CHEESE_LIBS)

dnl Multimedia keys
have_xfree=no
AC_COMPILE_IFELSE([
	#include <X11/XF86keysym.h>
        int main(int argc,char **argv) {
          return 0;
        }
], have_xfree=yes)
AC_MSG_CHECKING(for XFree86 headers)
AC_MSG_RESULT($have_xfree)
if test x"$have_xfree" = "xyes" ; then
	AC_DEFINE(HAVE_XFREE, 1, [defined if you have X11/XF86keysym.h])
fi

AC_ARG_ENABLE(mmkeys, AC_HELP_STRING([--disable-mmkeys],
					[don't build with Multimedia Keys support]))
if test x"$have_xfree" = xyes; then
	if test x"$enable_mmkeys" != xno; then
		enable_mmkeys=yes
		AC_DEFINE(HAVE_MMKEYS, 1, [define if Multimedia Keys are enabled])
	fi
else
	if test x"$enable_mmkeys" = xyes; then
		AC_MSG_ERROR([Multimedia keys explicitly requested but no support found])
	fi
fi

AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2

#*******************************************************************************

AC_OUTPUT([
Makefile
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
data/sounds/Makefile
help/Makefile
src/Makefile
po/Makefile.in
])

#**************************************************************************
