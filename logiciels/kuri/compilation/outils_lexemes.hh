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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

enum class GenreLexeme : unsigned int;

bool est_type_entier(GenreLexeme type);

bool est_type_entier_naturel(GenreLexeme type);

bool est_type_entier_relatif(GenreLexeme type);

bool est_type_reel(GenreLexeme type);

bool est_operateur_bool(GenreLexeme type);

bool est_assignation_operee(GenreLexeme type);

GenreLexeme operateur_pour_assignation_operee(GenreLexeme type);

bool est_operateur_comp(GenreLexeme type);

bool peut_etre_dereference(GenreLexeme id);

bool est_mot_cle(GenreLexeme id);

bool est_chaine_litterale(GenreLexeme id);

bool est_specifiant_type(GenreLexeme identifiant);

bool est_identifiant_type(GenreLexeme identifiant);

bool est_nombre_entier(GenreLexeme identifiant);

bool est_nombre(GenreLexeme identifiant);

bool est_operateur_unaire(GenreLexeme identifiant);

bool est_operateur_binaire(GenreLexeme identifiant);
