#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static int __attribute__((__pure__)) comp_double(const void *a, const void *b)
{
	double f1 = *(double *)a;
	double f2 = *(double *)b;

	return f1 < f2 ? -1 : f1 > f2 ? 1 : 0;
}

int main(void)
{
	double list[32768], d, mean, q1, q3, range;
	int n = 0, matches;
	while (!feof(stdin)) {
		matches = scanf("%le", &d);
		if (matches == 0 || isnan(d))
			continue;
		list[n] = d;
		n++;
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
