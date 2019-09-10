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

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/systeme_fichier/file.h"

#include "noeud.hh"
#include "noeud_image.h"

/* Adapted from Blender's BVM debug code. */

static constexpr auto fontname = "helvetica";
static constexpr auto fontsize = 20.0;
static constexpr auto node_label_size = 14.0;
static constexpr auto color_value = "gold1";

dls::chaine node_id(const Noeud *node, bool quoted)
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
	for (auto const &input : socket->parent->entrees) {
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
	for (auto const &output : socket->parent->sorties) {
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

inline void dump_node(dls::flux_chaine &flux, Noeud *node)
{
	constexpr auto shape = "box";
	constexpr auto style = "filled,rounded";
	constexpr auto color = "black";
	constexpr auto fillcolor = "gainsboro";
	constexpr auto penwidth = 1.0;

	flux << "// " << node->nom << "\n";
	flux << node_id(node);
	flux << "[";

	flux << "label=<<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"4\">";
	flux << "<TR><TD COLSPAN=\"2\">" << node->nom << "</TD></TR>";

	auto const numin = node->entrees.taille();
	auto const numout = node->sorties.taille();

	for (auto i = 0; (i < numin) || (i < numout); ++i) {
		flux << "<TR>";

		if (i < numin) {
			auto const &input = node->entree(i);
			auto const &name_in = input->nom;

			flux << "<TD";
			flux << " PORT=" << input_id(input, i);
			flux << " BORDER=\"1\"";
			flux << " BGCOLOR=\"" << color_value << "\"";
			flux << ">";
			flux << name_in;
			flux << "</TD>";
		}
		else {
			flux << "<TD></TD>";
		}

		if (i < numout) {
			auto const &output = node->sortie(i);
			auto const &name_out = output->nom;

			flux << "<TD";
			flux << " PORT=" << output_id(output, i);
			flux << " BORDER=\"1\"";
			flux << " BGCOLOR=\"" << color_value << "\"";
			flux << ">";
			flux << name_out;
			flux << "</TD>";
		}
		else {
			flux << "<TD></TD>";
		}

		flux << "</TR>";
	}

	flux << "</TABLE>>";

	flux << ",fontname=\"" << fontname << "\"";
	flux << ",fontsize=\"" << node_label_size << "\"";
	flux << ",shape=\"" << shape << "\"";
	flux << ",style=\"" << style << "\"";
	flux << ",color=\"" << color << "\"";
	flux << ",fillcolor=\"" << fillcolor << "\"";
	flux << ",penwidth=\"" << penwidth << "\"";
	flux << "];\n";
	flux << "\n";
}

inline void dump_link(dls::flux_chaine &flux, const PriseSortie *from, const PriseEntree *to)
{
	auto penwidth = 2.0;

	flux << node_id(from->parent) << ':' << output_id(from, -1l);
	flux << " -> ";
	flux << node_id(to->parent) << ':' << input_id(to, -1l);
	flux << "[";
	/* Note: without label an id seem necessary to avoid bugs in graphviz/dot */
	flux << "id=\"VAL" << node_id(to->parent, false) << ':' << input_id(to, -1l, false) << '"';
	flux << ",penwidth=\"" << penwidth << '"';
	flux << "];\n";
	flux << '\n';
}

inline void dump_node_links(dls::flux_chaine &flux, const Noeud *node)
{
	for (auto const &entree : node->entrees) {
		for (auto const &sortie : entree->liens) {
			dump_link(flux, sortie, entree);
		}
	}
}

dls::chaine chaine_graphe_dot(Graphe const &graphe)
{
	auto flux = dls::flux_chaine();

	flux << "digraph depgraph {\n";
	flux << "rankdir=LR\n";
	flux << "graph [";
	flux << "labbelloc=\"t\"";
	flux << ",fontsize=\"" << fontsize << "\"";
	flux << ",fontname=\"" << fontname << "\"";
	flux << ",label=\"Image Graph\"";
	flux << "]\n";

	for (auto const &node : graphe.noeuds()) {
		dump_node(flux, node);
	}

	for (auto const &node : graphe.noeuds()) {
		dump_node_links(flux, node);
	}

	flux << "}\n";

	return flux.chn();
}

ImprimeuseGraphe::ImprimeuseGraphe(Graphe *graph)
    : m_graph(graph)
{}

void ImprimeuseGraphe::operator()(filesystem::path const &path)
{
	auto fichier = dls::systeme_fichier::File(path.c_str(), "w" );

	if (!fichier) {
		return;
	}

	auto chn = chaine_graphe_dot(*m_graph);

	fichier.print("%s", chn.c_str());
}
