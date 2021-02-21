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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "structures/chaine.hh"

/**
 * Ces structures sont les mêmes que celles définies par le langage (tableaux
 * via « []TYPE », et chaine via « chaine ») ; elles sont donc la même
 * définition que celles du langage. Elles sont utilisées pour pouvoir passer
 * des messages sainement entre la compilatrice et les métaprogrammes. Par
 * sainement, on entend que l'interface binaire de l'application doit être la
 * même.
 */

namespace kuri {

/* Structure pour passer les lexèmes aux métaprogrammes, via compilatrice_lèxe_fichier
 */
struct Lexeme {
	int genre;
	chaine texte;
};

}
