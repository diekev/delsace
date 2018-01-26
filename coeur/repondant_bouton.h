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

#include <string>

namespace kangao {

/**
 * La classe RepondantBouton définie l'interface nécessaire pour attacher un
 * objet à plusieurs bouton dans une interface utilisateur. À chaque fois qu'un
 * bouton est cliqué, cette classe est invoquée.
 */
class RepondantBouton {
public:
	virtual ~RepondantBouton() = default;

	/**
	 * Fonction appelée quand un bouton attaché à ce répondant est cliqué.
	 *
	 * L'identifiant passé en paramètre est l'attache du bouton défini dans le
	 * script (bouton(attache="..."; métadonnée="...")).
	 */
	virtual void repond_clique(const std::string &identifiant, const std::string &metadonnee) = 0;

	/**
	 * Fonction appelée pour vérifié si un bouton doit ou non être désactivé.
	 */
	virtual bool evalue_predicat(const std::string &identifiant, const std::string &metadonnee) = 0;
};

}  /* namespace kangao */
