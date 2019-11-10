#include <iostream>
#include <vector>

#if 0
template <typename Iterator>
void bitonic_compare(Iterator first, Iterator last, bool up)
{
	auto distance = last - first;
	auto middle = distance / 2;

	while ((first + middle < last)) {
		if (*first > *(first + middle) == up) {
			auto temp = *first;
			*first = *(first + middle);
			*(first + middle) = temp;
		}

		++first;
	}
}

template <typename Iterator>
Iterator bitonic_merge(Iterator first, Iterator last, bool up)
{
	auto distance = last - first;

	if (distance == 1) {
		return first;
	}

	bitonic_compare(first, last, up);

	auto middle = first + (distance / 2);
	auto first1 = bitonic_merge(first, middle, true);
	auto last1 = bitonic_merge(middle, last, false);

	middle = last1;
	return first1;
}

template <typename Iterator>
Iterator bitonic_sort(Iterator first, Iterator last, bool up)
{
	auto distance = last - first;

	if (distance <= 1) {
		return first;
	}

	auto middle = first + distance / 2;
	auto first1 = bitonic_sort(first, middle, true);
	auto last1 = bitonic_sort(middle, last, false);

	return bitonic_merge(first1, last1, up);
}
#else

#include <cmath>
#include <random>

template <typename Iterator>
void bitonic_kernel(Iterator first, Iterator last, int p, int q)
{
	const auto distance = last - first;
	const auto d = 1 << (p - q);

	for (int i = 0; i < distance; ++i) {
		const bool up = ((i >> p) & 2) == 0;

		auto it1 = (first + i);
		auto it2 = (first + (i | d));

		if ((i & d) == 0 && (*it1 > *it2) == up) {
			auto tmp = *it1;
			*it1 = *it2;
			*it2 = tmp;
		}
	}
}

template <typename Iterator>
void bitonic_sort(Iterator first, Iterator last)
{
	const auto distance = last - first;
	const auto log = std::log(distance);

	for (int i = 0; i < log; ++i) {
		for (int j = 0; j <= i; ++j) {
			bitonic_kernel(first, last, i, j);
		}
	}
}
#endif

void print_vec(std::ostream &os, const std::vector<int> &v)
{
	for (const auto &i : v) {
		os << i << ' ';
	}

	os << '\n';
}

int main()
{
	std::ostream &os = std::cout;
	std::uniform_int_distribution<int> dist(0, 10000);
	std::mt19937 rng(245631);

	int num = 16;
	std::vector<int> v;
	v.reserve(num);

	for (int i = 0; i < num; ++i) {
		v.push_back(dist(rng));
	}

	print_vec(os, v);

#if 0
	bitonic_sort(v.begin(), v.end(), true);
#else
	bitonic_sort(v.begin(), v.end());
#endif

	print_vec(os, v);
}

