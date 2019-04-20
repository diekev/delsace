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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include <llvm/InitializePasses.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#pragma GCC diagnostic pop

#include "decoupage/analyseuse_grammaire.h"
#include "decoupage/contexte_generation_code.h"
#include "decoupage/decoupeuse.h"
#include "decoupage/erreur.h"
#include "decoupage/modules.hh"

#include <chrono/outils.hh>

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

static void initialise_llvm()
{
	if (llvm::InitializeNativeTarget()
			|| llvm::InitializeNativeTargetAsmParser()
			|| llvm::InitializeNativeTargetAsmPrinter())
	{
		std::cerr << "Ne peut pas initiliser LLVM !\n";
		exit(EXIT_FAILURE);
	}

	/* Initialise les passes. */
	auto &registre = *llvm::PassRegistry::getPassRegistry();
	llvm::initializeCore(registre);
	llvm::initializeScalarOpts(registre);
	llvm::initializeObjCARCOpts(registre);
	llvm::initializeVectorization(registre);
	llvm::initializeIPO(registre);
	llvm::initializeAnalysis(registre);
	llvm::initializeTransformUtils(registre);
	llvm::initializeInstCombine(registre);
	llvm::initializeInstrumentation(registre);
	llvm::initializeTarget(registre);

	/* Pour les passes de transformation de code, seuls celles d'IR à IR sont
	 * supportées. */
	llvm::initializeCodeGenPreparePass(registre);
	llvm::initializeAtomicExpandPass(registre);
	llvm::initializeWinEHPreparePass(registre);
	llvm::initializeDwarfEHPreparePass(registre);
	llvm::initializeSjLjEHPreparePass(registre);
	llvm::initializePreISelIntrinsicLoweringLegacyPassPass(registre);
	llvm::initializeGlobalMergePass(registre);
	llvm::initializeInterleavedAccessPass(registre);
	llvm::initializeUnreachableBlockElimLegacyPassPass(registre);
}

static void issitialise_llvm()
{
	llvm::llvm_shutdown();
}

/**
 * Ajoute les passes d'optimisation au ménageur en fonction du niveau
 * d'optimisation.
 */
static void ajoute_passes(
		llvm::legacy::FunctionPassManager &menageur_fonctions,
		uint niveau_optimisation,
		uint niveau_taille)
{
	llvm::PassManagerBuilder builder;
	builder.OptLevel = niveau_optimisation;
	builder.SizeLevel = niveau_taille;
	builder.DisableUnrollLoops = (niveau_optimisation == 0);

	/* À FAIRE : enlignage. */

	/* Pour plus d'informations sur les vectoriseurs, suivre le lien :
	 * http://llvm.org/docs/Vectorizers.html */
	builder.LoopVectorize = (niveau_optimisation > 1 && niveau_taille < 2);
	builder.SLPVectorize = (niveau_optimisation > 1 && niveau_taille < 2);

	builder.populateFunctionPassManager(menageur_fonctions);
}

/**
 * Initialise le ménageur de passes fonctions du contexte selon le niveau
 * d'optimisation.
 */
static void initialise_optimisation(
		NiveauOptimisation optimisation,
		ContexteGenerationCode &contexte)
{
	if (contexte.menageur_fonctions == nullptr) {
		contexte.menageur_fonctions = new llvm::legacy::FunctionPassManager(contexte.module_llvm);
	}

	switch (optimisation) {
		case NiveauOptimisation::Aucun:
			break;
		case NiveauOptimisation::O0:
			ajoute_passes(*contexte.menageur_fonctions, 0, 0);
			break;
		case NiveauOptimisation::O1:
			ajoute_passes(*contexte.menageur_fonctions, 1, 0);
			break;
		case NiveauOptimisation::O2:
			ajoute_passes(*contexte.menageur_fonctions, 2, 0);
			break;
		case NiveauOptimisation::Os:
			ajoute_passes(*contexte.menageur_fonctions, 2, 1);
			break;
		case NiveauOptimisation::Oz:
			ajoute_passes(*contexte.menageur_fonctions, 2, 2);
			break;
		case NiveauOptimisation::O3:
			ajoute_passes(*contexte.menageur_fonctions, 3, 0);
			break;
	}
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

static void cree_executable(const std::filesystem::path &dest, const std::filesystem::path &racine_kuri)
{
	/* Compile le fichier objet qui appelera 'fonction principale'. */
	if (!std::filesystem::exists("/tmp/execution_kuri.o")) {
		auto const &chemin_execution_S = racine_kuri / "fichiers/execution_kuri.S";

		std::stringstream ss;
		ss << "as -o /tmp/execution_kuri.o ";
		ss << chemin_execution_S;

		auto err = system(ss.str().c_str());

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

		auto dossier = chemin.parent_path();
		std::filesystem::current_path(dossier);

		auto nom_module = chemin.stem();

		auto contexte_generation = ContexteGenerationCode{};
		auto assembleuse = assembleuse_arbre(contexte_generation);

		os << "Lancement de la compilation à partir du fichier '" << chemin_fichier << "'..." << std::endl;
		charge_module(os, nom_module, contexte_generation, {}, true);

		if (ops.emet_arbre) {
			assembleuse.imprime_code(os);
		}

		auto coulisse_LLVM = false;
		if (coulisse_LLVM) {
			auto const triplet_cible = llvm::sys::getDefaultTargetTriple();

			initialise_llvm();

			auto erreur = std::string{""};
			auto cible = llvm::TargetRegistry::lookupTarget(triplet_cible, erreur);

			if (!cible) {
				std::cerr << erreur << '\n';
				return 1;
			}

			auto CPU = "generic";
			auto feature = "";
			auto options_cible = llvm::TargetOptions{};
			auto RM = llvm::Optional<llvm::Reloc::Model>();
			auto machine_cible = std::unique_ptr<llvm::TargetMachine>(
									 cible->createTargetMachine(
										 triplet_cible, CPU, feature, options_cible, RM));

			auto module = llvm::Module(nom_module.c_str(), contexte_generation.contexte);
			module.setDataLayout(machine_cible->createDataLayout());
			module.setTargetTriple(triplet_cible);

			contexte_generation.module_llvm = &module;

			initialise_optimisation(ops.optimisation, contexte_generation);

			os << "Génération du code..." << std::endl;
			assembleuse.genere_code_llvm(contexte_generation);
			mem_arbre = assembleuse.memoire_utilisee();
			nombre_noeuds = assembleuse.nombre_noeuds();

			if (ops.emet_code_intermediaire) {
				std::cerr <<  "------------------------------------------------------------------\n";
				module.print(llvm::errs(), nullptr);
				std::cerr <<  "------------------------------------------------------------------\n";
			}

			/* définition du fichier de sortie */
			if (ops.emet_fichier_objet) {
				os << "Écriture du code dans un fichier..." << std::endl;
				auto debut_fichier_objet = dls::chrono::maintenant();
				if (!ecris_fichier_objet(machine_cible.get(), module)) {
					resultat = 1;
				}
				temps_fichier_objet = dls::chrono::delta(debut_fichier_objet);

				auto debut_executable = dls::chrono::maintenant();
				cree_executable(ops.chemin_sortie, chemin_racine_kuri);
				temps_executable = dls::chrono::delta(debut_executable);
			}
		}
		else {
			os << "Génération du code..." << std::endl;

			std::ofstream of;
			of.open("/tmp/compilation_kuri.c");

			assembleuse.genere_code_C(contexte_generation, of);

			of.close();

			auto commande = std::string("gcc /tmp/compilation_kuri.c -o ") + ops.chemin_sortie;

			auto err = system(commande.c_str());

			if (err != 0) {
				std::cerr << "Ne peut pas créer l'executable !\n";
			}
		}

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

	issitialise_llvm();

	return resultat;
}
