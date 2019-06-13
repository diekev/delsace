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

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "decoupage/analyseuse_grammaire.hh"
#include "decoupage/contexte_generation_code.hh"
#include "decoupage/decoupeuse.hh"
#include "decoupage/erreur.hh"
#include "decoupage/modules.hh"

#include "biblinternes/chrono/outils.hh"

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

struct temps_seconde {
	double valeur;

	explicit temps_seconde(double v)
		: valeur(v)
	{}
};

static std::ostream &operator<<(std::ostream &os, const temps_seconde &t)
{
	auto valeur = static_cast<int>(t.valeur * 1000);

	if (valeur < 1) {
		valeur = static_cast<int>(t.valeur * 1000000);
		if (valeur < 10) {
			os << ' ' << ' ' << ' ';
		}
		else if (valeur < 100) {
			os << ' ' << ' ';
		}
		else if (valeur < 1000) {
			os << ' ';
		}

		os << valeur << "ns";
	}
	else {
		if (valeur < 10) {
			os << ' ' << ' ' << ' ';
		}
		else if (valeur < 100) {
			os << ' ' << ' ';
		}
		else if (valeur < 1000) {
			os << ' ';
		}

		os << valeur << "ms";
	}

	return os;
}

struct pourcentage {
	double valeur;

	explicit pourcentage(double v)
		: valeur(v)
	{}
};

static std::ostream &operator<<(std::ostream &os, const pourcentage &p)
{
	auto const valeur = static_cast<int>(p.valeur * 100) / 100.0;

	if (valeur < 10) {
		os << ' ' << ' ';
	}
	else if (valeur < 100) {
		os << ' ';
	}

	os << valeur << "%";

	return os;
}

struct taille_octet {
	size_t valeur;

	explicit taille_octet(size_t v)
		: valeur(v)
	{}
};

static std::ostream &operator<<(std::ostream &os, const taille_octet &taille)
{
	if (taille.valeur > (1024 * 1024 * 1024)) {
		os << (taille.valeur / (1024 * 1024 * 1024)) << "Go";
	}
	else if (taille.valeur > (1024 * 1024)) {
		os << (taille.valeur / (1024 * 1024)) << "Mo";
	}
	else if (taille.valeur > 1024) {
		os << (taille.valeur / 1024) << "Ko";
	}
	else {
		os << (taille.valeur) << "o";
	}

	return os;
}

int main(int argc, char *argv[])
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
		std::cout << "Parcer 0.1 alpha\n";
	}

	if (ops.erreur) {
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

	auto resultat = 0;
	auto debut_compilation   = dls::chrono::maintenant();
	auto debut_nettoyage     = 0.0;
	auto temps_nettoyage     = 0.0;
	auto temps_fichier_objet = 0.0;
	auto temps_executable    = 0.0;
	auto mem_arbre           = 0ul;
	auto mem_contexte        = 0ul;
	auto nombre_noeuds       = 0ul;

	auto metriques = Metriques{};

	try {
		/* enregistre le dossier d'origine */
		auto dossier_origine = std::filesystem::current_path();

		auto chemin = std::filesystem::path(chemin_fichier);

		if (chemin.is_relative()) {
			chemin = std::filesystem::absolute(chemin);
		}

		auto nom_module = chemin.stem();

		auto contexte_generation = ContexteGenerationCode{};
		auto assembleuse = assembleuse_arbre(contexte_generation);

		os << "Lancement de la compilation à partir du fichier '" << chemin_fichier << "'..." << std::endl;

		/* Change le dossier courant et lance la compilation. */
		auto dossier = chemin.parent_path();
		std::filesystem::current_path(dossier);

		charge_module(os, "", chemin, contexte_generation, {}, true);

		if (ops.emet_arbre) {
			assembleuse.imprime_code(os);
		}

		os << "Génération du code..." << std::endl;

		std::ofstream of;
		of.open("/tmp/conversion.kuri");

		//assembleuse.genere_code_C(contexte_generation, of);
		mem_arbre = assembleuse.memoire_utilisee();
		nombre_noeuds = assembleuse.nombre_noeuds();

		of.close();

		auto debut_executable = dls::chrono::maintenant();
//		auto commande = std::string("gcc /tmp/conversion.kuri ");

//		commande += " -o ";
//		commande += ops.chemin_sortie;

//		os << "Exécution de la commade '" << commande << "'..." << std::endl;

//		auto err = system(commande.c_str());

//		if (err != 0) {
//			std::cerr << "Ne peut pas créer l'executable !\n";
//		}
		temps_executable = dls::chrono::delta(debut_executable);

		/* restore le dossier d'origine */
		std::filesystem::current_path(dossier_origine);

		metriques = contexte_generation.rassemble_metriques();
		mem_contexte = contexte_generation.memoire_utilisee();

		os << "Nettoyage..." << std::endl;
		debut_nettoyage = dls::chrono::maintenant();
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
	}

	temps_nettoyage = dls::chrono::delta(debut_nettoyage);
	auto const temps_total = dls::chrono::delta(debut_compilation);

	auto const temps_scene = metriques.temps_tampon
							 + metriques.temps_decoupage
							 + metriques.temps_analyse
							 + metriques.temps_chargement
							 + metriques.temps_validation;

	auto const temps_coulisse = metriques.temps_generation
								+ temps_fichier_objet
								+ temps_executable;

	auto const temps_aggrege = temps_scene + temps_coulisse + temps_nettoyage;

	auto calc_pourcentage = [&](const double &x, const double &total)
	{
		return pourcentage(x * 100.0 / total);
	};

	auto const mem_totale = metriques.memoire_tampons
							+ metriques.memoire_morceaux
							+ mem_arbre
							+ mem_contexte;

	os << "------------------------------------------------------------------\n";
	os << "Temps total                  : " << temps_seconde(temps_total) << '\n';
	os << "Temps aggrégé                : " << temps_seconde(temps_aggrege) << '\n';
	os << "Nombre de modules            : " << metriques.nombre_modules << '\n';
	os << "Nombre de lignes             : " << metriques.nombre_lignes << '\n';
	os << "Nombre de lignes par seconde : " << static_cast<double>(metriques.nombre_lignes) / temps_aggrege << '\n';
	os << "Débit par seconde            : " << taille_octet(static_cast<size_t>(static_cast<double>(mem_totale) / temps_aggrege)) << '\n';

	os << '\n';
	os << "Métriques :\n";
	os << "\tNombre morceaux : " << metriques.nombre_morceaux << '\n';
	os << "\tNombre noeuds   : " << nombre_noeuds << '\n';

	os << '\n';
	os << "Mémoire : " << taille_octet(mem_totale) << '\n';
	os << "\tTampon   : " << taille_octet(metriques.memoire_tampons) << '\n';
	os << "\tMorceaux : " << taille_octet(metriques.memoire_morceaux) << '\n';
	os << "\tArbre    : " << taille_octet(mem_arbre) << '\n';
	os << "\tContexte : " << taille_octet(mem_contexte) << '\n';

	os << '\n';
	os << "Temps scène : " << temps_seconde(temps_scene)
	   << " (" << calc_pourcentage(temps_scene, temps_total) << ")\n";
	os << '\t' << "Temps chargement : " << temps_seconde(metriques.temps_chargement)
	   << " (" << calc_pourcentage(metriques.temps_chargement, temps_scene) << ")\n";
	os << '\t' << "Temps tampon     : " << temps_seconde(metriques.temps_tampon)
	   << " (" << calc_pourcentage(metriques.temps_tampon, temps_scene) << ")\n";
	os << '\t' << "Temps découpage  : " << temps_seconde(metriques.temps_decoupage)
	   << " (" << calc_pourcentage(metriques.temps_decoupage, temps_scene) << ") ("
	   << taille_octet(static_cast<size_t>(static_cast<double>(metriques.memoire_tampons) / metriques.temps_decoupage)) << ")\n";
	os << '\t' << "Temps analyse    : " << temps_seconde(metriques.temps_analyse)
	   << " (" << calc_pourcentage(metriques.temps_analyse, temps_scene) << ") ("
	   << taille_octet(static_cast<size_t>(static_cast<double>(metriques.memoire_morceaux) / metriques.temps_analyse)) << ")\n";
	os << '\t' << "Temps validation : " << temps_seconde(metriques.temps_validation)
	   << " (" << calc_pourcentage(metriques.temps_validation, temps_scene) << ")\n";

	os << '\n';
	os << "Temps coulisse : " << temps_seconde(temps_coulisse)
	   << " (" << calc_pourcentage(temps_coulisse, temps_total) << ")\n";
	os << '\t' << "Temps génération code : " << temps_seconde(metriques.temps_generation)
	   << " (" << calc_pourcentage(metriques.temps_generation, temps_coulisse) << ")\n";
	os << '\t' << "Temps fichier objet   : " << temps_seconde(temps_fichier_objet)
	   << " (" << calc_pourcentage(temps_fichier_objet, temps_coulisse) << ")\n";
	os << '\t' << "Temps exécutable      : " << temps_seconde(temps_executable)
	   << " (" << calc_pourcentage(temps_executable, temps_coulisse) << ")\n";

	os << '\n';
	os << "Temps Nettoyage : " << temps_seconde(temps_nettoyage)
	   << " (" << calc_pourcentage(temps_nettoyage, temps_total) << ")\n";

	if (ops.imprime_taille_memoire_objet) {
		imprime_taille_memoire_noeud(os);
	}

	os << std::endl;

	return resultat;
}
