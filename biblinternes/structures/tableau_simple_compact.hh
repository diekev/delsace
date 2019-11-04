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

#include "biblinternes/memoire/logeuse_memoire.hh"

namespace dls {

/**
 * Structure tableau dont le pointeur et la taille sont stockés dans les mêmes
 * bits, en utilisant le principe que dans un système 64-bit, seul les 48
 * derniers bits sont utilisés pour stocker l'adresse. Ainsi la taille est du
 * tableau est stockée dans les 16 premiers bits.
 *
 * La taille maximale du tableau est donc de 65536 éléments.
 */
template <typename T>
struct tableau_simple_compact {
private:
	/**
	 * Structure auxilliaire engloblant la logique de compression et de
	 * décompression du pointeur et de l'entier.
	 *
	 * Voir d'autres cas de figures :
	 * https://en.wikipedia.org/wiki/Tagged_pointer
	 * https://nikic.github.io/2012/02/02/Pointer-magic-for-efficient-dynamic-value-representations.html
	 */
	struct pointeur_et_entier {
		T *m_donnees = nullptr;

		static constexpr auto masque_pointeur = 0xffff000000000000;

		inline pointeur_et_entier(T *pointeur_ = nullptr, int entier_ = 0)
		{
			ajourne(pointeur_, entier_);
		}

		inline void ajourne(T *pointeur_, long entier_ = 0)
		{
			m_donnees = reinterpret_cast<T *>(
						reinterpret_cast<uint64_t>(pointeur_) | static_cast<uint64_t>(entier_) << 48l);
		}

		inline T *pointeur() const
		{
			return reinterpret_cast<T *>(reinterpret_cast<uint64_t>(m_donnees) & ~masque_pointeur);
		}

		inline int entier() const
		{
			return static_cast<int>((reinterpret_cast<uint64_t>(m_donnees) & masque_pointeur) >> 48l);
		}
	};

	pointeur_et_entier x{};

public:
	tableau_simple_compact() = default;

	tableau_simple_compact(tableau_simple_compact const &autre) = default;
	tableau_simple_compact &operator=(tableau_simple_compact const &autre) = default;

	tableau_simple_compact(tableau_simple_compact &&autre)
	{
		std::swap(x.m_donnees, autre.x.m_donnees);
	}

	tableau_simple_compact &operator=(tableau_simple_compact &&autre)
	{
		std::swap(x.m_donnees, autre.x.m_donnees);
		return *this;
	}

	~tableau_simple_compact()
	{
		auto ptr = donnees();
		memoire::deloge_tableau("tableau_simple_compact", ptr, taille());
	}

	T *donnees()
	{
		return x.pointeur();
	}

	T const *donnees() const
	{
		return x.pointeur();
	}

	int taille() const
	{
		return x.entier();
	}

	void pousse(T const &valeur)
	{
		assert(taille() < 65536);

		auto ptr = x.pointeur();

		memoire::reloge_tableau("tableau_simple_compact", ptr, taille(), taille() + 1);

		ptr[taille()] = valeur;

		x.ajourne(ptr, taille() + 1);
	}

	T operator[](long idx)
	{
		assert(idx >= 0 && idx < 65536);
		return donnees()[idx];
	}

	T operator[](long idx) const
	{
		assert(idx >= 0 && idx < 65536);
		return donnees()[idx];
	}
};

}  /* namespace dls */
