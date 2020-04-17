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

#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/matrice_eparse.hh"

#include "compilation/contexte_generation_code.h"
#include "compilation/lexeuse.hh"
#include "compilation/erreur.h"
#include "compilation/modules.hh"
#include "compilation/outils_lexemes.hh"

#include "options.hh"

using type_scalaire = double;
using type_matrice_ep = matrice_colonne_eparse<double>;

static auto idx_depuis_id(GenreLexeme id)
{
	return static_cast<int>(id);
}

static auto id_depuis_idx(int id)
{
	return static_cast<GenreLexeme>(id);
}

void test_markov_id_simple(dls::tableau<Lexeme> const &lexemes)
{
	static constexpr auto _0 = static_cast<type_scalaire>(0);
	static constexpr auto _1 = static_cast<type_scalaire>(1);

	/* construction de la matrice */
	auto nombre_id = static_cast<int>(GenreLexeme::COMMENTAIRE) + 1;
	auto matrice = type_matrice_ep(type_ligne(nombre_id), type_colonne(nombre_id));

	for (auto i = 0; i < lexemes.taille() - 1; ++i) {
		auto idx0 = idx_depuis_id(lexemes[i].genre);
		auto idx1 = idx_depuis_id(lexemes[i + 1].genre);

		matrice(type_ligne(idx0), type_colonne(idx1)) += _1;
	}

	tri_lignes_matrice(matrice);
	converti_fonction_repartition(matrice);

	auto gna = GNA();
	auto mot_courant = GenreLexeme::STRUCT;
	auto nombre_phrases = 5;

	while (nombre_phrases > 0) {
		std::cerr << chaine_du_lexeme(mot_courant);

		if (est_mot_cle(mot_courant)) {
			std::cerr << ' ';
		}

		if (mot_courant == GenreLexeme::ACCOLADE_FERMANTE) {
			std::cerr << '\n';
			nombre_phrases--;
		}

		/* prend le vecteur du mot_courant */
		auto idx = idx_depuis_id(mot_courant);

		/* génère un mot */
		auto prob = gna.uniforme(_0, _1);

		auto &ligne = matrice.lignes[idx];

		for (auto n : ligne) {
			if (prob <= n->valeur) {
				mot_courant = id_depuis_idx(static_cast<int>(n->colonne));
				break;
			}
		}
	}
}

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

	try {
		auto chemin = std::filesystem::path(chemin_fichier);

		if (chemin.is_relative()) {
			chemin = std::filesystem::absolute(chemin);
		}

		auto contexte = ContexteGenerationCode{};
		auto tampon = charge_fichier(chemin.c_str(), contexte, {});
		auto fichier = contexte.cree_fichier("", chemin.c_str());
		fichier->tampon = lng::tampon_source(tampon);

		auto lexeuse = Lexeuse(contexte, fichier);
		lexeuse.performe_lexage();

		test_markov_id_simple(fichier->lexemes);
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
	}

	return 0;
}
