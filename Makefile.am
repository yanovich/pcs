## Makefile.am -- Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS		 = subdir-objects
AM_MAKEFLAGS			 = --no-print-directory

AM_CFLAGS			 = -Wall
AM_CPPFLAGS			 = -I /usr/include
AM_CPPFLAGS			 += -DSYSCONFDIR='"$(sysconfdir)"'

noinst_LIBRARIES		 = libpcs.a
libpcs_a_SOURCES		 = \
				   log.c \
				   mark.c \
				   serverconf.c \
				   xmalloc.c

bin_PROGRAMS			 = \
				   pcs

pcs_LDADD			 = libpcs.a $(YAML_LIBS)

TESTS				 = \
				   t/t0001.sh

EXTRA_DIST			 = \
				   t/t0001.sh
