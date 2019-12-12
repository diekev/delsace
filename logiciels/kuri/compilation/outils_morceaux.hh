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

enum class id_morceau : unsigned int;

bool est_type_entier(id_morceau type);

bool est_type_entier_naturel(id_morceau type);

bool est_type_entier_relatif(id_morceau type);

bool est_type_reel(id_morceau type);

bool est_operateur_bool(id_morceau type);

bool est_assignation_operee(id_morceau type);

bool est_operateur_comp(id_morceau type);

bool peut_etre_dereference(id_morceau id);

bool est_mot_cle(id_morceau id);

bool est_chaine_litterale(id_morceau id);

bool est_specifiant_type(id_morceau identifiant);

bool est_identifiant_type(id_morceau identifiant);

bool est_nombre_entier(id_morceau identifiant);

bool est_nombre(id_morceau identifiant);

bool est_operateur_unaire(id_morceau identifiant);

bool est_operateur_binaire(id_morceau identifiant);
