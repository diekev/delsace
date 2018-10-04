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

#include <string>

struct DonneesMorceaux {
	std::string_view chaine;
	size_t ligne;
	size_t pos;
	int identifiant;
};

enum {
	ID_EXCLAMATION,
	ID_GUILLEMET,
	ID_DIESE,
	ID_POURCENT,
	ID_ESPERLUETTE,
	ID_APOSTROPHE,
	ID_PARENTHESE_OUVRANTE,
	ID_PARENTHESE_FERMANTE,
	ID_FOIS,
	ID_PLUS,
	ID_VIRGULE,
	ID_MOINS,
	ID_POINT,
	ID_DIVISE,
	ID_DOUBLE_POINTS,
	ID_POINT_VIRGULE,
	ID_INFERIEUR,
	ID_EGAL,
	ID_SUPERIEUR,
	ID_AROBASE,
	ID_CROCHET_OUVRANT,
	ID_CROCHET_FERMANT,
	ID_CHAPEAU,
	ID_ACCOLADE_OUVRANTE,
	ID_BARRE,
	ID_ACCOLADE_FERMANTE,
	ID_TILDE,
	ID_DIFFERENCE,
	ID_ESP_ESP,
	ID_DECALAGE_GAUCHE,
	ID_INFERIEUR_EGAL,
	ID_EGALITE,
	ID_SUPERIEUR_EGAL,
	ID_DECALAGE_DROITE,
	ID_BARRE_BARRE,
	ID_ARRETE,
	ID_ASSOCIE,
	ID_BOOL,
	ID_BOUCLE,
	ID_CONSTANTE,
	ID_CONTINUE,
	ID_DANS,
	ID_DE,
	ID_DEFERE,
	ID_E16,
	ID_E16NS,
	ID_E32,
	ID_E32NS,
	ID_E64,
	ID_E64NS,
	ID_E8,
	ID_E8NS,
	ID_FAUX,
	ID_FONCTION,
	ID_GABARIT,
	ID_POUR,
	ID_R16,
	ID_R32,
	ID_R64,
	ID_RETOURNE,
	ID_RIEN,
	ID_SI,
	ID_SINON,
	ID_SOIT,
	ID_STRUCTURE,
	ID_TRANSTYPE,
	ID_VARIABLE,
	ID_VRAI,
	ID_ENUM,
	ID_NOMBRE_REEL,
	ID_NOMBRE_ENTIER,
	ID_NOMBRE_HEXADECIMAL,
	ID_NOMBRE_OCTAL,
	ID_NOMBRE_BINAIRE,
	ID_TROIS_POINTS,
	ID_CHAINE_CARACTERE,
	ID_CHAINE_LITTERALE,
	ID_CARACTERE,
	ID_POINTEUR,
	ID_TABLEAU,
	ID_REFERENCE,
	ID_INCONNU,
};

const char *chaine_identifiant(int id);

bool est_caractere_special(char c, int &i);

int id_caractere_double(const std::string_view &chaine);

int id_chaine(const std::string_view &chaine);
