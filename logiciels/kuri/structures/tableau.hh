/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "tableau_statique.hh"

#include "utilitaires/logeuse_memoire.hh"
#include "utilitaires/macros.hh"
#include "utilitaires/synchrone.hh"

namespace kuri {

template <typename T, typename TypeIndex = int64_t>
struct tableau {
  private:
    T *m_éléments = nullptr;
    TypeIndex m_taille = 0;
    TypeIndex m_capacité = 0;

  public:
    using iteratrice = T *;
    using iteratrice_const = T const *;

    tableau() = default;

    explicit tableau(TypeIndex taille_initiale)
        : m_éléments(mémoire::loge_tableau<T>("kuri::tableau", taille_initiale)),
          m_taille(taille_initiale), m_capacité(taille_initiale)
    {
    }

    tableau(std::initializer_list<T> &&liste) : tableau(static_cast<TypeIndex>(liste.size()))
    {
        auto ptr = this->m_éléments;
        for (auto &&élément : liste) {
            *ptr++ = élément;
        }
    }

    tableau(tableau const &autre)
    {
        copie_données(autre.m_éléments, autre.m_taille);
    }

    tableau &operator=(tableau_statique<T> autre)
    {
        copie_données(autre.begin(), autre.taille());
        return *this;
    }

    tableau &operator=(tableau const &autre)
    {
        copie_données(autre.m_éléments, autre.m_taille);
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
        for (auto i = 0; i < m_taille; ++i) {
            this->m_éléments[i].~T();
        }

        mémoire::deloge_tableau("kuri::tableau", this->m_éléments, this->m_capacité);
    }

    T &operator[](TypeIndex i)
    {
        assert(i >= 0 && i < this->m_taille);
        return this->m_éléments[i];
    }

    T const &operator[](TypeIndex i) const
    {
        assert(i >= 0 && i < this->m_taille);
        return this->m_éléments[i];
    }

    void ajoute(T const &valeur)
    {
        agrandis(this->m_taille + 1);
        this->m_taille += 1;
        this->m_éléments[this->m_taille - 1] = valeur;
    }

    void ajoute(T &&valeur)
    {
        agrandis(this->m_taille + 1);
        this->m_taille += 1;
        this->m_éléments[this->m_taille - 1] = std::move(valeur);
    }

    void ajoute_au_début(T const &valeur)
    {
        agrandis(this->m_taille + 1);
        this->m_taille += 1;

        for (auto i = this->m_taille - 1; i >= 1; --i) {
            this->m_éléments[i] = this->m_éléments[i - 1];
        }

        this->m_éléments[0] = valeur;
    }

    void supprime_dernier()
    {
        if (this->m_taille == 0) {
            return;
        }

        this->m_éléments[this->m_taille - 1] = {};
        this->m_taille -= 1;
    }

    void supprime_premier()
    {
        if (this->m_taille == 0) {
            return;
        }

        for (auto i = 0; i < this->m_taille - 1; i++) {
            this->m_éléments[i] = this->m_éléments[i + 1];
        }

        this->m_taille -= 1;
    }

    void réserve(TypeIndex nombre)
    {
        if (m_capacité >= nombre) {
            return;
        }

        mémoire::reloge_tableau("kuri::tableau", this->m_éléments, this->m_capacité, nombre);
        this->m_capacité = nombre;
    }

    void remplace_données_par(kuri::tableau_statique<T> autre)
    {
        this->efface();
        this->réserve(TypeIndex(autre.taille()));
        POUR (autre) {
            this->ajoute(it);
        }
    }

  private:
    void agrandis(TypeIndex nombre)
    {
        if (m_capacité >= nombre) {
            return;
        }

        if (m_capacité == 0) {
            if (nombre < 8) {
                nombre = 8;
            }
        }
        else {
            /* Facteur d'agrandissement de 1.5.
             * Voir https://github.com/facebook/folly/blob/main/folly/docs/FBVector.md. */
            auto const mi_capacité = m_capacité >> 1;
            if (nombre < (m_capacité + mi_capacité)) {
                nombre = m_capacité + mi_capacité;
            }
        }

        mémoire::reloge_tableau("kuri::tableau", this->m_éléments, this->m_capacité, nombre);
        this->m_capacité = nombre;
    }

    void copie_données(T *données, TypeIndex taille)
    {
        mémoire::reloge_tableau("kuri::tableau", this->m_éléments, this->m_capacité, taille);
        this->m_taille = taille;
        this->m_capacité = taille;

        for (auto i = 0; i < taille; ++i) {
            this->m_éléments[i] = données[i];
        }
    }

  public:
    T *données()
    {
        return m_éléments;
    }

    T const *données() const
    {
        return m_éléments;
    }

    TypeIndex taille_mémoire() const
    {
        return m_capacité * static_cast<TypeIndex>(taille_de(T));
    }

    TypeIndex gaspillage_mémoire() const
    {
        return (m_capacité - m_taille) * static_cast<TypeIndex>(taille_de(T));
    }

    void efface()
    {
        m_taille = 0;
    }

    void redimensionne(TypeIndex nombre)
    {
        réserve(nombre);
        m_taille = nombre;
    }

    void rétrécis_capacité_sur_taille()
    {
        if (m_capacité == m_taille) {
            return;
        }

        mémoire::reloge_tableau("kuri::tableau", this->m_éléments, this->m_capacité, m_taille);
        this->m_capacité = m_taille;
    }

    void redimensionne(TypeIndex nombre, T valeur_defaut)
    {
        réserve(nombre);

        if (m_taille >= nombre) {
            return;
        }

        for (auto i = m_taille; i < nombre; ++i) {
            m_éléments[i] = valeur_defaut;
        }

        m_taille = nombre;
    }

    void réserve_delta(TypeIndex delta)
    {
        réserve(m_taille + delta);
    }

    TypeIndex taille() const
    {
        return m_taille;
    }

    TypeIndex capacité() const
    {
        return m_capacité;
    }

    bool est_vide() const
    {
        return this->m_taille == 0;
    }

    T *debut()
    {
        return this->m_éléments;
    }

    T const *debut() const
    {
        return this->m_éléments;
    }

    T *fin()
    {
        return this->begin() + this->m_taille;
    }

    T const *fin() const
    {
        return this->begin() + this->m_taille;
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
        return this->m_éléments[index];
    }

    T const &a(TypeIndex index) const
    {
        return this->m_éléments[index];
    }

    T &premier_élément()
    {
        return this->m_éléments[0];
    }

    T const &premier_élément() const
    {
        return this->m_éléments[0];
    }

    T &dernier_élément()
    {
        return this->m_éléments[this->m_taille - 1];
    }

    T const &dernier_élément() const
    {
        return this->m_éléments[this->m_taille - 1];
    }

    void permute(tableau &autre)
    {
        std::swap(m_éléments, autre.m_éléments);
        std::swap(m_capacité, autre.m_capacité);
        std::swap(m_taille, autre.m_taille);
    }

    void efface(T *debut_, T *fin_)
    {
        auto taille_à_effacer = fin_ - debut_;
        while (debut_ != fin_) {
            *debut_ = {};
            debut_++;
        }
        this->m_taille -= taille_à_effacer;
    }

    operator tableau_statique<T>() const
    {
        return {m_éléments, taille()};
    }
};

template <typename T>
using tableau_synchrone = kuri::Synchrone<kuri::tableau<T, int>>;

}  // namespace kuri
