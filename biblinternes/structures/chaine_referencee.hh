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

#include <atomic>
#include <cstring>  /* pour std::strlen */
#include <limits>

namespace dls {

/**
 * La classe reference_chaine détient une chaine référencée pour éviter d'avoir
 * à dupliquer la mémoire détenu dans une chaine. Cette classe peut être
 * considérer comme une version alternative de vue_chaine, mais avec un compte
 * de référence.
 *
 * L'idée principale est de pouvoir dans le future avoir une chaine avec copie
 * sur écriture (CSE).
 */
struct reference_chaine {
protected:
	struct __detentrice {
		/* pointeur vers une chaine partagée */
		char const *m_chaine = nullptr;

		long m_taille = 0;

		unsigned int m_empreinte = 0;

		std::atomic<int> m_compte_ref = 0;

		__detentrice() = default;

		__detentrice(char const *str, long taille);

		char const *c_str() const;

		long taille() const;

		unsigned int empreinte() const;

		void reference();

		void dereference();

		int compte_ref();
	};

	union {
		/* pointeur vers une chaine littérale */
		char const *m_donnees_si_chars;

		/* pointeur vers la détentrice partagée */
		__detentrice *m_donnees_si_detentrice;
	};

	/* également utiliser pour déterminer si la chaine est littérale ou non */
	long m_taille = 0;

	/* stocke l'empreinte pour éviter de toujours la recalculer */
	unsigned int m_empreinte = 0;

	static __detentrice *alloc_detentrice(char const *str, long taille);

	static __detentrice *singletonEmptyString__detentrice;

public:
	reference_chaine() = default;

	reference_chaine(char const *str);

	reference_chaine(char const *str, long taille);

	reference_chaine(reference_chaine const &src);

	~reference_chaine();

	/* accès */
	char const *c_str() const;

	long taille() const;

	unsigned int empreinte() const;

	__detentrice *detentrice();

	__detentrice const *detentrice() const;
};

/**
 * La classe detentrice_chaine nous permet de stocker une chaine référencée,
 * pour économiser de la mémoire. Son comportement est le même que pour
 * reference_chaine, sauf dans le cas d'une construction, où la détentrice
 * possède la chaine et la référence ne fait que pointer vers elle.
 */
struct detentrice_chaine : public reference_chaine {
	detentrice_chaine(char const *str, long taille);

	detentrice_chaine(reference_chaine const &src);
};

inline auto cree_ref_nonsure(reference_chaine const &chn)
{
	static_assert(sizeof(reference_chaine) == sizeof(detentrice_chaine), "Non-correspondance des tailles");
	return reinterpret_cast<detentrice_chaine const &>(chn);
}

}  /* namespace dls */
