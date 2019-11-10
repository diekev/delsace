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

#include "biblinternes/langage/tampon_source.hh"
#include "biblinternes/outils/fichier.hh"

#include "danjo/compilation/decoupeuse.h"
#include "danjo/compilation/analyseuse_disposition.h"

#include "danjo/erreur.h"

static auto verifie_fichier(std::filesystem::path const &chemin)
{
	if (chemin.extension() != ".jo") {
		return;
	}

	try {
		auto texte = dls::contenu_fichier(chemin.c_str());

		auto tampon = lng::tampon_source(texte.c_str());
		auto decoupeuse = danjo::Decoupeuse(tampon);
		decoupeuse.decoupe();

		auto debut = decoupeuse.morceaux().debut();
		auto fin = decoupeuse.morceaux().fin();

		std::cerr << "Analyse du fichier " << chemin << "...\n";

		auto valideuse = danjo::valideuse_propriete{};

		while (debut++ != fin) {
			if (!danjo::est_identifiant_controle(debut->identifiant)) {
				continue;
			}

			auto id_controle = debut->identifiant;

			if (!valideuse.cherche_magasin(id_controle)) {
				std::cerr << "Impossible de trouver le magasin pour '"
						  << danjo::chaine_identifiant(id_controle)
						  << "'\n";
				break;
			}

			/* Saute id controle. */
			++debut;

			while (debut++ != fin) {
				if (debut->identifiant == danjo::id_morceau::PARENTHESE_FERMANTE) {
					break;
				}

				if (!danjo::est_identifiant_propriete(debut->identifiant)) {
					continue;
				}

				auto id_propriete = debut->identifiant;

				if (!valideuse.est_propriete_valide(id_propriete)) {
					std::cerr << "'Attention : propriété '"
							  << chaine_identifiant(id_propriete)
							  << "' inutile pour type '"
							  << chaine_identifiant(id_controle)
							  << "'\n";
				}
			}
		}
	}
	catch (const danjo::ErreurFrappe &e) {
		std::cerr << e.quoi();
	}
}

static auto verifie_dossier(std::filesystem::path const &chemin)
{
	for (auto iter : std::filesystem::directory_iterator(chemin)) {
		if (!std::filesystem::is_regular_file(iter.path())) {
			continue;
		}

		verifie_fichier(iter.path());
	}
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		std::cerr << "Utilisation : " << argv[0] << " CHEMIN\n";
		return 1;
	}

	auto chemin = std::filesystem::path(argv[1]);

	if (std::filesystem::is_directory(chemin)) {
		verifie_dossier(chemin);
	}
	else if (std::filesystem::is_regular_file(chemin)) {
		verifie_fichier(chemin);
	}
	else {
		std::cerr << "Type de fichier inconnu !\n";
		return 1;
	}

	return 0;
}
