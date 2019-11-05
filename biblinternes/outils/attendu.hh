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
#include <utility>

namespace dls {

template <typename T>
struct inattendu {
private:
	T m_valeur{};

public:
	inattendu() = default;

	inattendu(T const &v)
		: m_valeur(v)
	{}

	T valeur() const
	{
		return m_valeur;
	}
};

template <typename T, typename E>
struct attendu {
private:
	union {
		T oui;
		E non;
	};

	bool ok = true;

public:
	attendu()
	{
		new (&oui) T();
	}

	attendu(T const &v)
	{
		new (&oui) T(v);
	}

	attendu(attendu const &autre)
		: ok(autre.ok)
	{
		if (ok) {
			new (&oui) T(autre.oui);
		}
		else {
			new (&non) T(autre.non);
		}
	}

	attendu(inattendu<E> const &autre)
		: ok(false)
	{
		new (&non) E(autre.valeur());
	}

	template <typename U = T>
	explicit attendu(U &&autre)
		: ok(autre.ok)
	{
		if (ok) {
			new (&oui) T(std::move(autre.oui));
		}
		else {
			new (&non) T(std::move(autre.non));
		}
	}

	T &operator*()
	{
		// if (!ok) { throw non; }
		return oui;
	}

	T const &operator*() const
	{
		// if (!ok) { throw non; }
		return oui;
	}

	T *operator->()
	{
		// if (!ok) { throw non; }
		return &**this;
	}

	T const *operator->() const
	{
		// if (!ok) { throw non; }
		return &**this;
	}

	E &erreur()
	{
		assert(!ok);
		return non;
	}

	E const &erreur() const
	{
		assert(!ok);
		return non;
	}

	bool valorise() const
	{
		return ok;
	}

	explicit operator bool() const noexcept
	{
		return ok;
	}

	T &valeur()
	{
		// if (!ok) { throw non; }
		return oui;
	}

	T const &valeur() const
	{
		// if (!ok) { throw non; }
		return oui;
	}

	template <typename U>
	T valeur_ou(U &&v)
	{
		if (ok) {
			return valeur();
		}

		return T(std::forward<U>(v));
	}

	std::enable_if_t<
		std::is_nothrow_move_constructible_v<T>
	&& std::is_swappable_v<T>
	&& std::is_nothrow_move_constructible_v<E>
	&& std::is_swappable_v<E>
	>
	swap(attendu &autre)
	{
		if (ok) {
			if (autre.ok) {
				using std::swap;
				swap(oui, autre.oui);
			}
			else {
				autre.swap(*this);
			}
		}
		else {
			if (!autre.ok) {
				using std::swap;
				swap(non, autre.non);
			}
			else {
				E t{std::move(non)};
				non.~E();
				new (&oui) T(std::move(autre.oui));
				autre.oui.~T();
				new (&autre.non) E(std::move(t));
				autre.ok = false;
			}
		}
	}
};

}  /* namespace dls */
