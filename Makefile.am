## Makefile.am -- Process this file with automake to produce Makefile.in
AM_MAKEFLAGS			 = --no-print-directory

noinst_LIBRARIES		 = libpcs.a
libpcs_a_SOURCES		 = \
				   cleanup.c \
				   fatal.c \
				   icp.c \
				   log.c \
				   xmalloc.c

bin_PROGRAMS			 = pcs test
pcs_SOURCES			 = \
				   pcs.c
pcs_LDADD			 = libpcs.a

test_SOURCES			 = \
				   servconf.c
test_LDADD			 = libpcs.a

piddir				 = $(localstatedir)/run

AM_CPPFLAGS			 = -I /usr/include
AM_CPPFLAGS			+= -DSYSCONFDIR='"$(sysconfdir)"'
AM_CPPFLAGS			+= -DPIDDIR='"$(piddir)"'

test_CPPFLAGS			 = $(AM_CPPFLAGS) $(YAML_CFLAGS)
test_LDFLAGS			 = $(YAML_LIBS)

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(piddir)
