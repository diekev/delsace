#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "algorithm.h"
#include "vector.h"

void print_int(void *elem)
{
	int i = *(int *)elem;
	printf("%d ", i);
}

void print_float(void *elem)
{
	float f = *(float *)elem;
	printf("%.6f\n", f);
}

int compare_int(void *va, void *vb)
{
	int a = *(int *)va;
	int b = *(int *)vb;

	return a - b;
}

int compare_float(void *va, void *vb)
{
	float a = *(float *)va;
	float b = *(float *)vb;

	return (a < b) ? -1 : ((a > b) ? 1 : 0);
}

int partition_pred(void *va)
{
	int i = *(int *)va;
	return (i == 0) ? 1 : 0;
}

///////////////// test struct

typedef struct test {
	int t;
} test;

int gi = 0;

test *create_test(void)
{
	 test *t = malloc(sizeof(test));
	 t->t = gi++;
	 return t;
}

void test_free(void *data)
{
	test *t = *(test **)data;
	free(t);
}

void print_test(void *data)
{
	test *t = *(test **)data;
	printf("%d ", t->t);
}

int test_pred(void *va)
{
	test *t = *(test **)va;
	return ((t->t & 1) == 0) ? 1 : 0;
}

///////////////// test struct

int main()
{
	vector *v = vector_new(10, sizeof(test *), test_free);
	vector_reserve(v, 10);

	for (int i = 0; i < 10; i++) {
		test *t = create_test();
		vector_push_back(v, &t);
	}

	stable_partition(v->begin(v), v->end(v), v->elem_size, test_pred);
	vector_print(v, print_test);
	vector_free(v);

	return 0;
}
