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

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/pile.hh"

enum class GenreNoeud : char;

struct AllocatriceNoeud;
struct EspaceDeTravail;
struct Lexeme;
struct NoeudExpression;
struct NoeudBloc;

struct AssembleuseArbre {
private:
	AllocatriceNoeud &m_allocatrice_noeud;

	size_t m_memoire_utilisee = 0;

	dls::pile<NoeudBloc *> m_blocs{};

public:
	explicit AssembleuseArbre(EspaceDeTravail &espace);
	~AssembleuseArbre() = default;

	NoeudBloc *empile_bloc();

	NoeudBloc *bloc_courant() const;

	void depile_bloc();

	/**
	 * Crée un noeud sans le désigner comme noeud courant, et retourne un
	 * pointeur vers celui-ci.
	 */
	NoeudExpression *cree_noeud(GenreNoeud type, Lexeme const *lexeme);
};
