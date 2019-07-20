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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "layout.h"

#include <cmath>
#include <limits>
#include "biblinternes/structures/tableau.hh"


/* http://www.graphviz.org/Documentation/TSE93.pdf */

class Node {
public:
	int width() { return 5; }
	int height() { return 5; }

	float rank;
};

class Edge {
public:
	Node *head;
	Node *v2;
};

int width(Node *node)
{
	return node->width();
}

int height(Node *node)
{
	return node->height();
}

/* minimum separation between nodes */
int node_sep()
{
	return 50;
}

/* weight (importance) of an egde, translates to keeping the edge short and aligned */
float omega()
{
	return 1.0f;
}

void rank();
std::size_t ordering();

static void position()
{
}

static void make_splines()
{
}

void draw_graph()
{
	/* place the nodes in discrete ranks */
	rank();

	/* set order of the nodes within ranks to avoid edge crossing */
	ordering();

	/* set layout coordinates of the nodes */
	position();

	/* find the spline control points for edges */
	make_splines();
}

void feasible_tree();

void normalize()
{
}

void balance()
{
}

Edge *leaf_edge()
{
	return nullptr;
}

Edge *enter_edge(Edge *e)
{
	return e;
}

void exchange(Edge *e, Edge *f)
{
	auto tmp = e;
	e = f;
	f = tmp;
}

void rank()
{
	/* construct a feasible spanning tree */
	feasible_tree();

	Edge *e;
	while ((e = leaf_edge()) != nullptr) {
		/* find a non-tree edge to replace e */
		auto f = enter_edge(e);
		exchange(e, f);
	}

	normalize();
	balance();
}

void init_rank()
{
}

std::size_t tight_tree()
{
	return 0;
}

float slack(Edge */*e*/)
{
	return 0.0f;
}

void init_cutvalues()
{
}

void feasible_tree()
{
	init_rank();

	dls::tableau<Node *> nodes;
	Node *incident_node;

	while (tight_tree() < nodes.taille()) {
		Edge *e = nullptr; // todo
		auto delta = slack(e);

		if (e->head == incident_node) {
			delta = -delta;
		}

		for (auto &node : nodes) {
			node->rank += delta;
		}
	}

	init_cutvalues();
}

std::size_t init_order()
{
	return 0;
}

void wmedian(std::size_t /*order*/, std::size_t /*i*/)
{

}

void transpose(std::size_t /*order*/)
{

}

std::size_t crossing(std::size_t /*order*/)
{
	return 0;
}

std::size_t ordering()
{
	auto order = init_order();
	auto best = order;
	auto max_iter = 5;

	for (int i = 0; i < max_iter; ++i) {
		wmedian(order, static_cast<size_t>(i));
		transpose(order);

		if (crossing(order) < crossing(best)) {
			best = order;
		}
	}

	return best;
}

#if 0

/* algorithm from http://yifanhu.net/PUB/graph_draw_small.pdf
 * "Efficient and High Quality Force-Directed Graph Drawing"
 */

float update_steplength(float step, float energy, float energy0, const float t);

template <typename Graph>
float force_directed_graph(Graph &graph, float x, float tolerance)
{
	auto converged = false;
	auto step = 5.0f; // initial_step_length
	auto energy = std::numeric_limits<float>::max();

	while (!converged) {
		auto x0 = x; // pos
		auto energy0 = energy;
		energy = 0.0f;

		for (auto &vertex : graph.vertices()) {
			auto f = 0.0f;
			auto xi = pos(vertex);

			/* todo */

			// for every edge v0 <-> v1:
			f += fa(v0, v1) / length(pos(v1) - pos(v0)) * (pos(v1) - pos(v0));

			xi += step * (f / std::abs(f));
			energy += std::abs(f) * std::abs(f);
		}

		step = update_steplength(step, energy, energy0);
		converged = (length(x - x0) < tolerance);
	}

	return x;
}

float update_steplength(float step, float energy, float energy0, const float t = 0.9f)
{
	if (energy < energy0) {
		++progress;

		if (progress >= 5) {
			progress = 0;
			step = step / t;
		}
	}
	else {
		progress = 0;
		step = step * t;
	}

	return step;
}
#endif
