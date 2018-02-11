# The project basics.
project('outlier', 'c',
	default_options : [
		'c_std=c99',
		'buildtype=debugoptimized',
		'warning_level=2',
	],
	license : 'BSD 2-clause',
	version : '1.0',
	meson_version : '>= 0.44.0',
)

# C compiler found here will run various checks later.
cc = meson.get_compiler('c')

# Include config.h as compiler argument, so that it is 1) always included to
# 2) before other parsing.  Or in other words, make config.h inclusion to be
# something computer does for me without me needing to think about it.
add_project_arguments('-include', 'config.h', language : 'c')

# Contents of the 'config_h' variables are wrote to config.h
config_h = configuration_data()
config_h.set_quoted('PACKAGE_NAME', meson.project_name())
config_h.set_quoted('PACKAGE_BUGREPORT', 'kerolasa@iki.fi')
# Use all features.  See features.h for details.
config_h.set('_GNU_SOURCE', 1)

# Validate 'meson configure build -Ddefault_multiplier=' value.  Because
# meson does not support float data type this is a bit klunky, but works.
default_multiplier = get_option('default_multiplier')
multiplier_arr = default_multiplier.split('.')
if 2 < multiplier_arr.length()
	error('invalid default_multiplier, should be: int.int')
endif
foreach i : multiplier_arr
	num = i.to_int()
endforeach
config_h.set('DEFAULT_MULTIPLIER', default_multiplier)

# By default try to compile with libxml2, but that can be turned off with
# 'meson configure -Duse_libxml2=false'.
if get_option('use_libxml2')
	libxml2_dep = dependency('libxml-2.0', required : false)
else
	# disabler() would be better, but see commit 486639ec70d08338fe03
	libxml2_dep = dependency('disabler-appears-to-disable-executable-build', required : false)
endif
# Set config.h variable to 1 if libxml2 is wanted and found, else it is
# zero.  Use '#if HAVE_LIBXML2' preprocessor directive when checking these.
config_h.set10('HAVE_LIBXML2', libxml2_dep.found())

# Check headers, and if found add HAVE_HEADER_NAME_H markup to config.h
# file.  Use '#ifdef' preprocessor directive when checking these.  If you
# are porting from autotools you should not need any code changes.
foreach h : '''
	stdio_ext.h
'''.split()
	if cc.has_header(h)
		config_h.set('HAVE_' + h.to_upper().underscorify(), 1,
			description : 'Define to 1 if you have the <' + h + '> header file')
	endif
endforeach

# Check functions.  Very similar to header check.
foreach f : '''
	__fpending
	getexecname
	posix_fadvise
'''.split()
	if cc.has_function(f, args : '-D_GNU_SOURCE')
		config_h.set('HAVE_' + f.to_upper(), 1,
			description : 'Define to 1 if you have the ' + f + ' function')
	endif
endforeach

# Check if program_invocation_short_name global variable exists.
if cc.compiles('
	#define _GNU_SOURCE
	#include <errno.h>
	int main(void) {
		program_invocation_short_name = "test";
		return 0;
	}
', name : 'program_invocation_short_name')
	config_h.set('HAVE_PROGRAM_INVOCATION_SHORT_NAME', 1,
		description : 'Define if program_invocation_short_name is defined')
endif

# Check if __progname global variable exists.  After these knowing how to
# perform libc implementation / compiler feature / and so on checks should
# be clear.
if cc.compiles('
	extern char *__progname;
	int main(void) {
		if (*__progname == 0)
			return 1;
		return 0;
	}
', name : '__progname')
	config_h.set('HAVE___PROGNAME', 1,
		description : 'Define if __progname is defined')
endif

# Write config.h file.
configure_file(
	output : 'config.h',
	configuration : config_h
)

# Add a version number to special header file that is similar with output of
# gnulib/build-aux/git-version-gen version.  Unfortunately vcs_tag() does
# not add a meson.vcs_version style variable that could be used as
# configuration_data() input.
git_version_h = vcs_tag(
	input : 'git-version.h.meson',
	output : 'git-version.h'
)

# The include directory contains config.h and git-version.h.
includes = include_directories('.')

# Instructions compile the executable, and markup to install it.
executable('outlier', 'outlier.c', git_version_h,
	dependencies : libxml2_dep,
	include_directories : includes,
	install : true
)

# Manual page has couple '@variable@' style strings that configure_file()
# will replace.  This is similar to autotools output variables done with
# AC_SUBST and AC_SUBST_FILE.
man_vars = configuration_data()
man_vars.set('version', meson.project_version())
man_vars.set('default_multiplier', default_multiplier)
man_page = configure_file(input : 'outlier.1.in',
	output : 'outlier.1',
	configuration : man_vars
)

# Use of configure_file() return value means generator must run before
# install_man() can proceed.  This is a meson way to do ordering.
install_man(man_page)

# Tests are defined in 'tests/meson.build' file.
subdir('tests')
