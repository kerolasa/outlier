#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.59])
AC_INIT([outlier],
	[m4_esyscmd([build-aux/git-version-gen .tarball-version])],
	[kerolasa@iki.fi],[],
	[https://github.com/kerolasa/outlier/])
AC_CONFIG_SRCDIR([outlier.c])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([
	-Wall
	-Wextra-portability
	1.12
	foreign
	dist-xz
	no-dist-gzip
	parallel-tests
	subdir-objects
	tar-pax
])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl Checks for programs.
AC_USE_SYSTEM_EXTENSIONS
AC_C_RESTRICT
AC_PROG_CC_C99

AC_CHECK_HEADERS([ \
	stdio_ext.h \
])

AC_CHECK_FUNCS([ \
	__fpending \
	posix_fadvise \
])

dnl Require pkg-config
m4_ifndef([PKG_PROG_PKG_CONFIG],
       [m4_fatal([Could not locate the pkg-config autoconf macros.  These are usually
located in /usr/share/aclocal/pkg.m4.  If your macros are in a different
location, try setting the environment variable
export ACLOCAL_FLAGS="-I/other/macro/dir"
before running ./autogen.sh or autoreconf again.])
])
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([LIBXML2], [libxml-2.0])

AC_MSG_CHECKING([whether program_invocation_short_name is defined])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
	#include <argp.h>
]], [[
	program_invocation_short_name = "test";
]])], [
	AC_MSG_RESULT([yes])
	AC_DEFINE([HAVE_PROGRAM_INVOCATION_SHORT_NAME], [1],
		[Define if program_invocation_short_name is defined])
], [
	AC_MSG_RESULT([no])
])
AC_MSG_CHECKING([whether __progname is defined])
AC_LINK_IFELSE([AC_LANG_PROGRAM([extern char *__progname;],
	   [if (*__progname == 0) return;])],
	   AC_DEFINE([HAVE___PROGNAME], [1], [Define if __progname is defined])
	   AC_MSG_RESULT([yes]),
	   AC_MSG_RESULT([no]))

AC_CONFIG_FILES([
	Makefile
])
AC_OUTPUT
