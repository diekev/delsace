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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include <cassert>
#include <numeric>
#include <ostream>

namespace dls {

template <typename T>
struct enum_drapeau {
private:
	T drapeaux = 0;

public:
	enum_drapeau() = default;

	enum_drapeau(T v)
		: drapeaux(v)
    {}

    inline constexpr void active(const T bit)
    {
        assert(bit >= 0);
        assert(bit < std::numeric_limits<T>::digits);
		drapeaux |= (1 << bit);
    }

    inline constexpr void desactive(T bit)
    {
        assert(bit >= 0);
        assert(bit < std::numeric_limits<T>::digits);
		drapeaux &= ~(1 << bit);
    }

    inline constexpr bool est_actif(T bit)
    {
        assert(bit >= 0);
        assert(bit < std::numeric_limits<T>::digits);
		return (drapeaux & (1 << bit)) != 0;
    }

    inline constexpr bool est_inactif(T bit)
    {
        return !est_actif(bit);
    }

    inline constexpr void reinit()
    {
		drapeaux = 0;
    }

    T valeur() const
    {
		return drapeaux;
    }

    constexpr operator T()
    {
		return drapeaux;
    }
};

template <typename T>
std::ostream &operator<<(std::ostream &os, enum_drapeau<T> const &ef)
{
    os << ef.valeur();
    return os;
}

template <typename T>
inline constexpr auto operator&(enum_drapeau<T> lhs, enum_drapeau<T> rhs)
{
	return static_cast<enum_drapeau<T>>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

template <typename T, typename V>
inline constexpr auto operator&(enum_drapeau<T> lhs, V rhs)
{
	return static_cast<enum_drapeau<T>>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

template <typename T>
inline constexpr auto operator|(enum_drapeau<T> lhs, enum_drapeau<T> rhs)
{
	return static_cast<enum_drapeau<T>>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

template <typename T, typename V>
inline constexpr auto operator|(enum_drapeau<T> lhs, V rhs)
{
	return static_cast<enum_drapeau<T>>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

template <typename T>
inline constexpr auto operator^(enum_drapeau<T> lhs, enum_drapeau<T> rhs)
{
	return static_cast<enum_drapeau<T>>(static_cast<T>(lhs) ^ static_cast<T>(rhs));
}

template <typename T>
inline constexpr auto operator~(enum_drapeau<T> lhs)
{
	return static_cast<enum_drapeau<T>>(~static_cast<T>(lhs));
}

template <typename T>
inline constexpr auto &operator&=(enum_drapeau<T> &lhs, enum_drapeau<T> rhs)
{
    return (lhs = lhs & rhs);
}

template <typename T, typename V>
inline constexpr auto &operator&=(enum_drapeau<T> &lhs, V rhs)
{
    return (lhs = lhs & rhs);
}

template <typename T>
inline constexpr auto &operator|=(enum_drapeau<T> &lhs, enum_drapeau<T> rhs)
{
    return (lhs = lhs | rhs);
}

template <typename T, typename V>
inline constexpr auto &operator|=(enum_drapeau<T> &lhs, V rhs)
{
    return (lhs = lhs | rhs);
}

template <typename T>
inline constexpr auto &operator^=(enum_drapeau<T> &lhs, enum_drapeau<T> rhs)
{
    return (lhs = lhs ^ rhs);
}

template <typename T, typename V>
inline constexpr auto &operator^=(enum_drapeau<T> &lhs, V rhs)
{
    return (lhs = lhs ^ rhs);
}

}
