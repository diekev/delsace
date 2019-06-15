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

namespace filesystem = std::experimental::filesystem;

static auto formatte(int nombre)
{
	auto resultat_tmp = std::to_string(nombre);

	const auto taille = resultat_tmp.size();

	if (taille <= 3) {
		return resultat_tmp;
	}

	const auto reste = taille % 3;
	std::string resultat = std::string{""};
	resultat.reserve(taille + taille / 3);

	for (auto i = 0ul; i < reste; ++i) {
		resultat.push_back(resultat_tmp[i]);
	}

	for (auto i = reste; i < taille; i += 3) {
		if (reste != 0 || i != reste) {
			resultat.push_back(' ');
		}

		resultat.push_back(resultat_tmp[i + 0]);
		resultat.push_back(resultat_tmp[i + 1]);
		resultat.push_back(resultat_tmp[i + 2]);
	}

	return resultat;
}

static auto commence_par(const std::string &ligne, const std::string &motif)
{
	if (motif.size() > ligne.size()) {
		return false;
	}

	auto index = 0ul;

	/* Évite les espaces blancs. */
	while (index < ligne.size() && (ligne[index] == '\t' || ligne[index] == ' ')) {
		++index;
	}

	return ligne.substr(index, motif.size()) == motif;
}

static auto fini_par(const std::string &ligne, const std::string &motif)
{
	if (motif.size() > ligne.size()) {
		return false;
	}

	return ligne.find(motif) != std::string::npos;

//	auto index = ligne.size() - 1;

//	/* Évite les espaces blancs. */
//	while (index >= 0 && (ligne[index] == '\t' || ligne[index] == ' ')) {
//		--index;
//	}

//	return ligne.substr((index + 1) - motif.size(), motif.size()) == motif;
}

static auto compte_lignes(std::istream &is)
{
	auto nombre_lignes = 0;
	auto nombre_commentaires = 0;

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
			continue;
		}

		if (commence_par(ligne, "/*")) {
			if (!fini_par(ligne, "*/")) {
				commentaire_c = true;
			}

			nombre_commentaires++;
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
