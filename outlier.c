#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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

int main(void)
{
	double *list, *lp, d, mean, q1, q3, range;
	int matches;
	size_t n = 0, list_sz = 0x8000;

	list = xmalloc(list_sz * sizeof(double));
	lp = list;
	while (!feof(stdin)) {
		matches = scanf("%le", &d);
		if (matches == 0 || isnan(d))
			continue;
		*lp = d;
		n++;
		if (list_sz < n) {
			list_sz *= 2;
			list = xrealloc(list, (list_sz * sizeof(double)));
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
	free(list);
	return 0;
}
