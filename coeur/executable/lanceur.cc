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

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

#include "decoupage/analyseuse_grammaire.h"
#include "decoupage/contexte_generation_code.h"
#include "decoupage/decoupeuse.h"
#include "decoupage/erreur.h"
#include "decoupage/modules.hh"
#include "decoupage/tampon_source.h"

#include <chronometrage/chronometre_de_portee.h>

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
)";

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

static void initialise_llvm()
{
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();
}

static bool ecris_fichier_objet(llvm::TargetMachine *machine_cible, llvm::Module &module)
{
	auto chemin_sortie = "/tmp/kuri.o";
	std::error_code ec;

	llvm::raw_fd_ostream dest(chemin_sortie, ec, llvm::sys::fs::F_None);

	if (ec) {
		std::cerr << "Ne put pas ouvrir le fichier '" << chemin_sortie << "'\n";
		return false;
	}

	llvm::legacy::PassManager pass;
	auto type_fichier = llvm::TargetMachine::CGFT_ObjectFile;

	if (machine_cible->addPassesToEmitFile(pass, dest, type_fichier)) {
		std::cerr << "La machine cible ne peut pas émettre ce type de fichier\n";
		return false;
	}

	pass.run(module);
	dest.flush();

	return true;
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
	const auto valeur = static_cast<int>(p.valeur * 100) / 100.0;

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

static void cree_executable(const std::filesystem::path &dest)
{
	if (!std::filesystem::exists("/tmp/execution_kuri.o")) {
		auto err = system("as -o /tmp/execution_kuri.o fichiers/execution_kuri.S");

		if (err != 0) {
			std::cerr << "Ne peut pas créer /tmp/execution_kuri.o !\n";
			return;
		}
	}

	if (!std::filesystem::exists("/tmp/kuri.o")) {
		std::cerr << "Le fichier objet n'a pas été émis !\n Utiliser la commande -o !\n";
		return;
	}

	std::stringstream ss;
	ss << "ld ";
	/* ce qui chargera le programme */
	ss << "-dynamic-linker /lib64/ld-linux-x86-64.so.2 ";
	ss << "-m elf_x86_64 ";
	ss << "--hash-style=gnu ";
	ss << "-lc ";
	ss << "/tmp/execution_kuri.o ";
	ss << "/tmp/kuri.o ";
	ss << "-o " << dest;

	auto err = system(ss.str().c_str());

	if (err != 0) {
		std::cerr << "Ne peut pas créer l'executable !\n";
	}
}

int main(int argc, char *argv[])
{
	std::ios::sync_with_stdio(false);

	if (argc < 2) {
		std::cerr << "Utilisation : " << argv[0] << " FICHIER [options...]\n";
		return 1;
	}

	const auto ops = genere_options_compilation(argc, argv);

	if (ops.imprime_aide) {
		std::cout << options;
	}

	if (ops.imprime_version) {
		std::cout << "Kuri 0.1 alpha\n";
	}

	if (ops.erreur) {
		return 1;
	}

	const auto chemin_fichier = ops.chemin_fichier;

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
	auto temps_generation_code = 0.0;
	auto debut_nettoyage       = 0.0;
	auto temps_nettoyage       = 0.0;
	auto temps_fichier_objet   = 0.0;
	auto temps_executable      = 0.0;
	auto mem_arbre             = 0ul;
	auto mem_contexte          = 0ul;
	auto nombre_noeuds         = 0ul;

	auto metriques = Metriques{};

	try {
		/* enregistre le dossier d'origine */
		auto dossier_origine = std::filesystem::current_path();

		auto chemin = std::filesystem::path(chemin_fichier);
		auto dossier = chemin.parent_path();
		std::filesystem::current_path(dossier);

		auto nom_module = chemin.stem();

		auto assembleuse = assembleuse_arbre();
		auto contexte_generation = ContexteGenerationCode{};
		contexte_generation.assembleuse = &assembleuse;

		os << "Lancement de la compilation à partir du fichier '" << chemin_fichier << "'..." << std::endl;
		charge_module(os, nom_module, contexte_generation, {}, true);

		if (ops.emet_arbre) {
			assembleuse.imprime_code(os);
		}

		const auto triplet_cible = llvm::sys::getDefaultTargetTriple();

		initialise_llvm();

		auto erreur = std::string{""};
		auto cible = llvm::TargetRegistry::lookupTarget(triplet_cible, erreur);

		if (!cible) {
			std::cerr << erreur << '\n';
			return 1;
		}

		auto CPU = "generic";
		auto feature = "";
		auto options = llvm::TargetOptions{};
		auto RM = llvm::Optional<llvm::Reloc::Model>();
		auto machine_cible = std::unique_ptr<llvm::TargetMachine>(
								 cible->createTargetMachine(
									 triplet_cible, CPU, feature, options, RM));

		auto module = llvm::Module(nom_module.c_str(), contexte_generation.contexte);
		module.setDataLayout(machine_cible->createDataLayout());
		module.setTargetTriple(triplet_cible);

		contexte_generation.module_llvm = &module;

		/* initialise ménageur passe fonction */
		auto fpm = std::make_unique<llvm::legacy::FunctionPassManager>(&module);

		/* Fais de simples optimisations "peephole" et de bit-twiddling.
		 * Désactivée pour le moment, afin de s'assurer que le code est un temps
		 * soit peu correcte en évitant de se faire avoir par une optimisation
		 * trop aggressive. */
		//fpm->add(llvm::createInstructionCombiningPass());
		/* Réassocie les expressions. */
		fpm->add(llvm::createReassociatePass());
		/* Élimine les sous-expressions communes. */
		fpm->add(llvm::createGVNPass());
		/* Simplifie le graphe de contrôle de flux (p.e. en enlevant les blocs
		 * inatteignables) */
		fpm->add(llvm::createCFGSimplificationPass());

		contexte_generation.menageur_pass_fonction = fpm.get();

		os << "Génération du code..." << std::endl;
		auto debut_generation_code = numero7::chronometrage::maintenant();
		assembleuse.genere_code_llvm(contexte_generation);
		temps_generation_code = numero7::chronometrage::maintenant() - debut_generation_code;
		mem_arbre = assembleuse.memoire_utilisee();
		nombre_noeuds = assembleuse.nombre_noeuds();

		if (ops.emet_code_intermediaire) {
			std::cerr <<  "------------------------------------------------------------------\n";
			module.dump();
			std::cerr <<  "------------------------------------------------------------------\n";
		}

		/* définition du fichier de sortie */
		if (ops.emet_fichier_objet) {
			os << "Écriture du code dans un fichier..." << std::endl;
			auto debut_fichier_objet = numero7::chronometrage::maintenant();
			if (!ecris_fichier_objet(machine_cible.get(), module)) {
				resultat = 1;
			}
			temps_fichier_objet = numero7::chronometrage::maintenant() - debut_fichier_objet;

			auto debut_executable = numero7::chronometrage::maintenant();
			cree_executable(ops.chemin_sortie);
			temps_executable = numero7::chronometrage::maintenant() - debut_executable;
		}

		/* restore le dossier d'origine */
		std::filesystem::current_path(dossier_origine);

		metriques = contexte_generation.rassemble_metriques();
		mem_contexte = contexte_generation.memoire_utilisee();

		os << "Nettoyage..." << std::endl;
		debut_nettoyage = numero7::chronometrage::maintenant();
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
	}

	temps_nettoyage = numero7::chronometrage::maintenant() - debut_nettoyage;

	const auto temps_scene = metriques.temps_tampon
							 + metriques.temps_decoupage
							 + metriques.temps_analyse
							 + metriques.temps_chargement;

	const auto temps_coulisse = temps_generation_code
								+ temps_fichier_objet
								+ temps_executable;

	const auto temps_total = temps_scene + temps_coulisse + temps_nettoyage;

	auto calc_pourcentage = [&](const double &x, const double &total)
	{
		return pourcentage(x * 100.0 / total);
	};

	os << "------------------------------------------------------------------\n";
	os << "Temps total                  : " << temps_seconde(temps_total) << '\n';
	os << "Nombre de modules            : " << metriques.nombre_modules << '\n';
	os << "Nombre de lignes             : " << metriques.nombre_lignes << '\n';
	os << "Nombre de lignes par seconde : " << metriques.nombre_lignes / temps_total << '\n';
	os << "Débit par seconde            : " << taille_octet(static_cast<size_t>(metriques.memoire_tampons / temps_total)) << '\n';

	const auto mem_totale = metriques.memoire_tampons
							+ metriques.memoire_morceaux
							+ mem_arbre
							+ mem_contexte;

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
	   << taille_octet(static_cast<size_t>(metriques.memoire_tampons / metriques.temps_decoupage)) << ")\n";
	os << '\t' << "Temps analyse    : " << temps_seconde(metriques.temps_analyse)
	   << " (" << calc_pourcentage(metriques.temps_analyse, temps_scene) << ") ("
	   << taille_octet(static_cast<size_t>(metriques.memoire_morceaux / metriques.temps_analyse)) << ")\n";

	os << '\n';
	os << "Temps coulisse : " << temps_seconde(temps_coulisse)
	   << " (" << calc_pourcentage(temps_coulisse, temps_total) << ")\n";
	os << '\t' << "Temps génération code : " << temps_seconde(temps_generation_code)
	   << " (" << calc_pourcentage(temps_generation_code, temps_coulisse) << ")\n";
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
