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

#include <filesystem>
#include <fstream>
#include <iostream>

#ifdef AVEC_LLVM
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

#include "compilation/coulisse_llvm.hh"
#endif

#include "compilation/analyseuse_grammaire.h"
#include "compilation/assembleuse_arbre.h"
#include "compilation/contexte_generation_code.h"
#include "compilation/coulisse_c.hh"
#include "compilation/decoupeuse.h"
#include "compilation/erreur.h"
#include "compilation/modules.hh"
#include "compilation/validation_semantique.hh"

#include "options.hh"

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/outils/format.hh"
#include "biblinternes/outils/tableau_donnees.hh"

#ifdef AVEC_LLVM
static void initialise_llvm()
{
	if (llvm::InitializeNativeTarget()
			|| llvm::InitializeNativeTargetAsmParser()
			|| llvm::InitializeNativeTargetAsmPrinter())
	{
		std::cerr << "Ne peut pas initialiser LLVM !\n";
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

static void cree_executable(const std::filesystem::path &dest, const std::filesystem::path &racine_kuri)
{
	/* Compile le fichier objet qui appelera 'fonction principale'. */
	if (!std::filesystem::exists("/tmp/execution_kuri.o")) {
		auto const &chemin_execution_S = racine_kuri / "fichiers/execution_kuri.S";

		dls::flux_chaine ss;
		ss << "as -o /tmp/execution_kuri.o ";
		ss << chemin_execution_S;

		auto err = system(ss.chn().c_str());

		if (err != 0) {
			std::cerr << "Ne peut pas créer /tmp/execution_kuri.o !\n";
			return;
		}
	}

	if (!std::filesystem::exists("/tmp/kuri.o")) {
		std::cerr << "Le fichier objet n'a pas été émis !\n Utiliser la commande -o !\n";
		return;
	}

	dls::flux_chaine ss;
	ss << "ld ";
	/* ce qui chargera le programme */
	ss << "-dynamic-linker /lib64/ld-linux-x86-64.so.2 ";
	ss << "-m elf_x86_64 ";
	ss << "--hash-style=gnu ";
	ss << "-lc ";
	ss << "/tmp/execution_kuri.o ";
	ss << "/tmp/kuri.o ";
	ss << "-o " << dest;

	auto err = system(ss.chn().c_str());

	if (err != 0) {
		std::cerr << "Ne peut pas créer l'executable !\n";
	}
}
#endif

static void imprime_stats(
		std::ostream &os,
		Metriques const &metriques,
		OptionsCompilation const &ops,
		dls::chrono::compte_seconde debut_compilation)
{
	auto const temps_total = debut_compilation.temps();

	auto const temps_scene = metriques.temps_tampon
							 + metriques.temps_decoupage
							 + metriques.temps_analyse
							 + metriques.temps_chargement
							 + metriques.temps_validation;

	auto const temps_coulisse = metriques.temps_generation
								+ metriques.temps_fichier_objet
								+ metriques.temps_executable;

	auto const temps_aggrege = temps_scene + temps_coulisse + metriques.temps_nettoyage;

	auto calc_pourcentage = [&](const double &x, const double &total)
	{
		return (x * 100.0 / total);
	};

	auto const mem_totale = metriques.memoire_tampons
							+ metriques.memoire_morceaux
							+ metriques.memoire_arbre
							+ metriques.memoire_contexte;

	auto memoire_consommee = memoire::consommee();

	auto const lignes_double = static_cast<double>(metriques.nombre_lignes);
	auto const debit_lignes = static_cast<int>(lignes_double / temps_aggrege);
	auto const debit_lignes_scene = static_cast<int>(lignes_double / temps_scene);
	auto const debit_lignes_coulisse = static_cast<int>(lignes_double / temps_coulisse);
	auto const debit_seconde = static_cast<int>(static_cast<double>(memoire_consommee) / temps_aggrege);

	auto tableau = Tableau({ "Nom", "Valeur", "Unité", "Pourcentage" });
	tableau.alignement(1, Alignement::DROITE);
	tableau.alignement(3, Alignement::DROITE);

	tableau.ajoute_ligne({ "Temps total", formatte_nombre(temps_total * 1000.0), "ms" });
	tableau.ajoute_ligne({ "Temps aggrégé", formatte_nombre(temps_aggrege * 1000.0), "ms" });
	tableau.ajoute_ligne({ "Nombre de modules", formatte_nombre(metriques.nombre_modules), "" });
	tableau.ajoute_ligne({ "Nombre de lignes", formatte_nombre(metriques.nombre_lignes), "" });
	tableau.ajoute_ligne({ "Débit de lignes par seconde", formatte_nombre(debit_lignes), "" });
	tableau.ajoute_ligne({ "Débit de lignes par seconde (scène)", formatte_nombre(debit_lignes_scene), "" });
	tableau.ajoute_ligne({ "Débit de lignes par seconde (coulisse)", formatte_nombre(debit_lignes_coulisse), "" });
	tableau.ajoute_ligne({ "Débit par seconde", formatte_nombre(debit_seconde), "o/s" });

	tableau.ajoute_ligne({ "Arbre Syntaxique", "", "" });
	tableau.ajoute_ligne({ "- Nombre Morceaux", formatte_nombre(metriques.nombre_morceaux), "" });
	tableau.ajoute_ligne({ "- Nombre Noeuds", formatte_nombre(metriques.nombre_noeuds), "" });

	tableau.ajoute_ligne({ "Mémoire", "", "" });
	tableau.ajoute_ligne({ "- Suivie", formatte_nombre(mem_totale), "o" });
	tableau.ajoute_ligne({ "- Effective", formatte_nombre(memoire_consommee), "o" });
	tableau.ajoute_ligne({ "- Tampon", formatte_nombre(metriques.memoire_tampons), "o" });
	tableau.ajoute_ligne({ "- Morceaux", formatte_nombre(metriques.memoire_morceaux), "o" });
	tableau.ajoute_ligne({ "- Arbre", formatte_nombre(metriques.memoire_arbre), "o" });
	tableau.ajoute_ligne({ "- Contexte", formatte_nombre(metriques.memoire_contexte), "o" });

	tableau.ajoute_ligne({ "Temps Scène", formatte_nombre(temps_scene * 1000.0), "ms", formatte_nombre(calc_pourcentage(temps_scene, temps_total)) });
	tableau.ajoute_ligne({ "- Chargement", formatte_nombre(metriques.temps_chargement * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_chargement, temps_scene)) });
	tableau.ajoute_ligne({ "- Tampon", formatte_nombre(metriques.temps_tampon * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_tampon, temps_scene)) });
	tableau.ajoute_ligne({ "- Découpage", formatte_nombre(metriques.temps_decoupage * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_decoupage, temps_scene)) });
	tableau.ajoute_ligne({ "- Analyse", formatte_nombre(metriques.temps_analyse * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_analyse, temps_scene)) });
	tableau.ajoute_ligne({ "- Validation", formatte_nombre(metriques.temps_validation * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_validation, temps_scene)) });

	tableau.ajoute_ligne({ "Temps Coulisse", formatte_nombre(temps_coulisse * 1000.0), "ms", formatte_nombre(calc_pourcentage(temps_coulisse, temps_total)) });
	tableau.ajoute_ligne({ "- Génération Code", formatte_nombre(metriques.temps_generation * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_generation, temps_coulisse)) });
	tableau.ajoute_ligne({ "- Fichier Objet", formatte_nombre(metriques.temps_fichier_objet * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_fichier_objet, temps_coulisse)) });
	tableau.ajoute_ligne({ "- Exécutable", formatte_nombre(metriques.temps_executable * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_executable, temps_coulisse)) });

	tableau.ajoute_ligne({ "Temps Nettoyage", formatte_nombre(metriques.temps_nettoyage * 1000.0), "ms" });

	imprime_tableau(tableau);

	if (ops.imprime_taille_memoire_objet) {
		imprime_taille_memoire_noeud(os);
	}

	return;
}

static void precompile_objet_r16(std::filesystem::path const &chemin_racine_kuri)
{
	auto chemin_fichier = chemin_racine_kuri / "fichiers/r16_tables.cc";
	auto chemin_objet = "/tmp/r16_tables.o";

	if (std::filesystem::exists(chemin_objet)) {
		return;
	}

	auto commande = dls::chaine("g++ -c ");
	commande += chemin_fichier.c_str();
	commande += " -o /tmp/r16_tables.o";

	std::cout << "Compilation des tables de conversion R16...\n";
	std::cout << "Exécution de la commande " << commande << std::endl;

	auto err = system(commande.c_str());

	if (err != 0) {
		std::cerr << "Impossible de compiler les tables de conversion R16 !\n";
		return;
	}

	std::cout << "Compilation réussie !" << std::endl;
}

int main(int argc, char *argv[])
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
		std::cerr << "Impossible d'ouvrir le fichier : " << chemin_fichier << '\n';
		return 1;
	}

	std::ostream &os = std::cout;

	auto resultat = 0;
	auto debut_compilation   = dls::chrono::compte_seconde();
	auto debut_nettoyage     = dls::chrono::compte_seconde(false);
	auto temps_fichier_objet = 0.0;
	auto temps_executable    = 0.0;
	auto est_errone = false;

	auto metriques = Metriques{};

	try {
		precompile_objet_r16(chemin_racine_kuri);

		/* enregistre le dossier d'origine */
		auto dossier_origine = std::filesystem::current_path();

		auto chemin = std::filesystem::path(chemin_fichier);

		if (chemin.is_relative()) {
			chemin = std::filesystem::absolute(chemin);
		}

		auto nom_fichier = chemin.stem();

		auto contexte_generation = ContexteGenerationCode{};
		contexte_generation.bit32 = ops.bit32;
		auto assembleuse = assembleuse_arbre(contexte_generation);

		os << "Lancement de la compilation à partir du fichier '" << chemin_fichier << "'..." << std::endl;

		/* Charge d'abord le module basique. */
		importe_module(os, chemin_racine_kuri, "Kuri", contexte_generation, {});

		/* Change le dossier courant et lance la compilation. */
		auto dossier = chemin.parent_path();
		std::filesystem::current_path(dossier);

		auto module = contexte_generation.cree_module("", dossier.c_str());
		charge_fichier(os, module, chemin_racine_kuri, nom_fichier.c_str(), contexte_generation, {});

		if (ops.emet_arbre) {
			assembleuse.imprime_code(os);
		}

#ifdef AVEC_LLVM
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

			auto module_llvm = llvm::Module("Module", contexte_generation.contexte);
			module_llvm.setDataLayout(machine_cible->createDataLayout());
			module_llvm.setTargetTriple(triplet_cible);

			contexte_generation.module_llvm = &module_llvm;

			initialise_optimisation(ops.optimisation, contexte_generation);

			os << "Validation sémantique du code..." << std::endl;
			noeud::performe_validation_semantique(assembleuse, contexte_generation);

			os << "Génération du code..." << std::endl;
			noeud::genere_code_llvm(assembleuse, contexte_generation);

			if (ops.emet_code_intermediaire) {
				std::cerr <<  "------------------------------------------------------------------\n";
				module_llvm.print(llvm::errs(), nullptr);
				std::cerr <<  "------------------------------------------------------------------\n";
			}

			/* définition du fichier de sortie */
			if (ops.emet_fichier_objet) {
				os << "Écriture du code dans un fichier..." << std::endl;
				auto debut_fichier_objet = dls::chrono::compte_seconde();
				if (!ecris_fichier_objet(machine_cible.get(), module_llvm)) {
					resultat = 1;
				}
				temps_fichier_objet = debut_fichier_objet.temps();

				auto debut_executable = dls::chrono::compte_seconde();
				cree_executable(ops.chemin_sortie, chemin_racine_kuri);
				temps_executable = debut_executable.temps();
			}
		}
		else
#endif
		{
			std::ofstream of;
			of.open("/tmp/compilation_kuri.c");

			os << "Validation sémantique du code..." << std::endl;
			noeud::performe_validation_semantique(assembleuse, contexte_generation);

			os << "Génération du code..." << std::endl;
			noeud::genere_code_C(assembleuse, contexte_generation, chemin_racine_kuri, of);

			of.close();

			auto debut_fichier_objet = dls::chrono::compte_seconde();
			auto commande = dls::chaine("gcc -c /tmp/compilation_kuri.c ");

			/* désactivation des erreurs concernant le manque de "const" quand
			 * on passe des variables générés temporairement par la coulisse à
			 * des fonctions qui dont les paramètres ne sont pas constants */
			commande += "-Wno-discarded-qualifiers ";

			switch (ops.optimisation) {
				case NiveauOptimisation::Aucun:
				case NiveauOptimisation::O0:
				{
					commande += "-O0 ";
					break;
				}
				case NiveauOptimisation::O1:
				{
					commande += "-O1 ";
					break;
				}
				case NiveauOptimisation::O2:
				{
					commande += "-O2 ";
					break;
				}
				case NiveauOptimisation::Os:
				{
					commande += "-Os ";
					break;
				}
				/* Oz est spécifique à LLVM, prend O3 car c'est le plus élevé le
				 * plus proche. */
				case NiveauOptimisation::Oz:
				case NiveauOptimisation::O3:
				{
					commande += "-O3 ";
					break;
				}
			}

			if (ops.bit32) {
				commande += "-m32 ";
			}

			for (auto const &def : assembleuse.definitions) {
				commande += " -D" + dls::chaine(def);
			}

			commande += " -o /tmp/compilation_kuri.o";

			os << "Exécution de la commande '" << commande << "'..." << std::endl;

			auto err = system(commande.c_str());

			temps_fichier_objet = debut_fichier_objet.temps();

			if (err != 0) {
				std::cerr << "Ne peut pas créer le fichier objet !\n";
				est_errone = true;
			}
			else {
				auto debut_executable = dls::chrono::compte_seconde();
				commande = dls::chaine("gcc /tmp/compilation_kuri.o /tmp/r16_tables.o ");

				for (auto const &bib : assembleuse.bibliotheques) {
					commande += " -l" + dls::chaine(bib);
				}

				commande += " -o ";
				commande += ops.chemin_sortie;

				os << "Exécution de la commande '" << commande << "'..." << std::endl;

				err = system(commande.c_str());

				if (err != 0) {
					std::cerr << "Ne peut pas créer l'exécutable !\n";
					est_errone = true;
				}

				temps_executable = debut_executable.temps();
			}
		}

		/* restore le dossier d'origine */
		std::filesystem::current_path(dossier_origine);

		metriques = contexte_generation.rassemble_metriques();
		metriques.memoire_contexte = contexte_generation.memoire_utilisee();
		metriques.memoire_arbre = assembleuse.memoire_utilisee();
		metriques.nombre_noeuds = assembleuse.nombre_noeuds();
		metriques.temps_executable = temps_executable;
		metriques.temps_fichier_objet = temps_fichier_objet;

		os << "Nettoyage..." << std::endl;
		debut_nettoyage = dls::chrono::compte_seconde();
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
		est_errone = true;
	}

	metriques.temps_nettoyage = debut_nettoyage.temps();

	if (!est_errone) {
		imprime_stats(os, metriques, ops, debut_compilation);
	}

#ifdef AVEC_LLVM
	issitialise_llvm();
#endif

	return resultat;
}
