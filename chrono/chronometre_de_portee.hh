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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "outils.hh"

#include <iostream>

namespace dls {
namespace chronometrage {

/**
 * Cette classe sert à chronométrer sa propre durée de vie, et ainsi l'étendue
 * du cadre dans lequel une de ses instance vit.
 */
class chronometre_de_portee {
	double m_debut = 0.0;
	std::string m_message = {};
	std::ostream &m_flux;

public:
	explicit chronometre_de_portee(
			const std::string &message,
			std::ostream &flux = std::cerr)
		: m_debut(maintenant())
		, m_message(message)
		, m_flux(flux)
	{}

	~chronometre_de_portee()
	{
		m_flux << m_message << " : " << (maintenant() - m_debut) << '\n';
	}
};

#define CHRONOMETRE_PORTEE(message, flux) \
	dls::chronometrage::chronometre_de_portee chrono_portee(message, flux);

}  /* namespace chronometrage */
}  /* namespace dls */
