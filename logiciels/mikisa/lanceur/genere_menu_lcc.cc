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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "lcc/lcc.hh"

int main()
{
	std::ios::sync_with_stdio(false);
	auto &os = std::cout;

	auto lcc = lcc::LCC();
	lcc::initialise(lcc);

	auto nom_fonctions = dls::tableau<dls::chaine>();

	for (auto const &paire_df : lcc.fonctions.table) {
		nom_fonctions.pousse(paire_df.first);
	}

	std::sort(begin(nom_fonctions), end(nom_fonctions));

	os << "menu \"Noeud Détail\" {\n";
	for (auto const &nom : nom_fonctions) {
		os << "\taction(valeur=\"";
		os << nom;
		os << "\"; attache=ajouter_noeud_detail; métadonnée=\"";
		os << nom;
		os << "\")\n";
	}
	os << "}\n";

	return 0;
}
