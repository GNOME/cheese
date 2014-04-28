# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.64])
AC_INIT([Cheese],
        [3.13.1],
        [https://bugzilla.gnome.org/enter_bug.cgi?product=cheese],
        [cheese],
        [https://wiki.gnome.org/Apps/Cheese])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([cheese-config.h])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.12 -Wall -Wno-portability dist-xz no-define no-dist-gzip nostdinc subdir-objects])
AM_MAINTAINER_MODE([enable])
AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O
AM_PROG_VALAC([0.23.3])

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
AC_SUBST([LIBCHEESE_LT_VERSION], [7:24:0])
AC_SUBST([LIBCHEESE_GTK_LT_VERSION], [24:8:1])

#*******************************************************************************
# Internationalization
#*******************************************************************************
AC_SUBST([GETTEXT_PACKAGE], [$PACKAGE_TARNAME])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [GETTEXT package name])
IT_PROG_INTLTOOL([0.50.0])

#*******************************************************************************
# Check required libraries
#*******************************************************************************


GLIB_REQUIRED="glib-2.0 >= 2.39.90"
GTK_REQUIRED="gtk+-3.0 >= 3.11.6"
GDK_PIXBUF_REQUIRED="gdk-pixbuf-2.0"
LIBGNOMEDESKTOP_REQUIRED="gnome-desktop-3.0 >= 2.91.6"
GSTREAMER_VERSION=0.11.0
GSTREAMER_REQUIRED="gstreamer-plugins-base-1.0 >= $GSTREAMER_VERSION gstreamer-1.0 >= $GSTREAMER_VERSION gstreamer-pbutils-1.0 >= $GSTREAMER_VERSION"
GSTREAMER_PLUGINS_BAD_VERSION=0.11.0
GSTREAMER_PLUGINS_BAD_REQUIRED="gstreamer-plugins-bad-1.0 >= $GSTREAMER_PLUGINS_BAD_VERSION"
CAIRO_REQUIRED="cairo >= 1.10.0"
PANGOCAIRO_REQUIRED="pangocairo >= 1.28.0"
LIBRSVG_REQUIRED="librsvg-2.0 >= 2.32.0"
CLUTTER_REQUIRED="clutter-1.0 >= 1.13.2"
CLUTTERGTK_REQUIRED="clutter-gtk-1.0 >= 0.91.8"
CLUTTERGST_REQUIRED="clutter-gst-2.0 >= 1.9.0"
LIBCANBERRA_REQUIRED="libcanberra-gtk3 >= 0.26"
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
AS_IF([test -n "$UDEV_REQUIRED"],
  [PKG_CHECK_EXISTS([$UDEV_REQUIRED],
    [AC_MSG_RESULT([found])
     AC_DEFINE([HAVE_UDEV], [], [Enable gudev support])],
    [AC_MSG_ERROR([$UDEV_REQUIRED not found])])],
  [AC_MSG_RESULT([not needed])])

AC_CHECK_HEADERS([sys/videoio.h])

AC_CHECK_HEADER([X11/extensions/XTest.h],
  [AC_CHECK_LIB([Xtst], [XTestFakeKeyEvent],
    [AC_SUBST([XTEST_LIBS], ["-lX11 -lXtst"])])])
AM_CONDITIONAL([HAVE_XTEST], test "x$ac_cv_lib_Xtst_XTestFakeKeyEvent" = "xyes")

PKG_CHECK_MODULES([CHEESE],
  [$GLIB_REQUIRED
  $X11_REQUIRED
  $GDK_PIXBUF_REQUIRED
  $GSTREAMER_REQUIRED
  $GSTREAMER_PLUGINS_BAD_REQUIRED
  $LIBGNOMEDESKTOP_REQUIRED
  $CAIRO_REQUIRED
  $PANGOCAIRO_REQUIRED
  $CLUTTER_REQUIRED
  $CLUTTERGST_REQUIRED
  $UDEV_REQUIRED])

PKG_CHECK_MODULES([CHEESE_GTK],
  [$GTK_REQUIRED
   $LIBRSVG_REQUIRED
   $CLUTTERGTK_REQUIRED
   $LIBCANBERRA_REQUIRED
   $UDEV_REQUIRED
   $GNOME_VIDEO_EFFECTS_REQUIRED])

# Resources.
AC_SUBST([GLIB_COMPILE_RESOURCES], [`$PKG_CONFIG --variable glib_compile_resources gio-2.0`])

# Recommend some runtime GStreamer plugins.
AC_PATH_PROGS([GST_INSPECT], [gst-inspect-1.0], [notfound])
AS_IF([test "x$GST_INSPECT" = "xnotfound"],
  [AC_MSG_WARN([unable to check for runtime GStreamer plugin dependencies])],
  [GST_RUNTIME_PLUGINS="camerabin vp8enc webmmux"
   for gst_runtime_plugin in $GST_RUNTIME_PLUGINS
   do
     AC_MSG_CHECKING([for $gst_runtime_plugin GStreamer plugin])
     AS_IF([$GST_INSPECT $gst_runtime_plugin >&AS_MESSAGE_LOG_FD],
       [AC_MSG_RESULT([found])],
       [AC_MSG_RESULT([not found])
        AC_MSG_WARN([$gst_runtime_plugin was not found. It needs to be installed before Cheese is run])])
   done])

# Check for GLib testing utilities.
AC_PATH_PROG([GTESTER], [gtester], [notfound])
AC_PATH_PROG([GTESTER_REPORT], [gtester-report], [notfound])
AS_IF([test "x$GTESTER" = "xnotfound" -o "x$GTESTER_REPORT" = "xnotfound"],
  [AC_MSG_WARN([testing disabled as the required utilities were not found])],
  [CHEESE_ENABLE_TESTS=true
   AC_SUBST([GTESTER])
   AC_SUBST([GTESTER_REPORT])])
AM_CONDITIONAL([CHEESE_ENABLE_TESTS], [test "x$CHEESE_ENABLE_TESTS" = "xtrue"])

# Check for lcov.
AC_ARG_ENABLE([lcov],
  [AS_HELP_STRING([--disable-lcov], [Disable lcov code coverage testing])])

AS_IF([test "x$enable_lcov" != "xno"],
  [AC_PATH_PROG([LCOV], [lcov], [notfound])
   AC_PATH_PROG([LCOV_GENHTML], [genhtml], [notfound])
   AS_IF([test "x$LCOV" = "xnotfound" -o "x$LCOV_GENHTML" = "xnotfound"],
     [have_lcov=no],
     [have_lcov=yes])],
  [have_lcov=no])

AS_IF([test "x$have_lcov" = "xyes"],
  [AC_SUBST([LCOV])
   AC_SUBST([LCOV_GENHTML])
   CFLAGS="$CFLAGS -O0 -fprofile-arcs -ftest-coverage"
   LDFLAGS="$LDFLAGS -lgcov"],
  [AS_IF([test "x$enable_lcov" = "xyes"],
     [AC_MSG_ERROR([lcov code coverage reporting requested but not found])])])

AM_CONDITIONAL([CHEESE_ENABLE_LCOV], [test "x$have_lcov" = "xyes"])

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
  [AC_CHECK_PROG([have_xsltproc], [xsltproc], [yes], [no])
   JH_CHECK_XML_CATALOG([-//OASIS//DTD DocBook XML V4.3//EN],
     [DocBook XML DTD V4.3], [have_docbookdtd=yes], [have_docbookdtd=no])
   JH_CHECK_XML_CATALOG(
     [http://docbook.sourceforge.net/release/xsl/current/html/refentry.xsl],
     [DocBook XSL Stylesheets], [have_docbookxsl=yes], [have_docbookxsl=no])
   AS_IF([test "x$have_xsltproc" = "xyes" -a "x$have_docbookdtd" = "xyes" \
     -a "x$have_docbookxsl" = "xyes"],
     [have_manutils=yes],
     [AS_IF([test "x$enable_man" = "xyes"],
       [AC_MSG_ERROR(
         [manpage generation requested but required utilities were not found])])
      have_manutils=no])],
  [have_manutils=no])

AM_CONDITIONAL([ENABLE_MAN], [test "x$have_manutils" = "xyes"])

# Mallard help
YELP_HELP_INIT

# AppData
APPDATA_XML

GLIB_GSETTINGS
#*******************************************************************************

AC_CONFIG_FILES([
Makefile
docs/reference/Makefile
docs/reference/version.xml
data/cheese.pc
data/cheese-gtk.pc
data/org.gnome.Cheese.desktop.in
data/org.gnome.Cheese.service
help/Makefile
po/Makefile.in
])
AC_OUTPUT

#**************************************************************************
