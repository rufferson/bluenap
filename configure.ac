AC_PREREQ(2.60)
AC_INIT(bluepan, 0.01)

AM_INIT_AUTOMAKE([foreign subdir-objects color-tests silent-rules
                                      tar-pax no-dist-gzip dist-xz])
AC_CONFIG_HEADERS(config.h)
AC_USE_SYSTEM_EXTENSIONS

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_MAINTAINER_MODE

AC_PREFIX_DEFAULT(/usr/local)

PKG_PROG_PKG_CONFIG

AC_LANG_C

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MKDIR_P

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.28, dummy=yes,
                AC_MSG_ERROR(GLib >= 2.28 is required))
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(DBUS, dbus-1 >= 1.6, dummy=yes,
                                  AC_MSG_ERROR(D-Bus >= 1.6 is required))
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

AC_OUTPUT(Makefile)
