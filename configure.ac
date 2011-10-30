# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.64])
AC_INIT([cheese],
        [3.3.1],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=cheese],
        [cheese],
        [http://projects.gnome.org/cheese/])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([cheese-config.h])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 -Wall -Wno-portability dist-xz no-define no-dist-gzip nostdinc subdir-objects])
AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AM_PROG_VALAC([0.12.0])

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

PKG_PROG_PKG_CONFIG([0.24])

AC_SUBST([CHEESE_WFLAGS], ["-Wall -DGSEAL_ENABLE"])

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
AC_SUBST([LIBCHEESE_LT_VERSION], [2:4:1])
AC_SUBST([LIBCHEESE_GTK_LT_VERSION], [20:6:0])

#*******************************************************************************
# Internationalization
#*******************************************************************************
AC_SUBST([GETTEXT_PACKAGE], [$PACKAGE_TARNAME])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [GETTEXT package name])

IT_PROG_INTLTOOL([0.40.0])

#*******************************************************************************
# Check required libraries
#*******************************************************************************


GLIB_REQUIRED="glib-2.0 >= 2.28.0"
GOBJECT_REQUIRED="gobject-2.0 >= 2.28.0"
GIO_REQUIRED="gio-2.0 >= 2.28.0"
GTK_REQUIRED="gtk+-3.0 >= 2.99.4"
GDK_REQUIRED="gdk-3.0 >= 2.99.4"
GDK_PIXBUF_REQUIRED="gdk-pixbuf-2.0"
LIBGNOMEDESKTOP_REQUIRED="gnome-desktop-3.0 >= 2.91.6"
GSTREAMER_VERSION=0.10.32
GSTREAMER_REQUIRED="gstreamer-plugins-base-0.10 >= $GSTREAMER_VERSION gstreamer-0.10 >= $GSTREAMER_VERSION"
CAIRO_REQUIRED="cairo >= 1.10.0"
PANGOCAIRO_REQUIRED="pangocairo >= 1.28.0"
LIBRSVG_REQUIRED="librsvg-2.0 >= 2.32.0"
CLUTTER_REQUIRED="clutter-1.0 >= 1.6.1"
CLUTTERGTK_REQUIRED="clutter-gtk-1.0 >= 0.91.8"
CLUTTERGST_REQUIRED="clutter-gst-1.0 >= 1.0.0"
GEE_REQUIRED="gee-1.0 >= 0.6.0"
LIBCANBERRA_REQUIRED="libcanberra-gtk3 >= 0.26"
MX_REQUIRED="mx-1.0"
X11_REQUIRED="x11"
GNOME_VIDEO_EFFECTS_REQUIRED="gnome-video-effects"

# Libtool requires this, but it is only expanded once anyway.
AC_CANONICAL_HOST

AC_MSG_CHECKING([host operating system])
AS_CASE([$host_os],
  [*linux-*], [AC_MSG_RESULT([Linux])
    UDEV_REQUIRED="gudev-1.0"],
  [AC_MSG_RESULT([unsupported operating system])])

AC_MSG_CHECKING([for gudev-1.0])
AS_IF([test -n $UDEV_REQUIRED],
  [PKG_CHECK_EXISTS([$UDEV_REQUIRED],
    [AC_MSG_RESULT([found])
     AC_DEFINE([HAVE_UDEV], [], [Enable gudev support])],
    [AC_MSG_ERROR([$UDEV_REQUIRED not found])])],
  [AC_MSG_RESULT([not needed])])

AC_CHECK_HEADERS([sys/videoio.h])

AC_CHECK_HEADER([X11/extensions/XTest.h],
  [AC_CHECK_LIB([Xtst], [XTestFakeKeyEvent],
    [AC_DEFINE([HAVE_XTEST], [1], [Defined if the XTEST X extension is present])
     AC_SUBST([XTEST_LIBS], ["-lX11 -lXtst"])])
  ])
AM_CONDITIONAL([HAVE_XTEST], test "x$HAVE_TEST" = "xyes")

PKG_CHECK_MODULES([CHEESE],
  [$GLIB_REQUIRED
  $GIO_REQUIRED
  $X11_REQUIRED
  $GOBJECT_REQUIRED
  $GDK_PIXBUF_REQUIRED
  $GSTREAMER_REQUIRED
  $CAIRO_REQUIRED
  $PANGOCAIRO_REQUIRED
  $CLUTTER_REQUIRED
  $CLUTTERGST_REQUIRED
  $MX_REQUIRED
  $UDEV_REQUIRED])
CHEESE_LIBS="$CHEESE_LIBS -lgstinterfaces-0.10"

PKG_CHECK_MODULES([CHEESE_GTK],
  [$GIO_REQUIRED
   $GTK_REQUIRED
   $GDK_REQUIRED
   $LIBGNOMEDESKTOP_REQUIRED
   $LIBRSVG_REQUIRED
   $CLUTTERGTK_REQUIRED
   $GEE_REQUIRED
   $LIBCANBERRA_REQUIRED
   $UDEV_REQUIRED
   $GNOME_VIDEO_EFFECTS_REQUIRED])

# Recommend some runtime GStreamer plugins.
AC_PATH_PROGS([GST_INSPECT], [gst-inspect-0.10 gst-inspect], [notfound])
AS_IF([test "x$GST_INSPECT" = "xnotfound"],
  [AC_MSG_WARN([unable to check for runtime GStreamer plugin dependencies])],
  [GST_RUNTIME_PLUGINS="camerabin theoraenc"
   for gst_runtime_plugin in $GST_RUNTIME_PLUGINS
   do
     AC_MSG_CHECKING([for $gst_runtime_plugin GStreamer plugin])
     AS_IF([$GST_INSPECT $gst_runtime_plugin >/dev/null],
       [AC_MSG_RESULT([found])],
       [AC_MSG_RESULT([not found])
        AC_MSG_WARN([$gst_runtime_plugin was not found. It needs to be installed before Cheese is run])])
   done])

AC_MSG_CHECKING([for glib-genmarshal script])
GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
AS_IF([test "x$GLIB_GENMARSHAL" = "x"],
  [AC_MSG_ERROR([glib-genmarshal not listed in glib-2.0 pkg-config file])],
  [AC_SUBST([GLIB_GENMARSHAL])
    AC_MSG_RESULT([$GLIB_GENMARSHAL])])

AC_MSG_CHECKING([for glib-mkenums script])
GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
AS_IF([test "x$GLIB_MKENUMS" = "x"],
  [AC_MSG_ERROR([glib-mkenums not listed in glib-2.0 pkg-config file])],
  [AC_SUBST([GLIB_MKENUMS])
   AC_MSG_RESULT([$GLIB_MKENUMS])])


GOBJECT_INTROSPECTION_CHECK([0.6.7])

# API documentation
GTK_DOC_CHECK([1.14], [--flavour no-tmpl])

# Manpage, generated with gtk-doc and xsltproc from DocBook XML.
AC_ARG_ENABLE([man],
  [AS_HELP_STRING([--disable-man], [Disable building the man page])])

AS_IF([test "x$enable_man" != "xno"],
  [AC_CHECK_PROG([have_xsltproc], [xsltproc], [yes], [no])],
  [have_xsltproc=no])

AM_CONDITIONAL([ENABLE_MAN], [test "x$have_xxsltproc" = "xyes"])

# User manual
GNOME_DOC_INIT([0.20])

GLIB_GSETTINGS
#*******************************************************************************

AC_CONFIG_FILES([
Makefile
docs/reference/Makefile
docs/reference/version.xml
data/cheese.pc
data/cheese-gtk.pc
data/cheese.desktop.in
help/Makefile
po/Makefile.in
])
AC_OUTPUT

#**************************************************************************
