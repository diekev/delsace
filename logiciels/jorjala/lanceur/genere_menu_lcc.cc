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

	auto nom_categories = dls::tableau<dls::chaine>();

	for (auto const &paire_df : lcc.fonctions.table_categories) {
		nom_categories.ajoute(paire_df.first);
	}

	std::sort(begin(nom_categories), end(nom_categories));

	os << "menu \"Noeud Détail\" {\n";
	os << "\tmenu \"Entrée\" {\n";
	os << "\t\taction(valeur=\"Charge Image\"; attache=ajouter_noeud_spécial_détail; métadonnée=\"Charge Image\")\n";
	os << "\t\taction(valeur=\"Cherche Caméra\"; attache=ajouter_noeud_spécial_détail; métadonnée=\"Cherche Caméra\")\n";
	os << "\t\taction(valeur=\"Crée Courbe Couleur\"; attache=ajouter_noeud_spécial_détail; métadonnée=\"Crée Courbe Couleur\")\n";
	os << "\t\taction(valeur=\"Crée Courbe Valeur\"; attache=ajouter_noeud_spécial_détail; métadonnée=\"Crée Courbe Valeur\")\n";
	os << "\t\taction(valeur=\"Crée Rampe Couleur\"; attache=ajouter_noeud_spécial_détail; métadonnée=\"Crée Rampe Couleur\")\n";
	os << "\t\taction(valeur=\"Entrée Détail\"; attache=ajouter_noeud_spécial_détail; métadonnée=\"Entrée Détail\")\n";
	os << "\t\taction(valeur=\"Entrée Attribut\"; attache=ajouter_noeud_spécial_détail; métadonnée=\"Entrée Attribut\")\n";
	os << "\t\taction(valeur=\"Info Exécution\"; attache=ajouter_noeud_spécial_détail; métadonnée=\"Info Exécution\")\n";
	os << "\t}\n";
	os << "\tmenu \"Sortie\" {\n";
	os << "\t\taction(valeur=\"Sortie Détail\"; attache=ajouter_noeud_spécial_détail; métadonnée=\"Sortie Détail\")\n";
	os << "\t\taction(valeur=\"Sortie Attribut\"; attache=ajouter_noeud_spécial_détail; métadonnée=\"Sortie Attribut\")\n";
	os << "\t}\n";

	for (auto const &categorie : nom_categories) {
		auto const &fonctions = lcc.fonctions.table_categories[categorie];

		os << "\tmenu \"" << categorie << "\" {\n";

		for (auto const &fonction : fonctions) {
			os << "\t\taction(valeur=\"";
			os << fonction;
			os << "\"; attache=ajouter_noeud_detail; métadonnée=\"";
			os << fonction;
			os << "\")\n";
		}
		os << "\t}\n";
	}

	os << "}\n";

	return 0;
}
