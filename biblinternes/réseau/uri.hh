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

namespace reseau {

// URI = scheme:[//authority]path[?query][#fragment]
// authority = [userinfo@]host[:port]
class uri {
	std::string m_uri{};
	std::string_view m_schema{};
	std::string_view m_autorite{};
	std::string_view m_chemin{};
	std::string_view m_requete{};
	std::string_view m_fragment{};
	std::string_view m_userinfo{};
	std::string_view m_hote{};
	std::string_view m_port{};

public:
	explicit uri(const std::string &chaine);

	bool est_valide() const;

	std::string_view schema() const;

	std::string_view autorite() const;

	std::string_view chemin() const;

	std::string_view requete() const;

	std::string_view fragment() const;

	std::string_view hote() const;

	std::string_view userinfo() const;

	std::string_view port() const;
};

}
