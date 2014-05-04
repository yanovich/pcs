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
				   const.c \
				   i-8042.c \
				   i-87015.c \
				   logger.c \
				   ni1000tk5000.c \
				   serverconf.c

libtools_a_SOURCES		 = \
				   log.c \
				   xmalloc.c

LDADD				 = \
				   libpcs.a \
				   libtools.a

bin_PROGRAMS			 = \
				   icp-raw \
				   lsicpdas \
				   pcs

icp_raw_LDADD			 = libicpdas.a libtools.a
lsicpdas_LDADD			 = libicpdas.a libtools.a
pcs_LDADD			 = libpcs.a libicpdas.a libtools.a $(YAML_LIBS)

TESTS				 = \
				   t/t2001 \
				   t/t0003.sh \
				   t/t0002.sh \
				   t/t0001.sh

noinst_PROGRAMS			 = \
				   t/t2001

EXTRA_DIST			 = \
				   t/t0003.sh \
				   t/t0002.sh \
				   t/t0001.sh
