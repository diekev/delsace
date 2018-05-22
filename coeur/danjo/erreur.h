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

namespace danjo {

/**
 * La classe ErreurFrappe représente une exception qui est lancée quand une
 * erreur de frappe est repérée.
 */
class ErreurFrappe {
	std::string m_quoi{""};

public:
	/**
	 * Construit le message d'erreur selon les paramètres passés.
	 */
	ErreurFrappe(
			const std::string_view &ligne,
			int numero_ligne,
			int position_ligne,
			const std::string &quoi);

	/**
	 * Retourne le message d'erreur.
	 */
	const char *quoi() const;
};

/**
 * La classe ErreurSyntactique représente une exception qui est lancée quand une
 * erreur de syntaxe est repérée.
 */
class ErreurSyntactique {
	std::string m_quoi{""};

public:
	/**
	 * Construit le message d'erreur selon les paramètres passés.
	 */
	ErreurSyntactique(
			const std::string_view &ligne,
			int numero_ligne,
			int position_ligne,
			const std::string &quoi,
			const std::string &contenu);

	/**
	 * Retourne le message d'erreur.
	 */
	const char *quoi() const;
};

}  /* namespace danjo */
