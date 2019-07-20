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

 /* Ce fichier est généré automatiquement. NE PAS ÉDITER ! */
 
#pragma once

#include "biblinternes/structures/vue_chaine.hh"

enum class id_morceau : unsigned int {
	EXCLAMATION,
	GUILLEMET,
	DIESE,
	POURCENT,
	ESPERLUETTE,
	APOSTROPHE,
	PARENTHESE_OUVRANTE,
	PARENTHESE_FERMANTE,
	FOIS,
	PLUS,
	VIRGULE,
	MOINS,
	POINT,
	DIVISE,
	DOUBLE_POINTS,
	POINT_VIRGULE,
	INFERIEUR,
	EGAL,
	SUPERIEUR,
	AROBASE,
	CROCHET_OUVRANT,
	CROCHET_FERMANT,
	CHAPEAU,
	ACCOLADE_OUVRANTE,
	BARRE,
	ACCOLADE_FERMANTE,
	TILDE,
	DIFFERENCE,
	MODULO_EGAL,
	ESP_ESP,
	ET_EGAL,
	FIN_COMMENTAIRE_C,
	MULTIPLIE_EGAL,
	PLUS_EGAL,
	MOINS_EGAL,
	DEBUT_COMMENTAIRE_C,
	DEBUT_COMMENTAIRE_CPP,
	DIVISE_EGAL,
	SCOPE,
	DECALAGE_GAUCHE,
	INFERIEUR_EGAL,
	EGALITE,
	SUPERIEUR_EGAL,
	DECALAGE_DROITE,
	OUX_EGAL,
	OU_EGAL,
	BARRE_BARRE,
	TROIS_POINTS,
	DEC_DROITE_EGAL,
	DEC_GAUCHE_EGAL,
	ALIGNAS,
	ALIGNOF,
	AND,
	AND_EQ,
	ASM,
	ATOMIC_CANCEL,
	ATOMIC_COMMIT,
	ATOMIC_NOEXCEPT,
	AUDIT,
	AUTO,
	AXIOM,
	BITAND,
	BITOR,
	BOOL,
	BREAK,
	CASE,
	CATCH,
	CHAR,
	CHAR16_T,
	CHAR32_T,
	CHAR8_T,
	CLASS,
	CO_AWAIT,
	CO_RETURN,
	CO_YIELD,
	COMPL,
	CONCEPT,
	CONST,
	CONST_CAST,
	CONSTEVAL,
	CONSTEXPR,
	CONTINUE,
	DECLTYPE,
	DEFAULT,
	DELETE,
	DO,
	DOUBLE,
	DYNAMIC_CAST,
	ELSE,
	ENUM,
	EXPLICIT,
	EXPORT,
	EXTERN,
	FALSE,
	FINAL,
	FLOAT,
	FOR,
	FRIEND,
	GOTO,
	IF,
	IMPORT,
	INLINE,
	INT,
	LONG,
	MODULE,
	MUTABLE,
	NAMESPACE,
	NEW,
	NOEXCEPT,
	NOT,
	NOT_EQ,
	NULLPTR,
	OPERATOR,
	OR,
	OR_EQ,
	OVERRIDE,
	PRIVATE,
	PROTECTED,
	PUBLIC,
	REFLEXPR,
	REGISTER,
	REINTERPRET_CAST,
	REQUIRES,
	RETURN,
	SHORT,
	SIGNED,
	SIZEOF,
	STATIC,
	STATIC_ASSERT,
	STATIC_CAST,
	STRUCT,
	SWITCH,
	SYNCHRONIZED,
	TEMPLATE,
	THIS,
	THREAD_LOCAL,
	THROW,
	TRANSACTION_SAFE,
	TRANSACTION_SAFE_DYNAMIC,
	TRUE,
	TRY,
	TYPEDEF,
	TYPEID,
	TYPENAME,
	UNION,
	UNSIGNED,
	USING,
	VIRTUAL,
	VOID,
	VOLATILE,
	WCHAR_T,
	WHILE,
	XOR,
	XOR_EQ,
	NOMBRE_REEL,
	NOMBRE_ENTIER,
	NOMBRE_HEXADECIMAL,
	NOMBRE_OCTAL,
	NOMBRE_BINAIRE,
	PLUS_UNAIRE,
	MOINS_UNAIRE,
	CHAINE_CARACTERE,
	CHAINE_LITTERALE,
	CARACTERE,
	POINTEUR,
	TABLEAU,
	REFERENCE,
	INCONNU,
	CARACTERE_BLANC,
	COMMENTAIRE,
};

inline id_morceau operator&(id_morceau id1, int id2)
{
	return static_cast<id_morceau>(static_cast<int>(id1) & id2);
}

inline id_morceau operator|(id_morceau id1, int id2)
{
	return static_cast<id_morceau>(static_cast<int>(id1) | id2);
}

inline id_morceau operator|(id_morceau id1, id_morceau id2)
{
	return static_cast<id_morceau>(static_cast<int>(id1) | static_cast<int>(id2));
}

inline id_morceau operator<<(id_morceau id1, int id2)
{
	return static_cast<id_morceau>(static_cast<int>(id1) << id2);
}

inline id_morceau operator>>(id_morceau id1, int id2)
{
	return static_cast<id_morceau>(static_cast<int>(id1) >> id2);
}

struct DonneesMorceaux {
	using type = id_morceau;
	static constexpr type INCONNU = id_morceau::INCONNU;
	dls::vue_chaine chaine;
	unsigned long ligne_pos;
	id_morceau identifiant;
	int module = 0;
};

const char *chaine_identifiant(id_morceau id);

void construit_tables_caractere_speciaux();

bool est_caractere_special(char c, id_morceau &i);

id_morceau id_digraphe(dls::vue_chaine const &chaine);

id_morceau id_trigraphe(dls::vue_chaine const &chaine);

id_morceau id_chaine(dls::vue_chaine const &chaine);
