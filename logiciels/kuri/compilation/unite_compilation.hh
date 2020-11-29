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
#include "biblinternes/structures/chaine.hh"

#include <iostream>

struct EspaceDeTravail;
struct Fichier;
struct Lexeme;
struct MetaProgramme;
struct NoeudDeclaration;
struct NoeudExpression;
struct Type;

#define ENUMERE_ETATS_UNITE \
	ENUMERE_ETAT_UNITE_EX(PRETE) \
	ENUMERE_ETAT_UNITE_EX(ATTEND_SUR_TYPE) \
	ENUMERE_ETAT_UNITE_EX(ATTEND_SUR_DECLARATION) \
	ENUMERE_ETAT_UNITE_EX(ATTEND_SUR_INTERFACE_KURI) \
	ENUMERE_ETAT_UNITE_EX(ATTEND_SUR_SYMBOLE) \
	ENUMERE_ETAT_UNITE_EX(ATTEND_SUR_OPERATEUR) \
	ENUMERE_ETAT_UNITE_EX(ATTEND_SUR_METAPROGRAMME)

struct UniteCompilation {
	enum class Etat {
#define ENUMERE_ETAT_UNITE_EX(etat) etat,
		ENUMERE_ETATS_UNITE
#undef ENUMERE_ETAT_UNITE_EX
	};

	explicit UniteCompilation(EspaceDeTravail *esp)
		: espace(esp)
	{}

	UniteCompilation *depend_sur = nullptr;

	Etat etat_{};
	Etat etat_original{};
	EspaceDeTravail *espace = nullptr;
	Fichier *fichier = nullptr;
	NoeudExpression *noeud = nullptr;
	NoeudExpression *operateur_attendu = nullptr;
	MetaProgramme *metaprogramme = nullptr;
	MetaProgramme *metaprogramme_attendu = nullptr;
	int index_courant = 0;
	int index_precedent = 0;
	bool message_recu = false;

	int cycle = 0;

	// pour les dépendances
	Type *type_attendu = nullptr;
	NoeudDeclaration *declaration_attendue = nullptr;
	Lexeme const *lexeme_attendu = nullptr;
	const char *fonction_interface_attendue = nullptr;

	Etat etat() const
	{
		return etat_;
	}

	inline void restaure_etat_original()
	{
		this->etat_ = this->etat_original;
	}

	inline void attend_sur_type(Type *type)
	{
		this->etat_ = (UniteCompilation::Etat::ATTEND_SUR_TYPE);
		this->type_attendu = type;
	}

	inline void attend_sur_interface_kuri(const char *nom_fonction)
	{
		this->fonction_interface_attendue = nom_fonction;
		this->etat_ = (UniteCompilation::Etat::ATTEND_SUR_INTERFACE_KURI);
	}

	inline void attend_sur_declaration(NoeudDeclaration *decl)
	{
		this->etat_ = UniteCompilation::Etat::ATTEND_SUR_DECLARATION;
		this->declaration_attendue = decl;
	}

	inline void attend_sur_symbole(Lexeme const *lexeme)
	{
		this->etat_ = UniteCompilation::Etat::ATTEND_SUR_SYMBOLE;
		this->lexeme_attendu = lexeme;
	}

	inline void attend_sur_operateur(NoeudExpression *expr)
	{
		this->etat_ = Etat::ATTEND_SUR_OPERATEUR;
		this->operateur_attendu = expr;
	}

	inline void attend_sur_metaprogramme(MetaProgramme *metaprogramme_attendu_)
	{
		this->etat_ = Etat::ATTEND_SUR_METAPROGRAMME;
		this->metaprogramme_attendu = metaprogramme_attendu_;
	}

	bool est_bloquee() const;

	dls::chaine commentaire() const;

	UniteCompilation *unite_attendue() const;
};

const char *chaine_etat_unite(UniteCompilation::Etat etat);

std::ostream &operator<<(std::ostream &os, UniteCompilation::Etat etat);

dls::chaine chaine_attentes_recursives(UniteCompilation *unite);
