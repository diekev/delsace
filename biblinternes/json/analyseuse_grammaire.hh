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

#include "assembleuse_objet.hh"
#include "morceaux.hh"

class analyseuse_grammaire : public lng::analyseuse<DonneesMorceau> {
	lng::tampon_source const &m_tampon;
	assembleuse_objet m_assembleuse{};

public:
	analyseuse_grammaire(
			dls::tableau<DonneesMorceau> &identifiants,
			lng::tampon_source const &tampon);

	void lance_analyse(std::ostream &os) override;

	assembleuse_objet::ptr_objet objet() const;

private:
	void analyse_objet();
	void analyse_valeur(const dls::vue_chaine &nom_objet);

	/**
	 * Lance une exception de type ErreurSyntactique contenant la chaîne passée
	 * en paramètre ainsi que plusieurs données sur l'identifiant courant
	 * contenues dans l'instance DonneesMorceaux lui correspondant.
	 */
	[[noreturn]] void lance_erreur(const dls::chaine &quoi, int type = 0);
};
