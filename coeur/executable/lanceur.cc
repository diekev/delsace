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

#include <fstream>
#include <iostream>

#include "danjo/interne/decoupeuse.h"
#include "danjo/interne/analyseuse_disposition.h"

#include "danjo/danjo.h"
#include "danjo/erreur.h"

/**
 * Génère un fichier dan à partir d'un fichier jo. Le fichier dan ne contiendra
 * que la définition de l'interface. Si le fichier jo indiqué par le chemin
 * n'existe pas ou ne contient pas de définition de disposition, la fonction
 * retourne sans erreur. Si le fichier jo contient des erreurs de frappe, une
 * exception sera lancée, cependant aucune analyse syntactique n'est effectuée.
 */
int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Usage : outils_danjo chemin_fichier.jo\n";
		return 1;
	}

	try {
		auto chemin = std::experimental::filesystem::path(argv[1]);

		auto texte = danjo::contenu_fichier(chemin);

		danjo::Decoupeuse decoupeuse(texte.c_str());
		decoupeuse.decoupe();

		auto debut = decoupeuse.begin();
		auto fin = decoupeuse.end();

		if (debut->identifiant != danjo::IDENTIFIANT_DISPOSITION) {
			return 1;
		}

		auto chemin_dan = chemin;
		chemin_dan.replace_extension("dan");

		std::ofstream fichier;
		fichier.open(chemin_dan.c_str());

		std::ostream &os = fichier;

		++debut;

		auto nom_disposition = debut->contenu;

		os << "feuille \"" << nom_disposition << "\" {\n";
		os << "\tinterface {\n";

		auto nom_propriete = std::string("");
		auto valeur_propriete = std::string("");

		while (debut++ != fin) {
			if (!danjo::est_identifiant_controle(debut->identifiant)) {
				continue;
			}

			if (debut->identifiant == danjo::IDENTIFIANT_ETIQUETTE) {
				continue;
			}

			auto identifiant = debut->identifiant;

			nom_propriete = "";
			valeur_propriete = "";

			while (debut->identifiant != danjo::IDENTIFIANT_PARENTHESE_FERMANTE) {
				if (debut->identifiant == danjo::IDENTIFIANT_VALEUR) {
					++debut;
					++debut;

					valeur_propriete = debut->contenu;
				}
				else if (debut->identifiant == danjo::IDENTIFIANT_ATTACHE) {
					++debut;
					++debut;

					nom_propriete = debut->contenu;
				}

				++debut;
			}

			os << "\t\t" << nom_propriete << ":";

			switch (identifiant) {
				case danjo::IDENTIFIANT_COULEUR:
					os << "couleur(" << valeur_propriete << ");\n";
					break;
				case danjo::IDENTIFIANT_VECTEUR:
					os << "vecteur(" << valeur_propriete << ");\n";
					break;
				case danjo::IDENTIFIANT_FICHIER_ENTREE:
				case danjo::IDENTIFIANT_FICHIER_SORTIE:
				case danjo::IDENTIFIANT_CHAINE:
				case danjo::IDENTIFIANT_LISTE:
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
		return 1;
	}

	return 0;
}
