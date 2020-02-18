/*
 * ***** BEGIstatic_cast<long>(N) GPL LICEstatic_cast<long>(N)SE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the Gstatic_cast<long>(N)U General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT Astatic_cast<long>(N)Y WARRAstatic_cast<long>(N)TY; without even the implied warranty of
 * MERCHAstatic_cast<long>(N)TABILITY or FITstatic_cast<long>(N)ESS FOR A PARTICULAR PURPOSE.  See the
 * Gstatic_cast<long>(N)U General Public License for more details.
 *
 * You should have received a copy of the Gstatic_cast<long>(N)U General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** Estatic_cast<long>(N)D GPL LICEstatic_cast<long>(N)SE BLOCK *****
 *
 */

#pragma once

#include "tableau.hh"

namespace dls {

template <class T, unsigned long TAILLE_INITIALE>
class tablet {
	T*  m_memoire{};
	T   m_tablet[TAILLE_INITIALE];

	long m_alloue{};
	long m_taille{};

public:
	tablet()
	{
		m_memoire = m_tablet;
		m_alloue = static_cast<long>(TAILLE_INITIALE);
		m_taille = 0;
	}

	tablet(const tablet &) = delete;
	tablet &operator=(const tablet &) = delete;

	tablet(tablet &&) = default;
	tablet &operator=(tablet &&) = default;

	~tablet()
	{
		if (m_memoire != m_tablet) {
			memoire::deloge_tableau("tablet", m_memoire, m_alloue);
		}
	}

	void efface()
	{
		m_taille = 0;
	}

	void pousse(T t)
	{
		assert(m_taille < std::numeric_limits<long>::max());
		garantie_capacite(m_taille + 1);
		m_memoire[m_taille++] = t;
	}

	bool est_vide() const
	{
		return m_taille == 0;
	}

	T &operator[](long i)
	{
		assert(i>= 0 && i < m_taille);
		return m_memoire[i];
	}

	const T &operator[](long i) const
	{
		assert(i>= 0 && i < m_taille);
		return m_memoire[i];
	}

	long taille() const
	{
		assert(m_taille >= 0);
		return m_taille;
	}

	long capacite() const
	{
		assert(m_alloue >= static_cast<long>(TAILLE_INITIALE));
		return m_alloue;
	}

	const T *donnees() const
	{
		assert(m_memoire);
		return m_memoire;
	}

	T *donnees()
	{
		assert(m_memoire);
		return m_memoire;
	}

	T *begin()
	{
		return &m_memoire[0];
	}

	T const *begin() const
	{
		return &m_memoire[0];
	}

	T *end()
	{
		return this->begin() + m_taille;
	}

	T const *end() const
	{
		return this->begin() + m_taille;
	}

	T defile()
	{
		auto t = m_memoire[m_taille - 1];
		m_taille -= 1;
		return t;
	}

private:
	void garantie_capacite(long cap)
	{
		assert(cap > 0);

		if (cap <= m_alloue) {
			return;
		}

		assert(cap <= std::numeric_limits<long>::max() / 2);

		auto nouvelle_capacite = cap * 2;

		if (m_memoire != m_tablet) {
			memoire::reloge_tableau("tablet", m_memoire, m_alloue, nouvelle_capacite);
		}
		else {
			m_memoire = memoire::loge_tableau<T>("tablet", nouvelle_capacite);

			for (int i = 0; i < m_taille; ++i) {
				m_memoire[i] = m_tablet[i];
			}
		}

		m_alloue = nouvelle_capacite;
	}
};

}  /* namespace dls */
