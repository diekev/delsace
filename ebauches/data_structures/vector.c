#include "vector.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *vector_begin(vector *v)
{
	return (char *)v->data;
}

void *vector_next(vector *v, void *elem)
{
	assert((elem >= v->begin(v)) && (elem <= v->end(v)));
	return (char *)elem + v->elem_size;
}

void *vector_end(vector *v)
{
	return (char *)v->data + v->size * v->elem_size;
}

vector *vector_new(int n, int elem_size, free_func_t free_func)
{
	vector *v = malloc(sizeof(vector));

	v->data = malloc(n * elem_size);
	v->capacity = n;
	v->size = 0;
	v->elem_size = elem_size;
	v->free_func = free_func;
	v->begin = vector_begin;
	v->end = vector_end;
	v->next = vector_next;

	return v;
}

void *vector_at(vector *v, int i)
{
	assert(i <= v->size);
	return (char *)v->data + i * v->elem_size;
}

void vector_free(vector *v)
{
	if (v->free_func != NULL) {
//		for (void *elem = v->begin(v); elem != v->end(v); elem = v->next(v, elem)) {
//			v->free_func(elem);
//		}
		for (int i = 0; i < v->size; i++) {
			v->free_func(vector_at(v, i));
		}
	}

	free(v->data);
	free(v);
	v = NULL;
}

void vector_resize(vector *v, int n)
{
	v->size = n;
	v->capacity = n;
	v->data = realloc(v->data, v->capacity * v->elem_size);
}

void vector_reserve(vector *v, int n)
{
	if (n > v->capacity) {
		v->capacity = n;
		v->data = realloc(v->data, v->capacity * v->elem_size);
	}
}

void vector_push_back(vector *v, void *elem)
{
	if (v->size == v->capacity) {
		v->capacity += 1;
		v->data = realloc(v->data, v->capacity * v->elem_size);
	}

	int offset = v->size * v->elem_size;
	void *addr = (char *)v->data + offset;
	memcpy(addr, elem, v->elem_size);
	v->size++;
}

void vector_shrink_to_fit(vector *v)
{
	if (v->size != 0) {
		v->capacity = v->size;
		v->data = realloc(v->data, v->capacity * v->elem_size);
	}
}

void vector_print(vector *v, void (*print_func)(void *))
{
	for (int i = 0; i < v->size; i++) {
		void *addr = (char *)v->data + i * v->elem_size;
		print_func(addr);
	}

	printf("\n");
}
