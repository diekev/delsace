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
	size_t ligne_pos;
	size_t identifiant;
};

enum {
	ID_DANS,
	ID_FINPOUR,
	ID_FINSI,
	ID_POUR,
	ID_SI,
	ID_SINON,
	ID_ETEND,
	ID_DEBUT_VARIABLE,
	ID_DEBUT_EXPRESSION,
	ID_FIN_VARIABLE,
	ID_FIN_EXPRESSION,
	ID_CHAINE_CARACTERE,
	ID_INCONNU,
};

const char *chaine_identifiant(int id);

void construit_tables_caractere_speciaux();

int id_chaine(const std::string_view &chaine);
