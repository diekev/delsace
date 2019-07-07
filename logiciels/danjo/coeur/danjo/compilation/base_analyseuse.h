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

#include "biblinternes/langage/analyseuse.hh"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "morceaux.h"

namespace lng {
class tampon_source;
}

namespace danjo {

/**
 * Classe de base pour définir des analyseuses syntactique.
 */
class base_analyseuse : public lng::analyseuse<DonneesMorceaux> {
	lng::tampon_source const &m_tampon;

public:
	base_analyseuse(lng::tampon_source const &tampon, dls::tableau<DonneesMorceaux> &identifiants);

protected:
	/**
	 * Lance une exception de type ErreurSyntactique contenant la chaîne passée
	 * en paramètre ainsi que plusieurs données sur l'identifiant courant
	 * contenues dans l'instance DonneesMorceaux lui correspondant.
	 */
	[[noreturn]] void lance_erreur(const dls::chaine &quoi);
};

} /* namespace danjo */
