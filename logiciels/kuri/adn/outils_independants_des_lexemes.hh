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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <string>

namespace kuri {
struct chaine;
struct chaine_statique;
}  // namespace kuri

kuri::chaine supprime_accents(kuri::chaine_statique avec_accent);
bool remplace(std::string &std_string, std::string_view motif, std::string_view remplacement);

void inclus_systeme(std::ostream &os, kuri::chaine_statique fichier);
void inclus(std::ostream &os, kuri::chaine_statique fichier);

void prodeclare_struct(std::ostream &os, kuri::chaine_statique nom);
void prodeclare_struct_espace(std::ostream &os,
                              kuri::chaine_statique nom,
                              kuri::chaine_statique espace,
                              kuri::chaine_statique param_gabarit);

void remplace_si_different(kuri::chaine_statique nom_source, kuri::chaine_statique nom_dest);
