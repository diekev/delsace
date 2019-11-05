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

#include <cassert>
#include <cstdint>
#include <iostream>

namespace dls {

/**
 * Différentes structures permettant de stocker des valeurs dans les bits
 * inutilisés des pointeurs.
 *
 * Voir :
 * https://en.wikipedia.org/wiki/Tagged_pointer
 * https://nikic.github.io/2012/02/02/Pointer-magic-for-efficient-dynamic-value-representations.html
 */

/**
 * Structure permettant de stocker des valeurs dans les bits hauts inutilisés
 * d'un pointeur sur les systèmes 64-bit.
 */
template <typename T>
struct pointeur_marque_haut {
private:
	/* utilisation d'une union pour éviter les transtypages barbants */
	union {
		T *comme_pointeur;
		uintptr_t comme_bits;
	};

	static constexpr uintptr_t masque_marque = 0xffff000000000000l;

public:
	inline pointeur_marque_haut(T *pointeur_ = nullptr, int marque_ = 0)
	{
		ajourne(pointeur_, marque_);
	}

	inline void ajourne(T *pointeur_, long marque_ = 0)
	{
		comme_pointeur = pointeur_;
		comme_bits |= static_cast<uintptr_t>(marque_) << 48l;
	}

	inline T *pointeur() const
	{
		return reinterpret_cast<T *>(comme_bits & ~masque_marque);
	}

	inline int marque() const
	{
		return static_cast<int>((comme_bits & masque_marque) >> 48l);
	}
};

/**
 * Structure permettant de stocker des valeurs dans les bits bas inutilisés d'un
 * pointeur aligné.
 */
template <typename T, int alignement>
struct pointeur_marque_bas {
	static_assert(
			alignement != 0 && ((alignement & (alignement - 1)) == 0),
			"Le paramètre d'alignement doit être une puissance de 2"
		);

	/* Pour un alignement sur 8 octets
	 * masque_marque = alignement - 1 = 8 - 7 = 0b111
	 * c'est-à-dire que les trois bits les plus petits sont libres pour stocker
	 * la marque.
	 */
	static const intptr_t masque_marque = alignement - 1;

	/* masque_pointeur est tout le contraire : 0b...11111000
	 * c'est-à-dire que le pointeur est stocké dans les bits sauf les 3 du bas.
	 */
	static const intptr_t masque_pointeur = ~masque_marque;

	/* utilisation d'une union pour éviter les transtypages barbants */
	union {
		T *comme_pointeur;
		intptr_t comme_bits;
	};

public:
	inline pointeur_marque_bas(T *pointeur_ = 0, int marque_ = 0)
	{
		ajourne(pointeur_, marque_);
	}

	inline void ajourne(T *pointeur_, int marque_ = 0)
	{
		/* assure que le pointeur est bel et bien aligné */
		assert((reinterpret_cast<intptr_t>(pointeur_) & masque_marque) == 0);
		/* assure que la marque ne soit pas trop grande */
		assert((marque_ & masque_pointeur) == 0);

		comme_pointeur = pointeur_;
		comme_bits |= marque_;
	}

	inline T *pointeur() const
	{
		return reinterpret_cast<T *>(comme_bits & masque_pointeur);
	}

	inline int marque() const
	{
		return comme_bits & masque_marque;
	}
};

/**
 * Structure mélanger les principes des deux structures du dessus : on peut y
 * stocker 2 valeurs, l'une dans les bits hauts, et une autre dans les bits bas.
 */
template <typename T, int alignement>
struct pointeur_marque_haut_bas {
	static_assert(
			alignement != 0 && ((alignement & (alignement - 1)) == 0),
			"Le paramètre d'alignement doit être une puissance de 2"
		);

	static const uintptr_t masque_marque_haut = 0xffff000000000000;

	/* Pour un alignement sur 8 octets
	 * masque_marque = alignement - 1 = 8 - 7 = 0b111
	 * c'est-à-dire que les trois bits les plus petits sont libres pour stocker
	 * la marque.
	 */
	static const uintptr_t masque_marque_bas = (alignement - 1) | masque_marque_haut;

	/* masque_pointeur est tout le contraire : 0b...11111000
	 * c'est-à-dire que le pointeur est stocké dans les bits sauf les 3 du bas.
	 */
	static const uintptr_t masque_pointeur = ~masque_marque_bas;

	/* utilisation d'une union pour éviter les transtypages barbants */
	union {
		T *comme_pointeur;
		uintptr_t comme_bits;
	};

public:
	inline pointeur_marque_haut_bas(T *pointeur_ = 0, int marque_haut_ = 0, int marque_bas_ = 0)
	{
		ajourne(pointeur_, marque_haut_, marque_bas_);
	}

	inline void ajourne(T *pointeur_, int marque_haut_ = 0, int marque_bas_ = 0)
	{
		/* assure que le pointeur est bel et bien aligné */
		assert((reinterpret_cast<uintptr_t>(pointeur_) & masque_marque_bas) == 0);
		/* assure que la marque ne soit pas trop grande */
		assert((static_cast<uintptr_t>(marque_bas_) & masque_pointeur) == 0);

		comme_pointeur = pointeur_;
		comme_bits |= static_cast<uintptr_t>(marque_bas_);
		comme_bits |= static_cast<uintptr_t>(marque_haut_) << 48l;
	}

	inline T *pointeur() const
	{
		return reinterpret_cast<T *>(comme_bits & masque_pointeur);
	}

	inline int marque_haut() const
	{
		return static_cast<int>((comme_bits & masque_marque_haut) >> 48l);
	}

	inline int marque_bas() const
	{
		return static_cast<int>(comme_bits & masque_marque_bas);
	}
};

}  /* namespace dls */
