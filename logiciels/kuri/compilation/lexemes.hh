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

#include "biblinternes/structures/chaine.hh"

enum class TypeLexeme : unsigned int {
	EXCLAMATION,
	GUILLEMET,
	DIESE,
	DOLLAR,
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
	DIRECTIVE,
	MODULO_EGAL,
	ESP_ESP,
	ET_EGAL,
	MULTIPLIE_EGAL,
	PLUS_EGAL,
	MOINS_EGAL,
	DIVISE_EGAL,
	DECLARATION_VARIABLE,
	DECALAGE_GAUCHE,
	INFERIEUR_EGAL,
	EGALITE,
	SUPERIEUR_EGAL,
	DECALAGE_DROITE,
	OUX_EGAL,
	OU_EGAL,
	BARRE_BARRE,
	TROIS_POINTS,
	DEC_GAUCHE_EGAL,
	DEC_DROITE_EGAL,
	ARRETE,
	BOOL,
	BOUCLE,
	CHAINE,
	CHARGE,
	CONTINUE,
	COROUT,
	DANS,
	DIFFERE,
	DISCR,
	DYN,
	DELOGE,
	EINI,
	EMPL,
	EXTERNE,
	FAUX,
	FONC,
	GARDE,
	IMPORTE,
	INFO_DE,
	LOGE,
	MEMOIRE,
	N128,
	N16,
	N32,
	N64,
	N8,
	NONSUR,
	NUL,
	OCTET,
	POUR,
	R128,
	R16,
	R32,
	R64,
	RELOGE,
	RETIENS,
	RETOURNE,
	RIEN,
	REPETE,
	SANSARRET,
	SAUFSI,
	SI,
	SINON,
	STRUCT,
	TAILLE_DE,
	TANTQUE,
	TRANSTYPE,
	TYPE_DE,
	UNION,
	VRAI,
	Z128,
	Z16,
	Z32,
	Z64,
	Z8,
	ENUM,
	ENUM_DRAPEAU,
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

inline TypeLexeme operator&(TypeLexeme id1, int id2)
{
	return static_cast<TypeLexeme>(static_cast<int>(id1) & id2);
}

inline TypeLexeme operator|(TypeLexeme id1, int id2)
{
	return static_cast<TypeLexeme>(static_cast<int>(id1) | id2);
}

inline TypeLexeme operator|(TypeLexeme id1, TypeLexeme id2)
{
	return static_cast<TypeLexeme>(static_cast<int>(id1) | static_cast<int>(id2));
}

inline TypeLexeme operator<<(TypeLexeme id1, int id2)
{
	return static_cast<TypeLexeme>(static_cast<int>(id1) << id2);
}

inline TypeLexeme operator>>(TypeLexeme id1, int id2)
{
	return static_cast<TypeLexeme>(static_cast<int>(id1) >> id2);
}

struct DonneesLexeme {
	using type = TypeLexeme;
	static constexpr type INCONNU = TypeLexeme::INCONNU;
	dls::vue_chaine_compacte chaine;
	TypeLexeme identifiant;
	int fichier = 0;
};

const char *chaine_identifiant(TypeLexeme id);

void construit_tables_caractere_speciaux();

bool est_caractere_special(char c, TypeLexeme &i);

TypeLexeme id_digraphe(const dls::vue_chaine_compacte &chaine);

TypeLexeme id_trigraphe(const dls::vue_chaine_compacte &chaine);

TypeLexeme id_chaine(const dls::vue_chaine_compacte &chaine);
