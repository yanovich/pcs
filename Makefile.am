## Makefile.am -- Process this file with automake to produce Makefile.in
AM_MAKEFLAGS			 = --no-print-directory

AM_CFLAGS			 = -Wall

AM_CPPFLAGS			 = -I /usr/include
AM_CPPFLAGS			+= -DSYSCONFDIR='"$(sysconfdir)"'
AM_CPPFLAGS			+= -DPIDDIR='"$(piddir)"'

noinst_LIBRARIES		 = libpcs.a
libpcs_a_SOURCES		 = \
				   analog_valve.c	\
				   cascade.c	\
				   counter.c	\
				   fuzzy.c	\
				   evaporation.c	\
				   heating.c	\
				   hot_water.c	\
				   icp.c	\
				   log.c	\
				   process.c	\
				   reference.c	\
				   relay.c	\
				   servconf.c	\
				   valve.c	\
				   xmalloc.c

libpcs_a_CPPFLAGS		 = $(AM_CPPFLAGS) $(YAML_CFLAGS)

LDADD				 = libpcs.a

bin_PROGRAMS			 = \
				   pcs \
				   fc-raw \
				   icp-raw \
				   lsicpdas

pcs_LDADD			 = $(LDADD) $(YAML_LIBS)

piddir				 = $(localstatedir)/run

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(piddir)
