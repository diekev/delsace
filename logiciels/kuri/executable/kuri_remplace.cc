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
#include <fstream>
#include <iostream>

#include "compilation/contexte_generation_code.h"
#include "compilation/decoupeuse.h"
#include "compilation/erreur.h"
#include "compilation/modules.hh"

#include "options.hh"

/* petit outil pour remplacer des mot-clés dans les scripts préexistants afin de
 * faciliter l'évolution du langage
 * nous pourrions également avoir un sorte de fichier de configuration pour
 * définir comment changer les choses, ou formater le code
 */

static void reecris_fichier(std::filesystem::path &chemin)
{
	std::cerr << "Réécriture du fichier " << chemin << "\n";

	try {
		if (chemin.is_relative()) {
			chemin = std::filesystem::absolute(chemin);
		}

		auto contexte = ContexteGenerationCode{};
		auto tampon = charge_fichier(chemin.c_str(), contexte, {});
		auto fichier = contexte.cree_fichier("", chemin.c_str());
		fichier->tampon = lng::tampon_source(tampon);

		auto decoupeuse = decoupeuse_texte(fichier, INCLUS_CARACTERES_BLANC | INCLUS_COMMENTAIRES);
		decoupeuse.genere_morceaux();

		auto os = std::ofstream(chemin);

		for (auto const &morceau : fichier->morceaux) {
#if 0
			if (morceau.identifiant == id_morceau::STRUCTURE) {
				os << "struct";
			}
			else {
				os << morceau.chaine;
			}
#else
			os << morceau.chaine;
#endif
		}
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
	}
}

int main(int argc, char **argv)
{
	std::ios::sync_with_stdio(false);

	auto const ops = genere_options_compilation(argc, argv);

	if (ops.erreur) {
		return 1;
	}

	auto const &chemin_racine_kuri = getenv("RACINE_KURI");

	if (chemin_racine_kuri == nullptr) {
		std::cerr << "Impossible de trouver le chemin racine de l'installation de kuri !\n";
		std::cerr << "Possible solution : veuillez faire en sorte que la variable d'environnement 'RACINE_KURI' soit définie !\n";
		return 1;
	}

	auto const chemin_fichier = ops.chemin_fichier;

	if (chemin_fichier == nullptr) {
		std::cerr << "Aucun fichier spécifié !\n";
		return 1;
	}

	if (!std::filesystem::exists(chemin_fichier)) {
		std::cerr << "Le chemin « " << chemin_fichier << " » ne pointe vers aucun fichier ou dossier !\n";
		return 1;
	}

	auto chemin = std::filesystem::path(chemin_fichier);

	if (std::filesystem::is_directory(chemin)) {
		for (auto donnees : std::filesystem::recursive_directory_iterator(chemin)) {
			chemin = donnees.path();

			if (chemin.extension() != ".kuri") {
				continue;
			}

			reecris_fichier(chemin);
		}
	}
	else {
		reecris_fichier(chemin);
	}

	return 0;
}
