## Makefile.am -- Process this file with automake to produce Makefile.in
AM_MAKEFLAGS			 = --no-print-directory

noinst_LIBRARIES		 = libpcs.a
libpcs_a_SOURCES		 = \
				   fuzzy.c \
				   icp.c \
				   log.c \
				   xmalloc.c

LDADD				 = libpcs.a

bin_PROGRAMS			 = pcs test icp-raw lsicpdas
test_SOURCES			 = \
				   servconf.c

piddir				 = $(localstatedir)/run

AM_CFLAGS			 = -Wall

AM_CPPFLAGS			 = -I /usr/include
AM_CPPFLAGS			+= -DSYSCONFDIR='"$(sysconfdir)"'
AM_CPPFLAGS			+= -DPIDDIR='"$(piddir)"'

test_CPPFLAGS			 = $(AM_CPPFLAGS) $(YAML_CFLAGS)
test_LDFLAGS			 = $(YAML_LIBS)

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(piddir)
