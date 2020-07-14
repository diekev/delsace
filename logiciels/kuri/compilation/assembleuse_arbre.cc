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

#include "compilatrice.hh"

assembleuse_arbre::assembleuse_arbre(Compilatrice &compilatrice)
	: m_allocatrice_noeud(compilatrice.allocatrice_noeud)
{
	this->empile_bloc();
}

NoeudBloc *assembleuse_arbre::empile_bloc()
{
	auto bloc = static_cast<NoeudBloc *>(cree_noeud(GenreNoeud::INSTRUCTION_COMPOSEE, nullptr));
	bloc->bloc_parent = bloc_courant();

	m_blocs.empile(bloc);

	return bloc;
}

NoeudBloc *assembleuse_arbre::bloc_courant() const
{
	if (m_blocs.est_vide()) {
		return nullptr;
	}

	return m_blocs.haut();
}

void assembleuse_arbre::depile_bloc()
{
	m_blocs.depile();
}

NoeudExpression *assembleuse_arbre::cree_noeud(GenreNoeud genre, Lexeme const *lexeme)
{
	auto noeud = m_allocatrice_noeud.cree_noeud(genre);

	if (noeud != nullptr) {
		noeud->genre = genre;
		noeud->lexeme = lexeme;
		noeud->bloc_parent = bloc_courant();

		if (noeud->lexeme && (noeud->lexeme->genre == GenreLexeme::CHAINE_CARACTERE || noeud->lexeme->genre == GenreLexeme::EXTERNE)) {
			noeud->ident = lexeme->ident;
		}
	}

	return noeud;
}
