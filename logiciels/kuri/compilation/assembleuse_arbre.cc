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

#include "contexte_generation_code.h"

assembleuse_arbre::assembleuse_arbre(ContexteGenerationCode &contexte)
	: m_contexte(contexte)
	, m_allocatrice_noeud(contexte.allocatrice_noeud)
{
	this->empile_bloc();

	/* Pour fprintf dans les messages d'erreurs, nous incluons toujours "stdio.h". */
	this->ajoute_inclusion("stdio.h");
	/* Pour malloc/free, nous incluons toujours "stdlib.h". */
	this->ajoute_inclusion("stdlib.h");
	/* Pour strlen, nous incluons toujours "string.h". */
	this->ajoute_inclusion("string.h");
	/* Pour les coroutines nous incluons toujours pthread */
	this->ajoute_inclusion("pthread.h");
	this->bibliotheques_dynamiques.pousse("pthread");
	this->definitions.pousse("_REENTRANT");
}

NoeudBloc *assembleuse_arbre::empile_bloc()
{
	auto bloc = static_cast<NoeudBloc *>(cree_noeud(GenreNoeud::INSTRUCTION_COMPOSEE, nullptr));
	bloc->parent = bloc_courant();
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

NoeudBase *assembleuse_arbre::cree_noeud(GenreNoeud genre, Lexeme const *lexeme)
{
	auto noeud = m_allocatrice_noeud.cree_noeud(genre);

	if (noeud != nullptr) {
		noeud->genre = genre;
		noeud->lexeme = lexeme;
		noeud->bloc_parent = bloc_courant();

		if (noeud->lexeme && noeud->lexeme->genre == GenreLexeme::CHAINE_CARACTERE) {
			noeud->ident = m_contexte.table_identifiants.identifiant_pour_chaine(noeud->lexeme->chaine);
		}
	}

	return noeud;
}

void assembleuse_arbre::ajoute_inclusion(const dls::chaine &fichier)
{
	if (deja_inclus.trouve(fichier) != deja_inclus.fin()) {
		return;
	}

	deja_inclus.insere(fichier);
	inclusions.pousse(fichier);
}
