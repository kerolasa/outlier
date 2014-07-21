AM_CPPFLAGS = -include $(top_builddir)/config.h

bin_PROGRAMS = outlier
outlier_SOURCES = outlier.c
outlier_CFLAGS = $(LIBXML2_CFLAGS)
outlier_LDADD = $(LIBXML2_LIBS)

dist_man_MANS = outlier.1

TESTS_ENVIRONMENT = top_srcdir=$(top_srcdir) PATH=$(top_builddir)$(PATH_SEPARATOR)$$PATH

TESTS = \
	tests/csv \
	tests/errors \
	tests/rrd \
	tests/simple \
	tests/whiskers \
	tests/yaml

EXTRA_DIST = \
	tests/expected \
	tests/inputs \
	$(TESTS)

clean-local-tests:
	rm -rf $(top_builddir)/tests/outputs

CLEAN_LOCALS = clean-local-tests
