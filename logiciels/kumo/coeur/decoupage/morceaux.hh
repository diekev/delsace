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
	PARENTHESE_OUVRANTE,
	PARENTHESE_FERMANTE,
	VIRGULE,
	POINT,
	DOUBLE_POINTS,
	POINT_VIRGULE,
	ACCOLADE_OUVRANTE,
	ACCOLADE_FERMANTE,
	AJOURNE,
	AUTO_INCREMENTE,
	BINAIRE,
	BIT,
	CASCADE,
	CHAINE,
	CLE,
	CLE_PRIMAIRE,
	DEFAUT,
	ENTIER,
	FAUX,
	NUL,
	OCTET,
	REEL,
	REFERENCE,
	SIGNE,
	SUPPRIME,
	TABLE,
	TAILLE,
	TEMPS,
	TEMPS_COURANT,
	TEMPS_DATE,
	TEXTE,
	VARIABLE,
	VRAI,
	ZEROFILL,
	CHAINE_CARACTERE,
	NOMBRE_ENTIER,
	NOMBRE_REEL,
	NOMBRE_BINAIRE,
	NOMBRE_OCTAL,
	NOMBRE_HEXADECIMAL,
	INCONNU,
};

struct DonneesMorceaux {
	using type = id_morceau;
	static constexpr type INCONNU = id_morceau::INCONNU;

	dls::vue_chaine chaine;
	unsigned long ligne_pos;
	id_morceau identifiant;
};

const char *chaine_identifiant(id_morceau id);

void construit_tables_caractere_speciaux();

bool est_caractere_special(char c, id_morceau &i);

id_morceau id_chaine(const dls::vue_chaine &chaine);
