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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#pragma GCC diagnostic pop

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico.hh"
#include "biblinternes/structures/dico_desordonne.hh"

struct Atome;
struct InstructionLabel;
struct EspaceDeTravail;
struct Type;
struct TypeFonction;
struct Instruction;

struct GeneratriceCodeLLVM {
	dls::dico<Atome const *, llvm::Value *> table_valeurs{};
	dls::dico<InstructionLabel const *, llvm::BasicBlock *> table_blocs{};
	dls::dico<Atome const *, llvm::Value *> table_globales{};
	dls::dico_desordonne<Type *, llvm::Type *> table_types{};
	dls::dico_desordonne<dls::chaine, llvm::Constant *> valeurs_chaines_globales{};
	EspaceDeTravail &m_espace;

	llvm::Function *m_fonction_courante = nullptr;
	llvm::LLVMContext m_contexte_llvm{};
	llvm::Module *m_module = nullptr;
	llvm::IRBuilder<> m_builder;
	llvm::legacy::FunctionPassManager *manager_fonctions = nullptr;

	GeneratriceCodeLLVM(EspaceDeTravail &espace);

	GeneratriceCodeLLVM(GeneratriceCodeLLVM const &) = delete;
	GeneratriceCodeLLVM &operator=(const GeneratriceCodeLLVM &) = delete;

	llvm::Type *converti_type_llvm(Type *type);

	llvm::FunctionType *converti_type_fonction(TypeFonction *type, bool est_externe);

	llvm::Value *genere_code_pour_atome(Atome *atome, bool pour_globale);

	void genere_code_pour_instruction(Instruction const *inst);

	void genere_code();

	llvm::Constant *valeur_pour_chaine(const dls::chaine &chaine, long taille_chaine);
};
