## Makefile.am -- Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS		 = subdir-objects
AM_MAKEFLAGS			 = --no-print-directory

AM_CFLAGS			 = -Wall
AM_CPPFLAGS			 = -I /usr/include
AM_CPPFLAGS			 += -DSYSCONFDIR='"$(sysconfdir)"'
AM_CPPFLAGS			 += -DPKGRUNDIR='"$(pkgrundir)"'

pkgrundir			  = $(localstatedir)/run/$(PACKAGE)

noinst_LIBRARIES		 = \
				   libicpdas.a \
				   libpcs.a \
				   libtools.a

libicpdas_a_SOURCES		 = \
				   icpdas.c

libpcs_a_SOURCES		 = \
				   analog-valve.c \
				   cascade.c \
				   central-heating.c \
				   const.c \
				   copy.c \
				   discrete-valve.c \
				   fuzzy-if-d.c \
				   fuzzy-if-s.c \
				   fuzzy-if-z.c \
				   fuzzy-then-d.c \
				   file-input.c \
				   file-output.c \
				   i-8024.c \
				   i-8041.c \
				   i-8042.c \
				   i-87015.c \
				   i-87017.c \
				   i-87040.c \
				   last-state.c \
				   linear.c \
				   logger.c \
				   logical-and.c \
				   logical-not.c \
				   logical-or.c \
				   logical-xor.c \
				   ni1000tk5000.c \
				   pcs-parser.c \
				   pt1000.c \
				   r404a.c \
				   pd.c \
				   serverconf.c \
				   timer.c \
				   trigger.c \
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

noinst_PROGRAMS			 =
EXTRA_DIST			 =
include $(srcdir)/t/test.am
