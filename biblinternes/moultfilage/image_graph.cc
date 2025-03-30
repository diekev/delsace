/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "image_graph.hh"

#include <cstring>
#include <cstdio>

#include <tbb/flow_graph.h>

using namespace tbb;
using namespace tbb::flow;

static const int num_image_buffers = 100;
static int image_size = 10000000;
// static int num_graph_buffers = 8;
static int img_number = 0;
static int num_images = 64;
static const int a_frequency = 11;
static const int b_frequency = 13;

image::image()
    : N(image_size)
	, data(new char[static_cast<size_t>(static_cast<unsigned>(N))])
{}

image::image(int image_number, bool a, bool b)
    : image()
{
	memset(data, '\0', static_cast<size_t>(N));
	data[0] = static_cast<char>(image_number - 32);

	if (a) data[N - 2] = 'A';
	if (b) data[N - 1] = 'B';
}

image::~image()
{
	delete [] data;
}

image *get_next_image()
{
	if (img_number < num_images) {
		bool a = (img_number % a_frequency == 0);
		bool b = (img_number % b_frequency == 0);

		return new image(img_number++, a, b);
	}

	return nullptr;
}

void preprocess_image(image *input_image, image *output_image)
{
	for (int i = 0; i < input_image->N; ++i) {
		output_image->data[i] = static_cast<char>(output_image->data[i] + 32);
	}
}

bool detect_with_A(image *input_image)
{
	for (int i = 0; i < input_image->N; ++i) {
		if (input_image->data[i] == 'a')
			return true;
	}

	return false;
}

bool detect_with_B(image *input_image)
{
	for (int i = 0; i < input_image->N; ++i) {
		if (input_image->data[i] == 'b')
			return true;
	}

	return false;
}

void output_image(image *input_image, bool found_a, bool found_b)
{
	bool a = false, b = false;
	int a_i = -1, b_i = -1;

	for (int i = 0; i < input_image->N; ++i) {
		if (input_image->data[i] == 'a') {
			a = true;
			a_i = i;
		}

		if (input_image->data[i] == 'b') {
			b = true;
			b_i = i;
		}
	}

	printf("Detected feature (a,b)=(%d,%d)=(%d,%d) at (%d,%d) for image %p:%d\n",
		   a, b, found_a, found_b, a_i, b_i, static_cast<void *>(input_image), input_image->data[0]);
}

void test_image_graph()
{
#if 0
	graph g;

	typedef std::tuple<image *, image *> resource_tuple;
	typedef std::pair<image *, bool> detection_pair;
	typedef std::tuple<detection_pair, detection_pair> detection_tuple;

	queue_node<image *> buffers(g);

	join_node<resource_tuple, reserving> resource_join(g);
	join_node<detection_tuple, tag_matching> detection_join(g,
	                                                        [](const detection_pair &p) -> size_t
	{
		return reinterpret_cast<size_t>(p.first);
	},
	[](const detection_pair &p) -> size_t
	{
		return reinterpret_cast<size_t>(p.first);
	});

	source_node<image *> src(g, [](image* &next_image) -> bool
	{
		next_image = get_next_image();
		return (next_image != nullptr);
	});

	make_edge(src, input_port<0>(resource_join));
	make_edge(buffers, input_port<1>(resource_join));

	function_node<resource_tuple, image *> preprocess_function(g, unlimited,
	                                                           [](const resource_tuple &in) -> image *
	{
		image *input_image = std::get<0>(in);
		image *output_image = std::get<1>(in);
		preprocess_image(input_image, output_image);

		if (input_image != nullptr) {
			delete input_image;
			printf("Delete image! (%s)\n", __func__);
		}

		return output_image;
	});

	make_edge(resource_join, preprocess_function);

	function_node<image *, detection_pair> detect_A(g, unlimited,
	                                                [](image *input_image) -> detection_pair
	{
		bool r = detect_with_A(input_image);
		return std::make_pair(input_image, r);
	});

	function_node<image *, detection_pair> detect_B(g, unlimited,
	                                                [](image *input_image) -> detection_pair
	{
		bool r = detect_with_B(input_image);
		return std::make_pair(input_image, r);
	});

	make_edge(preprocess_function, detect_A);
	make_edge(detect_A, input_port<0>(detection_join));
	make_edge(preprocess_function, detect_B);
	make_edge(detect_B, input_port<1>(detection_join));

	function_node<detection_tuple, image *> decide(g, serial,
	                                               [](const detection_tuple &t) -> image *
	{
		const detection_pair &a = std::get<0>(t);
		const detection_pair &b = std::get<1>(t);
		image *img = a.first;

		if (a.second || b.second) {
			output_image(img, a.second, b.second);
		}

		return img;
	});

	make_edge(detection_join, decide);
	make_edge(decide, buffers);

	// Put image buffers into the buffer queue
	for (int i = 0; i < num_graph_buffers; ++i) {
		image *img = new image;
		buffers.try_put(img);
	}

	g.wait_for_all();

	for (int i = 0; i < num_graph_buffers; ++i) {
		image *img = nullptr;

		if (!buffers.try_get(img)) {
			printf("ERROR: lost a buffer\n");
			continue;
		}

		printf("Delete image!\n");
		delete img;
	}
#endif
}
