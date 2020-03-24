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

enum class GenreLexeme : unsigned int {
	EXCLAMATION,
	GUILLEMET,
	DIRECTIVE,
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
	MODULO_EGAL,
	ESP_ESP,
	ET_EGAL,
	FIN_BLOC_COMMENTAIRE,
	MULTIPLIE_EGAL,
	PLUS_EGAL,
	MOINS_EGAL,
	RETOUR_TYPE,
	DEBUT_BLOC_COMMENTAIRE,
	DEBUT_LIGNE_COMMENTAIRE,
	DIVISE_EGAL,
	DECLARATION_CONSTANTE,
	DECLARATION_VARIABLE,
	DECALAGE_GAUCHE,
	INFERIEUR_EGAL,
	EGALITE,
	SUPERIEUR_EGAL,
	DECALAGE_DROITE,
	OUX_EGAL,
	OU_EGAL,
	BARRE_BARRE,
	NON_INITIALISATION,
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
	EINI_ERREUR,
	EMPL,
	ERREUR,
	EXTERNE,
	FAUX,
	FONC,
	GARDE,
	IMPORTE,
	INFO_DE,
	LOGE,
	MEMOIRE,
	N16,
	N32,
	N64,
	N8,
	NONATTEIGNABLE,
	NONSUR,
	NUL,
	OCTET,
	OPERATEUR,
	PIEGE,
	POUR,
	POUSSE_CONTEXTE,
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
	TENTE,
	TRANSTYPE,
	TYPE_DE,
	UNION,
	VRAI,
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
	EXPANSION_VARIADIQUE,
};

inline GenreLexeme operator&(GenreLexeme id1, int id2)
{
	return static_cast<GenreLexeme>(static_cast<int>(id1) & id2);
}

inline GenreLexeme operator|(GenreLexeme id1, int id2)
{
	return static_cast<GenreLexeme>(static_cast<int>(id1) | id2);
}

inline GenreLexeme operator|(GenreLexeme id1, GenreLexeme id2)
{
	return static_cast<GenreLexeme>(static_cast<int>(id1) | static_cast<int>(id2));
}

inline GenreLexeme operator<<(GenreLexeme id1, int id2)
{
	return static_cast<GenreLexeme>(static_cast<int>(id1) << id2);
}

inline GenreLexeme operator>>(GenreLexeme id1, int id2)
{
	return static_cast<GenreLexeme>(static_cast<int>(id1) >> id2);
}

struct DonneesLexeme {
	using type = GenreLexeme;
	static constexpr type INCONNU = GenreLexeme::INCONNU;
	dls::vue_chaine_compacte chaine;
	GenreLexeme genre;
	int fichier = 0;
	int ligne = 0;
	int colonne = 0;
};

const char *chaine_identifiant(GenreLexeme id);

void construit_tables_caractere_speciaux();

bool est_caractere_special(char c, GenreLexeme &i);

GenreLexeme id_digraphe(const dls::vue_chaine_compacte &chaine);

GenreLexeme id_trigraphe(const dls::vue_chaine_compacte &chaine);

GenreLexeme id_chaine(const dls::vue_chaine_compacte &chaine);
