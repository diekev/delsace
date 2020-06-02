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

#include <filesystem>
#include <iostream>

#include "compilation/compilatrice.hh"
#include "compilation/lexeuse.hh"
#include "compilation/erreur.h"
#include "compilation/modules.hh"
#include "compilation/outils_lexemes.hh"

#include "options.hh"

int main(int argc, char **argv)
{
	std::ios::sync_with_stdio(false);

	if (argc < 2) {
		std::cerr << "Utilisation : " << argv[0] << " FICHIER\n";
		return 1;
	}

	auto const &chemin_racine_kuri = getenv("RACINE_KURI");

	if (chemin_racine_kuri == nullptr) {
		std::cerr << "Impossible de trouver le chemin racine de l'installation de kuri !\n";
		std::cerr << "Possible solution : veuillez faire en sorte que la variable d'environnement 'RACINE_KURI' soit définie !\n";
		return 1;
	}

	auto const chemin_fichier = argv[1];

	if (chemin_fichier == nullptr) {
		std::cerr << "Aucun fichier spécifié !\n";
		return 1;
	}

	if (!std::filesystem::exists(chemin_fichier)) {
		std::cerr << "Impossible d'ouvrir le fichier : " << chemin_fichier << '\n';
		return 1;
	}

	std::ostream &os = std::cout;

	try {
		auto chemin = std::filesystem::path(chemin_fichier);

		if (chemin.is_relative()) {
			chemin = std::filesystem::absolute(chemin);
		}

		auto compilatrice = Compilatrice{};
		auto tampon = charge_fichier(chemin.c_str(), compilatrice, {});
		auto fichier = compilatrice.cree_fichier("", chemin.c_str());
		fichier->tampon = lng::tampon_source(tampon);

		auto lexeuse = Lexeuse(compilatrice, fichier, INCLUS_CARACTERES_BLANC | INCLUS_COMMENTAIRES);
		lexeuse.performe_lexage();

		for (auto const &lexeme : fichier->lexemes) {
			if (est_mot_cle(lexeme.genre)) {
				os << "<span class=mot-cle>" << lexeme.chaine << "</span>";
			}
			else if (est_chaine_litterale(lexeme.genre)) {
				os << "<span class=chn-lit>" << lexeme.chaine << "</span>";
			}
			else if (lexeme.genre == GenreLexeme::COMMENTAIRE) {
				os << "<span class=comment>" << lexeme.chaine << "</span>";
			}
			else {
				os << lexeme.chaine;
			}
		}
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
	}

	return 0;
}
