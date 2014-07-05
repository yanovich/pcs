dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT([Process Control Service], [0.1], [ynvich@gmail.com], [pcs])
AC_CONFIG_HEADERS([config.h:config-h.in])

AM_INIT_AUTOMAKE([1.11 foreign])

AC_PROG_CC
AC_PROG_MKDIR_P
AC_PROG_RANLIB

AC_CHECK_HEADERS(\
		 errno.h \
		 signal.h \
		 stdio.h \
		 string.h \
		 sys/time.h \
		 time.h \
		 unistd.h \
		 ,[],[AC_MSG_ERROR([a required header is missing])])

AC_CHECK_FUNCS([\
		getopt\
])

PKG_CHECK_MODULES(YAML, yaml-0.1 >= 0.1)
PKG_CHECK_MODULES(CURL, libcurl)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
