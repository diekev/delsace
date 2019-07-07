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

enum {
	ID_PARENTHESE_OUVRANTE,
	ID_PARENTHESE_FERMANTE,
	ID_VIRGULE,
	ID_POINT,
	ID_DOUBLE_POINTS,
	ID_POINT_VIRGULE,
	ID_ACCOLADE_OUVRANTE,
	ID_ACCOLADE_FERMANTE,
	ID_AJOURNE,
	ID_AUTO_INCREMENTE,
	ID_BINAIRE,
	ID_BIT,
	ID_CASCADE,
	ID_CHAINE,
	ID_CLE,
	ID_CLE_PRIMAIRE,
	ID_DEFAUT,
	ID_ENTIER,
	ID_FAUX,
	ID_NUL,
	ID_OCTET,
	ID_REEL,
	ID_REFERENCE,
	ID_SIGNE,
	ID_SUPPRIME,
	ID_TABLE,
	ID_TAILLE,
	ID_TEMPS,
	ID_TEMPS_COURANT,
	ID_TEMPS_DATE,
	ID_TEXTE,
	ID_VARIABLE,
	ID_VRAI,
	ID_ZEROFILL,
	ID_CHAINE_CARACTERE,
	ID_NOMBRE_ENTIER,
	ID_NOMBRE_REEL,
	ID_NOMBRE_BINAIRE,
	ID_NOMBRE_OCTAL,
	ID_NOMBRE_HEXADECIMAL,
	ID_INCONNU,
};

struct DonneesMorceaux {
	using type = unsigned long;
	static constexpr type INCONNU = ID_INCONNU;

	dls::vue_chaine chaine;
	unsigned long ligne_pos;
	unsigned long identifiant;
};

const char *chaine_identifiant(int id);

void construit_tables_caractere_speciaux();

bool est_caractere_special(char c, int &i);

int id_chaine(const dls::vue_chaine &chaine);
