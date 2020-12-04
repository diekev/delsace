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

struct IdentifiantCode;

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
	NOMBRE_REEL,
	NOMBRE_ENTIER,
	PLUS_UNAIRE,
	MOINS_UNAIRE,
	FOIS_UNAIRE,
	ESP_UNAIRE,
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
	ARRETE,
	BOOL,
	BOUCLE,
	CHAINE,
	CHARGE,
	COMME,
	CONTINUE,
	COROUT,
	DANS,
	DIFFERE,
	DISCR,
	DYN,
	DEFINIS,
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
	INIT_DE,
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
	TYPE_DE,
	TYPE_DE_DONNEES,
	UNION,
	VRAI,
	Z16,
	Z32,
	Z64,
	Z8,
	ENUM,
	ENUM_DRAPEAU,
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
struct Lexeme {
	using type = GenreLexeme;
	static constexpr type INCONNU = GenreLexeme::INCONNU;
	dls::vue_chaine_compacte chaine;

	union {
		unsigned long long valeur_entiere;
		double valeur_reelle;
		struct { char *pointeur; long taille; };
		IdentifiantCode *ident;
	};

	GenreLexeme genre;
	int fichier = 0;
	int ligne = 0;
	int colonne = 0;
};
#pragma GCC diagnostic pop

const char *chaine_du_genre_de_lexeme(GenreLexeme id);

const char *chaine_du_lexeme(GenreLexeme genre);

void construit_tables_caractere_speciaux();

GenreLexeme id_chaine(const dls::vue_chaine_compacte &chaine);
