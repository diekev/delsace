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

#include "graph_dumper.h"

#include <systeme_fichier/file.h>

#include <iomanip>
#include <sstream>

#include "depsgraph.h"
#include "object_graph.h"
#include "object.h"

/* Adapted from Blender's BVM debug code. */

static constexpr auto fontname = "helvetica";
static constexpr auto fontsize = 20.0;
static constexpr auto node_label_size = 14.0;
static constexpr auto color_value = "gold1";

inline static std::string node_id(const Noeud *noeud, bool quoted = true)
{
	std::stringstream ss;

	ss << "node_" << noeud;

	if (quoted) {
		std::stringstream ssq;
		ssq << std::quoted(ss.str());
		return ssq.str();
	}

	return ss.str();
}

inline size_t get_input_index(const PriseEntree *prise)
{
	auto i = 0ul;
	for (auto const &entree : prise->parent->entrees()) {
		if (entree->nom == prise->nom) {
			return i;
		}

		++i;
	}

	return -1ul;
}

inline size_t get_output_index(const PriseSortie *prise)
{
	auto i = 0ul;
	for (auto const &sortie : prise->parent->sorties()) {
		if (sortie->nom == prise->nom) {
			return i;
		}

		++i;
	}

	return -1ul;
}

inline static std::string input_id(const PriseEntree *prise, size_t index, bool quoted = true)
{
	if (index == -1ul) {
		index = get_input_index(prise);
	}

	std::stringstream ss;

	ss << "I" << prise->nom << "_" << index;

	if (quoted) {
		std::stringstream ssq;
		ssq << std::quoted(ss.str());
		return ssq.str();
	}

	return ss.str();
}

inline static std::string output_id(const PriseSortie *sortie, size_t index, bool quoted = true)
{
	if (index == -1ul) {
		index = get_output_index(sortie);
	}

	std::stringstream ss;

	ss << "O" << sortie->nom << "_" << index;

	if (quoted) {
		std::stringstream ssq;
		ssq << std::quoted(ss.str());
		return ssq.str();
	}

	return ss.str();
}

inline void dump_node(numero7::systeme_fichier::File &file, Noeud *noeud)
{
	constexpr auto shape = "box";
	constexpr auto style = "filled,rounded";
	constexpr auto color = "black";
	constexpr auto fillcolor = "gainsboro";
	auto penwidth = 1.0;

	file.print("// %s\n", noeud->nom().c_str());
	file.print("%s", node_id(noeud).c_str());
	file.print("[");

	file.print("label=<<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"4\">");
	file.print("<TR><TD COLSPAN=\"2\">%s</TD></TR>", noeud->nom().c_str());

	auto const numin = noeud->entrees().size();
	auto const numout = noeud->sorties().size();

	for (size_t i = 0; (i < numin) || (i < numout); ++i) {
		file.print("<TR>");

		if (i < numin) {
			auto const &input = noeud->entree(i);
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
			auto const &output = noeud->sortie(i);
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
	file.print(",fontsize=\"%f\"", static_cast<double>(node_label_size));
	file.print(",shape=\"%s\"", shape);
	file.print(",style=\"%s\"", style);
	file.print(",color=\"%s\"", color);
	file.print(",fillcolor=\"%s\"", fillcolor);
	file.print(",penwidth=\"%f\"", penwidth);
	file.print("];\n");
	file.print("\n");
}

inline void dump_link(numero7::systeme_fichier::File &file, const PriseSortie *de, const PriseEntree *a)
{
	auto penwidth = 2.0;

	file.print("%s:%s -> %s:%s",
			   node_id(de->parent).c_str(), output_id(de, -1ul).c_str(),
			   node_id(a->parent).c_str(), input_id(a, -1ul).c_str());

	file.print("[");

	/* Note: without label an id seem necessary to avoid bugs in graphviz/dot */
	file.print("id=\"VAL%s:%s\"", node_id(a->parent, false).c_str(), input_id(a, -1ul, false).c_str());
	file.print(",penwidth=\"%f\"", penwidth);

	file.print("];\n");
	file.print("\n");
}

inline void dump_node_links(numero7::systeme_fichier::File &file, const Noeud *noeud)
{
	for (auto const &entree : noeud->entrees()) {
		if (entree->lien) {
			dump_link(file, entree->lien, entree);
		}
	}
}

GraphDumper::GraphDumper(Graph *graph)
    : m_graph(graph)
{}

void GraphDumper::operator()(const std::experimental::filesystem::path &path)
{
	numero7::systeme_fichier::File file(path, "w");

	if (!file) {
		return;
	}

	file.print("digraph depgraph {\n");
	file.print("rankdir=LR\n");
	file.print("graph [");
	file.print("labbelloc=\"t\"");
	file.print(",fontsize=\"%f\"", fontsize);
	file.print("fontname=\"%s\"", fontname);
	file.print("label=\"Object Graph\"");
	file.print("]\n");

	for (auto const &noeud : m_graph->noeuds()) {
		dump_node(file, noeud.get());
	}

	for (auto const &noeud : m_graph->noeuds()) {
		dump_node_links(file, noeud.get());
	}

	file.print("}\n");
}

/* ************************************************************************** */

#define kmkz_inline static inline

kmkz_inline std::string node_id(const DepsNode *node, bool quoted = true)
{
	std::stringstream ss;

	ss << "node_" << node;

	if (quoted) {
		std::stringstream ssq;
		ssq << std::quoted(ss.str());
		return ssq.str();
	}

	return ss.str();
}

kmkz_inline std::string input_id(const DepsInputSocket */*socket*/, bool quoted = true)
{
	std::stringstream ss;

	ss << "IParent";

	if (quoted) {
		std::stringstream ssq;
		ssq << std::quoted(ss.str());
		return ssq.str();
	}

	return ss.str();
}

kmkz_inline std::string output_id(const DepsOutputSocket */*socket*/, bool quoted = true)
{
	std::stringstream ss;

	ss << "OChild";

	if (quoted) {
		std::stringstream ssq;
		ssq << std::quoted(ss.str());
		return ssq.str();
	}

	return ss.str();
}

kmkz_inline void dump_node(numero7::systeme_fichier::File &file, DepsNode *node)
{
	constexpr auto shape = "box";
	constexpr auto style = "filled,rounded";
	constexpr auto color = "black";
	constexpr auto fillcolor = "gainsboro";
	auto penwidth = 1.0;

	auto const ob_name = node->name();

	file.print("// %s\n", ob_name);
	file.print("%s", node_id(node).c_str());
	file.print("[");

	file.print("label=<<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"4\">");
	file.print("<TR><TD COLSPAN=\"2\">%s</TD></TR>", ob_name);

	file.print("<TR>");

	auto const &input = node->input();
	auto const &name_in = "Parent";

	file.print("<TD");
	file.print(" PORT=%s", input_id(input).c_str());
	file.print(" BORDER=\"1\"");
	file.print(" BGCOLOR=\"%s\"", color_value);
	file.print(">");
	file.print("%s", name_in);
	file.print("</TD>");

	auto const &output = node->output();
	auto const &name_out = "Child";

	file.print("<TD");
	file.print(" PORT=%s", output_id(output).c_str());
	file.print(" BORDER=\"1\"");
	file.print(" BGCOLOR=\"%s\"", color_value);
	file.print(">");
	file.print("%s", name_out);
	file.print("</TD>");

	file.print("</TR>");

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

kmkz_inline void dump_link(numero7::systeme_fichier::File &file,
                           const DepsOutputSocket *from,
                           const DepsInputSocket *to)
{
	auto penwidth = 2.0;

	file.print("%s:%s -> %s:%s",
	           node_id(from->parent).c_str(), output_id(from).c_str(),
	           node_id(to->parent).c_str(), input_id(to).c_str());

	file.print("[");

	/* Note: without label an id seem necessary to avoid bugs in graphviz/dot */
	file.print("id=\"VAL%s:%s\"", node_id(to->parent, false).c_str(), input_id(to, false).c_str());
	file.print(",penwidth=\"%f\"", penwidth);

	file.print("];\n");
	file.print("\n");
}

kmkz_inline void dump_node_links(numero7::systeme_fichier::File &file, const DepsNode *node)
{
	for (auto const &output : node->input()->links) {
		dump_link(file, output, node->input());
	}
}

DepsGraphDumper::DepsGraphDumper(Depsgraph *graph)
    : m_graph(graph)
{}

void DepsGraphDumper::operator()(const std::experimental::filesystem::path &path)
{
	numero7::systeme_fichier::File file(path, "w");

	if (!file) {
		return;
	}

	file.print("digraph depgraph {\n");
	file.print("rankdir=LR\n");
	file.print("graph [");
	file.print("labbelloc=\"t\"");
	file.print(",fontsize=\"%f\"", fontsize);
	file.print("fontname=\"%s\"", fontname);
	file.print("label=\"Dependency Graph\"");
	file.print("]\n");

	for (auto const &node : m_graph->nodes()) {
		dump_node(file, node.get());
	}

	for (auto const &node : m_graph->nodes()) {
		dump_node_links(file, node.get());
	}

	file.print("}\n");
}
