/* This is outlier analysis utility.
 *
 * The outlier has BSD 2-clause license which also known as "Simplified
 * BSD License" or "FreeBSD License".
 *
 * Copyright 2014- Sami Kerola. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR AND CONTRIBUTORS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of Sami Kerola.
 */

#include "git-version.h"

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if HAVE_LIBXML2
# include <libxml/parser.h>
# include <libxml/tree.h>
# include <libxml/xpath.h>
# include <libxml/xpathInternals.h>
# define RRD_DATA "/rrd/rra/database/row/v"
# define RRD_MIN "/rrd/ds/min"
# define RRD_MAX "/rrd/ds/max"
#endif

#ifdef HAVE_STDIO_EXT_H
# include <stdio_ext.h>
#endif

#ifndef HAVE_PROGRAM_INVOCATION_SHORT_NAME
# ifdef HAVE___PROGNAME
extern char *__progname;
#  define program_invocation_short_name __progname
# else
#  ifdef HAVE_GETEXECNAME
#   define program_invocation_short_name \
		prog_inv_sh_nm_from_file(getexecname(), 0)
#  else
#   define program_invocation_short_name \
		prog_inv_sh_nm_from_file(__FILE__, 1)
#  endif
static char prog_inv_sh_nm_buf[256];

static inline char *prog_inv_sh_nm_from_file(char *f, char stripext)
{
	char *t;

	if ((t = strrchr(f, '/')) != NULL)
		t++;
	else
		t = f;

	strncpy(prog_inv_sh_nm_buf, t, sizeof(prog_inv_sh_nm_buf) - 1);
	prog_inv_sh_nm_buf[sizeof(prog_inv_sh_nm_buf) - 1] = '\0';

	if (stripext && (t = strrchr(prog_inv_sh_nm_buf, '.')) != NULL)
		*t = '\0';

	return prog_inv_sh_nm_buf;
}
# endif
#endif

struct outlier_conf {
	double whiskers;
	double *list;
	size_t list_sz;
	double min;
	double max;
	unsigned int
#if HAVE_LIBXML2
		rrdxml:1,
#endif
		min_set:1,
		max_set:1,
		csv:1,
		yaml:1;
};

static void __attribute__((__noreturn__)) usage(FILE *restrict out)
{
	fputs("\nUsage:\n", out);
	fprintf(out, " %s [options] <file ...>\n", program_invocation_short_name);
	fputs("\nOptions:\n", out);
	fputs(" -m, --min <num>      minimum value for printout range\n", out);
	fputs(" -x, --max <num>      maximum value for printout range\n", out);
#if HAVE_LIBXML2
	fputs(" -r, --rrdxml         input is rrdtool --dump output\n", out);
#endif
	fputs(" -w, --whiskers <num> interquartile range multiplier\n", out);
	fputs("     --csv            output comma separated values\n", out);
	fputs("     --yaml           output yaml data\n", out);
	fputs(" -h, --help           display this help and exit\n", out);
	fputs(" -V, --version        output version information and exit\n", out);
	fputs("\n", out);
	fprintf(out, "Report bug to <%s>.\n", PACKAGE_BUGREPORT);
	fprintf(out, "For more details see: man 1 %s\n", PACKAGE_NAME);
	exit(out == stderr ? EXIT_FAILURE : EXIT_SUCCESS);
}

static inline int close_stream(FILE *restrict stream)
{
#ifdef HAVE___FPENDING
	const int some_pending = (__fpending(stream) != 0);
#endif
	const int prev_fail = (ferror(stream) != 0);
	const int fclose_fail = (fclose(stream) != 0);

	if (prev_fail || (fclose_fail && (
#ifdef HAVE___FPENDING
					  some_pending ||
#endif
					  errno != EBADF))) {
		if (!fclose_fail && !(errno == EPIPE))
			errno = 0;
		return EOF;
	}
	return 0;
}

static inline void close_stdout(void)
{
	if (close_stream(stdout) != 0 && !(errno == EPIPE)) {
		if (errno)
			warn("write error");
		else
			warnx("write error");
		_exit(EXIT_FAILURE);
	}
	if (close_stream(stderr) != 0)
		_exit(EXIT_FAILURE);
}

static __attribute__((malloc))
void *xmalloc(const size_t size)
{
	void *ret = malloc(size);

	if (!ret)
		err(EXIT_FAILURE, "cannot allocate %zu bytes", size);
	return ret;
}

static void *xrealloc(void *restrict ptr, const size_t size)
{
	void *ret = realloc(ptr, size);

	if (!ret)
		err(EXIT_FAILURE, "cannot allocate %zu bytes", size);
	return ret;
}

static double xstrtod(const char *restrict str, const char *restrict errmesg)
{
	double num;
	char *end = NULL;

	if (str == NULL || *str == '\0')
		goto err;
	errno = 0;
	num = strtod(str, &end);
	if (errno || str == end || (end && *end))
		goto err;
	switch (fpclassify(num)) {
	case FP_NORMAL:
	case FP_ZERO:
		break;
	default:
		errno = ERANGE;
		goto err;
	}
	return num;
 err:
	if (errno)
		err(EXIT_FAILURE, "%s: '%s'", errmesg, str);
	errx(EXIT_FAILURE, "%s: '%s'", errmesg, str);
}

static int comp_double(const void *restrict a, const void *restrict b)
{
	if (isless(*(const double *)a, *(const double *)b))
		return -1;
	else if (isless(*(const double *)b, *(const double *)a))
		return 1;
	return 0;
}

#if HAVE_LIBXML2
static void find_min_max(const xmlXPathContextPtr xpathCtx, struct outlier_conf *restrict conf)
{
	xmlXPathObjectPtr xpathObj;
	char *value;
	double d;
	int matches;

	if ((xpathObj = xmlXPathEvalExpression((const xmlChar *)RRD_MIN, xpathCtx)) && !conf->min_set) {
		value = (char *)xmlNodeGetContent(xpathObj->nodesetval->nodeTab[0]);
		matches = sscanf(value, "%le", &d);
		if (matches != 0 && !isnan(d)) {
			conf->min = d;
			conf->min_set = 1;
		}
	}
	if ((xpathObj = xmlXPathEvalExpression((const xmlChar *)RRD_MAX, xpathCtx)) && !conf->max_set) {
		value = (char *)xmlNodeGetContent(xpathObj->nodesetval->nodeTab[0]);
		matches = sscanf(value, "%le", &d);
		if (matches != 0 && !isnan(d)) {
			conf->max = d;
			conf->max_set = 1;
		}
	}
}

static size_t collect_data(const xmlNodeSetPtr nodes, struct outlier_conf *restrict conf)
{
	xmlNodePtr cur;
	char *value;
	int i, size;
	double *lp, d;
	size_t n = 0;
	int matches;

	size = (nodes) ? nodes->nodeNr : 0;
	lp = conf->list;
	for (i = 0; i < size; ++i) {
		assert(nodes->nodeTab[i]);
		if (nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
			cur = nodes->nodeTab[i];
			value = (char *)xmlNodeGetContent(cur);
			matches = sscanf(value, "%le", &d);
			if (matches == 0 || isnan(d))
				continue;
			*lp = d;
			n++;
			if (conf->list_sz == n) {
				assert(conf->list_sz);
				conf->list_sz *= 2;
				conf->list = xrealloc(conf->list, (conf->list_sz * sizeof(double)));
				lp = conf->list;
				lp += n - 1;
			}
			lp++;
		}
	}
	return n;
}

static size_t execute_xpath_expression(const char *restrict filename, struct outlier_conf *restrict conf)
{
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	size_t ret;

	if (!(doc = xmlParseFile(filename)))
		errx(EXIT_FAILURE, "unable to parse file: %s", filename);
	if (!(xpathCtx = xmlXPathNewContext(doc)))
		errx(EXIT_FAILURE, "unable to create new XPath context");
	find_min_max(xpathCtx, conf);
	if (!(xpathObj = xmlXPathEvalExpression((const xmlChar *)RRD_DATA, xpathCtx)))
		errx(EXIT_FAILURE, "unable to evaluate xpath expression: %s", RRD_DATA);
	ret = collect_data(xpathObj->nodesetval, conf);
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
	return ret;
}

static size_t read_rrdxml(const char *restrict file, struct outlier_conf *restrict conf)
{
	size_t ret;

	xmlInitParser();
	LIBXML_TEST_VERSION;
	ret = execute_xpath_expression(file, conf);
	xmlCleanupParser();
	return ret;
}
#endif /* HAVE_LIBXML2 */

static size_t read_digits(const char *restrict file, struct outlier_conf *restrict conf)
{
	FILE *fd;
	struct stat sbuf;
	double *lp, d;
	size_t n = 0;
	int matches;

	fd = fopen(file, "r");
	if (!fd)
		err(EXIT_FAILURE, "%s", file);
	if (fstat(fileno(fd), &sbuf)) {
		fclose(fd);
		err(EXIT_FAILURE, "cannot stat: %s", file);
	}
	if (S_ISDIR(sbuf.st_mode)) {
		warnx("%s: is directory, skipping", file);
		fclose(fd);
		return 0;
	}
#ifdef HAVE_POSIX_FADVISE
# ifdef POSIX_FADV_SEQUENTIAL
	errno = 0;
	if (posix_fadvise(fileno(fd), 0, 0, POSIX_FADV_SEQUENTIAL) != 0 && errno)
		err(EXIT_FAILURE, "fadvise %s", file);
# endif
#endif
	lp = conf->list;
	while (!feof(fd)) {
		errno = 0;
		matches = fscanf(fd, "%le", &d);
		if (matches == 0) {
			fgetc(fd);
			continue;
		}
		if (isnan(d) || matches == EOF)
			continue;
		*lp = d;
		n++;
		if (conf->list_sz == n) {
			assert(conf->list_sz);
			conf->list_sz *= 2;
			conf->list = xrealloc(conf->list, (conf->list_sz * sizeof(double)));
			lp = conf->list;
			lp += n - 1;
		}
		lp++;
	}
	fclose(fd);
	return n;
}

static double find_mean(const size_t n, const double *restrict list)
{
	if (n % 2 == 1)
		return list[n / 2];
	return ((list[(n / 2) - 1] + list[n / 2]) / 2);
}

static double find_quartile(const size_t n, const int q, const double *restrict list)
{
	return (((list[((n / 4) * q) - 1]) + (list[(n / 4) * q])) / 2);
}

static int process_file(const char *restrict file, struct outlier_conf *restrict conf)
{
	double mean, q1, q3, range, lof, hif;
	size_t n;

#if HAVE_LIBXML2
	if (conf->rrdxml)
		n = read_rrdxml(file, conf);
	else
#endif
		n = read_digits(file, conf);
	if (n < 1)
		return 1;
	qsort(conf->list, n, sizeof(double), &comp_double);
	q1 = find_quartile(n, 1, conf->list);
	mean = find_mean(n, conf->list);
	q3 = find_quartile(n, 3, conf->list);
	if (fpclassify(conf->whiskers) != FP_ZERO) {
		range = (q3 - q1) * conf->whiskers;
		if (conf->min_set && q1 - range < conf->min)
			lof = conf->min;
		else
			lof = q1 - range;
		if (conf->max_set && conf->max < q3 + range)
			hif = conf->max;
		else
			hif = q3 + range;
		if (conf->csv)
			printf("%f,%f,%f,%f,%f,%f,%zu\n",
			       lof, q1, mean, q3, hif, range, n);
		else if (conf->yaml)
			printf
			    ("    lof: %f\n    q1: %f\n    m: %f\n    q3: %f\n    hif: %f\n    range: %f\n    samples: %zu\n",
			     lof, q1, mean, q3, hif, range, n);
		else
			printf
			    ("lof: %f q1: %f m: %f q3: %f hif: %f (range: %f samples: %zu)\n",
			     lof, q1, mean, q3, hif, range, n);
	} else {
		if (conf->csv)
			printf("%f,%f,%f,%zu\n",
			       q1, mean, q3, n);
		else if (conf->yaml)
			printf
			    ("    q1: %f\n    m: %f\n    q3: %f\n    samples: %zu\n",
			     q1, mean, q3, n);
		else
			printf
			    ("q1: %f m: %f q3: %f (samples: %zu)\n",
			     q1, mean, q3, n);
	}
	return 0;
}

int main(const int argc, char **argv)
{
	struct outlier_conf conf;
	int c, ret = 0;
	enum {
		CSV_OPT = CHAR_MAX + 1,
		YAML_OPT
	};
	static const struct option longopts[] = {
		{"min", required_argument, NULL, 'm'},
		{"max", required_argument, NULL, 'x'},
#if HAVE_LIBXML2
		{"rrdxml", no_argument, NULL, 'r'},
#endif
		{"whiskers", required_argument, NULL, 'w'},
		{"csv", no_argument, NULL, CSV_OPT},
		{"yaml", no_argument, NULL, YAML_OPT},
		{"version", no_argument, NULL, 'V'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};
	const char *shortopts = "m:x:w:Vh"
#if HAVE_LIBXML2
	"r"
#endif
	;

	atexit(close_stdout);
	memset(&conf, 0, sizeof(conf));
	conf.whiskers = DEFAULT_MULTIPLIER;
	conf.list_sz = 0x400;
	while ((c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
		switch (c) {
		case 'm':
			conf.min = xstrtod(optarg, "failed to parse min");
			conf.min_set = 1;
			break;
		case 'x':
			conf.max = xstrtod(optarg, "failed to parse max");
			conf.max_set = 1;
			break;
#if HAVE_LIBXML2
		case 'r':
			conf.rrdxml = 1;
			break;
#endif
		case 'w':
			conf.whiskers = xstrtod(optarg, "failed to parse multiplier");
			break;
		case CSV_OPT:
			conf.csv = 1;
			break;
		case YAML_OPT:
			conf.yaml = 1;
			break;
		case 'V':
			printf("%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
			return EXIT_SUCCESS;
		case 'h':
			usage(stdout);
		default:
			usage(stderr);
		}
	}
	if (conf.csv && conf.yaml)
		errx(EXIT_FAILURE, "--csv and --yaml are mutually exclusive");
	conf.list = xmalloc(conf.list_sz * sizeof(double));
	if (argc == optind) {
		if (conf.csv && fpclassify(conf.whiskers) != FP_ZERO)
			printf("lof,q1,m,q3,hif,range,samples\n");
		else if (conf.csv)
			printf("q1,m,q3,samples\n");
		else if (conf.yaml)
			printf("---\noutlier:\n  stdin:\n");
		ret = process_file("/dev/stdin", &conf);
		if (ret)
			putchar('\n');
	} else {
		int i;

		if (conf.csv && fpclassify(conf.whiskers) != FP_ZERO)
			puts("name,lof,q1,m,q3,hif,range,samples");
		else if (conf.csv)
			puts("q1,m,q3,samples");
		else if (conf.yaml)
			printf("---\noutlier:\n");
		for (i = optind; i < argc; i++) {
			int r;

			if (conf.csv)
				printf("%s,", argv[i]);
			else if (conf.yaml)
				printf("  %s:\n", argv[i]);
			else
				printf("%s: ", argv[i]);
			r = process_file(argv[i], &conf);
			if (r)
				putchar('\n');
			ret |= r;
		}
	}
	free(conf.list);
	return ret;
}
