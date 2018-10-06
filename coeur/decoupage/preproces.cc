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

/* Préprocès des fichiers kuri afin de créer un fichier unique à partir de
 * plusieurs fichiers séparés. Les fichiers sont ouverts et collés dans le
 * fichier qui les importe à la place de l'expression '#!importe "fichier.kuri"'
 *
 * De fait la compilation d'un programme Kuri se fait en indiquant quel est le
 * fichier racine, et seuls les fichiers inclus à partir de celui-ci sont
 * considérés.
 *
 * Le préprocès ne copie pas les lignes commençant par le caractère '#' des
 * commentaires, ou les lignes vides.
 *
 * Le préprocès s'occupe également de maintenir un tableau indiquant de quel
 * couple (nom de fichier, ligne) vient chacune des lignes du tampon final.
 *
 * Il est également possible de contrôler quelles lignes sont copiés avec les
 * contrôles de flux suivant :
 *
 * #!si vrai
 * ceci est copié
 * #!sinon
 * ceci n'est pas copié
 * #!finsi
 *
 * #!si faux
 * ceci n'est pas copié, y comprix
 * #!sinon
 * ceci est copié
 * #!finsi
 *
 * algorithme :
 *	ouvre le fichier
 *	lis le fichier ligne par ligne
 *
 *	pour chaque ligne du fichier :
 *		ignore caractères vide (' ', '\t')
 *
 *		si ligne commence par '#':
 *			ignore
 *		si ligne commence par '#!':
 *			si instruction == 'sinon':
 *				vérifie qu'une instruction '#!si' a déjà été rencontrée
 *				ignore_ligne = !ignore_ligne
 *			si instruction == 'si':
 *				ignore_ligne = (instuction == 'vrai')
 *			si instruction == 'finsi':
 *				vérifie qu'une instruction '#!si' a déjà été rencontrée
 *			si non ignore_ligne et instruction == 'importe':
 *				trouve chemin absolu fichier
 *
 *				si fichier non déjà copié:
 *					ouvre fichier et copie son contenu à la place de la ligne
 *
 *		si non ignore_ligne :
 *			copie ligne dans le tampon
 *	finpour
 *
 *	inscris fichier comme étant copié
 */

#include <fstream>
#include <experimental/filesystem>
#include <iostream>

#include "preproces.hh"

static std::string trouve_chemin_absolu(Preproces &preproces, const std::string &chemin)
{
	if (std::experimental::filesystem::exists(chemin)) {
		return chemin;
	}

	for (const auto &chemin_inclusion : preproces.chemin_inclusions) {
		auto chemin_tmp = std::experimental::filesystem::path(chemin_inclusion);

		chemin_tmp /= chemin;

		if (std::experimental::filesystem::exists(chemin_tmp)) {
			return chemin_tmp;
		}
	}

	return chemin;
}

static void ajoute_chemin_courant(Preproces &preproces, const std::string &chemin)
{
	const auto pos = chemin.find_last_of('/');
	auto dossier = chemin.substr(0, pos);

	preproces.chemin_inclusions.insert(dossier);
}

static bool est_symbole(std::string::iterator iter, const char *chaine, size_t taille)
{
	for (size_t i = 0; i < taille; ++i) {
		if (*iter++ != chaine[i]) {
			return false;
		}
	}

	return true;
}

void charge_fichier(Preproces &preproces, const std::string &chemin)
{
	const auto chemin_ = trouve_chemin_absolu(preproces, chemin);

	if (preproces.fichiers.find(chemin_) != preproces.fichiers.end()) {
		return;
	}

	if (preproces.fichiers_visites.find(chemin_) != preproces.fichiers_visites.end()) {
		std::cerr << "Référence circulaire pour '" << chemin_ << "'\n";
		return;
	}

	std::ifstream fichier;
	fichier.open(chemin_.c_str());

	if (!fichier.is_open()) {
		std::cerr << "Impossible de charger le fichier '" << chemin_ << "'\n";
		return;
	}

	fichier.seekg(0, fichier.end);
	const auto taille_fichier = static_cast<std::string::size_type>(fichier.tellg());
	fichier.seekg(0, fichier.beg);

	preproces.tampon.reserve(preproces.tampon.size() + taille_fichier);

	ajoute_chemin_courant(preproces, chemin_);
	preproces.fichiers_visites.insert(chemin_);

	std::cout << "Chargement du fichier : '" << chemin_ << "'\n";

	size_t ligne_fichier = 0;
	bool si_rencontre = false;
	bool ignore_texte = false;

	std::string tampon;

	while (std::getline(fichier, tampon)) {
		++ligne_fichier;

		if (tampon.empty()) {
			continue;
		}

		tampon.append(1, '\n');

		auto iter = tampon.begin();

		while ((*iter == ' ' || *iter == '\t')) {
			++iter;
		}

		if (*iter == '#') {
			++iter;

			if (*iter != '!') {
				continue;
			}

			++iter;

			// On teste d'abord sinon, car le son 'si' est en conflit
			// avec le 'si' seul.
			if (est_symbole(iter, "sinon", 5)) {
				if (!si_rencontre) {
					std::cerr << "Rencontré '#!sinon' avant '#!si' !\n";
				}

				ignore_texte = (ignore_texte == false);
			}
			else if (est_symbole(iter, "si", 2)) {
				si_rencontre = true;

				iter += 3;

				ignore_texte = est_symbole(iter, "faux", 4);
			}
			else if (est_symbole(iter, "finsi", 5)) {
				if (!si_rencontre) {
					std::cerr << "Rencontré '#!finsi' avant '#!si' !\n";
				}

				si_rencontre = false;
				ignore_texte = false;
			}
			else if (!ignore_texte && est_symbole(iter, "importe", 7)) {
				iter += 7;

				while (*iter != '"') {
					++iter;
				}

				++iter;

				const auto pos = static_cast<size_t>(iter - tampon.begin());

				auto chemin_fichier = tampon.substr(pos, tampon.size() - pos - 2);
				charge_fichier(preproces, chemin_fichier);
			}

			continue;
		}

		if (ignore_texte) {
			continue;
		}

		preproces.tampon += tampon;
		preproces.donnees_lignes.push_back(DonneesLigne{chemin_, ligne_fichier});
	}

	preproces.nombre_lignes_total += ligne_fichier;
	preproces.fichiers.insert(chemin_);
}

void imprime_tampon(Preproces &preproces, std::ostream &os)
{
	os << preproces.tampon << '\n';
}

void imprime_donnees_lignes(Preproces &preproces, std::ostream &os)
{
	auto i = 0;

	for (const DonneesLigne &donnees : preproces.donnees_lignes) {
		os << i++ << " : "
		   << donnees.chemin_fichier << ':'
		   << donnees.numero_ligne << '\n';
	}
}
