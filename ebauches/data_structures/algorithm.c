#include "algorithm.h"

#include <string.h>

void *advance(void *it, int elem_size)
{
	return (char *)it + elem_size;
}

void *advance_n(void *it, int elem_size, int n)
{
	return (char *)it + n * elem_size;
}

int distance(void *first, void *last, int elem_size)
{
	return ((char *)last - (char *)first) / elem_size;
}

void *rotate(void *first, void *middle, void *end)
{
	const int front_size = (char *)middle - (char *)first;
	const int back_size  = (char *)end - (char *)middle;
	char buffer[front_size];

	memcpy(buffer, first, front_size);
	memmove(first, middle, back_size);
	memcpy((char *)end - front_size, buffer, front_size);

	return (char *)first + back_size;
}

void swap(void *first, void *last, int elem_size)
{
	char buffer[elem_size];
	memcpy(buffer, first, elem_size);
	memcpy(first, last, elem_size);
	memcpy(last, buffer, elem_size);
}

void reverse(void *first, void *last, int elem_size)
{
	while (first != last) {
		last = advance(last, -elem_size);
		swap(first, last, elem_size);
		first = advance(first, elem_size);
	}
}

void *find(void *first, void *last, void *elem, int elem_size, binary_pred cf)
{
	while (first != last) {
		if (cf(first, elem) == 0) {
			return first;
		}

		first = advance(first, elem_size);
	}

	return NULL;
}

void *find_if_not_n(void *first, int n, int elem_size, unary_pred p)
{
	for (int i = 0; i < n; i++) {
		first = advance(first, elem_size);

		if (p(first) == 0) {
			break;
		}
	}

	return first;
}

void *upper_bound(void *first, void *last, void *elem, int elem_size, binary_pred cf)
{
	int count = distance(first, last, elem_size);

	while (count > 0) {
		int step = count / 2;
		void *it = advance_n(first, elem_size, step);

		if (cf(elem, it) >= 0) {
			first = advance(it, elem_size);
			count -= step + 1;
		}
		else {
			count = step;
		}
	}

	return first;
}

void *lower_bound(void *first, void *last, void *elem, int elem_size, binary_pred cf)
{
	int count = distance(first, last, elem_size);

	while (count > 0) {
		int step = count / 2;
		void *it = advance_n(first, elem_size, step);

		if (cf(elem, it) < 0) {
			first = advance(it, elem_size);
			count -= step + 1;
		}
		else {
			count = step;
		}
	}

	return first;
}

void *partition(void *first, void *last, int elem_size, unary_pred p)
{
	while (1) {
		while ((first != last) && p(first)) {
			first = advance(first, elem_size);
		}

		if (first == last) {
			break;
		}

		last = advance(last, -elem_size);

		while ((first != last) && !p(last)) {
			last = advance(last, -elem_size);
		}

		if (first == last) {
			break;
		}

		swap(first, last, elem_size);
		first = advance(first, elem_size);
	}

	return first;
}

void *stable_partition(void *first, void *last, int elem_size, unary_pred p)
{
	const int len = distance(first, last, elem_size);

	if (len == 0) {
		return first;
	}

	if (len == 1) {
		return advance_n(first, elem_size, p(first));
	}

	void *middle = advance_n(first, elem_size, len / 2);

	return rotate(stable_partition(first, middle, elem_size, p),
	              middle,
	              stable_partition(middle, last, elem_size, p));
}

void insertion_sort(void *first, void *last, int elem_size, binary_pred cf)
{
	void *first2 = first;

	while (first != last) {
		void *mid = advance(first, elem_size);
		rotate(upper_bound(first2, first, first, elem_size, cf), first, mid);
		first = mid;
	}
}
