#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
	int *elems;
	int logical_len;
	int alloc_len;
} stack;

void stack_new(stack *s)
{
	s->logical_len = 0;
	s->alloc_len = 4;
	s->elems = (int *)malloc(4 * sizeof(int));

	assert(s->elems != NULL);
}

void stack_dispose(stack *s)
{
	free(s->elems);
}

void stack_push(stack *s, int value)
{
	if (s->logical_len == s->alloc_len) {
		s->alloc_len *= 2;
		s->elems = (int *)realloc(s->elems, s->alloc_len * sizeof(int));
	}

	s->elems[s->logical_len++] = value;
}

int stack_pop(stack *s)
{
	return s->elems[--s->logical_len];
}

int main()
{
	stack s;

	stack_new(&s);

	for (int i = 0; i < 5; i++) {
		stack_push(&s, i);
	}

	for (int i = 0; i < 5; i++) {
		int v = stack_pop(&s);
		printf("%d\n", v);
	}

	return 0;
}

