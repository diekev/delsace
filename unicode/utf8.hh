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

namespace unicode::utf8 {

using char8_t = unsigned char;
using chaine_utf8 = std::basic_string<char8_t>;

/**
 * Retourne le nombre d'octets Unicode (entre 1 et 4) qui composent le début la
 * séquence précisée. Retourne 0 si la sequence d'octets n'est pas valide en
 * Unicode UTF-8.
 */
int nombre_octets(const char8_t *sequence);

/**
 * Retourne vrai si la chaine spécifiée en paramètre est bel et bien une chaine
 * UTF-8 valide.
 */
bool est_valide(const chaine_utf8 &chaine);

/* À FAIRE :
 * - fonction pour assainir les caractères :
 *	- url "é" -> %E9
 *	- html "é" -> &#233;
 *	- autres "é" -> \xE9, \E9
 * - nombre_points_code(), valide et compte le nombre de points de code
 * - rapport d'erreur détaillé : à quel octet avons nous une erreur, et quel
 *   est l'erreur
 * - stratégies de recouvrements en cas d'erreur :
 *   - arrête et retourne immédiatement
 *   - saute les plages d'unités de code
 *   - remplace les plages d'unités de code (�)
 */

}  /* namespace unicode::utf8 */
