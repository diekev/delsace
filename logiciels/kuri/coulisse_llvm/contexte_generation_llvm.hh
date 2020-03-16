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
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#pragma GCC diagnostic pop

#include "biblinternes/structures/dico.hh"
#include "biblinternes/structures/tableau.hh"
#include "biblinternes/structures/vue_chaine_compacte.hh"

#include "constructrice_code.hh"

namespace llvm {
class BasicBlock;
class Type;
class Value;
}  /* namespace llvm */

struct Fichier;
struct IdentifiantCode;
struct NoeudDeclarationFonction;
struct Type;
struct Typeuse;

struct ContexteGenerationLLVM {
	dls::dico<IdentifiantCode *, llvm::Value *> table_ident_globales{};
	dls::dico<IdentifiantCode *, llvm::Value *> table_ident_locales{};
	IdentifiantCode *ident_contexte = nullptr;
	Typeuse &typeuse;
	Type *type_contexte = nullptr;

	ConstructriceCode constructrice;

	llvm::Module *module_llvm = nullptr;
	llvm::LLVMContext contexte{};
	llvm::Function *fonction = nullptr;
	llvm::legacy::FunctionPassManager *manager_fonctions = nullptr;
	llvm::BasicBlock *m_bloc_courant = nullptr;

	dls::tableau<Fichier *> const &fichiers;

	using paire_bloc = std::pair<dls::vue_chaine_compacte, llvm::BasicBlock *>;

	dls::tableau<paire_bloc> m_pile_continue{};
	dls::tableau<paire_bloc> m_pile_arrete{};

	NoeudDeclarationFonction *donnees_fonction = nullptr;

	explicit ContexteGenerationLLVM(Typeuse &t, dls::tableau<Fichier *> const &fs);

	/* Désactive la copie, car il ne peut y avoir qu'un seul contexte par
	 * compilation. */
	ContexteGenerationLLVM(const ContexteGenerationLLVM &) = delete;
	ContexteGenerationLLVM &operator=(const ContexteGenerationLLVM &) = delete;

	~ContexteGenerationLLVM();

	void commence_fonction(llvm::Function *f, NoeudDeclarationFonction *df);

	void termine_fonction();

	llvm::BasicBlock *bloc_courant() const;

	void bloc_courant(llvm::BasicBlock *bloc);

	void empile_bloc_continue(dls::vue_chaine_compacte chaine, llvm::BasicBlock *bloc);

	void depile_bloc_continue();

	llvm::BasicBlock *bloc_continue(dls::vue_chaine_compacte chaine);

	void empile_bloc_arrete(dls::vue_chaine_compacte chaine, llvm::BasicBlock *bloc);

	void depile_bloc_arrete();

	llvm::BasicBlock *bloc_arrete(dls::vue_chaine_compacte chaine);

	llvm::Value *valeur(IdentifiantCode *ident);

	void ajoute_locale(IdentifiantCode *ident, llvm::Value *val);

	void ajoute_globale(IdentifiantCode *ident, llvm::Value *val);
};
