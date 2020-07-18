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

#include "coulisse_llvm/generation_code_llvm.hh"
#endif

#include "biblexternes/iprof/prof.h"

#include <thread>

#include "compilation/assembleuse_arbre.h"
#include "compilation/compilatrice.hh"
#include "compilation/erreur.h"
#include "compilation/modules.hh"
#include "compilation/profilage.hh"
#include "compilation/tacheronne.hh"

#include "coulisse_c/generation_code_c.hh"

#include "representation_intermediaire/constructrice_ri.hh"

#include "date.hh"
#include "options.hh"

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/outils/format.hh"
#include "biblinternes/outils/tableau_donnees.hh"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/systeme_fichier/shared_library.h"

#define AVEC_THREADS

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
		llvm::legacy::FunctionPassManager &manager_fonctions,
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

	builder.populateFunctionPassManager(manager_fonctions);
}

/**
 * Initialise le ménageur de passes fonctions du contexte selon le niveau
 * d'optimisation.
 */
static void initialise_optimisation(
		NiveauOptimisation optimisation,
		GeneratriceCodeLLVM &contexte)
{
	if (contexte.manager_fonctions == nullptr) {
		contexte.manager_fonctions = new llvm::legacy::FunctionPassManager(contexte.m_module);
	}

	switch (optimisation) {
		case NiveauOptimisation::AUCUN:
			break;
		case NiveauOptimisation::O0:
			ajoute_passes(*contexte.manager_fonctions, 0, 0);
			break;
		case NiveauOptimisation::O1:
			ajoute_passes(*contexte.manager_fonctions, 1, 0);
			break;
		case NiveauOptimisation::O2:
			ajoute_passes(*contexte.manager_fonctions, 2, 0);
			break;
		case NiveauOptimisation::Os:
			ajoute_passes(*contexte.manager_fonctions, 2, 1);
			break;
		case NiveauOptimisation::Oz:
			ajoute_passes(*contexte.manager_fonctions, 2, 2);
			break;
		case NiveauOptimisation::O3:
			ajoute_passes(*contexte.manager_fonctions, 3, 0);
			break;
	}
}

static bool ecris_fichier_objet(llvm::TargetMachine *machine_cible, llvm::Module &module)
{
	Prof(ecris_fichier_objet);
#if 1
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
#else
	// https://stackoverflow.com/questions/1419139/llvm-linking-problem?rq=1
	std::error_code ec;
	llvm::raw_fd_ostream dest("/tmp/kuri.ll", ec, llvm::sys::fs::F_None);
	module.print(dest, nullptr);

	system("llvm-as /tmp/kuri.ll -o /tmp/kuri.bc");
	system("llc /tmp/kuri.bc -o /tmp/kuri.s");
#endif
	return true;
}

#ifndef NDEBUG
static bool valide_llvm_ir(llvm::Module &module)
{
	std::error_code ec;
	llvm::raw_fd_ostream dest("/tmp/kuri.ll", ec, llvm::sys::fs::F_None);
	module.print(dest, nullptr);

	auto err = system("llvm-as /tmp/kuri.ll -o /tmp/kuri.bc");
	return err == 0;
}
#endif

static bool cree_executable(const kuri::chaine &dest, const std::filesystem::path &racine_kuri)
{
	Prof(cree_executable);
	/* Compile le fichier objet qui appelera 'fonction principale'. */
	if (!std::filesystem::exists("/tmp/execution_kuri.o")) {
		auto const &chemin_execution_S = racine_kuri / "fichiers/execution_kuri.S";

		dls::flux_chaine ss;
		ss << "as -o /tmp/execution_kuri.o ";
		ss << chemin_execution_S;

		auto err = system(ss.chn().c_str());

		if (err != 0) {
			std::cerr << "Ne peut pas créer /tmp/execution_kuri.o !\n";
			return false;
		}
	}

	if (!std::filesystem::exists("/tmp/kuri.o")) {
		std::cerr << "Le fichier objet n'a pas été émis !\n Utiliser la commande -o !\n";
		return false;
	}

	dls::flux_chaine ss;
#if 1
	ss << "gcc ";
	ss << racine_kuri / "fichiers/point_d_entree.c";
	ss << " /tmp/kuri.o /tmp/r16_tables.o -o " << dest;
#else
	ss << "ld ";
	/* ce qui chargera le programme */
	ss << "-dynamic-linker /lib64/ld-linux-x86-64.so.2 ";
	ss << "-m elf_x86_64 ";
	ss << "--hash-style=gnu ";
	ss << "-lc ";
	ss << "/tmp/execution_kuri.o ";
	ss << "/tmp/kuri.o ";
	ss << "-o " << dest;
#endif

	auto err = system(ss.chn().c_str());

	if (err != 0) {
		std::cerr << "Ne peut pas créer l'executable !\n";
		return false;
	}

	return true;
}
#endif

static void imprime_stats(
		Metriques const &metriques,
		dls::chrono::compte_seconde debut_compilation)
{
	Prof(imprime_stats);
	auto const temps_total = debut_compilation.temps();

	auto const temps_scene = metriques.temps_tampon
							 + metriques.temps_decoupage
							 + metriques.temps_analyse
							 + metriques.temps_chargement
							 + metriques.temps_validation
							 + metriques.temps_metaprogrammes
							 + metriques.temps_ri;

	auto const temps_coulisse = metriques.temps_generation
								+ metriques.temps_fichier_objet
								+ metriques.temps_executable;

	auto const temps_aggrege = temps_scene + temps_coulisse + metriques.temps_nettoyage;

	auto calc_pourcentage = [&](const double &x, const double &total)
	{
		return (x * 100.0 / total);
	};

	auto const mem_totale = metriques.memoire_tampons
							+ metriques.memoire_lexemes
							+ metriques.memoire_arbre
							+ metriques.memoire_compilatrice
							+ metriques.memoire_graphe
							+ metriques.memoire_types
							+ metriques.memoire_operateurs
							+ metriques.memoire_ri;

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
	tableau.ajoute_ligne({ "Lignes / seconde", formatte_nombre(debit_lignes), "" });
	tableau.ajoute_ligne({ "Lignes / seconde (scène)", formatte_nombre(debit_lignes_scene), "" });
	tableau.ajoute_ligne({ "Lignes / seconde (coulisse)", formatte_nombre(debit_lignes_coulisse), "" });
	tableau.ajoute_ligne({ "Débit par seconde", formatte_nombre(debit_seconde), "o/s" });

	tableau.ajoute_ligne({ "Arbre Syntaxique", "", "" });
	tableau.ajoute_ligne({ "- Nombre Identifiants", formatte_nombre(metriques.nombre_identifiants), "" });
	tableau.ajoute_ligne({ "- Nombre Lexèmes", formatte_nombre(metriques.nombre_lexemes), "" });
	tableau.ajoute_ligne({ "- Nombre Noeuds", formatte_nombre(metriques.nombre_noeuds), "" });
	tableau.ajoute_ligne({ "- Nombre Noeuds Déps", formatte_nombre(metriques.nombre_noeuds_deps), "" });
	tableau.ajoute_ligne({ "- Nombre Opérateurs", formatte_nombre(metriques.nombre_operateurs), "" });
	tableau.ajoute_ligne({ "- Nombre Types", formatte_nombre(metriques.nombre_types), "" });

	tableau.ajoute_ligne({ "Mémoire", "", "" });
	tableau.ajoute_ligne({ "- Suivie", formatte_nombre(mem_totale), "o" });
	tableau.ajoute_ligne({ "- Effective", formatte_nombre(memoire_consommee), "o" });
	tableau.ajoute_ligne({ "- Arbre", formatte_nombre(metriques.memoire_arbre), "o" });
	tableau.ajoute_ligne({ "- Compilatrice", formatte_nombre(metriques.memoire_compilatrice), "o" });
	tableau.ajoute_ligne({ "- Graphe", formatte_nombre(metriques.memoire_graphe), "o" });
	tableau.ajoute_ligne({ "- Lexèmes", formatte_nombre(metriques.memoire_lexemes), "o" });
	tableau.ajoute_ligne({ "- MV", formatte_nombre(metriques.memoire_mv), "o" });
	tableau.ajoute_ligne({ "- Opérateurs", formatte_nombre(metriques.memoire_operateurs), "o" });
	tableau.ajoute_ligne({ "- RI", formatte_nombre(metriques.memoire_ri), "o" });
	tableau.ajoute_ligne({ "- Tampon", formatte_nombre(metriques.memoire_tampons), "o" });
	tableau.ajoute_ligne({ "- Types", formatte_nombre(metriques.memoire_types), "o" });
	tableau.ajoute_ligne({ "Nombre allocations", formatte_nombre(memoire::nombre_allocations()), "" });
	tableau.ajoute_ligne({ "Nombre métaprogrammes", formatte_nombre(metriques.nombre_metaprogrammes_executes), "" });

	tableau.ajoute_ligne({ "Temps Scène", formatte_nombre(temps_scene * 1000.0), "ms", formatte_nombre(calc_pourcentage(temps_scene, temps_total)) });
	tableau.ajoute_ligne({ "- Chargement", formatte_nombre(metriques.temps_chargement * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_chargement, temps_scene)) });
	tableau.ajoute_ligne({ "- Tampon", formatte_nombre(metriques.temps_tampon * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_tampon, temps_scene)) });
	tableau.ajoute_ligne({ "- Lexage", formatte_nombre(metriques.temps_decoupage * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_decoupage, temps_scene)) });
	tableau.ajoute_ligne({ "- Métaprogrammes", formatte_nombre(metriques.temps_metaprogrammes * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_analyse, temps_scene)) });
	tableau.ajoute_ligne({ "- Syntaxage", formatte_nombre(metriques.temps_analyse * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_analyse, temps_scene)) });
	tableau.ajoute_ligne({ "- Typage", formatte_nombre(metriques.temps_validation * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_validation, temps_scene)) });
	tableau.ajoute_ligne({ "- RI", formatte_nombre(metriques.temps_ri * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_ri, temps_scene)) });

	tableau.ajoute_ligne({ "Temps Coulisse", formatte_nombre(temps_coulisse * 1000.0), "ms", formatte_nombre(calc_pourcentage(temps_coulisse, temps_total)) });
	tableau.ajoute_ligne({ "- Génération Code", formatte_nombre(metriques.temps_generation * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_generation, temps_coulisse)) });
	tableau.ajoute_ligne({ "- Fichier Objet", formatte_nombre(metriques.temps_fichier_objet * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_fichier_objet, temps_coulisse)) });
	tableau.ajoute_ligne({ "- Exécutable", formatte_nombre(metriques.temps_executable * 1000.0), "ms", formatte_nombre(calc_pourcentage(metriques.temps_executable, temps_coulisse)) });

	tableau.ajoute_ligne({ "Temps Nettoyage", formatte_nombre(metriques.temps_nettoyage * 1000.0), "ms" });

	imprime_tableau(tableau);

	return;
}

static void precompile_objet_r16(std::filesystem::path const &chemin_racine_kuri)
{
	// objet pour la liaison statique de la bibliothèque
	{
		auto chemin_fichier = chemin_racine_kuri / "fichiers/r16_tables.cc";
		auto chemin_objet = "/tmp/r16_tables.o";

		if (!std::filesystem::exists(chemin_objet)) {
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

			std::cout << "Compilation du fichier statique réussie !" << std::endl;
		}
	}

	// objet pour la liaison dynamique de la bibliothèque, pour les métaprogrammes
	{
		auto chemin_fichier = chemin_racine_kuri / "fichiers/r16_tables.cc";
		auto chemin_objet = "/tmp/r16_tables.so";

		if (!std::filesystem::exists(chemin_objet)) {
			auto commande = dls::chaine("g++ -shared -fPIC ");
			commande += chemin_fichier.c_str();
			commande += " -o /tmp/r16_tables.so";

			std::cout << "Compilation des tables de conversion R16...\n";
			std::cout << "Exécution de la commande " << commande << std::endl;

			auto err = system(commande.c_str());

			if (err != 0) {
				std::cerr << "Impossible de compiler les tables de conversion R16 !\n";
				return;
			}

			std::cout << "Compilation du fichier dynamique réussie !" << std::endl;
		}
	}
}

void lance_tacheronne(Tacheronne *tacheronne)
{
	try {
		tacheronne->gere_tache();
	}
	catch (const erreur::frappe &e) {
		std::cerr << e.message() << '\n';
		tacheronne->compilatrice.possede_erreur = true;
	}
}

void lance_file_execution(Compilatrice *compilatrice)
{
	auto constructrice_ri = ConstructriceRI(*compilatrice);

	// en dehors de la boucle pour réutiliser la mémoire
	dls::tableau<AtomeGlobale *> globales;
	dls::tableau<AtomeFonction *> fonctions;

	while (!compilatrice->compilation_terminee()) {
		if (compilatrice->file_execution->est_vide()) {
			continue;
		}

		// N'effronte pas sinon les autres threads pourraient s'arrêter alors que le métaprogramme pourrait ajouter des choses à compiler.
		auto unite = compilatrice->file_execution->front();
		auto noeud = unite.noeud;
		auto espace = unite.espace;

		auto index_dans_table_type = 1u;

		globales.efface();
		fonctions.efface();

		traverse_graphe(noeud->fonction->noeud_dependance, [&](NoeudDependance *noeud_dep)
		{
			if (noeud_dep->type == TypeNoeudDependance::FONCTION) {
				auto decl_noeud = static_cast<NoeudDeclarationFonction *>(noeud_dep->noeud_syntaxique);

				if ((decl_noeud->drapeaux & CODE_BINAIRE_FUT_GENERE) != 0) {
					return;
				}

				auto atome_fonction = espace->trouve_fonction(decl_noeud->nom_broye);
				fonctions.pousse(atome_fonction);
				decl_noeud->drapeaux |= CODE_BINAIRE_FUT_GENERE;
			}
			else if (noeud_dep->type == TypeNoeudDependance::TYPE) {
				auto type = noeud_dep->type_;

				if ((type->drapeaux & CODE_BINAIRE_TYPE_FUT_GENERE) != 0) {
					return;
				}

				type->index_dans_table_types = index_dans_table_type++;

				if (type->genre == GenreType::STRUCTURE || type->genre == GenreType::UNION) {
					auto nom_fonction = "initialise_" + dls::vers_chaine(type);
					auto atome_fonction = espace->trouve_fonction(nom_fonction);
					fonctions.pousse(atome_fonction);
					type->drapeaux |= CODE_BINAIRE_TYPE_FUT_GENERE;
				}
			}
			else if (noeud_dep->type == TypeNoeudDependance::GLOBALE) {
				auto decl_noeud = static_cast<NoeudDeclaration *>(noeud_dep->noeud_syntaxique);

				if ((decl_noeud->drapeaux & EST_CONSTANTE) != 0) {
					return;
				}

				if ((decl_noeud->drapeaux & CODE_BINAIRE_FUT_GENERE) != 0) {
					return;
				}

				auto atome_globale = espace->trouve_globale(decl_noeud);

				if (atome_globale->index == -1) {
					atome_globale->index = compilatrice->mv.ajoute_globale(decl_noeud->type, decl_noeud->ident);
				}

				globales.pousse(atome_globale);

				decl_noeud->drapeaux |= CODE_BINAIRE_FUT_GENERE;
			}
		});

		POUR_TABLEAU_PAGE (espace->graphe_dependance->noeuds) {
			it.fut_visite = false;
		}

		auto fonction = espace->trouve_fonction(noeud->fonction->nom_broye);

		if (!fonction) {
			std::cerr << "Impossible de trouver la fonction métaprogramme pour " << noeud->fonction->nom_broye << '\n';
			imprime_fichier_ligne(*espace, *noeud->lexeme);
		}

		//desassemble(fonction->chunk, noeud->fonction->nom_broye.c_str(), std::cout);

		if (globales.taille() != 0) {
			auto fonc_init = constructrice_ri.genere_fonction_init_globales_et_appel(globales, fonction);
			fonctions.pousse(fonc_init);
		}

		POUR (fonctions) {
			genere_code_binaire_pour_fonction(it, &compilatrice->mv);
		}

		auto res = compilatrice->mv.interprete(fonction);

		if (res == MachineVirtuelle::ResultatInterpretation::ERREUR) {
			// À FAIRE : erreur de compilation si une erreur d'exécution
			std::cerr << "Erreur lors de l'exécution du métaprogramme !\n-- ";
			imprime_fichier_ligne(*espace, *noeud->lexeme);
		}
		else {
			if (noeud->ident == ID::assert_) {
				auto resultat = *reinterpret_cast<bool *>(compilatrice->mv.pointeur_pile);

				if (!resultat) {
					// À FAIRE : erreur de compilation si une assertion échoue
					std::cerr << "Échec de l'assertion !\n-- ";
					imprime_fichier_ligne(*espace, *noeud->lexeme);
				}
			}
		}

		compilatrice->file_execution->effronte();
	}
}

static int genere_code_coulisse(
		Compilatrice &compilatrice,
		EspaceDeTravail &espace,
		double &temps_executable,
		double &temps_fichier_objet)
{
	auto const &ops = espace.options;

	if (!ops.cree_executable) {
		return 0;
	}

#ifdef AVEC_LLVM
	if (ops.type_coulisse == TypeCoulisse::LLVM) {
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
		auto RM = llvm::Optional<llvm::Reloc::Model>(llvm::Reloc::PIC_);
		auto machine_cible = std::unique_ptr<llvm::TargetMachine>(
					cible->createTargetMachine(
						triplet_cible, CPU, feature, options_cible, RM));

		auto generatrice = GeneratriceCodeLLVM(espace);

		auto module_llvm = llvm::Module("Module", generatrice.m_contexte_llvm);
		module_llvm.setDataLayout(machine_cible->createDataLayout());
		module_llvm.setTargetTriple(triplet_cible);

		generatrice.m_module = &module_llvm;

		std::cout << "Génération du code..." << std::endl;
		auto temps_generation = dls::chrono::compte_seconde();

		initialise_optimisation(ops.niveau_optimisation, generatrice);

		generatrice.genere_code();

		compilatrice.temps_generation = temps_generation.temps();

#ifndef NDEBUG
		if (!valide_llvm_ir(module_llvm)) {
			compilatrice.possede_erreur = true;
		}
#endif

		/* définition du fichier de sortie */
		if (!compilatrice.possede_erreur && ops.cree_executable) {
			std::cout << "Écriture du code dans un fichier..." << std::endl;
			auto debut_fichier_objet = dls::chrono::compte_seconde();
			if (!ecris_fichier_objet(machine_cible.get(), module_llvm)) {
				compilatrice.possede_erreur = true;
				return 1;
			}
			temps_fichier_objet = debut_fichier_objet.temps();

			auto debut_executable = dls::chrono::compte_seconde();
			if (!cree_executable(ops.nom_sortie, compilatrice.racine_kuri.c_str())) {
				compilatrice.possede_erreur = true;
				return 1;
			}

			temps_executable = debut_executable.temps();
		}
	}
	else
#endif
	{
		std::ofstream of;
		of.open("/tmp/compilation_kuri.c");

		std::cout << "Génération du code..." << std::endl;
		genere_code_C(compilatrice, espace, compilatrice.racine_kuri, of);

		of.close();

		auto debut_fichier_objet = dls::chrono::compte_seconde();
		auto commande = dls::chaine("gcc -c /tmp/compilation_kuri.c ");

		/* désactivation des erreurs concernant le manque de "const" quand
		 * on passe des variables générés temporairement par la coulisse à
		 * des fonctions qui dont les paramètres ne sont pas constants */
		commande += "-Wno-discarded-qualifiers ";
		/* désactivation des avertissements de passage d'une variable au
		 * lieu d'une chaine littérale à printf et al. */
		commande += "-Wno-format-security ";

		switch (ops.niveau_optimisation) {
			case NiveauOptimisation::AUCUN:
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

		if (ops.architecture_cible == ArchitectureCible::X86) {
			commande += "-m32 ";
		}

		for (auto const &def : *compilatrice.definitions.verrou_lecture()) {
			commande += " -D" + dls::chaine(def);
		}

		for (auto const &chm : *compilatrice.chemins.verrou_lecture()) {
			commande += " ";
			commande += chm;
		}

		commande += " -o /tmp/compilation_kuri.o";

		std::cout << "Exécution de la commande '" << commande << "'..." << std::endl;

		auto err = system(commande.c_str());

		temps_fichier_objet = debut_fichier_objet.temps();

		if (err != 0) {
			std::cerr << "Ne peut pas créer le fichier objet !\n";
			compilatrice.possede_erreur = true;
		}
		else {
			auto debut_executable = dls::chrono::compte_seconde();
			commande = dls::chaine("gcc /tmp/compilation_kuri.o /tmp/r16_tables.o ");

			for (auto const &chm : *compilatrice.chemins.verrou_lecture()) {
				commande += " ";
				commande += chm;
			}

			for (auto const &bib : *compilatrice.bibliotheques_statiques.verrou_lecture()) {
				commande += " " + bib;
			}

			for (auto const &bib : *compilatrice.bibliotheques_dynamiques.verrou_lecture()) {
				commande += " -l" + bib;
			}

			commande += " -o ";
			commande += dls::chaine(ops.nom_sortie.pointeur, ops.nom_sortie.taille);

			std::cout << "Exécution de la commande '" << commande << "'..." << std::endl;

			err = system(commande.c_str());

			if (err != 0) {
				std::cerr << "Ne peut pas créer l'exécutable !\n";
				compilatrice.possede_erreur = true;
				return 1;
			}

			temps_executable = debut_executable.temps();
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	Prof(main);

	std::ios::sync_with_stdio(false);

	if (argc < 2) {
		std::cerr << "Utilisation : " << argv[0] << " FICHIER\n";
		return 1;
	}

	auto const &chemin_racine_kuri = getenv("RACINE_KURI");

	if (chemin_racine_kuri == nullptr) {
		std::cerr << "Impossible de trouver le chemin racine de l'installation de kuri !\n";
		std::cerr << "Possible solution : veuillez faire en sorte que la variable d'environnement 'RACINE_KURI' soit définie !\n";
		return 1;
	}

	auto const chemin_fichier = argv[1];

	if (!std::filesystem::exists(chemin_fichier)) {
		std::cerr << "Impossible d'ouvrir le fichier : " << chemin_fichier << '\n';
		return 1;
	}

	auto ops = OptionsCompilation();
	initialise_options_compilation(ops);

	std::ostream &os = std::cout;

	auto resultat = 0;
	auto debut_compilation   = dls::chrono::compte_seconde();
	auto debut_nettoyage     = dls::chrono::compte_seconde(false);
	auto temps_fichier_objet = 0.0;
	auto temps_executable    = 0.0;

	precompile_objet_r16(chemin_racine_kuri);

	auto metriques = Metriques{};
	auto compilatrice = Compilatrice{};

	try {
		/* enregistre le dossier d'origine */
		auto dossier_origine = std::filesystem::current_path();

		auto chemin = std::filesystem::path(chemin_fichier);

		if (chemin.is_relative()) {
			chemin = std::filesystem::absolute(chemin);
		}

		auto nom_fichier = chemin.stem();

		compilatrice.racine_kuri = chemin_racine_kuri;
		compilatrice.bit32 = ops.architecture_cible == ArchitectureCible::X86;

		auto &constructrice_ri = compilatrice.constructrice_ri;

		/* Charge d'abord le module basique. */
		auto espace_defaut = compilatrice.demarre_un_espace_de_travail(ops, "Espace 1");

		auto dossier = chemin.parent_path();
		std::filesystem::current_path(dossier);

		os << "Lancement de la compilation à partir du fichier '" << chemin_fichier << "'..." << std::endl;

		auto module = espace_defaut->cree_module("", dossier.c_str());
		compilatrice.ajoute_fichier_a_la_compilation(espace_defaut, nom_fichier.c_str(), module, {});

		auto tacheronne = Tacheronne(compilatrice);

#ifdef AVEC_THREADS
		std::thread file_tacheronne(lance_tacheronne, &tacheronne);
		std::thread file_execution(lance_file_execution, &compilatrice);

		file_tacheronne.join();
		file_execution.join();
#else
		lance_tacheronne(&tacheronne);
		lance_file_execution(&compilatrice);
#endif

		if (!compilatrice.possede_erreur) {
			POUR_TABLEAU_PAGE ((*compilatrice.espaces_de_travail.verrou_ecriture())) {
				if (genere_code_coulisse(compilatrice, it, temps_executable, temps_fichier_objet)) {
					compilatrice.possede_erreur = true;
					break;
				}
			}
		}

		if (compilatrice.chaines_ajoutees_a_la_compilation->taille()) {
			auto fichier_chaines = std::ofstream(".chaines_ajoutées");

			auto d = hui_systeme();

			fichier_chaines << "Fichier créé le " << d.jour << "/" << d.mois << "/" << d.annee
							<< " à " << d.heure << ':' << d.minute << ':' << d.seconde << "\n\n";

			POUR (*compilatrice.chaines_ajoutees_a_la_compilation.verrou_lecture()) {
				fichier_chaines << it;
				fichier_chaines << '\n';
			}
		}

		auto temps_ri = constructrice_ri.temps_generation + tacheronne.constructrice_ri.temps_generation;
		auto memoire_ri = constructrice_ri.memoire_utilisee() + tacheronne.constructrice_ri.memoire_utilisee();

		/* restore le dossier d'origine */
		std::filesystem::current_path(dossier_origine);

		metriques = compilatrice.rassemble_metriques();
		metriques.memoire_compilatrice = compilatrice.memoire_utilisee();
		metriques.temps_executable = temps_executable;
		metriques.temps_fichier_objet = temps_fichier_objet;
		metriques.temps_ri = temps_ri;
		metriques.memoire_ri = memoire_ri;
		metriques.temps_decoupage = compilatrice.temps_lexage;
		metriques.temps_validation = compilatrice.temps_validation;
		metriques.nombre_identifiants = static_cast<size_t>(compilatrice.table_identifiants->taille());
		metriques.temps_metaprogrammes = compilatrice.mv.temps_execution_metaprogammes;
		metriques.nombre_metaprogrammes_executes = static_cast<size_t>(compilatrice.mv.nombre_de_metaprogrammes_executes);

		os << "Nettoyage..." << std::endl;
		debut_nettoyage = dls::chrono::compte_seconde();
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
		compilatrice.possede_erreur = true;
		resultat = static_cast<int>(erreur_frappe.type());
	}

	metriques.temps_nettoyage = debut_nettoyage.temps();

	if (!compilatrice.possede_erreur && ops.emets_metriques) {
		imprime_stats(metriques, debut_compilation);
	}

#ifdef AVEC_LLVM
	issitialise_llvm();
#endif

#ifdef PROFILAGE
	imprime_profilage(std::cerr);
#endif

	return resultat;
}
