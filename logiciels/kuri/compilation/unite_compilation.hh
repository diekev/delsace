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

#include <iostream>

struct Fichier;
struct Lexeme;
struct NoeudDeclaration;
struct NoeudExpression;
struct Type;

#define ENUMERE_ETATS_UNITE \
	ENUMERE_ETAT_UNITE_EX(PARSAGE_ATTENDU) \
	ENUMERE_ETAT_UNITE_EX(TYPAGE_ENTETE_FONCTION_ATTENDU) \
	ENUMERE_ETAT_UNITE_EX(TYPAGE_ATTENDU) \
	ENUMERE_ETAT_UNITE_EX(RI_ATTENDUE) \
	ENUMERE_ETAT_UNITE_EX(CODE_MACHINE_ATTENDU) \
	ENUMERE_ETAT_UNITE_EX(ATTEND_SUR_TYPE) \
	ENUMERE_ETAT_UNITE_EX(ATTEND_SUR_DECLARATION) \
	ENUMERE_ETAT_UNITE_EX(ATTEND_SUR_INTERFACE_KURI) \
	ENUMERE_ETAT_UNITE_EX(ATTEND_SUR_SYMBOLE)

// À FAIRE : les unités devront également dépendre sur les opérateurs quand nous en aurons plus d'une tacheronne
struct UniteCompilation {
	enum class Etat {
#define ENUMERE_ETAT_UNITE_EX(etat) etat,
		ENUMERE_ETATS_UNITE
#undef ENUMERE_ETAT_UNITE_EX
	};

	Etat etat_{};
	Etat etat_original{};
	REMBOURRE(4);
	Fichier *fichier = nullptr;
	NoeudExpression *noeud = nullptr;
	int index_reprise = 0;

	int cycle = 0;

	// pour les dépendances
	Type *type_attendu = nullptr;
	NoeudDeclaration *declaration_attendue = nullptr;
	Lexeme const *lexeme_attendu = nullptr;

	Etat etat() const
	{
		return etat_;
	}

	inline void restaure_etat_original()
	{
		this->etat_ = this->etat_original;
	}

	inline void change_etat(Etat etat_vers)
	{
		this->etat_ = etat_vers;
		this->cycle = 0;
	}

	static inline UniteCompilation cree_pour_ri(NoeudExpression *noeud)
	{
		auto unite = UniteCompilation();
		unite.noeud = noeud;
		unite.change_etat(UniteCompilation::Etat::RI_ATTENDUE);
		return unite;
	}

	inline void attend_sur_type(Type *type)
	{
		this->change_etat(UniteCompilation::Etat::ATTEND_SUR_TYPE);
		this->type_attendu = type;
	}

	inline void attend_sur_interface_kuri()
	{
		this->change_etat(UniteCompilation::Etat::ATTEND_SUR_INTERFACE_KURI);
	}

	inline void attend_sur_declaration(NoeudDeclaration *decl)
	{
		this->change_etat(UniteCompilation::Etat::ATTEND_SUR_DECLARATION);
		this->declaration_attendue = decl;
	}

	inline void attend_sur_symbole(Lexeme const *lexeme)
	{
		this->change_etat(UniteCompilation::Etat::ATTEND_SUR_SYMBOLE);
		this->lexeme_attendu = lexeme;
	}
};

const char *chaine_etat_unite(UniteCompilation::Etat etat);

std::ostream &operator<<(std::ostream &os, UniteCompilation::Etat etat);
