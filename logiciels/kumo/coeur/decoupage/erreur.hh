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

#include "biblinternes/structures/chaine.hh"

struct DonneesMorceaux;

enum class id_morceau : unsigned int;

namespace lng {
class tampon_source;
}

namespace erreur {

enum {
	NORMAL,
	DECOUPAGE,

	AUCUNE_ERREUR,
};

class frappe {
	dls::chaine m_message;
	int m_type;

public:
	frappe(const char *message, int type);

	int type() const;

	const char *message() const;
};

[[noreturn]] void lance_erreur(
		const dls::chaine &quoi,
		lng::tampon_source const &tampon,
		const DonneesMorceaux &morceau,
		int type = NORMAL);

}
