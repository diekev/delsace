/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "ensemble.hh"

namespace kuri {

template <typename T, unsigned long TAILLE_INITIALE>
struct ensemblon {
  private:
    T m_ensemblon[TAILLE_INITIALE];
    kuri::ensemble<T> m_ensemble{};

    long m_taille = 0;

  public:
    ensemblon() = default;

    ensemblon(ensemblon const &autre)
    {
        copie_donnees(autre);
    }

    ensemblon &operator=(ensemblon const &autre)
    {
        copie_donnees(autre);
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

    bool est_stocke_dans_classe() const
    {
        return m_taille <= static_cast<long>(TAILLE_INITIALE);
    }

    void permute(ensemblon &autre)
    {
        if (this->est_stocke_dans_classe() && autre.est_stocke_dans_classe()) {
            for (auto i = 0; i < TAILLE_INITIALE; ++i) {
                std::swap(m_ensemblon[i], autre.m_ensemblon[i]);
            }
        }
        else if (this->est_stocke_dans_classe()) {
            permute_donnees(*this, autre);
        }
        else if (autre.est_stocke_dans_classe()) {
            permute_donnees(autre, *this);
        }
        else {
            m_ensemble.permute(autre.m_ensemble);
        }

        std::swap(m_taille, autre.m_taille);
    }

    void copie_donnees(ensemblon const &autre)
    {
        m_taille = 0;
        m_ensemble.efface();

        if (autre.est_stocke_dans_classe()) {
            for (auto i = 0; i < autre.taille(); ++i) {
                m_ensemblon[i] = autre.m_ensemblon[i];
                m_taille += 1;
            }

            return;
        }

        m_ensemble = autre.m_ensemble;
    }

    void insere(T const &valeur)
    {
        if (possede(valeur)) {
            return;
        }

        if (est_stocke_dans_classe()) {
            if (m_taille + 1 <= static_cast<long>(TAILLE_INITIALE)) {
                m_ensemblon[m_taille] = valeur;
                m_taille += 1;
                return;
            }

            for (auto i = 0; i < taille(); ++i) {
                m_ensemble.insere(m_ensemblon[i]);
            }
        }

        m_ensemble.insere(valeur);
        m_taille += 1;
    }

    bool possede(T const &valeur) const
    {
        if (est_stocke_dans_classe()) {
            for (auto i = 0; i < taille(); ++i) {
                if (m_ensemblon[i] == valeur) {
                    return true;
                }
            }

            return false;
        }

        return m_ensemble.possede(valeur);
    }

    long taille() const
    {
        return m_taille;
    }

    T const *donnees_ensemblon() const
    {
        return m_ensemblon;
    }

    kuri::ensemble<T> const &ensemble() const
    {
        return m_ensemble;
    }

    void efface()
    {
        if (!est_stocke_dans_classe()) {
            m_ensemble.efface();
        }

        m_taille = 0;
    }

  private:
    void permute_donnees(ensemblon &ensemblon_local, ensemblon &ensemblon_memoire)
    {
        for (auto i = 0; i < TAILLE_INITIALE; ++i) {
            std::swap(ensemblon_local.m_ensemblon[i], ensemblon_memoire.m_ensemblon[i]);
        }

        ensemblon_local.m_ensemble.permute(ensemblon_memoire.m_ensemble);
    }
};

template <typename T, unsigned long TAILLE_INITIALE, typename Rappel>
void pour_chaque_element(ensemblon<T, TAILLE_INITIALE> const &ens, Rappel rappel)
{
    if (ens.est_stocke_dans_classe()) {
        for (auto i = 0; i < ens.taille(); ++i) {
            auto action = rappel(ens.donnees_ensemblon()[i]);

            if (action == DecisionIteration::Arrete) {
                break;
            }
        }

        return;
    }

    ens.ensemble().pour_chaque_element(rappel);
}

}  // namespace kuri
