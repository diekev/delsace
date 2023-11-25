/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include <type_traits>

#define CONCATENE_IMPL(s1, s2) s1##s2
#define CONCATENE(s1, s2) CONCATENE_IMPL(s1, s2)

#ifndef __COUNTER__
#    define VARIABLE_ANONYME(str) CONCATENE(str, __COUNTER__)
#else
#    define VARIABLE_ANONYME(str) CONCATENE(str, __LINE__)
#endif

#define TOUJOURS_ENLIGNE [[gnu::always_inline]]
#define TOUJOURS_HORSLIGNE [[gnu::noinline]]

#define INUTILISE(x) static_cast<void>(x)

#define CHAINE_IMPL(x) #x
#define CHAINE(x) CHAINE_IMPL(x)

#define PRAGMA_IMPL(x) _Pragma(#x)
#define A_FAIRE(x) PRAGMA_IMPL(message("À FAIRE : " CHAINE(x)))

#define PROBABLE(x) (__builtin_expect((x), 1))
#define IMPROBABLE(x) (__builtin_expect((x), 0))

/* clang-format off */
#if defined(__clang__) || defined(__GNUC__)
#    define REMBOURRE(x) \
        _Pragma("clang diagnostic push") \
        _Pragma("clang diagnostic ignored \"-Wunused-private-field\"")  \
        char VARIABLE_ANONYME(_pad)[x] \
        _Pragma("clang diagnostic pop")
#else
#    define REMBOURRE(x)
#endif
/* clang-format on */

#define COPIE_CONSTRUCT(x)                                                                        \
    x(x const &) = default;                                                                       \
    x &operator=(x const &) = default

#define EMPECHE_COPIE(x)                                                                          \
    x(x const &) = delete;                                                                        \
    x &operator=(x const &) = delete

#define COPIE_CONSTRUCT_MOUV(x)                                                                   \
    x(x &&) = default;                                                                            \
    x &operator=(x &&) = default

#define DEFINIS_OPERATEURS_DRAPEAU_IMPL(_type_drapeau_, _type_)                                   \
    inline constexpr auto operator&(_type_drapeau_ lhs, _type_drapeau_ rhs)                       \
    {                                                                                             \
        return static_cast<_type_drapeau_>(static_cast<_type_>(lhs) & static_cast<_type_>(rhs));  \
    }                                                                                             \
    inline constexpr auto operator&(_type_drapeau_ lhs, _type_ rhs)                               \
    {                                                                                             \
        return static_cast<_type_drapeau_>(static_cast<_type_>(lhs) & rhs);                       \
    }                                                                                             \
    inline constexpr auto operator|(_type_drapeau_ lhs, _type_drapeau_ rhs)                       \
    {                                                                                             \
        return static_cast<_type_drapeau_>(static_cast<_type_>(lhs) | static_cast<_type_>(rhs));  \
    }                                                                                             \
    inline constexpr auto operator^(_type_drapeau_ lhs, _type_drapeau_ rhs)                       \
    {                                                                                             \
        return static_cast<_type_drapeau_>(static_cast<_type_>(lhs) ^ static_cast<_type_>(rhs));  \
    }                                                                                             \
    inline constexpr auto operator~(_type_drapeau_ lhs)                                           \
    {                                                                                             \
        return static_cast<_type_drapeau_>(~static_cast<_type_>(lhs));                            \
    }                                                                                             \
    inline constexpr auto &operator&=(_type_drapeau_ &lhs, _type_drapeau_ rhs)                    \
    {                                                                                             \
        return (lhs = lhs & rhs);                                                                 \
    }                                                                                             \
    inline constexpr auto &operator|=(_type_drapeau_ &lhs, _type_drapeau_ rhs)                    \
    {                                                                                             \
        return (lhs = lhs | rhs);                                                                 \
    }                                                                                             \
    inline constexpr auto &operator^=(_type_drapeau_ &lhs, _type_drapeau_ rhs)                    \
    {                                                                                             \
        return (lhs = lhs ^ rhs);                                                                 \
    }

#define DEFINIS_OPERATEURS_DRAPEAU(_type_drapeau_)                                                \
    DEFINIS_OPERATEURS_DRAPEAU_IMPL(_type_drapeau_, std::underlying_type_t<_type_drapeau_>)

#define taille_de(x) static_cast<int64_t>(sizeof(x))

#define POINTEUR_NUL(Type)                                                                        \
    static inline Type *nul()                                                                     \
    {                                                                                             \
        return static_cast<Type *>(nullptr);                                                      \
    }                                                                                             \
    static inline Type const *nul_const()                                                         \
    {                                                                                             \
        return static_cast<Type const *>(nullptr);                                                \
    }

#define POUR(x) for (auto &it : (x))

#define POUR_NOMME(nom, x) for (auto &nom : (x))

#define POUR_INDEX(variable)                                                                      \
    if (auto index_it = -1)                                                                       \
        for (auto &it : (variable))                                                               \
            if (++index_it, true)
