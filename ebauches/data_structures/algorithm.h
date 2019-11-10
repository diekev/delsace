#pragma once

void *advance(void *it, int elem_size);
void *advance_n(void *it, int elem_size, int n);

int distance(void *first, void *last, int elem_size);

void *rotate(void *first, void *middle, void *end);

void swap(void *first, void *last, int elem_size);

void reverse(void *first, void *last, int elem_size);

typedef int (*unary_pred)(void *);
typedef int (*binary_pred)(void*, void*);

void *find(void *first, void *last, void *elem, int elem_size, binary_pred cf);
void *find_if_not_n(void *first, int n, int elem_size, unary_pred p);

void *upper_bound(void *first, void *last, void *elem, int elem_size, binary_pred cf);
void *lower_bound(void *first, void *last, void *elem, int elem_size, binary_pred cf);

void *partition(void *first, void *last, int elem_size, unary_pred p);
void *stable_partition(void *first, void *last, int elem_size, unary_pred p);

void insertion_sort(void *first, void *last, int elem_size, binary_pred cf);
