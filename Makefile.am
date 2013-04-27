## Makefile.am -- Process this file with automake to produce Makefile.in
bin_PROGRAMS = pcs test
pcs_SOURCES = \
		 pcs.c

test_SOURCES = \
		 test.c

AM_CPPFLAGS = -I /usr/include -DSYSCONFDIR='"$(sysconfdir)"'

test_CPPFLAGS = $(AM_CPPFLAGS) $(YAML_CFLAGS)
test_LDFLAGS = $(YAML_LIBS)
