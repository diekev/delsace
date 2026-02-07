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

#include <assert.h>
#include <functional>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>

namespace dls::chrono {

/**
 * Structure pour enrober les fonctions maintenant() et delta(temps) afin de
 * mieux controler leurs précisions (heure/minute/secondes...)
 */
template <int64_t D>
struct compte_temps {
private:
	double m_temps = 0.0;

	/**
	 * Retourne le temps courrant en microseconde.
	 */
	[[nodiscard]] inline double maintenant() const noexcept
	{
		struct timeval now;
		gettimeofday(&now, nullptr);

		return static_cast<double>(now.tv_sec) * 1000000.0 + static_cast<double>(now.tv_usec);
	}

	/**
	 * Retourne le temps en microseconde s'étant écoulé depuis le temps passé en paramètre.
	 */
	[[nodiscard]] inline double delta(double temps) const noexcept
	{
		return maintenant() - temps;
	}

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

    compte_temps &operator=(double valeur)
    {
        this->m_temps = valeur;
    }
};

using compte_microseconde = compte_temps<1>;
using compte_milliseconde = compte_temps<1000>;
using compte_seconde = compte_temps<1000000>;
using compte_minute = compte_temps<60000000>;
using compte_heure = compte_temps<3600000000>;

template <int64_t N>
struct chrono_rappel {
private:
    compte_temps<N> chrono{};
    std::function<void(double)> m_rappel{};

public:
    chrono_rappel(std::function<void(double)> rappel)
        : m_rappel(rappel)
    {}

    ~chrono_rappel()
    {
        m_rappel(chrono.temps());
    }
};

using chrono_rappel_microseconde = chrono_rappel<1>;
using chrono_rappel_milliseconde = chrono_rappel<1000>;
using chrono_rappel_seconde = chrono_rappel<1000000>;
using chrono_rappel_minute = chrono_rappel<60000000>;
using chrono_rappel_heure = chrono_rappel<3600000000>;

/**
 * Structure définissant un chronomètre pouvant être arrêté et repris. Elle
 * s'appele seulement 'metre' car avec l'espace de nom, cela donne
 * chrono::metre, forçant ainsi une bonne utilisation des espaces de nom.
 */
template <int64_t D>
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
			m_total = arrete();
			reprend();
		}

		return m_total;
	}
};

using metre_microseconde = metre<1>;
using metre_milliseconde = metre<1000>;
using metre_seconde = metre<1000000>;
using metre_minute = metre<60000000>;
using metre_heure = metre<3600000000>;

inline void dors_millisecondes(int millisecondes)
{
	assert(millisecondes >= 0);
	usleep(static_cast<unsigned>(millisecondes * 1000));
}

inline void dors_microsecondes(int microsecondes)
{
	assert(microsecondes >= 0);
	usleep(static_cast<unsigned>(microsecondes));
}

}  /* namespace dls::chrono */
