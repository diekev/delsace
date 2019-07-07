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

namespace memoire {

template <typename T>
struct chaine_pour_type {
	static const char *CT()
	{
		return typeid(T).name();
	}
};

template <>
struct chaine_pour_type<char> {
	static constexpr const char *CT()
	{
		return "dls::tableau<char>";
	}
};

template <>
struct chaine_pour_type<short> {
	static constexpr const char *CT()
	{
		return "dls::tableau<short>";
	}
};

template <>
struct chaine_pour_type<int> {
	static constexpr const char *CT()
	{
		return "dls::tableau<int>";
	}
};

template <>
struct chaine_pour_type<long> {
	static constexpr const char *CT()
	{
		return "dls::tableau<long>";
	}
};

template <>
struct chaine_pour_type<long long> {
	static constexpr const char *CT()
	{
		return "dls::tableau<long long>";
	}
};

template <>
struct chaine_pour_type<unsigned char> {
	static constexpr const char *CT()
	{
		return "dls::tableau<unsigned char>";
	}
};

template <>
struct chaine_pour_type<unsigned short> {
	static constexpr const char *CT()
	{
		return "dls::tableau<unsigned short>";
	}
};

template <>
struct chaine_pour_type<unsigned int> {
	static constexpr const char *CT()
	{
		return "dls::tableau<unsigned int>";
	}
};

template <>
struct chaine_pour_type<unsigned long> {
	static constexpr const char *CT()
	{
		return "dls::tableau<unsigned long>";
	}
};

template <>
struct chaine_pour_type<unsigned long long> {
	static constexpr const char *CT()
	{
		return "dls::tableau<unsigned long long>";
	}
};

template <>
struct chaine_pour_type<float> {
	static constexpr const char *CT()
	{
		return "dls::tableau<float>";
	}
};

template <>
struct chaine_pour_type<double> {
	static constexpr const char *CT()
	{
		return "dls::tableau<double>";
	}
};

/* struct utilisée dans les conteneurs standards enrobés dans nos structures en
 * attendant d'avoir les nôtres */
template <typename T>
struct logeuse_guardee {
	using size_type       = size_t;
	using difference_type = std::ptrdiff_t;
	using pointer         = T *;
	using const_pointer   = const T *;
	using reference       = T&;
	using const_reference = const T&;
	using value_type      = T;

	logeuse_guardee() = default;

	logeuse_guardee(logeuse_guardee const &) = default;

	T *allocate(unsigned long n, void const *hint = nullptr)
	{
		static_cast<void>(hint);

		auto p = memoire::loge_tableau<T>(chaine_pour_type<T>::CT(), static_cast<long>(n));

		if (p == nullptr) {
			throw std::bad_alloc();
		}

		return p;
	}

	void deallocate(T *p, unsigned long n)
	{
		if (p != nullptr) {
			memoire::deloge_tableau(chaine_pour_type<T>::CT(), p, static_cast<long>(n));
		}
	}

	T *address(T &x) const
	{
		return &x;
	}

	T const *address(T const &x) const
	{
		return &x;
	}

	logeuse_guardee<T> &operator=(logeuse_guardee const &)
	{
		return *this;
	}

	void construct(T *p, const T& val)
	{
		if (p != nullptr) {
			new (p) T(val);
		}
	}

	void destroy(T *p)
	{
		p->~T();
	}

	unsigned long max_size() const
	{
		return -1ul;
	}

	template <class U>
	struct rebind {
		typedef logeuse_guardee<U> other;
	};

	template <class U>
	logeuse_guardee(logeuse_guardee<U> const &)
	{}

	template <class U>
	logeuse_guardee& operator=(logeuse_guardee<U> const &)
	{
		return *this;
	}

	inline bool operator==(logeuse_guardee const &) const
	{
		return true;
	}

	inline bool operator!=(logeuse_guardee const &autre) const
	{
		return !operator==(autre);
	}
};

}  /* namespace memoire */
