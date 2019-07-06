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
#include "biblinternes/langage/tampon_source.hh"

#include "morceaux.hh"

class assembleuse_arbre;

class analyseuse_grammaire : public lng::analyseuse<DonneesMorceaux> {
	assembleuse_arbre &m_assembleuse;
	lng::tampon_source const &m_tampon;

public:
	analyseuse_grammaire(
			dls::tableau<DonneesMorceaux> &identifiants,
			lng::tampon_source const &tampon,
	        assembleuse_arbre &assembleuse);

	void lance_analyse(std::ostream &os) override;

private:
	/**
	 * Lance une exception de type ErreurSyntactique contenant la chaîne passée
	 * en paramètre ainsi que plusieurs données sur l'identifiant courant
	 * contenues dans l'instance DonneesMorceaux lui correspondant.
	 */
	[[noreturn]] void lance_erreur(const std::string &quoi, int type = 0);

	void analyse_page();
	void analyse_expression();
	void analyse_si();
	void analyse_pour();
};
