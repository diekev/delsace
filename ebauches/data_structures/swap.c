#include <stdio.h>
#include <string.h>

void swap(void *a, void *b, int size)
{
	char buffer[size];
	memcpy(buffer, a, size);
	memcpy(a, b, size);
	memcpy(b, buffer, size);
}

typedef int(cmpfunc_t)(void*, void*);

int int_compare(void *x, void *y)
{
	int a = *(int *)x;
	int b = *(int *)y;

	if (a > b) {
		return 1;
	}
	else if (a < b) {
		return -1;
	}

	return 0;
}

void *lsearch(void *key, void *base, int size, int elem_size, cmpfunc_t cmp)
{
	for (int i = 0; i < size; i++) {
		void *elemAddr = (char *)base + i * elem_size;

		if (cmp(key, elemAddr) == 0) {
			return elemAddr;
		}
	}

	return NULL;
}

int main()
{
	float a = 11, b = 5;

	printf("a = %f, b = %f\n", a, b);

	swap(&a, &b, sizeof(float));

	printf("a = %f, b = %f\n", a, b);

	int array[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	int key = 5;

	int *index = (int *)lsearch(&key, array, 10, sizeof(int), int_compare);

	printf("Key %d is at index %ld in the array\n", key, index - array);

	return 0;
}

