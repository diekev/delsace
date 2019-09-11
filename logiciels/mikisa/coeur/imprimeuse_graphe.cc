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

static constexpr auto nom_police = "helvetica";
static constexpr auto taille_police = 20.0;
static constexpr auto taille_lable_noeud = 14.0;
static constexpr auto valeur_couleur = "gold1";

dls::chaine id_dot_pour_noeud(Noeud const *noeud, bool quoted)
{
	dls::flux_chaine ss;

	ss << "noeud_" << noeud;

	if (quoted) {
		dls::flux_chaine ssq;
		ssq << std::quoted(ss.chn());
		return ssq.chn();
	}

	return ss.chn();
}

inline long index_entree(PriseEntree const *prise)
{
	auto i = 0l;
	for (auto const &input : prise->parent->entrees) {
		if (input->nom == prise->nom) {
			return i;
		}

		++i;
	}

	return -1l;
}

inline long index_sortie(PriseSortie const *prise)
{
	auto i = 0l;
	for (auto const &output : prise->parent->sorties) {
		if (output->nom == prise->nom) {
			return i;
		}

		++i;
	}

	return -1l;
}

inline static dls::chaine id_dot_pour_entree(
		PriseEntree const *prise,
		long index,
		bool apostrophes = true)
{
	if (index == -1l) {
		index = index_entree(prise);
	}

	dls::flux_chaine ss;

	ss << "E" << prise->nom << "_" << index;

	if (apostrophes) {
		dls::flux_chaine ssq;
		ssq << std::quoted(ss.chn());
		return ssq.chn();
	}

	return ss.chn();
}

inline static dls::chaine id_dot_pour_sortie(
		PriseSortie const *prise,
		long index,
		bool apostrophes = true)
{
	if (index == -1l) {
		index = index_sortie(prise);
	}

	dls::flux_chaine ss;

	ss << "S" << prise->nom << "_" << index;

	if (apostrophes) {
		dls::flux_chaine ssq;
		ssq << std::quoted(ss.chn());
		return ssq.chn();
	}

	return ss.chn();
}

inline void imprime_noeud(dls::flux_chaine &flux, Noeud *noeud)
{
	constexpr auto forme = "box";
	constexpr auto style = "filled,rounded";
	constexpr auto couleur = "black";
	constexpr auto couleur_remplissage = "gainsboro";
	constexpr auto largeur_stylo = 1.0;

	flux << "// " << noeud->nom << "\n";
	flux << id_dot_pour_noeud(noeud);
	flux << "[";

	flux << "label=<<TABLE BORDER=\"0\" CELLBORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"4\">";
	flux << "<TR><TD COLSPAN=\"2\">" << noeud->nom << "</TD></TR>";

	auto const numin = noeud->entrees.taille();
	auto const numout = noeud->sorties.taille();

	for (auto i = 0; (i < numin) || (i < numout); ++i) {
		flux << "<TR>";

		if (i < numin) {
			auto const &input = noeud->entree(i);
			auto const &name_in = input->nom;

			flux << "<TD";
			flux << " PORT=" << id_dot_pour_entree(input, i);
			flux << " BORDER=\"1\"";
			flux << " BGCOLOR=\"" << valeur_couleur << "\"";
			flux << ">";
			flux << name_in;
			flux << "</TD>";
		}
		else {
			flux << "<TD></TD>";
		}

		if (i < numout) {
			auto const &output = noeud->sortie(i);
			auto const &name_out = output->nom;

			flux << "<TD";
			flux << " PORT=" << id_dot_pour_sortie(output, i);
			flux << " BORDER=\"1\"";
			flux << " BGCOLOR=\"" << valeur_couleur << "\"";
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

	flux << ",fontname=\"" << nom_police << "\"";
	flux << ",fontsize=\"" << taille_lable_noeud << "\"";
	flux << ",shape=\"" << forme << "\"";
	flux << ",style=\"" << style << "\"";
	flux << ",color=\"" << couleur << "\"";
	flux << ",fillcolor=\"" << couleur_remplissage << "\"";
	flux << ",penwidth=\"" << largeur_stylo << "\"";
	flux << "];\n";
	flux << "\n";
}

inline void imprime_lien(
		dls::flux_chaine &flux,
		PriseSortie const *sortie,
		PriseEntree const *entree)
{
	auto largeur_stylo = 2.0;

	flux << id_dot_pour_noeud(sortie->parent) << ':' << id_dot_pour_sortie(sortie, -1l);
	flux << " -> ";
	flux << id_dot_pour_noeud(entree->parent) << ':' << id_dot_pour_entree(entree, -1l);
	flux << "[";
	/* Note: graphviz/dot semble requérir soit un id, soit un label */
	flux << "id=\"VAL" << id_dot_pour_noeud(entree->parent, false) << ':' << id_dot_pour_entree(entree, -1l, false) << '"';
	flux << ",penwidth=\"" << largeur_stylo << '"';
	flux << "];\n";
	flux << '\n';
}

inline void imprime_liens_noeud(dls::flux_chaine &flux, Noeud const *noeud)
{
	for (auto const &entree : noeud->entrees) {
		for (auto const &sortie : entree->liens) {
			imprime_lien(flux, sortie, entree);
		}
	}
}

dls::chaine chaine_dot_pour_graphe(Graphe const &graphe)
{
	auto flux = dls::flux_chaine();

	flux << "digraph depgraph {\n";

	if (graphe.type == type_graphe::DETAIL) {
		flux << "rankdir=LR\n";
	}
	else {
		flux << "rankdir=BT\n";
	}

	flux << "nodesep=100\n";
	flux << "ranksep=1\n";
	flux << "graph [";
	flux << "labbelloc=\"t\"";
	flux << ",fontsize=\"" << taille_police << "\"";
	flux << ",fontname=\"" << nom_police << "\"";
	flux << ",label=\"Image Graph\"";
	flux << "]\n";

	for (auto const &noeud : graphe.noeuds()) {
		imprime_noeud(flux, noeud);
	}

	for (auto const &noeud : graphe.noeuds()) {
		imprime_liens_noeud(flux, noeud);
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

	auto chn = chaine_dot_pour_graphe(*m_graph);

	fichier.print("%s", chn.c_str());
}
