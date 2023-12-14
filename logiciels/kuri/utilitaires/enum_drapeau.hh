/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include <type_traits>

namespace kuri {

template <typename T>
class énum_drapeau {
  private:
    T m_valeur{};

  public:
    using type_sous_jacent = std::underlying_type_t<T>;
    using enum_type = T;

    énum_drapeau() = default;

    énum_drapeau(T valeur) : m_valeur(valeur)
    {
    }

    explicit operator T &() noexcept
    {
        return m_valeur;
    }

    explicit operator const T &() const noexcept
    {
        return m_valeur;
    }

    explicit operator type_sous_jacent &() noexcept
    {
        return static_cast<type_sous_jacent>(m_valeur);
    }

    explicit operator const type_sous_jacent &() const noexcept
    {
        return static_cast<type_sous_jacent>(m_valeur);
    }
};

template <typename T>
auto operator&(énum_drapeau<T> lhs, énum_drapeau<T> rhs)
{
    using type_sous_jacent = typename énum_drapeau<T>::type_sous_jacent;
    return énum_drapeau<T>(T(type_sous_jacent(lhs) & type_sous_jacent(rhs)));
}

template <typename T>
auto operator|(énum_drapeau<T> lhs, énum_drapeau<T> rhs)
{
    using type_sous_jacent = typename énum_drapeau<T>::type_sous_jacent;
    return énum_drapeau<T>(T(type_sous_jacent(lhs) | type_sous_jacent(rhs)));
}

template <typename T>
auto operator^(énum_drapeau<T> lhs, énum_drapeau<T> rhs)
{
    using type_sous_jacent = typename énum_drapeau<T>::type_sous_jacent;
    return énum_drapeau<T>(T(type_sous_jacent(lhs) ^ type_sous_jacent(rhs)));
}

template <typename T>
auto operator~(énum_drapeau<T> lhs)
{
    using type_sous_jacent = typename énum_drapeau<T>::type_sous_jacent;
    return énum_drapeau<T>(T(~type_sous_jacent(lhs)));
}

template <typename T>
auto &operator&=(énum_drapeau<T> &lhs, énum_drapeau<T> rhs)
{
    return (lhs = lhs & rhs);
}

template <typename T>
auto &operator|=(énum_drapeau<T> &lhs, énum_drapeau<T> rhs)
{
    return (lhs = lhs | rhs);
}

template <typename T>
auto &operator^=(énum_drapeau<T> &lhs, énum_drapeau<T> rhs)
{
    return (lhs = lhs ^ rhs);
}

}  // namespace kuri
