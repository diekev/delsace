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

#include "imprimeuse_graphe.h"

#include <iomanip>

#include <systeme_fichier/file.h>

#include "biblinternes/graphe/graphe.h"

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "noeud_image.h"

/* Adapted from Blender's BVM debug code. */

static constexpr auto fontname = "helvetica";
static constexpr auto fontsize = 20.0;
static constexpr auto node_label_size = 14.0;
static constexpr auto color_value = "gold1";

inline static dls::chaine node_id(const Noeud *node, bool quoted = true)
{
	dls::flux_chaine ss;

	ss << "node_" << node;

	if (quoted) {
		dls::flux_chaine ssq;
		ssq << std::quoted(ss.chn());
		return ssq.chn();
	}

	return ss.chn();
}

inline long get_input_index(const PriseEntree *socket)
{
	auto i = 0l;
	for (auto const &input : socket->parent->entrees()) {
		if (input->nom == socket->nom) {
			return i;
		}

		++i;
	}

	return -1l;
}

inline long get_output_index(const PriseSortie *socket)
{
	auto i = 0l;
	for (auto const &output : socket->parent->sorties()) {
		if (output->nom == socket->nom) {
			return i;
		}

		++i;
	}

	return -1l;
}

inline static dls::chaine input_id(const PriseEntree *socket, long index, bool quoted = true)
{
	if (index == -1l) {
		index = get_input_index(socket);
	}

	dls::flux_chaine ss;

	ss << "I" << socket->nom << "_" << index;

	if (quoted) {
		dls::flux_chaine ssq;
		ssq << std::quoted(ss.chn());
		return ssq.chn();
	}

	return ss.chn();
}

inline static dls::chaine output_id(const PriseSortie *socket, long index, bool quoted = true)
{
	if (index == -1l) {
		index = get_output_index(socket);
	}

	dls::flux_chaine ss;

	ss << "O" << socket->nom << "_" << index;

	if (quoted) {
		dls::flux_chaine ssq;
		ssq << std::quoted(ss.chn());
		return ssq.chn();
	}

	return ss.chn();
}

inline void dump_node(dls::systeme_fichier::File &file, Noeud *node)
{
	constexpr auto shape = "box";
	constexpr auto style = "filled,rounded";
	constexpr auto color = "black";
	constexpr auto fillcolor = "gainsboro";
	constexpr auto penwidth = 1.0;

	file.print("// %s\n", node->nom().c_str());
	file.print("%s", node_id(node).c_str());
	file.print("[");

	file.print("label=<<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"4\">");
	file.print("<TR><TD COLSPAN=\"2\">%s</TD></TR>", node->nom().c_str());

	auto const numin = static_cast<long>(node->entrees().size());
	auto const numout = static_cast<long>(node->sorties().size());

	for (auto i = 0; (i < numin) || (i < numout); ++i) {
		file.print("<TR>");

		if (i < numin) {
			auto const &input = node->entree(i);
			auto const &name_in = input->nom;

			file.print("<TD");
			file.print(" PORT=%s", input_id(input, i).c_str());
			file.print(" BORDER=\"1\"");
			file.print(" BGCOLOR=\"%s\"", color_value);
			file.print(">");
			file.print("%s", name_in.c_str());
			file.print("</TD>");
		}
		else {
			file.print("<TD></TD>");
		}

		if (i < numout) {
			auto const &output = node->sortie(i);
			auto const &name_out = output->nom;

			file.print("<TD");
			file.print(" PORT=%s", output_id(output, i).c_str());
			file.print(" BORDER=\"1\"");
			file.print(" BGCOLOR=\"%s\"", color_value);
			file.print(">");
			file.print("%s", name_out.c_str());
			file.print("</TD>");
		}
		else {
			file.print("<TD></TD>");
		}

		file.print("</TR>");
	}

	file.print("</TABLE>>");

	file.print(",fontname=\"%s\"", fontname);
	file.print(",fontsize=\"%f\"", node_label_size);
	file.print(",shape=\"%s\"", shape);
	file.print(",style=\"%s\"", style);
	file.print(",color=\"%s\"", color);
	file.print(",fillcolor=\"%s\"", fillcolor);
	file.print(",penwidth=\"%f\"", penwidth);
	file.print("];\n");
	file.print("\n");
}

inline void dump_link(dls::systeme_fichier::File &file, const PriseSortie *from, const PriseEntree *to)
{
	auto penwidth = 2.0;

	file.print("%s:%s -> %s:%s",
			   node_id(from->parent).c_str(), output_id(from, -1l).c_str(),
			   node_id(to->parent).c_str(), input_id(to, -1l).c_str());

	file.print("[");

	/* Note: without label an id seem necessary to avoid bugs in graphviz/dot */
	file.print("id=\"VAL%s:%s\"", node_id(to->parent, false).c_str(), input_id(to, -1l, false).c_str());
	file.print(",penwidth=\"%f\"", penwidth);

	file.print("];\n");
	file.print("\n");
}

inline void dump_node_links(dls::systeme_fichier::File &file, const Noeud *node)
{
	for (auto const &entree : node->entrees()) {
		for (auto const &sortie : entree->liens) {
			dump_link(file, sortie, entree);
		}
	}
}

ImprimeuseGraphe::ImprimeuseGraphe(Graphe *graph)
    : m_graph(graph)
{}

void ImprimeuseGraphe::operator()(filesystem::path const &path)
{
	dls::systeme_fichier::File file(path, "w");

	if (!file) {
		return;
	}

	file.print("digraph depgraph {\n");
	file.print("rankdir=LR\n");
	file.print("graph [");
	file.print("labbelloc=\"t\"");
	file.print(",fontsize=\"%f\"", fontsize);
	file.print("fontname=\"%s\"", fontname);
	file.print("label=\"Image Graph\"");
	file.print("]\n");

	for (auto const &node : m_graph->noeuds()) {
		dump_node(file, node);
	}

	for (auto const &node : m_graph->noeuds()) {
		dump_node_links(file, node);
	}

	file.print("}\n");
}
