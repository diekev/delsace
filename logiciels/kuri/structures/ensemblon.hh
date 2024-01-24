/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "ensemble.hh"

namespace kuri {

template <typename T, uint64_t TAILLE_INITIALE>
struct ensemblon {
  private:
    T m_ensemblon[TAILLE_INITIALE];
    kuri::ensemble<T> m_ensemble{};

    int64_t m_taille = 0;

  public:
    ensemblon() = default;

    ensemblon(ensemblon const &autre)
    {
        copie_données(autre);
    }

    ensemblon &operator=(ensemblon const &autre)
    {
        copie_données(autre);
        return *this;
    }

    ensemblon(ensemblon &&autre)
    {
        permute(autre);
    }

    ensemblon &operator=(ensemblon &&autre)
    {
        permute(autre);
        return *this;
    }

    ~ensemblon() = default;

    bool est_stocké_dans_classe() const
    {
        return m_taille <= static_cast<int64_t>(TAILLE_INITIALE);
    }

    void permute(ensemblon &autre)
    {
        if (this->est_stocké_dans_classe() && autre.est_stocké_dans_classe()) {
            for (auto i = 0; i < TAILLE_INITIALE; ++i) {
                std::swap(m_ensemblon[i], autre.m_ensemblon[i]);
            }
        }
        else if (this->est_stocké_dans_classe()) {
            permute_données(*this, autre);
        }
        else if (autre.est_stocké_dans_classe()) {
            permute_données(autre, *this);
        }
        else {
            m_ensemble.permute(autre.m_ensemble);
        }

        std::swap(m_taille, autre.m_taille);
    }

    void copie_données(ensemblon const &autre)
    {
        m_taille = 0;
        m_ensemble.efface();

        if (autre.est_stocké_dans_classe()) {
            for (auto i = 0; i < autre.taille(); ++i) {
                m_ensemblon[i] = autre.m_ensemblon[i];
                m_taille += 1;
            }

            return;
        }

        m_ensemble = autre.m_ensemble;
    }

    void insère(T const &valeur)
    {
        if (possède(valeur)) {
            return;
        }

        if (est_stocké_dans_classe()) {
            if (m_taille + 1 <= static_cast<int64_t>(TAILLE_INITIALE)) {
                m_ensemblon[m_taille] = valeur;
                m_taille += 1;
                return;
            }

            for (auto i = 0; i < taille(); ++i) {
                m_ensemble.insère(m_ensemblon[i]);
            }
        }

        m_ensemble.insère(valeur);
        m_taille += 1;
    }

    bool possède(T const &valeur) const
    {
        if (est_stocké_dans_classe()) {
            for (auto i = 0; i < taille(); ++i) {
                if (m_ensemblon[i] == valeur) {
                    return true;
                }
            }

            return false;
        }

        return m_ensemble.possède(valeur);
    }

    int64_t taille() const
    {
        return m_taille;
    }

    int64_t mémoire_utilisée() const
    {
        if (est_stocké_dans_classe()) {
            return 0;
        }
        return m_ensemble.taille_mémoire();
    }

    T const *données_ensemblon() const
    {
        return m_ensemblon;
    }

    kuri::ensemble<T> const &ensemble() const
    {
        return m_ensemble;
    }

    void efface()
    {
        if (!est_stocké_dans_classe()) {
            m_ensemble.efface();
        }

        m_taille = 0;
    }

  private:
    void permute_données(ensemblon &ensemblon_local, ensemblon &ensemblon_memoire)
    {
        for (auto i = 0; i < TAILLE_INITIALE; ++i) {
            std::swap(ensemblon_local.m_ensemblon[i], ensemblon_memoire.m_ensemblon[i]);
        }

        ensemblon_local.m_ensemble.permute(ensemblon_memoire.m_ensemble);
    }
};

template <typename T, uint64_t TAILLE_INITIALE, typename Rappel>
void pour_chaque_élément(ensemblon<T, TAILLE_INITIALE> const &ens, Rappel rappel)
{
    if (ens.est_stocké_dans_classe()) {
        for (auto i = 0; i < ens.taille(); ++i) {
            auto action = rappel(ens.données_ensemblon()[i]);

            if (action == DécisionItération::Arrête) {
                break;
            }
        }

        return;
    }

    ens.ensemble().pour_chaque_element(rappel);
}

}  // namespace kuri
