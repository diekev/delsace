#include <algorithm>
#include <tuple>

using vertex = std::tuple<float, float>;

template <typename ForwardIterator, typename InputIterator, typename OutputIterator>
OutputIterator find_all_lower_bounds(ForwardIterator haystack_begin,
                                     ForwardIterator haystack_end,
                                     InputIterator needles_begin,
                                     InputIterator needles_end,
                                     OutputIterator result)
{
	return std::transform(needles_begin, needles_end, result, [=](ForwardIterator &needle)
			{
				auto iter = std::lower_bound(haystack_begin, haystack_end, needle);
				return std::distance(haystack_begin, iter);
			});
}

void sort_vertices(std::vector<vertex> input)
{
	std::vector<vertex> vertices = input;
	std::vector<size_t> indices(input.size());

	// sort vertices to bring duplicates together
	std::sort(vertices.begin(), vertices.end());

	// find unique vertices and erase redundancies
	auto redundant_begin = std::unique(vertices.begin(), vertices.end());
	vertices.erase(redundant_begin, vertices.end());

	// find index of each vertex in the list of unique vertices
	find_all_lower_bounds(vertices.begin(), vertices.end(), input.begin(), input.end(), indices.begin());
}

