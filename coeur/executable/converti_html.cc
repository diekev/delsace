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

#include <cstring>
#include <filesystem>
#include <iostream>

#include "decoupage/contexte_generation_code.h"
#include "decoupage/decoupeuse.h"
#include "decoupage/erreur.h"
#include "decoupage/modules.hh"

static const char *options =
R"(kuri [OPTIONS...] FICHIER

-a, --aide
	imprime cette aide

-d, --dest FICHIER
	Utilisé pour spécifié le nom du programme produit, utilise a.out par défaut.

--émet-llvm
	émet la représentation intermédiaire du code LLVM

--émet-arbre
	émet l'arbre syntactic

-m, --mémoire
	imprime la mémoire utilisée

-t, --temps
	imprime le temps utilisé

-v, --version
	imprime la version

-O0
	Ne performe aucune optimisation. Ceci est le défaut.

-O1
	Optimise le code. Augmente le temps de compilation.

-O2
	Optimise le code encore plus. Augmente le temps de compilation.

-Os
	Comme -O2, mais minimise la taille du code. Augmente le temps de compilation.

-Oz
	Comme -Os, mais minimise encore plus la taille du code. Augmente le temps de compilation.

-O3
	Optimise le code toujours plus. Augmente le temps de compilation.
)";

enum class NiveauOptimisation : char {
	Aucun,
	O0,
	O1,
	O2,
	Os,
	Oz,
	O3,
};

struct OptionsCompilation {
	const char *chemin_fichier = nullptr;
	const char *chemin_sortie = "a.out";
	bool emet_fichier_objet = true;
	bool emet_code_intermediaire = false;
	bool emet_arbre = false;
	bool imprime_taille_memoire_objet = false;
	bool imprime_temps = false;
	bool imprime_version = false;
	bool imprime_aide = false;
	bool erreur = false;

	NiveauOptimisation optimisation = NiveauOptimisation::Aucun;
	char pad[7];
};

static OptionsCompilation genere_options_compilation(int argc, char **argv)
{
	OptionsCompilation opts;

	for (int i = 1; i < argc; ++i) {
		if (std::strcmp(argv[i], "-a") == 0) {
			opts.imprime_aide = true;
		}
		else if (std::strcmp(argv[i], "--aide") == 0) {
			opts.imprime_aide = true;
		}
		else if (std::strcmp(argv[i], "-t") == 0) {
			opts.imprime_temps = true;
		}
		else if (std::strcmp(argv[i], "--temps") == 0) {
			opts.imprime_temps = true;
		}
		else if (std::strcmp(argv[i], "-v") == 0) {
			opts.imprime_version = true;
		}
		else if (std::strcmp(argv[i], "--version") == 0) {
			opts.imprime_version = true;
		}
		else if (std::strcmp(argv[i], "--émet-llvm") == 0) {
			opts.emet_code_intermediaire = true;
		}
		else if (std::strcmp(argv[i], "-o") == 0) {
			opts.emet_fichier_objet = true;
		}
		else if (std::strcmp(argv[i], "--émet-arbre") == 0) {
			opts.emet_arbre = true;
		}
		else if (std::strcmp(argv[i], "-m") == 0) {
			opts.imprime_taille_memoire_objet = true;
		}
		else if (std::strcmp(argv[i], "-O0") == 0) {
			opts.optimisation = NiveauOptimisation::O0;
		}
		else if (std::strcmp(argv[i], "-O1") == 0) {
			opts.optimisation = NiveauOptimisation::O1;
		}
		else if (std::strcmp(argv[i], "-O2") == 0) {
			opts.optimisation = NiveauOptimisation::O2;
		}
		else if (std::strcmp(argv[i], "-Os") == 0) {
			opts.optimisation = NiveauOptimisation::Os;
		}
		else if (std::strcmp(argv[i], "-Oz") == 0) {
			opts.optimisation = NiveauOptimisation::Oz;
		}
		else if (std::strcmp(argv[i], "-O3") == 0) {
			opts.optimisation = NiveauOptimisation::O3;
		}
		else if (std::strcmp(argv[i], "-d") == 0) {
			if (i + 1 < argc) {
				opts.chemin_sortie = argv[i + 1];
				++i;
			}
		}
		else if (std::strcmp(argv[i], "--dest") == 0) {
			if (i + 1 < argc) {
				opts.chemin_sortie = argv[i + 1];
				++i;
			}
		}
		else {
			if (argv[i][0] == '-') {
				std::cerr << "Argument inconnu " << argv[i] << '\n';
				opts.erreur = true;
				break;
			}

			opts.chemin_fichier = argv[i];
		}
	}

	return opts;
}

static auto est_mot_cle(id_morceau id)
{
	switch (id) {
		default:
		{
			return false;
		}
		case id_morceau::FONCTION:
		case id_morceau::STRUCTURE:
		case id_morceau::DYN:
		case id_morceau::SOIT:
		case id_morceau::RETOURNE:
		case id_morceau::ENUM:
		case id_morceau::RETIENS:
		case id_morceau::DE:
		case id_morceau::EXTERNE:
		case id_morceau::IMPORTE:
		case id_morceau::POUR:
		case id_morceau::DANS:
		case id_morceau::BOUCLE:
		case id_morceau::TANTQUE:
		case id_morceau::SINON:
		case id_morceau::SI:
		case id_morceau::SAUFSI:
		case id_morceau::LOGE:
		case id_morceau::DELOGE:
		case id_morceau::RELOGE:
		case id_morceau::ASSOCIE:
		case id_morceau::Z8:
		case id_morceau::Z16:
		case id_morceau::Z32:
		case id_morceau::Z64:
		case id_morceau::N8:
		case id_morceau::N16:
		case id_morceau::N32:
		case id_morceau::N64:
		case id_morceau::R16:
		case id_morceau::R32:
		case id_morceau::R64:
		case id_morceau::EINI:
		case id_morceau::BOOL:
		case id_morceau::RIEN:
		case id_morceau::CHAINE:
		case id_morceau::OCTET:
		{
			return true;
		}
	}
}

static auto est_chaine_litterale(id_morceau id)
{
	switch (id) {
		default:
		{
			return false;
		}
		case id_morceau::CHAINE_LITTERALE:
		case id_morceau::CARACTERE:
		{
			return true;
		}
	}
}

int main(int argc, char **argv)
{
	std::ios::sync_with_stdio(false);

	if (argc < 2) {
		std::cerr << "Utilisation : " << argv[0] << " FICHIER [options...]\n";
		return 1;
	}

	auto const ops = genere_options_compilation(argc, argv);

	if (ops.imprime_aide) {
		std::cout << options;
	}

	if (ops.imprime_version) {
		std::cout << "Kuri 0.1 alpha\n";
	}

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
		std::cerr << "Impossible d'ouvrir le fichier : " << chemin_fichier << '\n';
		return 1;
	}

	std::ostream &os = std::cout;

	try {
		auto chemin = std::filesystem::path(chemin_fichier);

		if (chemin.is_relative()) {
			chemin = std::filesystem::absolute(chemin);
		}

		auto contexte = ContexteGenerationCode{};
		auto tampon = charge_fichier(chemin, contexte, {});
		auto module = contexte.cree_module("", chemin);
		module->tampon = lng::tampon_source(tampon);

		auto decoupeuse = decoupeuse_texte(module, INCLUS_CARACTERES_BLANC | INCLUS_COMMENTAIRES);
		decoupeuse.genere_morceaux();

		for (auto const &morceau : module->morceaux) {
			if (est_mot_cle(morceau.identifiant)) {
				os << "<span class=mot-cle>" << morceau.chaine << "</span>";
			}
			else if (est_chaine_litterale(morceau.identifiant)) {
				os << "<span class=chn-lit>" << morceau.chaine << "</span>";
			}
			else if (morceau.identifiant == id_morceau::COMMENTAIRE) {
				os << "<span class=comment>" << morceau.chaine << "</span>";
			}
			else {
				os << morceau.chaine;
			}
		}
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
	}

	return 0;
}
