test_scripts = [
	'csv',
	'errors',
	'rrd',
	'simple',
	'whiskers',
	'yaml',
]
env = environment()
env.set('top_srcdir', meson.source_root())
env.set('PATH', meson.build_root() + ':/usr/bin:/bin:/usr/sbin:/sbin')

foreach t : test_scripts
	exec = find_program([ t, meson.current_source_dir() ], required : true)
	test(t, exec, env : env, timeout : 1)
endforeach
