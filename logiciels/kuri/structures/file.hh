/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "tableau_page.hh"

namespace kuri {

template <typename T>
struct file {
    using type_valeur = T;
    using type_référence = T &;
    using type_référence_const = T const &;
    using type_taille = int64_t;

  private:
    struct noeud {
        noeud *suivant = nullptr;
        noeud *précédent = nullptr;

        T données{};
    };

    noeud *premier = nullptr;
    noeud *dernier = nullptr;
    int64_t m_taille = 0;

    noeud *liste_libre = nullptr;

    tableau_page<noeud> m_noeuds{};

  public:
    file() = default;

    EMPECHE_COPIE(file);

    bool est_vide() const
    {
        return premier == nullptr;
    }

    type_taille taille() const
    {
        return m_taille;
    }

    type_référence front()
    {
        return premier->données;
    }

    type_référence_const front() const
    {
        return premier->données;
    }

    void enfile(type_référence_const valeur)
    {
        auto élément = donne_noeud_libre();
        élément->données = valeur;

        if (premier == nullptr) {
            assert(dernier == nullptr);
            premier = élément;
        }
        else {
            assert(dernier->suivant == nullptr);
            dernier->suivant = élément;
            élément->précédent = dernier;
        }

        dernier = élément;
        m_taille += 1;
    }

    void enfile(tableau<type_valeur> const &valeurs)
    {
        for (auto const &valeur : valeurs) {
            enfile(valeur);
        }
    }

    void efface()
    {
        if (dernier) {
            assert(premier != nullptr);
            dernier->suivant = liste_libre;
            liste_libre = premier;
        }
        else {
            assert(premier == nullptr);
        }

        premier = nullptr;
        dernier = nullptr;
        m_taille = 0;
    }

    template <typename Predicat>
    void efface_si(Predicat &&predicat)
    {
        auto élément = premier;

        while (élément != nullptr) {
            auto élément_suivant = élément->suivant;
            if (predicat(élément->données)) {
                if (élément->précédent) {
                    élément->précédent->suivant = élément_suivant;
                }
                if (élément->suivant) {
                    élément->suivant->précédent = élément->précédent;
                }

                if (élément == premier || élément == dernier) {
                    if (premier == dernier) {
                        premier = élément_suivant;
                        dernier = élément_suivant;
                    }
                    else if (élément == premier) {
                        premier = élément_suivant;
                    }
                    else if (élément == dernier) {
                        dernier = élément->précédent;
                    }
                }

                élément->données.~T();
                détruit_noeud(élément);

                m_taille -= 1;
                assert(m_taille >= 0);
            }

            élément = élément_suivant;
        }
    }

    type_valeur défile()
    {
        auto t = front();
        auto suivant = premier->suivant;
        if (suivant) {
            suivant->précédent = nullptr;
        }
        détruit_noeud(premier);
        premier = suivant;
        if (premier == nullptr) {
            dernier = nullptr;
        }
        m_taille -= 1;
        assert(m_taille >= 0);
        return t;
    }

    tableau<type_valeur> défile(int64_t compte)
    {
        auto ret = tableau<type_valeur>(compte);

        for (auto i = 0; i < compte; ++i) {
            ret.ajoute(défile());
        }

        return ret;
    }

    struct itératrice {
        typedef std::forward_iterator_tag iterator_category;
        typedef type_valeur value_type;
        typedef long int difference_type;
        typedef type_valeur *pointer;
        typedef type_valeur &reference;

        noeud *élément = nullptr;

        bool operator==(itératrice autre)
        {
            return élément == autre.élément;
        }

        bool operator!=(itératrice autre)
        {
            return élément != autre.élément;
        }

        itératrice &operator++()
        {
            élément = élément->suivant;
            return *this;
        }

        type_valeur &operator*()
        {
            return élément->données;
        }
    };

    struct itératrice_const {
        typedef std::forward_iterator_tag iterator_category;
        typedef const type_valeur value_type;
        typedef long int difference_type;
        typedef const type_valeur *pointer;
        typedef const type_valeur &reference;

        noeud *élément = nullptr;

        bool operator==(itératrice_const autre)
        {
            return élément == autre.élément;
        }

        bool operator!=(itératrice_const autre)
        {
            return élément != autre.élément;
        }

        itératrice_const &operator++()
        {
            élément = élément->suivant;
            return *this;
        }

        const type_valeur &operator*()
        {
            return élément->données;
        }
    };

    itératrice begin()
    {
        return itératrice{premier};
    }

    itératrice end()
    {
        return itératrice{nullptr};
    }

    itératrice_const begin() const
    {
        return itératrice_const{premier};
    }

    itératrice_const end() const
    {
        return itératrice_const{nullptr};
    }

    int64_t taille_mémoire() const
    {
        return m_noeuds.mémoire_utilisée();
    }

  private:
    noeud *donne_noeud_libre()
    {
        if (liste_libre) {
            auto résultat = liste_libre;
            liste_libre = résultat->suivant;
            new (résultat) noeud;
            return résultat;
        }

        return m_noeuds.ajoute_élément();
    }

    void détruit_noeud(noeud *élément)
    {
        élément->suivant = liste_libre;
        élément->précédent = nullptr;
        liste_libre = élément;
    }
};

} /* namespace kuri */
