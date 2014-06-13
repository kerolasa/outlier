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

AC_CONFIG_FILES([
	Makefile
])
AC_OUTPUT
