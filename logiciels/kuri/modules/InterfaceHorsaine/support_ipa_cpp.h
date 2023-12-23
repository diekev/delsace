
#pragma once

#include <cstring>
#include <string>
#include <type_traits>

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
    }                                                                                             \
    inline bool drapeau_est_actif(_type_drapeau_ const v, _type_drapeau_ const d)                 \
    {                                                                                             \
        return (d & v) != _type_drapeau_(0);                                                      \
    }

#define DEFINIS_OPERATEURS_DRAPEAU(_type_drapeau_)                                                \
    DEFINIS_OPERATEURS_DRAPEAU_IMPL(_type_drapeau_, std::underlying_type_t<_type_drapeau_>)

namespace ESPACE_DE_NOM {

template <typename TypeC, typename TypeCPP>
class iteratrice_tableau
    : public std::
          iterator<std::random_access_iterator_tag, TypeCPP, long, const TypeCPP *, TypeCPP> {
    TypeC *m_ptr = nullptr;

  public:
    explicit iteratrice_tableau(TypeC *ptr_ = nullptr) : m_ptr(ptr_)
    {
    }

    iteratrice_tableau &operator++()
    {
        ++m_ptr;
        return *this;
    }

    iteratrice_tableau operator++(int)
    {
        auto retval = *this;
        ++(*this);
        return retval;
    }

    bool operator==(iteratrice_tableau other) const
    {
        return m_ptr == other.m_ptr;
    }
    bool operator!=(iteratrice_tableau other) const
    {
        return !(*this == other);
    }
    typename iteratrice_tableau::reference operator*() const
    {
        return TypeCPP(*m_ptr);
    }
};

template <typename TypeC, typename TypeCPP>
class tableau {
    TypeC *m_donnees = nullptr;
    long m_taille = 0;

  public:
    using iteratrice = iteratrice_tableau<TypeC, TypeCPP>;

    tableau(TypeC *donnees_, long taille_) : m_donnees(donnees_), m_taille(taille_)
    {
    }

    TypeC *données_crues()
    {
        return m_donnees;
    }
    const TypeC *données_crues() const
    {
        return m_donnees;
    }
    long taille() const
    {
        return m_taille;
    }

    bool est_vide() const
    {
        return taille() == 0;
    }

    TypeCPP operator[](size_t i)
    {
        return TypeCPP(m_donnees[i]);
    }

    iteratrice begin()
    {
        return iteratrice(m_donnees);
    }
    iteratrice end()
    {
        return iteratrice(m_donnees + m_taille);
    }
};

}  // namespace ESPACE_DE_NOM
