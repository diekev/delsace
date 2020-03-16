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

#include "contexte_generation_llvm.hh"

ContexteGenerationLLVM::ContexteGenerationLLVM(Typeuse &t, const dls::tableau<Fichier *> &fs)
	: typeuse(t)
	, constructrice(*this, this->contexte)
	, fichiers(fs)
{}

ContexteGenerationLLVM::~ContexteGenerationLLVM()
{
	delete manager_fonctions;
}

void ContexteGenerationLLVM::commence_fonction(llvm::Function *f, NoeudDeclarationFonction *df)
{
	fonction = f;
	donnees_fonction = df;
}

void ContexteGenerationLLVM::termine_fonction()
{
	fonction = nullptr;
	m_bloc_courant = nullptr;
	donnees_fonction = nullptr;
	table_ident_locales.efface();
}

llvm::BasicBlock *ContexteGenerationLLVM::bloc_courant() const
{
	return m_bloc_courant;
}

void ContexteGenerationLLVM::bloc_courant(llvm::BasicBlock *bloc)
{
	m_bloc_courant = bloc;
	constructrice.bloc_courant(bloc);
}

void ContexteGenerationLLVM::empile_bloc_continue(dls::vue_chaine_compacte chaine, llvm::BasicBlock *bloc)
{
	m_pile_continue.pousse({chaine, bloc});
}

void ContexteGenerationLLVM::depile_bloc_continue()
{
	m_pile_continue.pop_back();
}

llvm::BasicBlock *ContexteGenerationLLVM::bloc_continue(dls::vue_chaine_compacte chaine)
{
	if (m_pile_continue.est_vide()) {
		return nullptr;
	}

	if (chaine.est_vide()) {
		return m_pile_continue.back().second;
	}

	for (auto const &paire : m_pile_continue) {
		if (paire.first == chaine) {
			return paire.second;
		}
	}

	return nullptr;
}

void ContexteGenerationLLVM::empile_bloc_arrete(dls::vue_chaine_compacte chaine, llvm::BasicBlock *bloc)
{
	m_pile_arrete.pousse({chaine, bloc});
}

void ContexteGenerationLLVM::depile_bloc_arrete()
{
	m_pile_arrete.pop_back();
}

llvm::BasicBlock *ContexteGenerationLLVM::bloc_arrete(dls::vue_chaine_compacte chaine)
{
	if (m_pile_arrete.est_vide()) {
		return nullptr;
	}

	if (chaine.est_vide()) {
		return m_pile_arrete.back().second;
	}

	for (auto const &paire : m_pile_arrete) {
		if (paire.first == chaine) {
			return paire.second;
		}
	}

	return nullptr;
}

llvm::Value *ContexteGenerationLLVM::valeur(IdentifiantCode *ident)
{
	auto iter = table_ident_locales.trouve(ident);

	if (iter != table_ident_locales.fin()) {
		return iter->second;
	}

	return table_ident_globales[ident];
}

void ContexteGenerationLLVM::ajoute_locale(IdentifiantCode *ident, llvm::Value *val)
{
	table_ident_locales[ident] = val;
}

void ContexteGenerationLLVM::ajoute_globale(IdentifiantCode *ident, llvm::Value *val)
{
	table_ident_globales[ident] = val;
}
