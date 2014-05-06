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
				   fuzzy-if-d.c \
				   fuzzy-if-s.c \
				   fuzzy-if-z.c \
				   fuzzy-then-d.c \
				   i-8042.c \
				   i-87015.c \
				   logger.c \
				   ni1000tk5000.c \
				   pd.c \
				   serverconf.c \
				   weighted-sum.c

libtools_a_SOURCES		 = \
				   log.c \
				   xmalloc.c

LDADD				 = \
				   libpcs.a \
				   libtools.a

bin_PROGRAMS			 = \
				   dcon-raw \
				   lsicpdas \
				   pcs

dcon_raw_LDADD			 = libicpdas.a libtools.a
lsicpdas_LDADD			 = libicpdas.a libtools.a
pcs_LDADD			 = libpcs.a libicpdas.a libtools.a $(YAML_LIBS)

TESTS				 = \
				   t/t2007 \
				   t/t2006 \
				   t/t2005 \
				   t/t2004 \
				   t/t2003 \
				   t/t2002 \
				   t/t2001 \
				   t/t0003.sh \
				   t/t0002.sh \
				   t/t0001.sh

noinst_PROGRAMS			 = \
				   t/t2007 \
				   t/t2006 \
				   t/t2005 \
				   t/t2004 \
				   t/t2003 \
				   t/t2002 \
				   t/t2001

EXTRA_DIST			 = \
				   t/t0003.sh \
				   t/t0002.sh \
				   t/t0001.sh
