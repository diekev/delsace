/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <cstring>

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/structures/vue_chaine_compacte.hh"

#include "chaine_statique.hh"

/**
 * Ces structures sont les mêmes que celles définies par le langage (tableaux
 * via « [..]TYPE », et chaine via « chaine ») ; elles sont donc la même
 * définition que celles du langage. Elles sont utilisées pour pouvoir passer
 * des messages sainement entre la compilatrice et les métaprogrammes. Par
 * sainement, on entend que l'interface binaire de l'application doit être la
 * même.
 */

namespace kuri {

struct chaine {
  private:
    using TypeIndex = int64_t;

    char *pointeur_ = nullptr;
    TypeIndex taille_ = 0;
    TypeIndex capacite_ = 0;

  public:
    chaine() = default;

    chaine(chaine const &autre) : chaine()
    {
        if (this != &autre) {
            réserve(autre.taille());
            taille_ = 0;
            for (auto i = 0; i < autre.taille(); ++i) {
                ajoute_réserve(autre.pointeur()[i]);
            }
        }
    }

    chaine(chaine &&autre)
    {
        permute(autre);
    }

    chaine(const char *c_str, int64_t taille) : chaine()
    {
        réserve(taille);

        for (auto i = 0; i < taille; ++i) {
            ajoute_réserve(c_str[i]);
        }
    }

    template <size_t N>
    chaine(const char (&c)[N]) : chaine(c, static_cast<TypeIndex>(N))
    {
    }

    chaine(const char *c_str) : chaine(c_str, static_cast<int64_t>(std::strlen(c_str)))
    {
    }

    chaine(dls::vue_chaine_compacte const &chn) : chaine(chn.pointeur(), chn.taille())
    {
    }

    chaine(chaine_statique chn) : chaine(chn.pointeur(), chn.taille())
    {
    }

    ~chaine()
    {
        memoire::deloge_tableau("chaine", this->pointeur_, this->capacite_);
    }

    chaine &operator=(chaine const &autre)
    {
        if (this != &autre) {
            réserve(autre.taille());
            taille_ = 0;
            for (auto i = 0; i < autre.taille(); ++i) {
                ajoute_réserve(autre.pointeur()[i]);
            }
        }

        return *this;
    }

    chaine &operator=(chaine &&autre)
    {
        permute(autre);
        return *this;
    }

    char &operator[](TypeIndex i)
    {
        assert(i >= 0 && i < this->taille_);
        return this->pointeur_[i];
    }

    char const &operator[](TypeIndex i) const
    {
        assert(i >= 0 && i < this->taille_);
        return this->pointeur_[i];
    }

    char *begin()
    {
        return this->pointeur_;
    }

    char const *begin() const
    {
        return this->pointeur_;
    }

    char *end()
    {
        return this->begin() + this->taille_;
    }

    char const *end() const
    {
        return this->begin() + this->taille_;
    }

    void ajoute(char c)
    {
        réserve(taille() + 1);
        ajoute_réserve(c);
    }

    void ajoute_réserve(char c)
    {
        this->pointeur_[this->taille_] = c;
        this->taille_ += 1;
    }

    void réserve(TypeIndex nouvelle_taille)
    {
        if (nouvelle_taille <= this->capacite_) {
            return;
        }

        memoire::reloge_tableau("chaine", this->pointeur_, this->capacite_, nouvelle_taille);
        this->capacite_ = nouvelle_taille;
    }

    void redimensionne(TypeIndex nouvelle_taille)
    {
        réserve(nouvelle_taille);
        taille_ = nouvelle_taille;
    }

    void efface()
    {
        taille_ = 0;
    }

    char *pointeur()
    {
        return pointeur_;
    }

    char const *pointeur() const
    {
        return pointeur_;
    }

    TypeIndex taille() const
    {
        return taille_;
    }

    TypeIndex capacite() const
    {
        return capacite_;
    }

    chaine_statique sous_chaine(TypeIndex index) const
    {
        return chaine_statique(this->pointeur() + index, this->taille() - index);
    }

    operator dls::vue_chaine_compacte() const
    {
        return {pointeur(), taille()};
    }

    operator chaine_statique() const
    {
        return {pointeur(), taille()};
    }

    void permute(chaine &autre)
    {
        std::swap(pointeur_, autre.pointeur_);
        std::swap(taille_, autre.taille_);
        std::swap(capacite_, autre.capacite_);
    }

    explicit operator bool() const
    {
        return taille() != 0;
    }
};

bool operator==(chaine const &chn1, chaine const &chn2);

bool operator==(chaine const &chn1, chaine_statique const &chn2);

bool operator==(chaine_statique const &chn1, chaine const &chn2);

bool operator==(chaine const &chn1, const char *chn2);

bool operator==(const char *chn1, chaine const &chn2);

bool operator!=(kuri::chaine const &chn1, chaine const &chn2);

bool operator!=(chaine const &chn1, chaine_statique const &chn2);

bool operator!=(chaine_statique const &chn1, chaine const &chn2);

bool operator!=(chaine const &chn1, const char *chn2);

bool operator!=(const char *chn1, chaine const &chn2);

std::ostream &operator<<(std::ostream &os, chaine const &chn);

int64_t distance_levenshtein(chaine_statique const &chn1, chaine_statique const &chn2);

}  // namespace kuri

namespace std {

template <>
struct hash<kuri::chaine> {
    std::size_t operator()(kuri::chaine const &chn) const
    {
        auto h = std::hash<std::string_view>{};
        return h(std::string_view(chn.pointeur(), static_cast<size_t>(chn.taille())));
    }
};

} /* namespace std */

/*

struct Allocatrice;

struct Allocatrice {
private:
    int64_t m_nombre_allocations = 0;
    int64_t m_taille_allouee = 0;

public:
    template <typename T, typename... Args>
    T *loge(const char *ident, Args &&... args)
    {
        m_nombre_allocations += 1;
        m_taille_allouee += static_cast<int64_t>(sizeof(T));
        return new T(args...);
    }

    template <typename T>
    void deloge(T *&ptr)
    {
        delete ptr;
        ptr = nullptr;
        m_taille_allouee -= static_cast<int64_t>(sizeof(T));
    }

    template <typename T>
    T *loge_tableau(const char *ident, int64_t elements)
    {
        m_nombre_allocations += 1;
        m_taille_allouee += static_cast<int64_t>(sizeof(T)) * elements;
        return new T[elements];
    }

    template <typename T>
    void reloge_tableau(const char *ident, T *&ptr, int64_t ancienne_taille, int64_t
nouvelle_taille);

    template <typename T>
    void deloge_tableau(T *&ptr, int64_t taille);

    int64_t nombre_allocation() const
    {
        return m_nombre_allocations;
    }

    int64_t taille_allouee() const
    {
        return m_taille_allouee;
    }
};

*/
