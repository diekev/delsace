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

#include "gabarit.hh"

#include "../decoupage/analyseuse_grammaire.hh"
#include "../decoupage/assembleuse_arbre.hh"
#include "../decoupage/decoupeuse.hh"

namespace tori {

std::string calcul_gabarit(std::string const &gabarit, ObjetDictionnaire &objet)
{
	auto const tampon = lng::tampon_source{gabarit};

	auto decoupeuse = decoupeuse_texte(tampon);
	decoupeuse.genere_morceaux();

	auto assembleuse = assembleuse_arbre{};

	auto analyseuse = analyseuse_grammaire(decoupeuse.morceaux(), tampon, assembleuse);
	analyseuse.lance_analyse(std::cerr);

	return assembleuse.genere_code(objet);
}

}  /* namespace tori */
