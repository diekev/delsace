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
#include "decoupage/preproces.hh"
#include "decoupage/tampon_source.h"

#include <chronometrage/chronometre_de_portee.h>

struct OptionsCompilation {
	const char *chemin_fichier = nullptr;
	bool emet_fichier_objet = false;
	bool emet_code_intermediaire = false;
	bool emet_arbre = false;
	bool imprime_taille_memoire_objet = false;
	char pad[4];
};

static OptionsCompilation genere_options_compilation(int argc, char **argv)
{
	OptionsCompilation opts;

	for (int i = 1; i < argc; ++i) {
		if (std::strcmp(argv[i], "-ir") == 0) {
			opts.emet_code_intermediaire = true;
		}
		else if (std::strcmp(argv[i], "-o") == 0) {
			opts.emet_fichier_objet = true;
		}
		else if (std::strcmp(argv[i], "-a") == 0) {
			opts.emet_arbre = true;
		}
		else if (std::strcmp(argv[i], "-m") == 0) {
			opts.imprime_taille_memoire_objet = true;
		}
		else {
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

static void cree_executable()
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
	ss << "/tmp/kuri.o";

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
	auto temps_chargement      = 0.0;
	auto temps_tampon          = 0.0;
	auto temps_decoupage       = 0.0;
	auto temps_analyse         = 0.0;
	auto temps_generation_code = 0.0;
	auto debut_nettoyage       = 0.0;
	auto temps_nettoyage       = 0.0;
	auto mem_morceaux          = 0ul;
	auto mem_arbre             = 0ul;
	auto mem_contexte          = 0ul;
	auto nombre_morceaux       = 0ul;
	auto nombre_noeuds         = 0ul;

	os << "Ouverture de '" << chemin_fichier << "'..." << std::endl;
	auto debut_chargement = numero7::chronometrage::maintenant();
	auto preproces = Preproces{};
	charge_fichier(preproces, chemin_fichier);
	temps_chargement = numero7::chronometrage::maintenant() - debut_chargement;

	os << "Génération du tampon texte..." << std::endl;
	const auto debut_tampon = numero7::chronometrage::maintenant();
	auto tampon = TamponSource(preproces.tampon);
	temps_tampon = numero7::chronometrage::maintenant() - debut_tampon;

	try {
		auto contexte_generation = ContexteGenerationCode(tampon);
		auto decoupeuse = decoupeuse_texte(tampon);

		os << "Découpage du texte..." << std::endl;
		const auto debut_decoupeuse = numero7::chronometrage::maintenant();
		decoupeuse.genere_morceaux();
		mem_morceaux = decoupeuse.memoire_morceaux();
		nombre_morceaux = decoupeuse.morceaux().size();
		temps_decoupage = numero7::chronometrage::maintenant() - debut_decoupeuse;

		auto assembleuse = assembleuse_arbre();
		auto analyseuse = analyseuse_grammaire(contexte_generation, decoupeuse.morceaux(), tampon, &assembleuse);

		os << "Analyse des morceaux..." << std::endl;
		const auto debut_analyseuse = numero7::chronometrage::maintenant();
		analyseuse.lance_analyse();
#ifdef DEBOGUE_IDENTIFIANT
		analyseuse.imprime_identifiants_plus_utilises(os);
#endif
		mem_arbre = assembleuse.memoire_utilisee();
		nombre_noeuds = assembleuse.nombre_noeuds();
		temps_analyse = numero7::chronometrage::maintenant() - debut_analyseuse;

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

		auto module = llvm::Module(chemin_fichier, contexte_generation.contexte);
		module.setDataLayout(machine_cible->createDataLayout());
		module.setTargetTriple(triplet_cible);

		contexte_generation.module = &module;

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
		const auto debut_generation_code = numero7::chronometrage::maintenant();
		assembleuse.genere_code_llvm(contexte_generation);
		temps_generation_code = numero7::chronometrage::maintenant() - debut_generation_code;

		mem_contexte = contexte_generation.memoire_utilisee();

		if (ops.emet_code_intermediaire) {
			std::cerr <<  "------------------------------------------------------------------\n";
			module.dump();
			std::cerr <<  "------------------------------------------------------------------\n";
		}

		if (ops.emet_arbre) {
			assembleuse.imprime_code(os);
		}

		/* définition du fichier de sortie */
		if (ops.emet_fichier_objet) {
			os << "Écriture du code dans un fichier..." << std::endl;
			if (!ecris_fichier_objet(machine_cible.get(), module)) {
				resultat = 1;
			}

			cree_executable();
		}

		os << "Nettoyage..." << std::endl;
		debut_nettoyage = numero7::chronometrage::maintenant();
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
	}

	temps_nettoyage = numero7::chronometrage::maintenant() - debut_nettoyage;

	const auto temps_scene = temps_tampon
							 + temps_decoupage
							 + temps_analyse
							 + temps_chargement;

	const auto temps_coulisse = temps_generation_code;

	const auto temps_total = temps_scene + temps_coulisse + temps_nettoyage;

	auto calc_pourcentage = [&](const double &x, const double &total)
	{
		return pourcentage(x * 100.0 / total);
	};

	os << "------------------------------------------------------------------\n";
	os << "Temps total                  : " << temps_seconde(temps_total) << '\n';
	os << "Nombre de lignes             : " << tampon.nombre_lignes() << '\n';
	os << "Nombre de lignes par seconde : " << tampon.nombre_lignes() / temps_total << '\n';
	os << "Débit par seconde            : " << taille_octet(static_cast<size_t>(tampon.taille_donnees() / temps_total)) << '\n';

	const auto mem_totale = tampon.taille_donnees()
							+ mem_morceaux
							+ mem_arbre
							+ mem_contexte;

	os << '\n';
	os << "Métriques :\n";
	os << "\tNombre morceaux : " << nombre_morceaux << '\n';
	os << "\tNombre noeuds   : " << nombre_noeuds << '\n';

	os << '\n';
	os << "Mémoire : " << taille_octet(mem_totale) << '\n';
	os << "\tTampon   : " << taille_octet(tampon.taille_donnees()) << '\n';
	os << "\tMorceaux : " << taille_octet(mem_morceaux) << '\n';
	os << "\tArbre    : " << taille_octet(mem_arbre) << '\n';
	os << "\tContexte : " << taille_octet(mem_contexte) << '\n';

	os << '\n';
	os << "Temps scène : " << temps_seconde(temps_scene)
	   << " (" << calc_pourcentage(temps_scene, temps_total) << ")\n";
	os << '\t' << "Temps chargement : " << temps_seconde(temps_chargement)
	   << " (" << calc_pourcentage(temps_chargement, temps_scene) << ")\n";
	os << '\t' << "Temps tampon     : " << temps_seconde(temps_tampon)
	   << " (" << calc_pourcentage(temps_tampon, temps_scene) << ")\n";
	os << '\t' << "Temps découpage  : " << temps_seconde(temps_decoupage)
	   << " (" << calc_pourcentage(temps_decoupage, temps_scene) << ") ("
	   << taille_octet(static_cast<size_t>(tampon.taille_donnees() / temps_decoupage)) << ")\n";
	os << '\t' << "Temps analyse    : " << temps_seconde(temps_analyse)
	   << " (" << calc_pourcentage(temps_analyse, temps_scene) << ") ("
	   << taille_octet(static_cast<size_t>(mem_morceaux / temps_analyse)) << ")\n";

	os << '\n';
	os << "Temps coulisse : " << temps_seconde(temps_coulisse)
	   << " (" << calc_pourcentage(temps_coulisse, temps_total) << ")\n";
	os << '\t' << "Temps génération code : " << temps_seconde(temps_generation_code)
	   << " (" << calc_pourcentage(temps_generation_code, temps_coulisse) << ")\n";

	os << '\n';
	os << "Temps Nettoyage : " << temps_seconde(temps_nettoyage)
	   << " (" << calc_pourcentage(temps_nettoyage, temps_total) << ")\n";

	if (ops.imprime_taille_memoire_objet) {
		imprime_taille_memoire_noeud(os);
	}

	os << std::endl;

	return resultat;
}
