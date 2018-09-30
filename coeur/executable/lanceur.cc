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

#include "decoupage/analyseuse_grammaire.h"
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
	const auto taille_fichier = fichier.tellg();
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

#if 0
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#endif

int main(int argc, char *argv[])
{
#if 0
	auto context = llvm::LLVMContext();
	auto module = new llvm::Module("top", context);

	auto constructeur = llvm::IRBuilder<>(context);

	std::vector<llvm::Type *> putsArgs;
	putsArgs.push_back(constructeur.getInt8Ty()->getPointerTo());
	llvm::ArrayRef<llvm::Type*>  argsRef(putsArgs);

	auto *putsType =
	  llvm::FunctionType::get(constructeur.getInt32Ty(), argsRef, false);
	llvm::Constant *putsFunc = module->getOrInsertFunction("puts", putsType);

	auto type_fonction = llvm::FunctionType::get(constructeur.getInt32Ty(), false);
	auto fonction_main = llvm::Function::Create(type_fonction, llvm::Function::ExternalLinkage, "main", module);

	auto entree = llvm::BasicBlock::Create(context, "entrypoint", fonction_main);
	constructeur.SetInsertPoint(entree);

	auto valeur = constructeur.CreateGlobalStringPtr("hello world!\n");

	constructeur.CreateCall(putsFunc, valeur);
	constructeur.CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0, false));

	module->dump();
#else
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

	auto temps_chargement = 0.0;
	auto temps_tampon     = 0.0;
	auto temps_decoupage  = 0.0;
	auto temps_analyse    = 0.0;

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

		auto analyseuse = analyseuse_grammaire(tampon);

		const auto debut_analyseuse = numero7::chronometrage::maintenant();
		analyseuse.lance_analyse(decoupeuse.morceaux());
		temps_analyse = numero7::chronometrage::maintenant() - debut_analyseuse;
	}
	catch (const erreur::frappe &erreur_frappe) {
		std::cerr << erreur_frappe.message() << '\n';
	}

	const auto temps_total = temps_tampon
							 + temps_decoupage
							 + temps_analyse
							 + temps_chargement;

	auto pourcentage = [&](const double &x)
	{
		return x * 100.0 / temps_total;
	};

	std::ostream &os = std::cout;

	os << "Nombre de lignes : " << tampon.nombre_lignes() << '\n';
	os << "Temps scène : " << temps_total << '\n';
	os << '\t' << "Temps chargement : " << temps_chargement
	   << " (" << pourcentage(temps_chargement) << "%)\n";
	os << '\t' << "Temps tampon     : " << temps_tampon
	   << " (" << pourcentage(temps_tampon) << "%)\n";
	os << '\t' << "Temps découpage  : " << temps_decoupage
	   << " (" << pourcentage(temps_decoupage) << "%)\n";
	os << '\t' << "Temps analyse    : " << temps_analyse
	   << " (" << pourcentage(temps_analyse) << "%)\n";
#endif
}
