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

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_MULTIPLIER 1.5
#define RRD_DATA "/rrd/rra/database/row/v"
#define RRD_MIN "/rrd/ds/min"
#define RRD_MAX "/rrd/ds/max"

struct outlier_conf {
	double whiskers;
	double *list;
	size_t list_sz;
	double min;
	double max;
	unsigned int rrdxml:1, min_set:1, max_set:1;
};

static void __attribute__((__noreturn__)) usage(FILE *out)
{
	fputs("\nUsage:\n", out);
	fprintf(out, " %s [options] <file ...>\n", program_invocation_short_name);
	fputs("\nOptions:\n", out);
	fputs(" -m, --min <num>      minimum value for printout range\n", out);
	fputs(" -x, --max <num>      maximum value for printout range\n", out);
	fputs(" -r, --rrdxml         input is rrdtool --dump output\n", out);
	fputs(" -w, --whiskers <num> interquartile range multiplier\n", out);
	fputs(" -h, --help           display this help and exit\n", out);
	fputs(" -V, --version        output version information and exit\n", out);
	fputs("\n", out);
	fprintf(out, "For more details see %s(1).\n", PACKAGE_NAME);
	exit(out == stderr ? EXIT_FAILURE : EXIT_SUCCESS);
}

static void *xmalloc(const size_t size)
{
	void *ret = malloc(size);

	if (!ret && size)
		err(EXIT_FAILURE, "cannot allocate %zu bytes", size);
	return ret;
}

static void *xrealloc(void *ptr, const size_t size)
{
	void *ret = realloc(ptr, size);

	if (!ret && size)
		err(EXIT_FAILURE, "cannot allocate %zu bytes", size);
	return ret;
}

static double xstrtod(const char *str, const char *errmesg)
{
	double num;
	char *end = NULL;

	if (str == NULL || *str == '\0')
		goto err;
	errno = 0;
	num = strtod(str, &end);
	if (errno || str == end || (end && *end))
		goto err;
	return num;
 err:
	if (errno)
		err(EXIT_FAILURE, "%s: '%s'", errmesg, str);
	errx(EXIT_FAILURE, "%s: '%s'", errmesg, str);
}

static int __attribute__((__pure__)) comp_double(const void *a, const void *b)
{
	return *(double *)a < *(double *)b ? -1 : *(double *)a > *(double *)b ? 1 : 0;
}

static void find_min_max(xmlXPathContextPtr xpathCtx, struct outlier_conf *conf)
{
	xmlXPathObjectPtr xpathObj;
	char *value;
	double d;
	int matches;

	if ((xpathObj = xmlXPathEvalExpression(BAD_CAST RRD_MIN, xpathCtx)) && !conf->min_set) {
		value = (char *)xmlNodeGetContent(xpathObj->nodesetval->nodeTab[0]);
		matches = sscanf(value, "%le", &d);
		if (matches != 0 && !isnan(d)) {
			conf->min = d;
			conf->min_set = 1;
		}
	}
	if ((xpathObj = xmlXPathEvalExpression(BAD_CAST RRD_MAX, xpathCtx)) && !conf->max_set) {
		value = (char *)xmlNodeGetContent(xpathObj->nodesetval->nodeTab[0]);
		matches = sscanf(value, "%le", &d);
		if (matches != 0 && !isnan(d)) {
			conf->max = d;
			conf->max_set = 1;
		}
	}
}

static size_t collect_data(xmlNodeSetPtr nodes, struct outlier_conf *conf)
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
			if (conf->list_sz < n) {
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

static size_t execute_xpath_expression(const char *filename, struct outlier_conf *conf)
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
	if (!(xpathObj = xmlXPathEvalExpression(BAD_CAST RRD_DATA, xpathCtx)))
		errx(EXIT_FAILURE, "unable to evaluate xpath expression: %s", RRD_DATA);
	ret = collect_data(xpathObj->nodesetval, conf);
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
	return ret;
}

static size_t read_rrdxml(char *file, struct outlier_conf *conf)
{
	size_t ret;

	xmlInitParser();
	LIBXML_TEST_VERSION;
	ret = execute_xpath_expression(file, conf);
	xmlCleanupParser();
	return ret;
}

static size_t read_digits(char *file, struct outlier_conf *conf)
{
	FILE *fd;
	double *lp, d;
	size_t n = 0;
	int matches;

	fd = fopen(file, "r");
	if (!fd)
		err(EXIT_FAILURE, "%s", file);
	lp = conf->list;
	while (!feof(fd)) {
		matches = fscanf(fd, "%le", &d);
		if (matches == 0 || isnan(d))
			continue;
		*lp = d;
		n++;
		if (conf->list_sz < n) {
			conf->list_sz *= 2;
			conf->list = xrealloc(conf->list, (conf->list_sz * sizeof(double)));
			lp = conf->list;
			lp += n - 1;
		}
		lp++;
	}
	return n;
}

static double find_mean(size_t n, double *list)
{
	if (n / 2 == 1)
		return list[n / 2];
	return ((list[n / 2] + list[(n / 2) + 1]) / 2);
}

static double find_quartile(size_t n, int q, double *list)
{
	if (n / 2 == 1)
		return list[(n / 4) * q];
	return (((list[(n / 4) * q]) + (list[((n / 4) * q) + 1])) / 2);
}

static int process_file(char *file, struct outlier_conf *conf)
{
	double mean, q1, q3, range, lof, hif;
	size_t n;

	if (conf->rrdxml)
		n = read_rrdxml(file, conf);
	else
		n = read_digits(file, conf);
	if (n < 1)
		return 1;
	qsort(conf->list, n, sizeof(double), &comp_double);
	q1 = find_quartile(n, 1, conf->list);
	mean = find_mean(n, conf->list);
	q3 = find_quartile(n, 3, conf->list);
	range = (q3 - q1) * conf->whiskers;
	if (conf->min_set && q1 - range < conf->min)
		lof = conf->min;
	else
		lof = q1 - range;
	if (conf->max_set && conf->max < q3 + range)
		hif = conf->max;
	else
		hif = q3 + range;
	printf("lof: %f q1: %f m: %f q3: %f hif: %f (range: %f samples: %zu)\n",
	       lof, q1, mean, q3, hif, range, n);
	return 0;
}

int main(int argc, char **argv)
{
	struct outlier_conf conf;
	int c, ret = 0;
	static const struct option longopts[] = {
		{"min", required_argument, NULL, 'm'},
		{"max", required_argument, NULL, 'x'},
		{"rrdxml", no_argument, NULL, 'r'},
		{"whiskers", required_argument, NULL, 'w'},
		{"version", no_argument, NULL, 'V'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};
	memset(&conf, 0, sizeof(conf));
	conf.whiskers = DEFAULT_MULTIPLIER;
	conf.list_sz = 0x8000;
	while ((c = getopt_long(argc, argv, "mxrw:Vh", longopts, NULL)) != -1) {
		switch (c) {
		case 'm':
			conf.min = xstrtod(optarg, "failed to parse min");
			conf.min_set = 1;
			break;
		case 'x':
			conf.max = xstrtod(optarg, "failed to parse max");
			conf.max_set = 1;
			break;
		case 'r':
			conf.rrdxml = 1;
			break;
		case 'w':
			conf.whiskers = xstrtod(optarg, "failed to parse multiplier");
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
	conf.list = xmalloc(conf.list_sz * sizeof(double));
	if (argc == optind)
		ret = process_file("/dev/stdin", &conf);
	else {
		int i;

		for (i = optind; i < argc; i++) {
			printf("%s: ", argv[i]);
			ret |= process_file(argv[i], &conf);
		}
	}
	free(conf.list);
	return ret;
}
