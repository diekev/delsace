/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#pragma once

#include "biblinternes/moultfilage/synchrone.hh"

#include "tableau.hh"

namespace kuri {

/**
 * Tableau dont les éléments sont allouées dans des pages fixes de données.
 * Ces pages nous permettent d'avoir un adressage stable : ajouter un élément
 * dans le tableau n'invalidera pas les adresses existantes stockées ailleurs.
 */
template <typename T, size_t TAILLE_PAGE = 128>
struct tableau_page {
  private:
    struct page {
        T *données = nullptr;
        int64_t occupe = 0;

        T *begin()
        {
            return données;
        }

        T const *begin() const
        {
            return données;
        }

        T *end()
        {
            return données + occupe;
        }

        T const *end() const
        {
            return données + occupe;
        }
    };

    tableau<page> m_pages{};
    page *m_page_courante = nullptr;
    int64_t m_nombre_éléments = 0;

  public:
    tableau_page() = default;

    tableau_page(tableau_page const &) = delete;
    tableau_page &operator=(tableau_page const &) = delete;

    tableau_page(tableau_page &&) = default;
    tableau_page &operator=(tableau_page &&) = default;

    ~tableau_page()
    {
        for (auto &it : m_pages) {
            for (auto i = 0; i < it.occupe; ++i) {
                it.données[i].~T();
            }

            memoire::deloge_tableau("page", it.données, TAILLE_PAGE);
        }
    }

    template <typename... Args>
    T *ajoute_élément(Args &&...args)
    {
        if (!m_page_courante || m_page_courante->occupe == TAILLE_PAGE) {
            ajoute_page();
        }

        auto ptr = &m_page_courante->données[m_page_courante->occupe];
        m_page_courante->occupe += 1;
        m_nombre_éléments += 1;

        new (ptr) T(std::move(args)...);

        return ptr;
    }

    int64_t taille() const
    {
        return m_nombre_éléments;
    }

    int64_t mémoire_utilisée() const
    {
        return m_pages.taille() * static_cast<int64_t>(TAILLE_PAGE * sizeof(T) + sizeof(page));
    }

    int64_t gaspillage_mémoire() const
    {
        return mémoire_utilisée() - (m_nombre_éléments * int64_t(sizeof(T))) -
               m_pages.taille() * int64_t(sizeof(page));
    }

    T &a_l_index(int64_t index)
    {
        return this->operator[](index);
    }

    T const &a_l_index(int64_t index) const
    {
        return this->operator[](index);
    }

    T &operator[](int64_t i)
    {
        assert(i >= 0);
        assert(i < taille());

        auto idx_page = i / static_cast<int64_t>(TAILLE_PAGE);
        auto idx_elem = i % static_cast<int64_t>(TAILLE_PAGE);

        return m_pages[idx_page].données[idx_elem];
    }

    T const &operator[](int64_t i) const
    {
        assert(i >= 0);
        assert(i < taille());

        auto idx_page = i / static_cast<int64_t>(TAILLE_PAGE);
        auto idx_elem = i % static_cast<int64_t>(TAILLE_PAGE);

        return m_pages[idx_page].données[idx_elem];
    }

    page *begin()
    {
        return m_pages.begin();
    }

    page const *begin() const
    {
        return m_pages.begin();
    }

    page *end()
    {
        return m_pages.end();
    }

    page const *end() const
    {
        return m_pages.end();
    }

  private:
    void ajoute_page()
    {
        auto p = page();
        p.données = memoire::loge_tableau<T>("page", TAILLE_PAGE);
        m_pages.ajoute(p);

        m_page_courante = &m_pages.dernier_élément();
    }
};

template <typename T>
using tableau_page_synchrone = dls::outils::Synchrone<tableau_page<T>>;

#define POUR_TABLEAU_PAGE(x)                                                                      \
    for (auto &p : x)                                                                             \
        for (auto &it : p)

#define POUR_TABLEAU_PAGE_NOMME(nom_iter, x)                                                      \
    for (auto &p##nom_iter : x)                                                                   \
        for (auto &nom_iter : p##nom_iter)

template <typename T, size_t TAILLE_PAGE, typename Rappel>
void pour_chaque_élément(tableau_page<T, TAILLE_PAGE> const &tableau, Rappel rappel)
{
    POUR_TABLEAU_PAGE (tableau) {
        rappel(it);
    }
}

}  // namespace kuri
