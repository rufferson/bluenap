AM_CFLAGS = $(WARNING_CFLAGS) $(MISC_CFLAGS) -g
AM_LDFLAGS = $(MISC_LDFLAGS)

AM_CFLAGS += @DBUS_CFLAGS@ @GLIB_CFLAGS@
AM_LDFLAGS += @GLIB_LIBS@ @DBUS_LIBS@

bin_PROGRAMS = bluenap
bluenap_SOURCES = bluenap.c
