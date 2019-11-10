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

#include "biblinternes/structures/chaine.hh"

namespace lng {

/**
 * Retourne le nombre d'octets Unicode (entre 1 et 4) qui composent le début la
 * séquence précisée. Retourne 0 si la sequence d'octets n'est pas valide en
 * Unicode (UTF-8).
 */
int nombre_octets(const char *sequence);

/**
 * Trouve le décalage pour le caractère de la chaine à la position i. La
 * fonction prend en compte la possibilité qu'un caractère soit invalide et le
 * saute au cas où.
 */
long decalage_pour_caractere(dls::vue_chaine const &chaine, long i);

/**
 * Retourne une chaine correspondant à la chaine spécifiée dénuée d'accents.
 */
dls::chaine supprime_accents(dls::chaine const &chaine);

}  /* namespace lng */
