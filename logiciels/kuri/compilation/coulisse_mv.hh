/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "coulisse.hh"

#include "arbre_syntaxique/noeud_code.hh"

struct Programme;

struct CoulisseMV final : public Coulisse {
  private:
    /* Pour la création des infos types. */
    ConvertisseuseNoeudCode convertisseuse_noeud_code{};

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
