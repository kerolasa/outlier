# An array of tests to make loops easy.
test_scripts = '''
	csv
	errors
	simple
	whiskers
	yaml
'''.split()

# Skip the test when libxml2 is not used.
if libxml2_dep.found()
	test_scripts += [
		'rrd',
	]
endif

env = environment()

# Autotools use 'top_srcdir' where sources are.  Meson does not have it, but
# rather than rewriting tests variables needed earlier are trivial to add to
# environment.
env.set('top_srcdir', meson.source_root())

# This makes sure invocation of test command(s) refer to executables where
# meson writes them.  Else you might test system versions of binaries, and
# miss failures.
env.set('PATH', meson.build_root() + ':/usr/bin:/bin:/usr/sbin:/sbin')

# Run the tests.  Timeout may need tuning if this this used in other
# projects.
foreach t : test_scripts
	exec = find_program([ t, meson.current_source_dir() ], required : true)
	test(t, exec, env : env, timeout : 1)
endforeach
