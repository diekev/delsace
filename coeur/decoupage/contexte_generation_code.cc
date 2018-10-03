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

#include "contexte_generation_code.h"

void ContexteGenerationCode::pousse_block(llvm::BasicBlock *block)
{
	Block b;
	b.block = block;
	pile_block.push(b);
}

void ContexteGenerationCode::jete_block()
{
	pile_block.pop();
}

llvm::BasicBlock *ContexteGenerationCode::block_courant() const
{
	if (pile_block.empty()) {
		return nullptr;
	}

	return pile_block.top().block;
}

void ContexteGenerationCode::pousse_globale(const std::string &nom, llvm::Value *valeur, int type)
{
	globales.insert({nom, {valeur, type, 0}});
}

llvm::Value *ContexteGenerationCode::valeur_globale(const std::string &nom)
{
	auto iter = globales.find(nom);

	if (iter == globales.end()) {
		return nullptr;
	}

	return iter->second.valeur;
}

int ContexteGenerationCode::type_globale(const std::string &nom)
{
	auto iter = globales.find(nom);

	if (iter == globales.end()) {
		return -1;
	}

	return iter->second.type;
}

void ContexteGenerationCode::pousse_locale(const std::string &nom, llvm::Value *valeur, int type)
{
	pile_block.top().locals.insert({nom, {valeur, type, 0}});
}

llvm::Value *ContexteGenerationCode::valeur_locale(const std::string &nom)
{
	auto iter = pile_block.top().locals.find(nom);

	if (iter == pile_block.top().locals.end()) {
		return nullptr;
	}

	return iter->second.valeur;
}

int ContexteGenerationCode::type_locale(const std::string &nom)
{
	auto iter = pile_block.top().locals.find(nom);

	if (iter == pile_block.top().locals.end()) {
		return -1;
	}

	return iter->second.type;
}

void ContexteGenerationCode::ajoute_donnees_fonctions(const std::string &nom, const DonneesFonction &donnees)
{
	fonctions.insert({nom, donnees});
}

DonneesFonction ContexteGenerationCode::donnees_fonction(const std::string &nom)
{
	return fonctions[nom];
}
