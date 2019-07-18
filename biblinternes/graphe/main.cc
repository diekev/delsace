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

#include "dataflow.h"
#include "graph.h"

#include "../../biblexternes/docopt/docopt.hh"

static const char usage[] = R"(
Théorie des graphes.

Usage:
    graph_theory --graphe
    graph_theory --planification
    graph_theory (-a | --aide)
    graph_theory --version

Options:
    -a, --aide       Affiche cette écran.
    --version        Montre la version du logiciel.
    --graphe         Lance le test du graphe générique.
    --planification  Lance le test du planificateur.
)";

static void test_graph(std::ostream &os)
{
	Graph g;
	Vertex v0("v0"), v1("v1"), v2("v2"), v3("v3");

	g.addVertex({ &v0, &v1, &v2, &v3 });
	g.addEdge(&v0, &v1);
	g.addEdge(&v1, &v2);
	g.addEdge(&v1, &v3);
	g.addEdge(&v0, &v3);

	g.walk(os);
	g.walk_edges(os);
}

int main(int argc, char *argv[])
{
	auto args = dls::docopt::docopt(usage, { argv + 1, argv + argc }, true, "Théorie des Graphes 0.1");

	const auto do_graphe = dls::docopt::get_bool(args, "--graphe");
	const auto do_plan = dls::docopt::get_bool(args, "--planification");

	if (do_graphe) {
		test_graph(std::cerr);
	}
	else if (do_plan) {
		test_planification();
	}

	return 0;
}
