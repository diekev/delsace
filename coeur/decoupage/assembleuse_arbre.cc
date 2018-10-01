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

#include "assembleuse_arbre.h"

#include <llvm/IR/Module.h>

#include "arbre_syntactic.h"

assembleuse_arbre::~assembleuse_arbre()
{
	for (auto noeud : m_noeuds) {
		delete noeud;
	}
}

Noeud *assembleuse_arbre::ajoute_noeud(int type, const std::string &chaine, int id, bool ajoute)
{
	auto noeud = cree_noeud(type, chaine, id);

	if (!m_pile.empty() && ajoute) {
		m_pile.top()->ajoute_noeud(noeud);
	}

	m_pile.push(noeud);

	return noeud;
}

void assembleuse_arbre::ajoute_noeud(Noeud *noeud)
{
	m_pile.top()->ajoute_noeud(noeud);
}

Noeud *assembleuse_arbre::cree_noeud(int type, const std::string &chaine, int id)
{
	Noeud *noeud = nullptr;

	switch (type) {
		case NOEUD_RACINE:
			noeud = new NoeudRacine(chaine, id);
			break;
		case NOEUD_APPEL_FONCTION:
			noeud = new NoeudAppelFonction(chaine, id);
			break;
		case NOEUD_DECLARATION_FONCTION:
			noeud = new NoeudDeclarationFonction(chaine, id);
			break;
		case NOEUD_EXPRESSION:
			noeud = new NoeudExpression(chaine, id);
			break;
		case NOEUD_ASSIGNATION_VARIABLE:
			noeud = new NoeudAssignationVariable(chaine, id);
			break;
		case NOEUD_VARIABLE:
			noeud = new NoeudVariable(chaine, id);
			break;
		case NOEUD_NOMBRE_ENTIER:
			noeud = new NoeudNombreEntier(chaine, id);
			break;
		case NOEUD_NOMBRE_REEL:
			noeud = new NoeudNombreReel(chaine, id);
			break;
		case NOEUD_OPERATION:
			noeud = new NoeudOperation(chaine, id);
			break;
		case NOEUD_RETOUR:
			noeud = new NoeudRetour(chaine, id);
			break;
	}

	if (noeud != nullptr) {
		m_noeuds.push_back(noeud);
	}

	return noeud;
}

void assembleuse_arbre::sors_noeud(int /*type*/)
{
	m_pile.pop();
}

void assembleuse_arbre::imprime_code(std::ostream &os)
{
	m_pile.top()->imprime_code(os, 0);
}

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>

void assembleuse_arbre::genere_code_llvm()
{
	/* choix de la cible */
	const auto triplet_cible = llvm::sys::getDefaultTargetTriple();

	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargets();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
	llvm::InitializeAllAsmPrinters();

	auto erreur = std::string{""};
	auto cible = llvm::TargetRegistry::lookupTarget(triplet_cible, erreur);

	if (!cible) {
		std::cerr << erreur;
		return;
	}

	auto CPU = "generic";
	auto feature = "";
	auto options = llvm::TargetOptions{};
	auto RM = llvm::Optional<llvm::Reloc::Model>();
	auto machine_cible = cible->createTargetMachine(triplet_cible, CPU, feature, options, RM);

	ContexteGenerationCode contexte_generation;
	auto module = new llvm::Module("top", contexte_generation.contexte);
	module->setDataLayout(machine_cible->createDataLayout());
	module->setTargetTriple(triplet_cible);

	contexte_generation.module = module;

	m_pile.top()->genere_code_llvm(contexte_generation);

	/* définition du fichier de sortie */
	auto chemin_sortie = "/tmp/kuri.o";
	std::error_code ec;

	llvm::raw_fd_ostream dest(chemin_sortie, ec, llvm::sys::fs::F_None);

	if (ec) {
		std::cerr << "Ne put pas ouvrir le fichier '" << chemin_sortie << "'\n";
		delete contexte_generation.module;
		return;
	}

	llvm::legacy::PassManager pass;
	auto type_fichier = llvm::TargetMachine::CGFT_ObjectFile;

	if (machine_cible->addPassesToEmitFile(pass, dest, type_fichier)) {
		std::cerr << "La machine cible ne peut pas émettre ce type de fichier\n";
		delete contexte_generation.module;
		return;
	}

	contexte_generation.module->dump();

	pass.run(*module);
	dest.flush();

	delete contexte_generation.module;
}
