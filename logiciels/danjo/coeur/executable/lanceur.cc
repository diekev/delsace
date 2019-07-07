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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include <cassert>
#include <fstream>
#include <iostream>

#include "biblinternes/langage/tampon_source.hh"

#include "danjo/compilation/decoupeuse.h"
#include "danjo/compilation/analyseuse_disposition.h"

#include "danjo/danjo.h"
#include "danjo/erreur.h"

static void imprime_morceaux(danjo::Decoupeuse::iterateur debut, danjo::Decoupeuse::iterateur fin)
{
	while (debut != fin) {
		std::cerr << chaine_identifiant(debut->identifiant) << " : " << debut->chaine << '\n';
		++debut;
	}
}

static void cree_fichier_dan(const std::experimental::filesystem::path &chemin)
{
	try {
		auto texte = danjo::contenu_fichier(chemin);

		auto tampon = lng::tampon_source(texte.c_str());
		auto decoupeuse = danjo::Decoupeuse(tampon);
		decoupeuse.decoupe();

		auto debut = decoupeuse.begin();
		auto fin = decoupeuse.end();

		if (debut->identifiant != danjo::id_morceau::DISPOSITION) {
			return;
		}

		auto chemin_dan = chemin;
		chemin_dan.replace_extension("dan");

		std::ofstream fichier;
		fichier.open(chemin_dan.c_str());

		std::ostream &os = fichier;

		++debut;

		auto nom_disposition = debut->chaine;

		os << "feuille \"" << nom_disposition << "\" {\n";
		os << "\tentreface {\n";

		auto nom_propriete = dls::chaine("");
		auto valeur_propriete = dls::chaine("");

		while (debut++ != fin) {
			if (!danjo::est_identifiant_controle(debut->identifiant)) {
				continue;
			}

			if (debut->identifiant == danjo::id_morceau::ETIQUETTE) {
				continue;
			}

			auto identifiant = debut->identifiant;

			nom_propriete.efface();
			valeur_propriete.efface();

			while (debut->identifiant != danjo::id_morceau::PARENTHESE_FERMANTE) {
				if (debut->identifiant == danjo::id_morceau::VALEUR) {
					++debut;
					++debut;

					valeur_propriete = debut->chaine;
				}
				else if (debut->identifiant == danjo::id_morceau::ATTACHE) {
					++debut;
					++debut;

					nom_propriete = debut->chaine;
				}

				++debut;
			}

			if (nom_propriete.est_vide()) {
				//imprime_morceaux(decoupeuse.begin(), decoupeuse.end());
				std::cerr << "Fichier " << chemin << " : attache manquante !\n";
				continue;
			}

			os << "\t\t" << nom_propriete << ":";

			switch (identifiant) {
				case danjo::id_morceau::COULEUR:
					os << "couleur(" << valeur_propriete << ");\n";
					break;
				case danjo::id_morceau::VECTEUR:
					os << "vecteur(" << valeur_propriete << ");\n";
					break;
				case danjo::id_morceau::FICHIER_ENTREE:
				case danjo::id_morceau::FICHIER_SORTIE:
				case danjo::id_morceau::CHAINE:
				case danjo::id_morceau::LISTE:
				case danjo::id_morceau::ENUM:
					os << "\"" << valeur_propriete << "\";\n";
					break;
				default:
					os << valeur_propriete << ";\n";
					break;
			}
		}

		os << "\t}\n";
		os << "}";
	}
	catch (const danjo::ErreurFrappe &e) {
		std::cerr << e.quoi();
		return;
	}
}

/**
 * Génère un fichier dan à partir d'un fichier jo. Le fichier dan ne contiendra
 * que la définition de l'entreface. Si le fichier jo indiqué par le chemin
 * n'existe pas ou ne contient pas de définition de disposition, la fonction
 * retourne sans erreur. Si le fichier jo contient des erreurs de frappe, une
 * exception sera lancée, cependant aucune analyse syntactique n'est effectuée.
 */
int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Usage : outils_danjo chemin_fichier.jo|chemin_dossier\n";
		return 1;
	}

	auto chemin = std::experimental::filesystem::path(argv[1]);

	if (std::experimental::filesystem::is_directory(chemin)) {
		for (const auto &donnees : std::experimental::filesystem::directory_iterator(chemin)) {
			auto chemin_fichier = donnees.path();

			if (std::experimental::filesystem::is_directory(chemin_fichier)) {
				continue;
			}

			if (chemin_fichier.extension() != ".jo") {
				continue;
			}

			cree_fichier_dan(chemin_fichier);
		}
	}
	else {
		cree_fichier_dan(chemin);
	}

	return 0;
}
