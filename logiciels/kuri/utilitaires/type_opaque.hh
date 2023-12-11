/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include <iosfwd>
#include <utility>

namespace kuri {

/**
 * La classe type_opaque sert à créer des types distincts depuis un autre type.
 *
 * Cette classe ne donne que la construction d'un type opaque depuis un type
 * opacifiée ; la construction se faisant de manière explicite :
 *
 *    auto valeur_opaque = mon_type_opaque(valeur_opacifiée);
 *
 * Les opérateurs devront être surchargés au cas par cas.
 *
 * Pour créer un type opaque, on peut utiliser :
 *
 * struct nom_du_type : public type_opaque<nom_du_type, type_opacifié> {
 *      // Requis pour avoir accès aux constructeurs.
 *      using type_opaque::type_opaque;
 * };
 *
 * Le macro CREE_TYPE_OPAQUE est disponible pour faire de même (le point virgule
 * à la fin est requis) :
 *
 * CREE_TYPE_OPAQUE(nom_du_type, type_opacifié);
 */
template <typename Tag, class T>
class type_opaque {
    T m_valeur{};

  public:
    type_opaque() = default;

    explicit type_opaque(T const &valeur) : m_valeur(valeur)
    {
    }

    explicit type_opaque(T &&valeur) noexcept(std::is_nothrow_move_constructible<T>::value)
        : m_valeur(std::move(valeur))
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

    friend void swap(type_opaque &a, type_opaque &b) noexcept
    {
        using std::swap;
        swap(static_cast<T &>(a), static_cast<T &>(b));
    }
};

template <typename Tag, class T>
inline bool operator==(type_opaque<Tag, T> a, type_opaque<Tag, T> b)
{
    return static_cast<T>(a) == static_cast<T>(b);
}

template <typename Tag, class T>
inline bool operator!=(type_opaque<Tag, T> a, type_opaque<Tag, T> b)
{
    return !(a == b);
}

template <typename Tag, class T>
std::ostream &operator<<(std::ostream &os, type_opaque<Tag, T> const &v)
{
    return os << static_cast<T const &>(v);
}

#define CREE_TYPE_OPAQUE(nom_type, type_opacifie)                                                 \
    struct nom_type : public kuri::type_opaque<nom_type, type_opacifie> {                         \
        using type_opaque::type_opaque;                                                           \
    }

}  // namespace kuri
