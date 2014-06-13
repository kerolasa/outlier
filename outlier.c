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

#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

static void __attribute__((__noreturn__)) usage(FILE *out)
{
	fputs("\nUsage:\n", out);
	fprintf(out, " %s [options] file.rrd <file.rrd ...>\n", program_invocation_short_name);
	fputs("\nOptions:\n", out);
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

static int __attribute__((__pure__)) comp_double(const void *a, const void *b)
{
	double f1 = *(double *)a;
	double f2 = *(double *)b;

	return f1 < f2 ? -1 : f1 > f2 ? 1 : 0;
}

static int process_file(FILE *fd, double *list, size_t *list_sz)
{
	double *lp, d, mean, q1, q3, range;
	size_t n = 0;
	int matches;

	lp = list;
	while (!feof(fd)) {
		matches = scanf("%le", &d);
		if (matches == 0 || isnan(d))
			continue;
		*lp = d;
		n++;
		if (*list_sz < n) {
			*list_sz *= 2;
			list = xrealloc(list, (*list_sz * sizeof(double)));
			lp = list;
			lp += n - 1;
		}
		lp++;
	}
	qsort(list, n, sizeof(double), &comp_double);
	q1 = list[n / 4];
	mean = list[n / 2];
	q3 = list[(n / 4) * 3];
	range = q3 - q1;
	printf("lof: %f q1: %f m: %f q3: %f hof: %f (range: %f)\n", q1 - range,
	       q1, mean, q3, q3 + range, range);
	return 0;
}

int main(int argc, char **argv)
{
	double *list;
	int c;
	size_t list_sz = 0x8000;
	static const struct option longopts[] = {
		{"version", no_argument, NULL, 'V'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, "Vh", longopts, NULL)) != -1) {
		switch (c) {
		case 'V':
			printf("%s version %s\n", PACKAGE_NAME,
			       PACKAGE_VERSION);
			return EXIT_SUCCESS;
		case 'h':
			usage(stdout);
		default:
			usage(stderr);
		}
	}
	list = xmalloc(list_sz * sizeof(double));
	if (argc == 1)
		process_file(stdin, list, &list_sz);
	else {
		FILE *fd;
		int i;

		for (i = 1; i < argc; i++) {
			fd = fopen(argv[i], "r");
			if (!fd)
				err(EXIT_FAILURE, "%s", argv[i]);
			printf("%s: ", argv[i]);
			process_file(stdin, list, &list_sz);
		}
	}
	free(list);
	return 0;
}
