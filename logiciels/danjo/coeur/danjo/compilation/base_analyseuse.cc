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

#include "base_analyseuse.h"

#include "biblinternes/langage/tampon_source.hh"

#include "erreur.h"

namespace danjo {

base_analyseuse::base_analyseuse(lng::tampon_source const &tampon, dls::tableau<DonneesMorceaux> &identifiants)
	: lng::analyseuse<DonneesMorceaux>(identifiants)
	, m_tampon(tampon)
{}

void base_analyseuse::lance_erreur(const std::string &quoi)
{
	const auto numero_ligne = (donnees().ligne_pos >> 32);
	const auto position_ligne = donnees().ligne_pos & 0xffffffff;
	const auto ligne = m_tampon[numero_ligne];
	const auto contenu = donnees().chaine;

	throw ErreurSyntactique(ligne, numero_ligne + 1, position_ligne, quoi, contenu);
}

}  /* namespace danjo */
