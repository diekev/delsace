/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/moultfilage/synchrone.hh"

#include "tableau_statique.hh"

#include "utilitaires/macros.hh"

namespace kuri {

template <typename T, typename TypeIndex = int64_t>
struct tableau {
  private:
    T *pointeur = nullptr;
    TypeIndex taille_ = 0;
    TypeIndex capacite = 0;

  public:
    using iteratrice = T *;
    using iteratrice_const = T const *;

    tableau() = default;

    explicit tableau(TypeIndex taille_initiale)
        : pointeur(memoire::loge_tableau<T>("kuri::tableau", taille_initiale)),
          taille_(taille_initiale), capacite(taille_initiale)
    {
    }

    tableau(std::initializer_list<T> &&liste) : tableau(static_cast<TypeIndex>(liste.size()))
    {
        auto ptr = this->pointeur;
        for (auto &&elem : liste) {
            *ptr++ = elem;
        }
    }

    tableau(tableau const &autre)
    {
        memoire::reloge_tableau("kuri::tableau", this->pointeur, this->capacite, autre.taille_);
        this->taille_ = autre.taille_;
        this->capacite = autre.taille_;

        for (auto i = 0; i < autre.taille_; ++i) {
            this->pointeur[i] = autre.pointeur[i];
        }
    }

    tableau &operator=(tableau const &autre)
    {
        memoire::reloge_tableau("kuri::tableau", this->pointeur, this->capacite, autre.taille_);
        this->taille_ = autre.taille_;
        this->capacite = autre.taille_;

        for (auto i = 0; i < autre.taille_; ++i) {
            this->pointeur[i] = autre.pointeur[i];
        }

        return *this;
    }

    tableau(tableau &&autre)
    {
        permute(autre);
    }

    tableau &operator=(tableau &&autre)
    {
        permute(autre);
        return *this;
    }

    ~tableau()
    {
        for (auto i = 0; i < taille_; ++i) {
            this->pointeur[i].~T();
        }

        memoire::deloge_tableau("kuri::tableau", this->pointeur, this->capacite);
    }

    T &operator[](TypeIndex i)
    {
        assert(i >= 0 && i < this->taille_);
        return this->pointeur[i];
    }

    T const &operator[](TypeIndex i) const
    {
        assert(i >= 0 && i < this->taille_);
        return this->pointeur[i];
    }

    void ajoute(T const &valeur)
    {
        reserve(this->taille_ + 1);
        this->taille_ += 1;
        this->pointeur[this->taille_ - 1] = valeur;
    }

    void ajoute(T &&valeur)
    {
        reserve(this->taille_ + 1);
        this->taille_ += 1;
        this->pointeur[this->taille_ - 1] = std::move(valeur);
    }

    void pousse_front(T const &valeur)
    {
        reserve(this->taille_ + 1);
        this->taille_ += 1;

        for (auto i = this->taille_ - 1; i >= 1; --i) {
            this->pointeur[i] = this->pointeur[i - 1];
        }

        this->pointeur[0] = valeur;
    }

    void supprime_dernier()
    {
        if (this->taille_ == 0) {
            return;
        }

        this->pointeur[this->taille_ - 1] = {};
        this->taille_ -= 1;
    }

    void supprime_premier()
    {
        if (this->taille_ == 0) {
            return;
        }

        for (auto i = 0; i < this->taille_ - 1; i++) {
            this->pointeur[i] = this->pointeur[i + 1];
        }

        this->taille_ -= 1;
    }

    void reserve(TypeIndex nombre)
    {
        if (capacite >= nombre) {
            return;
        }

        if (capacite == 0) {
            if (nombre < 8) {
                nombre = 8;
            }
        }
        else if (nombre < (capacite * 2)) {
            nombre = capacite * 2;
        }

        memoire::reloge_tableau("kuri::tableau", this->pointeur, this->capacite, nombre);
        this->capacite = nombre;
    }

    T *donnees()
    {
        return pointeur;
    }

    T const *donnees() const
    {
        return pointeur;
    }

    TypeIndex taille_memoire() const
    {
        return capacite * static_cast<TypeIndex>(taille_de(T));
    }

    void efface()
    {
        taille_ = 0;
    }

    void redimensionne(TypeIndex nombre)
    {
        reserve(nombre);
        taille_ = nombre;
    }

    void rétrécis_capacité_sur_taille()
    {
        if (capacite == taille_) {
            return;
        }

        memoire::reloge_tableau("kuri::tableau", this->pointeur, this->capacite, taille_);
        this->capacite = taille_;
    }

    void redimensionne(TypeIndex nombre, T valeur_defaut)
    {
        reserve(nombre);

        if (taille_ >= nombre) {
            return;
        }

        for (auto i = taille_; i < nombre; ++i) {
            pointeur[i] = valeur_defaut;
        }

        taille_ = nombre;
    }

    void reserve_delta(TypeIndex delta)
    {
        reserve(taille_ + delta);
    }

    TypeIndex taille() const
    {
        return taille_;
    }

    bool est_vide() const
    {
        return this->taille_ == 0;
    }

    T *debut()
    {
        return this->pointeur;
    }

    T const *debut() const
    {
        return this->pointeur;
    }

    T *fin()
    {
        return this->begin() + this->taille_;
    }

    T const *fin() const
    {
        return this->begin() + this->taille_;
    }

    T *begin()
    {
        return debut();
    }

    T const *begin() const
    {
        return debut();
    }

    T *end()
    {
        return fin();
    }

    T const *end() const
    {
        return fin();
    }

    T &a(TypeIndex index)
    {
        return this->pointeur[index];
    }

    T const &a(TypeIndex index) const
    {
        return this->pointeur[index];
    }

    T &premiere()
    {
        return this->pointeur[0];
    }

    T const &premiere() const
    {
        return this->pointeur[0];
    }

    T &dernière()
    {
        return this->pointeur[this->taille_ - 1];
    }

    T const &dernière() const
    {
        return this->pointeur[this->taille_ - 1];
    }

    void permute(tableau &autre)
    {
        std::swap(pointeur, autre.pointeur);
        std::swap(capacite, autre.capacite);
        std::swap(taille_, autre.taille_);
    }

    void efface(T *debut_, T *fin_)
    {
        auto taille_a_effacer = fin_ - debut_;
        while (debut_ != fin_) {
            *debut_ = {};
            debut_++;
        }
        this->taille_ -= taille_a_effacer;
    }

    operator tableau_statique<T>() const
    {
        return {pointeur, taille()};
    }
};

template <typename T>
using tableau_synchrone = dls::outils::Synchrone<kuri::tableau<T, int>>;

}  // namespace kuri
