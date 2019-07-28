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

#include <sys/time.h>

namespace dls::chrono {

/**
 * Retourne le temps courrant en seconde.
 */
[[nodiscard]] inline double maintenant() noexcept
{
	struct timeval now;
	gettimeofday(&now, nullptr);

	return static_cast<double>(now.tv_sec) + static_cast<double>(now.tv_usec) * 1e-6;
}

/**
 * Retourne le temps en seconde s'étant écoulé depuis le temps passé en paramètre.
 */
[[nodiscard]] inline double delta(double temps) noexcept
{
	return maintenant() - temps;
}

/**
 * Structure pour enrober les fonctions maintenant() et delta(temps) afin de
 * mieux controler leurs précisions (heure/minute/secondes...)
 */
template <int D>
struct compte_temps {
private:
	double m_temps = 0.0;

public:
	explicit compte_temps(bool commence_ = true)
	{
		if (commence_) {
			commence();
		}
	}

	inline void commence()
	{
		m_temps = maintenant();
	}

	[[nodiscard]] inline double temps() const
	{
		return delta(m_temps) / static_cast<double>(D);
	}
};

using compte_seconde = compte_temps<1>;
using compte_minute = compte_temps<60>;
using compte_heure = compte_temps<3600>;

/**
 * Structure définissant un chronomètre pouvant être arrêté et repris. Elle
 * s'appele seulement 'metre' car avec l'espace de nom, cela donne
 * chrono::metre, forçant ainsi une bonne utilisation des espaces de nom.
 */
template <int D>
struct metre {
private:
	compte_temps<D> m_compteuse{false};
	double m_total = 0.0;
	bool m_lance = false;

public:
	inline void commence()
	{
		m_lance = true;
		m_total = 0.0;
		m_compteuse.commence();
	}

	[[nodiscard]] inline double arrete()
	{
		if (m_lance) {
			m_total += m_compteuse.temps();
			m_lance = false;
		}

		return m_total;
	}

	inline void reprend()
	{
		if (!m_lance) {
			m_compteuse.commence();
			m_lance = true;
		}
	}

	[[nodiscard]] inline double lis()
	{
		if (m_lance) {
			arrete();
			reprend();
		}

		return m_total;
	}
};

using metre_seconde = metre<1>;
using metre_minute = metre<60>;
using metre_heure = metre<3600>;

}  /* namespace dls::chrono */
