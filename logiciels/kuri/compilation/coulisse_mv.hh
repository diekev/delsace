/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "coulisse.hh"

#include "arbre_syntaxique/noeud_code.hh"

struct Programme;

struct CoulisseMV final : public Coulisse {
    /* Pour la création des infos types. */
    ConvertisseuseNoeudCode convertisseuse_noeud_code{};

    bool cree_fichier_objet(Compilatrice &compilatrice,
                            EspaceDeTravail &espace,
                            Programme *programme,
                            ConstructriceRI &constructrice_ri,
                            Broyeuse &) override;

    bool cree_executable(Compilatrice &compilatrice,
                         EspaceDeTravail &espace,
                         Programme *programme) override;
};
