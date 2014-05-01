## Makefile.am -- Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS		 = subdir-objects
AM_MAKEFLAGS			 = --no-print-directory

AM_CFLAGS			 = -Wall
AM_CPPFLAGS			 = -I /usr/include
AM_CPPFLAGS			 += -DSYSCONFDIR='"$(sysconfdir)"'

noinst_LIBRARIES		 = \
				   libicpdas.a \
				   libpcs.a \
				   libtools.a

libicpdas_a_SOURCES		 = \
				   icpdas.c

libpcs_a_SOURCES		 = \
				   i-8042.c \
				   mark.c \
				   serverconf.c

libtools_a_SOURCES		 = \
				   log.c \
				   xmalloc.c

LDADD				 = \
				   libtools.a

bin_PROGRAMS			 = \
				   pcs

pcs_LDADD			 = libpcs.a libicpdas.a libtools.a $(YAML_LIBS)

TESTS				 = \
				   t/t0001.sh

EXTRA_DIST			 = \
				   t/t0001.sh
