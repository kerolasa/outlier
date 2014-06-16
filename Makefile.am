AM_CPPFLAGS = -include $(top_builddir)/config.h

bin_PROGRAMS = outlier
outlier_SOURCES = outlier.c
outlier_CFLAGS = $(LIBXML2_CFLAGS)
outlier_LDADD = $(LIBXML2_LIBS)

dist_man_MANS = outlier.1
