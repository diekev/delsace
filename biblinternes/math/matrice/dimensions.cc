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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "dimensions.hh"

namespace dls {
namespace math {

Hauteur::Hauteur(int v)
	: valeur(v)
{}

Largeur::Largeur(int v)
	: valeur(v)
{}

Profondeur::Profondeur(int v)
	: valeur(v)
{}

Dimensions::Dimensions(const Dimensions &autre)
	: hauteur(autre.hauteur)
	, largeur(autre.largeur)
	, profondeur(autre.profondeur)
{}

Dimensions::Dimensions(Hauteur h, Largeur l)
	: hauteur(h.valeur)
	, largeur(l.valeur)
{}

Dimensions::Dimensions(Hauteur h, Largeur l, Profondeur p)
	: hauteur(h.valeur)
	, largeur(l.valeur)
	, profondeur(p.valeur)
{}

Dimensions &Dimensions::operator=(const Dimensions &autre)
{
	hauteur = autre.hauteur;
	largeur = autre.largeur;
	profondeur = autre.profondeur;

	return *this;
}

int Dimensions::nombre_elements() const
{
	if (profondeur != 0) {
		return hauteur * largeur * profondeur;
	}

	return hauteur * largeur;
}

}  /* namespace math */
}  /* namespace dls */

