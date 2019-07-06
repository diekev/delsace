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

#pragma once

#include <string>

#include "morceaux.hh"

/**
 * Retourne vrai si le caractère spécifié est un nombre décimal, c'est-à-dire
 * entre 0 et 9.
 */
constexpr bool est_nombre_decimal(char c)
{
	return (c >= '0') && (c <= '9');
}

/**
 * Extrait un nombre depuis une chaîne de caractère spécifiée par 'debut' et
 * 'fin'. La chaine est stockée dans 'chaine' et son identifiant dans
 * 'id_nombre'.
 *
 * Retourne le nombre de caractère de la chaîne [debut, fin] qui a été consommé.
 */
size_t extrait_nombre(const char *debut, const char *fin, id_morceau &id_nombre);

/**
 * Converti une chaîne de caractère en un nombre entier de type 'long'. Si la
 * chaîne de caractère représente un nombre qui ne peut être représenté par un
 * entier de type 'long' (64-bit), la valeur maximale 0xffffffff est retournée.
 */
long converti_chaine_nombre_entier(const std::string_view &chaine, id_morceau identifiant);

/**
 * Converti une chaîne de caractère en un nombre réel de type 'double'.
 */
double converti_chaine_nombre_reel(const std::string_view &chaine, id_morceau identifiant);
