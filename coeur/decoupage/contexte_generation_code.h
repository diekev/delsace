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

#pragma once

#include <llvm/IR/LLVMContext.h>

#include <stack>
#include <unordered_map>

namespace llvm {
class BasicBlock;
class Value;
}  /* namespace llvm */

struct DonneesArgument {
	size_t index;
	int type;
	int pad;
};

struct DonneesFonction {
	std::unordered_map<std::string, DonneesArgument> args;
	int type_retour;
	int pad;
};

struct DonneesVariable {
	llvm::Value *valeur;
	int type;
	int pad;
};

struct Block {
	llvm::BasicBlock *block;
	std::unordered_map<std::string, DonneesVariable> locals;
};

struct ContexteGenerationCode {
	llvm::Module *module;
	llvm::LLVMContext contexte;

	void pousse_block(llvm::BasicBlock *block);

	void jete_block();

	llvm::BasicBlock *block_courant() const;

	void pousse_globale(const std::string &nom, llvm::Value *valeur, int type);

	llvm::Value *valeur_globale(const std::string &nom);

	int type_globale(const std::string &nom);

	void pousse_locale(const std::string &nom, llvm::Value *valeur, int type);

	llvm::Value *valeur_locale(const std::string &nom);

	int type_locale(const std::string &nom);

	void ajoute_donnees_fonctions(const std::string &nom, const DonneesFonction &donnees);

	DonneesFonction donnees_fonction(const std::string &nom);

private:
	std::stack<Block> pile_block;
	std::unordered_map<std::string, DonneesVariable> globales;
	std::unordered_map<std::string, DonneesFonction> fonctions;
};
