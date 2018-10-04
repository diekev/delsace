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

struct DonneesMorceaux;
struct TamponSource;

namespace erreur {

class frappe {
	std::string m_message;

public:
	explicit frappe(const char *message);

	const char *message() const;
};

[[noreturn]] void lance_erreur(
		const std::string &quoi,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau);

[[noreturn]] void lance_erreur_nombre_arguments(
		const size_t nombre_arguments,
		const size_t nombre_recus,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau);

[[noreturn]] void lance_erreur_type_arguments(
		const int type_arg,
		const int type_enf,
		const std::string_view &nom_arg,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau);

[[noreturn]] void lance_erreur_argument_inconnu(
		const std::string_view &nom_arg,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau);

[[noreturn]] void lance_erreur_redeclaration_argument(
		const std::string_view &nom_arg,
		const TamponSource &tampon,
		const DonneesMorceaux &morceau);

}
