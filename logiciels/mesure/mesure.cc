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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <fstream>
#include <iostream>
#include <experimental/filesystem>

#include "biblinternes/chrono/chronometre_de_portee.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/chaine.hh"

namespace filesystem = std::experimental::filesystem;

static auto formatte(int nombre)
{
	auto resultat_tmp = dls::chaine(std::to_string(nombre));

	const auto taille = resultat_tmp.taille();

	if (taille <= 3) {
		return resultat_tmp;
	}

	const auto reste = taille % 3;
	auto resultat = dls::chaine{""};
	resultat.reserve(taille + taille / 3);

	for (auto i = 0l; i < reste; ++i) {
		resultat.pousse(resultat_tmp[i]);
	}

	for (auto i = reste; i < taille; i += 3) {
		if (reste != 0 || i != reste) {
			resultat.pousse(' ');
		}

		resultat.pousse(resultat_tmp[i + 0]);
		resultat.pousse(resultat_tmp[i + 1]);
		resultat.pousse(resultat_tmp[i + 2]);
	}

	return resultat;
}

static auto commence_par(dls::chaine const &ligne, dls::chaine const &motif)
{
	if (motif.taille() > ligne.taille()) {
		return false;
	}

	auto index = 0l;

	/* Évite les espaces blancs. */
	while (index < ligne.taille() && (ligne[index] == '\t' || ligne[index] == ' ')) {
		++index;
	}

	return ligne.sous_chaine(index, motif.taille()) == motif;
}

static auto fini_par(dls::chaine const &ligne, dls::chaine const &motif)
{
	if (motif.taille() > ligne.taille()) {
		return false;
	}

	return ligne.trouve(motif) != dls::chaine::npos;

//	auto index = ligne.taille() - 1;

//	/* Évite les espaces blancs. */
//	while (index >= 0 && (ligne[index] == '\t' || ligne[index] == ' ')) {
//		--index;
//	}

//	return ligne.substr((index + 1) - motif.taille(), motif.taille()) == motif;
}

/* Considère les lignes comme étant vides si elles ne contiennent que des
 * caractères blancs ou des séquences de début ou de fin d'instructions comme
 * }); à la fin des lambdas. */
static auto est_ligne_vide(dls::chaine const &ligne)
{
	for (auto c : ligne) {
		if (!dls::outils::est_element(c, ' ', '\t', '\n', '\r', '\v', '{', '}', '(', ')', '[', ']', ';')) {
			return false;
		}
	}

	return true;
}

static auto compte_lignes(std::istream &is)
{
	auto nombre_lignes = 0;
	auto nombre_commentaires = 0;
	auto nombre_inutiles = 0;

	std::string ligne;
	auto commentaire_c = false;

	while (std::getline(is, ligne)) {
		if (ligne.empty()) {
			continue;
		}

		if (commentaire_c) {
			if (fini_par(ligne, "*/")) {
				commentaire_c = false;
			}

			nombre_commentaires++;
			continue;
		}

		if (commence_par(ligne, "//")) {
			nombre_commentaires++;
			continue;
		}

		if (commence_par(ligne, "/*")) {
			if (!fini_par(ligne, "*/")) {
				commentaire_c = true;
			}

			nombre_commentaires++;
			continue;
		}

		if (est_ligne_vide(ligne)) {
			++nombre_inutiles;
			continue;
		}

		nombre_lignes++;
	}

	return std::pair<int, int>(nombre_lignes, nombre_commentaires);
}

enum {
	FICHIER_SOURCE,
	FICHIER_ENTETE,
	FICHIER_INCONNU,
};

static auto est_fichier_entete(filesystem::path const &extension)
{
	return dls::outils::est_element(extension, ".h", ".hh", ".hpp");
}

static auto est_fichier_source(filesystem::path const &extension)
{
	return dls::outils::est_element(extension, ".c", ".cc", ".cpp", ".tcc");
}

static auto type_fichier(filesystem::path const &chemin)
{
	auto const extension = chemin.extension();

	if (est_fichier_entete(extension)) {
		return FICHIER_ENTETE;
	}

	if (est_fichier_source(extension)) {
		return FICHIER_SOURCE;
	}

	return FICHIER_INCONNU;
}

int main()
{
	auto nombre_total_lignes = 0;
	auto nombre_total_lignes_source = 0;
	auto nombre_total_lignes_entete = 0;
	auto nombre_total_commentaires = 0;
	auto nombre_total_commentaires_source = 0;
	auto nombre_total_commentaires_entete = 0;
	auto nombre_fichiers = 0;

	std::ostream &os = std::cout;

	CHRONOMETRE_PORTEE("Temps d'exécution", os);

	for (const auto &entree : filesystem::recursive_directory_iterator(filesystem::current_path())) {
		const auto chemin_fichier = entree.path();

		if (filesystem::is_directory(chemin_fichier)) {
			continue;
		}

		auto type = type_fichier(chemin_fichier);

		if (type == FICHIER_INCONNU) {
			continue;
		}

		++nombre_fichiers;

		std::ifstream fichier;
		fichier.open(chemin_fichier.c_str());

		if (!fichier.is_open()) {
			continue;
		}

		auto const nombre_lignes = compte_lignes(fichier);
		nombre_total_lignes += nombre_lignes.first;
		nombre_total_commentaires += nombre_lignes.second;

		if (type == FICHIER_ENTETE) {
			nombre_total_lignes_entete += nombre_lignes.first;
			nombre_total_commentaires_entete += nombre_lignes.second;
		}
		else if (type == FICHIER_SOURCE) {
			nombre_total_lignes_source += nombre_lignes.first;
			nombre_total_commentaires_source += nombre_lignes.second;
		}
	}

	os << "Il y a " << formatte(nombre_total_lignes + nombre_total_commentaires)
	   << " lignes en tout, dans " << formatte(nombre_fichiers) << " fichiers.\n";
	os << '\n';
	os << "Fichiers sources :\n";
	os << "    - " << formatte(nombre_total_lignes_source) << " lignes de programmes\n";
	os << "    - " << formatte(nombre_total_commentaires_source) << " lignes de commentaires\n";
	os << "Ratio commentaires / lignes programmes : " << static_cast<double>(nombre_total_commentaires_source) / nombre_total_lignes_source << '\n';
	os << '\n';
	os << "Fichiers entêtes :\n";
	os << "    - " << formatte(nombre_total_lignes_entete) << " lignes de programmes\n";
	os << "    - " << formatte(nombre_total_commentaires_entete) << " lignes de commentaires\n";
	os << "Ratio commentaires / lignes programmes : " << static_cast<double>(nombre_total_commentaires_entete) / nombre_total_lignes_entete << '\n';
	os << '\n';
	os << "Total :\n";
	os << "    - " << formatte(nombre_total_lignes) << " lignes de programmes\n";
	os << "    - " << formatte(nombre_total_commentaires) << " lignes de commentaires\n";
	os << "Ratio commentaires / lignes programmes : " << static_cast<double>(nombre_total_commentaires) / nombre_total_lignes << '\n';
	os << '\n';
}
