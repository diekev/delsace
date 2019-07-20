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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <any>
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "morceaux.h"

namespace danjo {

class Manipulable;

struct Symbole {
	std::any valeur{};
	id_morceau identifiant{};
};

bool est_operateur(id_morceau identifiant);

bool precedence_faible(id_morceau identifiant1, id_morceau identifiant2);

Symbole evalue_expression(const dls::tableau<Symbole> &expression, Manipulable *manipulable);

void imprime_valeur_symbole(Symbole symbole, std::ostream &os);

}  /* namespace danjo */
