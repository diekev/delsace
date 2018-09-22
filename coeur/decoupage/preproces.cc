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
#include <iostream>
#include <set>
#include <stack>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

inline bool existe(const std::string &chemin)
{
	struct stat tampon;
	return (stat(chemin.c_str(), &tampon) == 0);
}

struct DonneesLigne {
	std::string chemin_fichier;
	int numero_ligne;
};

struct Preproces {
	std::stack<std::string> dossier_courant;
	std::set<std::string> chemin_inclusions;
	std::set<std::string> fichiers;
	std::set<std::string> fichiers_visites;
	std::string tampon;
	std::vector<DonneesLigne> donnees_lignes;
	size_t nombre_lignes_total = 0;
};

std::string trouve_chemin_absolu(Preproces &preproces, const std::string &chemin)
{
	if (existe(chemin)) {
		return chemin;
	}

	for (const auto &chemin_inclusion : preproces.chemin_inclusions) {
		auto chemin_tmp = chemin_inclusion;

		if (chemin_tmp.back() != '/') {
			chemin_tmp.push_back('/');
		}

		chemin_tmp += chemin;

		if (existe(chemin_tmp)) {
			return chemin_tmp;
		}
	}

	return chemin;
}

std::string trouve_vrai_chemin(const std::string &chemin)
{
	char tampon[1024];
	auto r = getcwd(tampon, 1024);

	if (r == nullptr) {
		std::cerr << "Une erreur est survenue lors de la résolution du vrai chemin de '" << chemin << "'\n";
	}

	std::string vrai_chemin;
	vrai_chemin += tampon;

	if (vrai_chemin.back() != '/') {
		vrai_chemin += '/';
	}

	vrai_chemin += chemin;

	return vrai_chemin;
}

void ajoute_chemin_courant(Preproces &preproces, const std::string &chemin)
{
	const auto pos = chemin.find_last_of('/');
	auto dossier = chemin.substr(0, pos);

	preproces.chemin_inclusions.insert(dossier);
}

void ajoute_chemin_inclusion(Preproces &preproces, const std::string &chemin)
{
	preproces.chemin_inclusions.insert(chemin);
}

bool est_symbole(std::string::iterator iter, const char *chaine, size_t taille)
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

	ajoute_chemin_courant(preproces, chemin_);
	preproces.fichiers_visites.insert(chemin_);
	std::cout << "Chargement du fichier : '" << chemin_ << "'\n";

	int ligne_fichier = 0;
	bool si_rencontre = false;
	bool ignore_texte = false;

	std::string tampon;

	while (std::getline(fichier, tampon)) {
		++ligne_fichier;

		if (tampon.empty()) {
			continue;
		}

		tampon.push_back('\n');

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

				const auto pos = iter - tampon.begin();

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

#if 0
/* Trouve le chemin absolu du fichier racine.
 * Trouve le chemin absolu du dossier du chemin racine.
 * Ajout du chemin du dossier à la pile des chemins du projet.
 * Quand on a une importation, si le fichier ne se trouve pas dans le dossier courant.
 * Si le fichier n'est pas trouvé, recherche dans le CHEMIN système. */
int main(int argc, char *argv[])
{
	/* Les 'streams' de C++ sont synchronisés avec stdio de C, mais puisque nous
	 * n'utilisons pas les flux de C, nous pouvons désactiver la synchronisation
	 * afin d'accélérer les flux. */
	std::ios::sync_with_stdio(false);

	if (argc != 2) {
		std::cerr << "Usage : " << argv[0] << " fichier\n";
		return 1;
	}

	Preproces preproces;

	ajoute_chemin_inclusion(preproces, "/usr/include/");
	ajoute_chemin_inclusion(preproces, "/usr/include/kuri/include/");

	auto chemin_racine = trouve_vrai_chemin(argv[1]);
	charge_fichier(preproces, chemin_racine);

	std::cout << "Nombre de fichiers      : " << preproces.fichiers.size() << '\n';
	std::cout << "Nombre de lignes total  : " << preproces.nombre_lignes_total << '\n';
	std::cout << "Nombre de lignes généré : " << preproces.donnees_lignes.size() << '\n';

	return 0;
}
#endif
