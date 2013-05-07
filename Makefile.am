## Makefile.am -- Process this file with automake to produce Makefile.in
AM_MAKEFLAGS			 = --no-print-directory

AM_CFLAGS			 = -Wall

AM_CPPFLAGS			 = -I /usr/include
AM_CPPFLAGS			+= -DSYSCONFDIR='"$(sysconfdir)"'
AM_CPPFLAGS			+= -DPIDDIR='"$(piddir)"'

noinst_LIBRARIES		 = libpcs.a
libpcs_a_SOURCES		 = \
				   cascade.c	\
				   fuzzy.c	\
				   heating.c	\
				   hot_water.c	\
				   icp.c	\
				   log.c	\
				   process.c	\
				   servconf.c	\
				   valve.c	\
				   xmalloc.c

libpcs_a_CPPFLAGS		 = $(AM_CPPFLAGS) $(YAML_CFLAGS)

LDADD				 = libpcs.a

bin_PROGRAMS			 = pcs icp-raw lsicpdas

pcs_LDFLAGS			 = $(YAML_LIBS)

piddir				 = $(localstatedir)/run

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(piddir)
