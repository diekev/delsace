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
#include <experimental/filesystem>
#include <fstream>
#include <iostream>

#include <cstdio>

#include "decoupage/analyseuse_grammaire.h"
#include "decoupage/arbre_syntactic.h"
#include "decoupage/decoupeuse.h"
#include "decoupage/erreur.h"
#include "decoupage/tampon_source.h"

#include <chronometrage/chronometre_de_portee.h>

static std::string charge_fichier(const char *chemin_fichier)
{
	std::ifstream fichier;
	fichier.open(chemin_fichier);

	if (!fichier) {
		return "";
	}

	fichier.seekg(0, fichier.end);
	const auto taille_fichier = static_cast<std::string::size_type>(fichier.tellg());
	fichier.seekg(0, fichier.beg);

	std::string texte;
	texte.reserve(taille_fichier);

	std::string tampon;

	while (std::getline(fichier, tampon)) {
		/* restore le caractère de fin de ligne */
		tampon.append(1, '\n');

		texte += tampon;
	}

	return texte;
}

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

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

int main(int argc, char *argv[])
{
	std::ios::sync_with_stdio(false);

	if (argc != 2) {
		std::cerr << "Utilisation : " << argv[0] << " FICHIER\n";
		return 1;
	}

	const auto chemin_fichier = argv[1];

	if (!std::experimental::filesystem::exists(chemin_fichier)) {
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

	os << "Ouverture de '" << chemin_fichier << "'...\n";
	auto debut_chargement = numero7::chronometrage::maintenant();
	auto texte = charge_fichier(chemin_fichier);
	temps_chargement = numero7::chronometrage::maintenant() - debut_chargement;

	const auto debut_tampon = numero7::chronometrage::maintenant();
	auto tampon = TamponSource(texte);
	temps_tampon = numero7::chronometrage::maintenant() - debut_tampon;

	try {
		auto decoupeuse = decoupeuse_texte(tampon);

		const auto debut_decoupeuse = numero7::chronometrage::maintenant();
		decoupeuse.genere_morceaux();
		temps_decoupage = numero7::chronometrage::maintenant() - debut_decoupeuse;

		auto assembleuse = assembleuse_arbre();
		auto analyseuse = analyseuse_grammaire(tampon, &assembleuse);

		const auto debut_analyseuse = numero7::chronometrage::maintenant();
		analyseuse.lance_analyse(decoupeuse.morceaux());
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
		auto machine_cible = cible->createTargetMachine(triplet_cible, CPU, feature, options, RM);

		ContexteGenerationCode contexte_generation;
		auto module = llvm::Module(chemin_fichier, contexte_generation.contexte);
		module.setDataLayout(machine_cible->createDataLayout());
		module.setTargetTriple(triplet_cible);

		contexte_generation.module = &module;

		const auto debut_generation_code = numero7::chronometrage::maintenant();
		assembleuse.genere_code_llvm(contexte_generation);
		temps_generation_code = numero7::chronometrage::maintenant() - debut_generation_code;

		/* définition du fichier de sortie */
		const auto emet_fichier_objet = false;

		if (emet_fichier_objet) {
			if (!ecris_fichier_objet(machine_cible, module)) {
				resultat = 1;
			}
		}
		else {
			module.dump();
		}

		delete machine_cible;
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
	}
	catch (const char *e) {
		std::cerr << e << '\n';
	}

	const auto temps_scene = temps_tampon
							 + temps_decoupage
							 + temps_analyse
							 + temps_chargement;

	const auto temps_coulisse = temps_generation_code;

	const auto temps_total = temps_scene + temps_coulisse;

	auto pourcentage = [&](const double &x, const double &total)
	{
		return x * 100.0 / total;
	};

	os << "------------------------------------------------------------------\n";
	os << "Nombre de lignes : " << tampon.nombre_lignes() << '\n';
	os << "Temps total : " << temps_total << '\n';

	os << "Temps scène : " << temps_scene
	   << " (" << pourcentage(temps_scene, temps_total) << "%)\n";
	os << '\t' << "Temps chargement : " << temps_chargement
	   << " (" << pourcentage(temps_chargement, temps_scene) << "%)\n";
	os << '\t' << "Temps tampon     : " << temps_tampon
	   << " (" << pourcentage(temps_tampon, temps_scene) << "%)\n";
	os << '\t' << "Temps découpage  : " << temps_decoupage
	   << " (" << pourcentage(temps_decoupage, temps_scene) << "%)\n";
	os << '\t' << "Temps analyse    : " << temps_analyse
	   << " (" << pourcentage(temps_analyse, temps_scene) << "%)\n";

	os << "Temps coulisse : " << temps_coulisse
	   << " (" << pourcentage(temps_coulisse, temps_total) << "%)\n";
	os << '\t' << "Temps génération code : " << temps_generation_code
	   << " (" << pourcentage(temps_generation_code, temps_coulisse) << "%)\n";

	return resultat;
}
