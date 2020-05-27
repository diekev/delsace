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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "profilage.hh"

#include <iostream>

InfoProfilage s_info_profilage;

static void imprime_infos_profilage()
{
	auto premier_info = s_info_profilage.premiere;

	while (premier_info != nullptr) {
		std::cerr << premier_info->fonction << " : " << premier_info->temps << "µs\n";
		premier_info = premier_info->suivante;
	}
}

Chronometre::Chronometre(InfoProfilage &info)
	: m_info(info)
{
	m_temps.commence();
}

Chronometre::~Chronometre()
{
	m_info.temps += m_temps.temps();
}

ImprimeuseTemps::~ImprimeuseTemps()
{
	imprime_infos_profilage();
}
