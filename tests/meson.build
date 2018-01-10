test_scripts = [
	'csv',
	'errors',
	'simple',
	'whiskers',
	'yaml',
]
if libxml2_dep.found()
	test_scripts += [
		'rrd',
	]
endif
env = environment()
env.set('top_srcdir', meson.source_root())
env.set('PATH', meson.build_root() + ':/usr/bin:/bin:/usr/sbin:/sbin')

foreach t : test_scripts
	exec = find_program([ t, meson.current_source_dir() ], required : true)
	test(t, exec, env : env, timeout : 1)
endforeach
