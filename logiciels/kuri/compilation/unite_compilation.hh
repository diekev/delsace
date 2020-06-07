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

#include "biblinternes/outils/definitions.h"

struct Fichier;
struct Lexeme;
struct NoeudDeclaration;
struct NoeudExpression;
struct Type;

struct UniteCompilation {
	enum Etat {
		PARSAGE_ATTENDU,
		TYPAGE_ENTETE_FONCTION_ATTENDU,
		TYPAGE_ATTENDU,
		RI_ATTENDUE,
		CODE_MACHINE_ATTENDU,

		ATTEND_SUR_TYPE,
		ATTEND_SUR_DECLARATION,
		ATTEND_SUR_INTERFACE_KURI,
		ATTEND_SUR_SYMBOLE,
	};

	Etat etat{};
	REMBOURRE(4);
	Fichier *fichier = nullptr;
	NoeudExpression *noeud = nullptr;

	int cycle = 0;

	// pour les dépendances
	Type *type_attendu = nullptr;
	NoeudDeclaration *declaration_attendue = nullptr;
	Lexeme const *lexeme_attendu = nullptr;

	inline void attend_sur_type(Type *type)
	{
		this->etat = UniteCompilation::Etat::ATTEND_SUR_TYPE;
		this->type_attendu = type;
	}

	inline void attend_sur_interface_kuri()
	{
		this->etat = UniteCompilation::Etat::ATTEND_SUR_INTERFACE_KURI;
	}

	inline void attend_sur_declaration(NoeudDeclaration *decl)
	{
		this->etat = UniteCompilation::Etat::ATTEND_SUR_DECLARATION;
		this->declaration_attendue = decl;
	}

	inline void attend_sur_symbole(Lexeme const *lexeme)
	{
		this->etat = UniteCompilation::Etat::ATTEND_SUR_SYMBOLE;
		this->lexeme_attendu = lexeme;
	}
};
