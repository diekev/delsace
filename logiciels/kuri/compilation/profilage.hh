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

#pragma once

#include "biblinternes/chrono/chronometrage.hh"

#undef PROFILAGE

struct InfoProfilage {
	InfoProfilage *premiere = nullptr;
	InfoProfilage *derniere = nullptr;
	InfoProfilage *suivante = nullptr;
	InfoProfilage *precedente = nullptr;
	double temps = 0.0;
	const char *fonction;
	int nombre_appels = 0;
};

extern InfoProfilage s_info_profilage;

struct Chronometre {
	dls::chrono::compte_microseconde m_temps{};
	InfoProfilage &m_info;

	Chronometre(InfoProfilage &info);

	~Chronometre();
};

struct ImprimeuseTemps {
	ImprimeuseTemps() = default;

	~ImprimeuseTemps();
};

#ifdef PROFILAGE
#define INITIALISE_PROFILAGE \
	auto imprimeuse = ImprimeuseTemps()
#else
#define INITIALISE_PROFILAGE
#endif

#ifdef PROFILAGE
#define PROFILE_FONCTION \
	static InfoProfilage info_profilage; \
	if (s_info_profilage.premiere == nullptr) { \
		s_info_profilage.premiere = &info_profilage; \
		s_info_profilage.derniere = &info_profilage; \
	} \
	else if (info_profilage.precedente == nullptr) { \
		if (s_info_profilage.derniere != nullptr) { \
			s_info_profilage.derniere->suivante = &info_profilage; \
		} \
		info_profilage.precedente = s_info_profilage.derniere; \
		s_info_profilage.derniere = &info_profilage; \
	} \
	info_profilage.fonction = __func__; \
	info_profilage.nombre_appels += 1; \
	auto chrono_profile = Chronometre(info_profilage);
#else
#define PROFILE_FONCTION
#endif
