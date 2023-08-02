/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "coulisse.hh"

bool initialise_llvm();
void issitialise_llvm();

struct CoulisseLLVM final : public Coulisse {
    bool cree_fichier_objet(Compilatrice &compilatrice,
                            EspaceDeTravail &espace,
                            Programme *programme,
                            ConstructriceRI &constructrice_ri,
                            Broyeuse &) override;

    bool cree_executable(Compilatrice &compilatrice,
                         EspaceDeTravail &espace,
                         Programme *programme) override;
};
