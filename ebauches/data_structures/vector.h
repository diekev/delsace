#pragma once

typedef void (*free_func_t)(void *);

typedef struct vector {
	void *data;
	int capacity;
	int size;
	int elem_size;

	free_func_t free_func;

	void *(*begin)(struct vector *);
	void *(*next)(struct vector *, void *);
	void *(*end)(struct vector *);
} vector;

void *vector_begin(vector *v);
void *vector_next(vector *v, void *elem);
void *vector_end(vector *v);

vector *vector_new(int n, int elem_size, free_func_t free_func);
void vector_free(vector *v);

void *vector_at(vector *v, int i);

void vector_push_back(vector *v, void *elem);

void vector_resize(vector *v, int n);
void vector_reserve(vector *v, int n);
void vector_shrink_to_fit(vector *v);

void vector_print(vector *v, void (*print_func)(void *));
