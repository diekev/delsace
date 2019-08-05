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

#include "objet.hh"

#include "lumiere.hh"
#include "maillage.hh"

namespace kdo {

Objet::~Objet()
{
	if (type == TypeObjet::LUMIERE) {
		delete lumiere;
	}
	else if (type == TypeObjet::MAILLAGE) {
		memoire::deloge("Maillage", maillage);
	}
}

Objet::Objet(Lumiere *l)
	: type(TypeObjet::LUMIERE)
	, nom("lumière")
	, lumiere(l)
{}

Objet::Objet(Maillage *m)
	: type(TypeObjet::MAILLAGE)
	, nom(m->nom())
	, maillage(m)
{}

dls::chaine Objet::chemin() const
{
	if (type == TypeObjet::LUMIERE) {
		return "/objets/lumières/" + nom + "/";
	}

	return "/objets/maillages/" + nom + "/";
}

}  /* namespace kdo */
