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

#include <cassert>
#include <iostream>

#include "biblinternes/memoire/logeuse_memoire.hh"

#include "contexte_generation_code.h"

assembleuse_arbre::assembleuse_arbre(ContexteGenerationCode &contexte)
{
	contexte.assembleuse = this;
	this->empile_noeud(lcc::noeud::type_noeud::RACINE, contexte, {});
}

assembleuse_arbre::~assembleuse_arbre()
{
	for (auto noeud : m_noeuds) {
		memoire::deloge("lcc::noeud::base", noeud);
	}
}

lcc::noeud::base *assembleuse_arbre::empile_noeud(lcc::noeud::type_noeud type, ContexteGenerationCode &contexte, DonneesMorceaux const &morceau, bool ajoute)
{
	auto noeud = cree_noeud(type, contexte, morceau);

	if (!m_pile.empty() && ajoute) {
		this->ajoute_noeud(noeud);
	}

	m_pile.push(noeud);

	return noeud;
}

void assembleuse_arbre::ajoute_noeud(lcc::noeud::base *noeud)
{
	m_pile.top()->ajoute_noeud(noeud);
}

lcc::noeud::base *assembleuse_arbre::cree_noeud(
		lcc::noeud::type_noeud type,
		ContexteGenerationCode &contexte,
		DonneesMorceaux const &morceau)
{
	auto noeud = memoire::loge<lcc::noeud::base>(
				"lcc::noeud::base",
				contexte,
				morceau,
				type);

	if (noeud != nullptr) {
		m_noeuds.pousse(noeud);
	}

	return noeud;
}

void assembleuse_arbre::depile_noeud(lcc::noeud::type_noeud type)
{
	assert(m_pile.top()->type == type);
	m_pile.pop();
	static_cast<void>(type);
}

void assembleuse_arbre::imprime_code(std::ostream &os)
{
	os << "------------------------------------------------------------------\n";
	m_pile.top()->imprime_code(os, 0);
	os << "------------------------------------------------------------------\n";
}

void assembleuse_arbre::genere_code(ContexteGenerationCode &contexte_generation, compileuse_lng &compileuse)
{
	if (m_pile.empty()) {
		return;
	}

	lcc::noeud::genere_code(m_pile.top(), contexte_generation, compileuse, false);
}

void assembleuse_arbre::supprime_noeud(lcc::noeud::base *noeud)
{
}

size_t assembleuse_arbre::memoire_utilisee() const
{
	return m_memoire_utilisee + nombre_noeuds() * sizeof(lcc::noeud::base *);
}

size_t assembleuse_arbre::nombre_noeuds() const
{
	return static_cast<size_t>(m_noeuds.taille());
}

void imprime_taille_memoire_noeud(std::ostream &os)
{
	os << "------------------------------------------------------------------\n";
	os << "lcc::noeud::base              : " << sizeof(lcc::noeud::base) << '\n';
	os << "DonneesMorceaux               : " << sizeof(DonneesMorceaux) << '\n';
	os << "std::list<lcc::noeud::base *> : " << sizeof(std::list<lcc::noeud::base *>) << '\n';
	os << "std::any                      : " << sizeof(std::any) << '\n';
	os << "------------------------------------------------------------------\n";
}
