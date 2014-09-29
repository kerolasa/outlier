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
	contrib/outlier.spec \
	tests/expected \
	tests/inputs \
	$(TESTS)

clean-local:
	rm -rf $(top_builddir)/tests/outputs

extrasdir = $(datadir)/@PACKAGE@
dist_extras_DATA = ChangeLog

dist-hook: gen-ChangeLog
.PHONY: gen-ChangeLog
gen-ChangeLog:
	$(AM_V_GEN)if test -d .git; then                        \
		git log --abbrev-commit --pretty=oneline --graph >| $(distdir)/cl-t &&  \
		{ rm -f $(distdir)/ChangeLog &&                 \
		  mv $(distdir)/cl-t $(distdir)/ChangeLog ; }   \
	fi
