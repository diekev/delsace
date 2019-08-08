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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

class Noeud;

struct ContexteEvaluation;
struct DonneesAval;

enum {
	NOEUD_IMAGE_DEFAUT = 0,
	NOEUD_IMAGE_SORTIE = 1,
	NOEUD_OBJET_SORTIE = 2,
	NOEUD_OBJET        = 3,
	NOEUD_COMPOSITE    = 4,
};

void synchronise_donnees_operatrice(Noeud *noeud);

void execute_noeud(
		Noeud *noeud,
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval);
