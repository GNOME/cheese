# Process this file with autoconf to produce a configure script.

AC_INIT(cheese, 2.23.6)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
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

GLIB_REQUIRED=2.15.5
GIO_REQUIRED=2.15.5
GTK_REQUIRED=2.10.0
LIBGNOMEUI_REQUIRED=2.14.0
GCONF_REQUIRED=2.16.0
GSTREAMER_REQUIRED=0.10.16
CAIRO_REQUIRED=1.2.4
GNOME_VFS_REQUIRED=2.18.0
LIBEBOOK_REQUIRED=1.12.0
DBUS_REQUIRED=1.0
HAL_REQUIRED=0.5.9
DBUS_GLIB_REQUIRED=0.7
LIBRSVG_REQUIRED=2.18.0
PANGOCAIRO_REQUIRED=1.18.0

PKG_CHECK_MODULES(CHEESE, \ 
  glib-2.0 >= $GLIB_REQUIRED \
  gio-2.0 >= $GIO_REQUIRED \
  gtk+-2.0 >= $GTK_REQUIRED \
  libgnomeui-2.0 >= $LIBGNOMEUI_REQUIRED \
  gconf-2.0 >= $GCONF_REQUIRED \
  gstreamer-0.10 >= $GSTREAMER_REQUIRED \
  gstreamer-plugins-base-0.10 >= $GSTREAMER_REQUIRED \
  gnome-vfs-2.0 >= $GNOME_VFS_REQUIRED \
  libebook-1.2 >= $LIBEBOOK_REQUIRED \
  cairo >= $CAIRO_REQUIRED \
  dbus-1 >= $DBUS_REQUIRED \
  hal >= $HAL_REQUIRED \
  dbus-glib-1 >= $DBUS_GLIB_REQUIRED \
  pangocairo >= $PANGOCAIRO_REQUIRED \
  librsvg-2.0 >= $LIBRSVG_REQUIRED)
AC_SUBST(CHEESE_CFLAGS)

CHEESE_LIBS="$CHEESE_LIBS -lgstinterfaces-0.10"

AC_SUBST(CHEESE_LIBS)

AC_ARG_ENABLE(hildon, AS_HELP_STRING([--enable-hildon],[compile for Hildon environment (default=no)]),,enable_hildon=no)

if test "x$enable_hildon" = "xyes"; then
   PKG_CHECK_MODULES([HILDON], hildon-1 >= 1.0.5,enable_hildon=yes,enable_hildon=no)
else
   enable_hildon=no
fi

AM_CONDITIONAL(WITH_HILDON, test "x$enable_hildon" = "xyes")

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
