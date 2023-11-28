/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "coulisse.hh"

struct CoulisseASM final : public Coulisse {
  private:
    bool génère_code_impl(Compilatrice &compilatrice,
                          EspaceDeTravail &espace,
                          Programme const *programme,
                          CompilatriceRI &compilatrice_ri,
                          Broyeuse &) override;

    bool crée_fichier_objet_impl(Compilatrice &compilatrice,
                                 EspaceDeTravail &espace,
                                 Programme const *programme,
                                 CompilatriceRI &compilatrice_ri) override;

    bool crée_exécutable_impl(Compilatrice &compilatrice,
                              EspaceDeTravail &espace,
                              Programme const *programme) override;
};
