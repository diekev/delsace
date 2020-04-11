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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
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
#pragma GCC diagnostic pop

#include "biblinternes/structures/dico.hh"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/vue_chaine_compacte.hh"

struct IdentifiantCode;
struct ContexteGenerationLLVM;
struct OperateurBinaire;
struct NoeudExpression;
struct Type;

struct ConstructriceCode {
private:
	ContexteGenerationLLVM &contexte;
	llvm::IRBuilder<> builder;

	dls::dico<dls::chaine, llvm::Constant *> valeurs_chaines_globales{};

public:
	ConstructriceCode(ContexteGenerationLLVM &contexte_, llvm::LLVMContext &contexte_llvm);

	void bloc_courant(llvm::BasicBlock *bloc);

	llvm::Value *alloue_param(IdentifiantCode *ident, Type *type, llvm::Value *valeur);

	llvm::Value *alloue_variable(IdentifiantCode *ident, Type *type, NoeudExpression *expression);

	llvm::Value *alloue_globale(IdentifiantCode *ident, Type *type, NoeudExpression *expression, bool est_externe);

	llvm::Value *cree_valeur_defaut_pour_type(Type *type);

	llvm::Value *charge(llvm::Value *valeur, Type *type);

	void stocke(llvm::Value *valeur, llvm::Value *ptr);

	llvm::Constant *cree_chaine(dls::vue_chaine_compacte const &chaine);

	llvm::Value *accede_membre_structure(llvm::Value *structure, unsigned index, bool expr_gauche);

	llvm::Value *nombre_entier(long valeur);

	void incremente(llvm::Value *valeur, Type *type);

	llvm::Constant *valeur_pour_chaine(const dls::chaine &chn);

	llvm::Value *appel_operateur(OperateurBinaire const *op, llvm::Value *valeur1, llvm::Value *valeur2);
};
