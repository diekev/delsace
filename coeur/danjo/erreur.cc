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

#include "erreur.h"

#include <sstream>

namespace danjo {

ErreurFrappe::ErreurFrappe(
		const std::string_view &ligne,
		int numero_ligne,
		int position_ligne,
		const std::string &quoi)
{
	std::stringstream ss;

	ss << "Erreur de frappe : ligne " << numero_ligne
	   << ", position " << position_ligne << " :\n"
	   << ligne << '\n'
	   << quoi << '\n';

	/* cppcheck-suppress useInitializationList */
	m_quoi = ss.str();
}

const char *ErreurFrappe::quoi() const
{
	return m_quoi.c_str();
}

ErreurSyntactique::ErreurSyntactique(
		const std::string_view &ligne,
		int numero_ligne,
		int position_ligne,
		const std::string &quoi,
		const std::string &contenu)
{
	std::stringstream ss;

	ss << "Erreur syntactique : ligne " << numero_ligne
	   << ", position "<< position_ligne<< " :\n"
	   << ligne << '\n'
	   << quoi << '\n'
	   << "Obtenu : " << contenu << '\n';

	/* cppcheck-suppress useInitializationList */
	m_quoi = ss.str();
}

const char *ErreurSyntactique::quoi() const
{
	return m_quoi.c_str();
}

}  /* namespace danjo */
