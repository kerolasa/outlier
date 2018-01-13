project('outlier', 'c',
	default_options : ['c_std=c99'],
	version : '1.0')


cc = meson.get_compiler('c')

c_flags = '''
	-D_GNU_SOURCE
	-O3
	-g
	-fno-strict-aliasing
	-Wstrict-prototypes
	-Wall
	-Wextra
'''.split()

foreach arg : c_flags
	if cc.has_argument(arg)
		add_project_arguments(arg, language : 'c')
	endif
endforeach

add_project_arguments('-include', 'config.h', language : 'c')

conf = configuration_data()
conf.set_quoted('PACKAGE_NAME', meson.project_name())
conf.set_quoted('PACKAGE_BUGREPORT', 'kerolasa@iki.fi')
conf.set('_GNU_SOURCE', 1)

libxml2_dep = dependency('libxml-2.0', required : false)
conf.set10('HAVE_LIBXML2', libxml2_dep.found())

foreach h : [
	'stdio_ext.h',
]
	if cc.has_header(h)
		conf.set('HAVE_' + h.to_upper().underscorify(), 1,
			description : 'Define to 1 if you have the <' + h + '> header file')
	endif
endforeach

foreach f : [
	'__fpending',
	'posix_fadvise',
]
	if cc.has_function(f, args : '-D_GNU_SOURCE')
		conf.set('HAVE_' + f.to_upper(), 1,
			description : 'Define to 1 if you have the ' + f + ' function')
	endif
endforeach

if cc.compiles('
	#define _GNU_SOURCE
	#include <errno.h>
	int main(void) {
		program_invocation_short_name = "test";
		return 0;
	}
', name : 'program_invocation_short_name')
	conf.set('HAVE_PROGRAM_INVOCATION_SHORT_NAME', 1,
		description : 'Define if program_invocation_short_name is defined')
endif

if cc.compiles('
	extern char *__progname;
	int main(void) {
		if (*__progname == 0)
			return 1;
		return 0;
	}
', name : '__progname')
	conf.set('HAVE___PROGNAME', 1,
		description : 'Define if __progname is defined')
endif

config_h = configure_file(
	output : 'config.h',
	configuration : conf)

git_version_h = vcs_tag(
	input : 'git-version.h.meson',
	output : 'git-version.h'
)

includes = include_directories('.')
executable('outlier', 'outlier.c', git_version_h,
	dependencies : libxml2_dep,
	include_directories : includes,
	install : true
)

install_man ('outlier.1')

subdir('tests')