## Makefile.am -- Process this file with automake to produce Makefile.in
AM_MAKEFLAGS = --no-print-directory

bin_PROGRAMS = pcs test
pcs_SOURCES = \
		 pcs.c

test_SOURCES = \
		 test.c

piddir = $(localstatedir)/run

AM_CPPFLAGS =  -I /usr/include
AM_CPPFLAGS += -DSYSCONFDIR='"$(sysconfdir)"'
AM_CPPFLAGS += -DPIDDIR='"$(piddir)"'

test_CPPFLAGS = $(AM_CPPFLAGS) $(YAML_CFLAGS)
test_LDFLAGS = $(YAML_LIBS)

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(piddir)
