/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "coulisse.hh"

#include "structures/tableau.hh"

struct Bibliotheque;

struct CoulisseC final : public Coulisse {
    kuri::tableau<Bibliotheque *> m_bibliotheques{};

    bool cree_fichier_objet(Compilatrice &compilatrice,
                            EspaceDeTravail &espace,
                            Programme *programme,
                            ConstructriceRI &constructrice_ri) override;

    bool cree_executable(Compilatrice &compilatrice,
                         EspaceDeTravail &espace,
                         Programme *programme) override;
};
