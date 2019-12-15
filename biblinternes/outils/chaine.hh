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

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

namespace dls {

tableau<chaine> morcelle(chaine const &texte, char const delimiteur);

tableau<chaine> morcelle(chaine const &texte, chaine const &delimitrice);

bool remplace_chaine(chaine &chn, chaine const &de, chaine const &vers);

void remplace_souschaine(
		dls::chaine &str,
		dls::chaine const &substr,
		dls::chaine const &rep);

chaine garnis_chaine(chaine const &chn, long taille);

chaine chaine_depuis_entier(int nombre);

chaine premier_n_caracteres(chaine const &chn, long n);

chaine dernier_n_caracteres(chaine const &chn, long n);

long compte(const dls::chaine &str, char c);

long compte_commun(dls::chaine const &rhs, dls::chaine const &lhs);

long distance_levenshtein(dls::vue_chaine_compacte const &chn1, dls::vue_chaine_compacte const &chn2);

}  /* namespace dls */
