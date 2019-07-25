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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/math/limites.hh"

class IteratricePosition {
	limites3i m_lim;
	dls::math::vec3i m_etat;

public:
	IteratricePosition(limites3i const &lim)
		: m_lim(lim)
		, m_etat(lim.min)
	{}

	dls::math::vec3i suivante()
	{
		auto etat = m_etat;

		m_etat.x += 1;

		if (m_etat.x >= m_lim.max.x) {
			m_etat.x = m_lim.min.x;

			m_etat.y += 1;

			if (m_etat.y >= m_lim.max.y) {
				m_etat.y = m_lim.min.y;

				m_etat.z += 1;
			}
		}

		return etat;
	}

	bool fini() const
	{
		return m_etat.z >= m_lim.max.z;
	}
};

class IteratricePositionInv {
	limites3i m_lim;
	dls::math::vec3i m_etat;

public:
	IteratricePositionInv(limites3i const &lim)
		: m_lim(lim)
		, m_etat(lim.max)
	{}

	dls::math::vec3i suivante()
	{
		auto etat = m_etat;

		m_etat.x -= 1;

		if (m_etat.x < m_lim.min.x) {
			m_etat.x = m_lim.max.x - 1;

			m_etat.y -= 1;

			if (m_etat.y < m_lim.min.y) {
				m_etat.y = m_lim.max.y - 1;

				m_etat.z -= 1;
			}
		}

		return etat;
	}

	bool fini() const
	{
		return m_etat.z < m_lim.min.z;
	}
};
