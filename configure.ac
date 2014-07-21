#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.69])
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
AC_PROG_CC

AC_CHECK_HEADERS([ \
	stdio_ext.h \
])

AC_CHECK_FUNCS([ \
	__fpending \
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

AC_CONFIG_FILES([
	Makefile
])
AC_OUTPUT
