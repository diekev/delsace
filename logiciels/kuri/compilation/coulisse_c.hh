/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "coulisse.hh"

#include "structures/tableau.hh"

struct Bibliotheque;

struct CoulisseC final : public Coulisse {
    kuri::tableau<Bibliotheque *> m_bibliothèques{};

    struct FichierC {
        kuri::chaine chemin_fichier{};
        kuri::chaine chemin_fichier_objet{};
    };

    kuri::tableau<FichierC> m_fichiers{};

    FichierC &ajoute_fichier_c();

  private:
    bool génère_code_impl(Compilatrice &compilatrice,
                          EspaceDeTravail &espace,
                          Programme *programme,
                          CompilatriceRI &compilatrice_ri,
                          Broyeuse &) override;

    bool crée_fichier_objet_impl(Compilatrice &compilatrice,
                                 EspaceDeTravail &espace,
                                 Programme *programme,
                                 CompilatriceRI &compilatrice_ri) override;

    bool crée_exécutable_impl(Compilatrice &compilatrice,
                              EspaceDeTravail &espace,
                              Programme *programme) override;
};
