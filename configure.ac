# Process this file with autoconf to produce a configure script.

AC_INIT(cheese, 0.3.0)

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

dnl make sure we keep ACLOCAL_FLAGS around for maintainer builds to work
AC_SUBST(ACLOCAL_AMFLAGS, "\${ACLOCAL_FLAGS}")

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AM_PROG_LIBTOOL

CFLAGS="${CFLAGS} -Wall -Werror"


#*******************************************************************************
# Internationalization
#*******************************************************************************
GETTEXT_PACKAGE=cheese
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

IT_PROG_INTLTOOL([0.35.0])
AM_GLIB_GNU_GETTEXT


#*******************************************************************************
# Check required libraries
#*******************************************************************************

GLIB_REQUIRED=2.10.0
GTK_REQUIRED=2.10.0
LIBGLADE_REQUIRED=2.6.0
LIBGNOMEUI_REQUIRED=2.14.0
GCONF_REQUIRED=2.16.0
GSTREAMER_REQUIRED=0.10.15
CAIRO_REQUIRED=1.2.4
GNOME_VFS_REQUIRED=2.18.0
LIBEBOOK_REQUIRED=1.12.0
DBUS_REQUIRED=1.0
HAL_REQUIRED=0.5.9
DBUS_GLIB_REQUIRED=0.7

PKG_CHECK_MODULES(CHEESE, \ 
  glib-2.0 >= $GLIB_REQUIRED \
  gtk+-2.0 >= $GTK_REQUIRED \
  libgnomeui-2.0 >= $LIBGNOMEUI_REQUIRED \
  libglade-2.0 >= $LIBGLADE_REQUIRED \
  gconf-2.0 >= $GCONF_REQUIRED \
  gstreamer-0.10 >= $GSTREAMER_REQUIRED \
  gstreamer-plugins-base-0.10 >= $GSTREAMER_REQUIRED \
  gnome-vfs-2.0 >= $GNOME_VFS_REQUIRED \
  libebook-1.2 >= $LIBEBOOK_REQUIRED \
  cairo >= $CAIRO_REQUIRED \
  dbus-1 >= $DBUS_REQUIRED \
  hal >= $HAL_REQUIRED \
  dbus-glib-1 >= $DBUS_GLIB_REQUIRED \
  xxf86vm)

AC_SUBST(CHEESE_CFLAGS)

CHEESE_LIBS="$CHEESE_LIBS -lgstinterfaces-0.10"

AC_SUBST(CHEESE_LIBS)

AC_PATH_PROG(GCONFTOOL, gconftool-2)
AM_GCONF_SOURCE_2

#*******************************************************************************

AC_OUTPUT([
Makefile
data/Makefile
data/effects/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/22x22/Makefile
data/icons/24x24/Makefile
data/icons/32x32/Makefile
data/icons/48x48/Makefile
data/icons/scalable/Makefile
src/Makefile
po/Makefile.in
])

#**************************************************************************
