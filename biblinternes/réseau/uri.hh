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
	dls::vue_chaine m_schema{};
	dls::vue_chaine m_autorite{};
	dls::vue_chaine m_chemin{};
	dls::vue_chaine m_requete{};
	dls::vue_chaine m_fragment{};
	dls::vue_chaine m_userinfo{};
	dls::vue_chaine m_hote{};
	dls::vue_chaine m_port{};

public:
	explicit uri(const std::string &chaine);

	bool est_valide() const;

	dls::vue_chaine schema() const;

	dls::vue_chaine autorite() const;

	dls::vue_chaine chemin() const;

	dls::vue_chaine requete() const;

	dls::vue_chaine fragment() const;

	dls::vue_chaine hote() const;

	dls::vue_chaine userinfo() const;

	dls::vue_chaine port() const;
};

}
