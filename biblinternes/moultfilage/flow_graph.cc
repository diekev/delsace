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

#include "flow_graph.hh"

void test_flow_graph()
{
	using namespace tbb::flow;

	int result = 0;

	graph g;

	broadcast_node<int> input(g);

	function_node<int, int> squarer(g, unlimited, square());
	function_node<int, int> cuber(g, unlimited, cube());

    join_node<std::tuple<int, int>, queueing> join(g);
    function_node<std::tuple<int, int>, int> summer(g, serial, sum(result));

	make_edge(input, squarer);
	make_edge(input, cuber);

	make_edge(squarer, get<0>(join.input_ports()));
	make_edge(cuber,   get<1>(join.input_ports()));
	make_edge(join, summer);

	for (int i = 1; i <= 10; ++i) {
		input.try_put(i);
	}

	g.wait_for_all();

	printf("Final result is %d\n", result);

#if 0
	continue_node<continue_msg> a(g, body("A"));
	continue_node<continue_msg> b(g, body("B"));
	continue_node<continue_msg> c(g, body("C"));
	continue_node<continue_msg> d(g, body("D"));
	continue_node<continue_msg> e(g, body("E"));

	make_edge(input, a);
	make_edge(input, b);
	make_edge(a, c);
	make_edge(b, c);
	make_edge(c, d);
	make_edge(a, e);
#endif
}
