/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "coulisse.hh"

#include "structures/tableau.hh"

struct Bibliotheque;

struct CoulisseC final : public Coulisse {
    kuri::tableau<Bibliotheque *> m_bibliotheques{};

    struct FichierC {
        kuri::chaine chemin_fichier{};
        kuri::chaine chemin_fichier_objet{};
    };

    kuri::tableau<FichierC> m_fichiers{};

    bool crée_fichier_objet(Compilatrice &compilatrice,
                            EspaceDeTravail &espace,
                            Programme *programme,
                            ConstructriceRI &constructrice_ri,
                            Broyeuse &) override;

    bool crée_exécutable(Compilatrice &compilatrice,
                         EspaceDeTravail &espace,
                         Programme *programme) override;

    FichierC &ajoute_fichier_c();
};
