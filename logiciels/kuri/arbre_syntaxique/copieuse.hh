/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include "noeud_expression.hh"

#include "structures/table_hachage.hh"

struct AssembleuseArbre;
struct NoeudExpression;
struct NoeudBloc;

struct Copieuse {
  private:
    AssembleuseArbre *assem;
    kuri::table_hachage<NoeudExpression const *, NoeudExpression *> noeuds_copies{"noeud_copiés"};

  public:
    Copieuse(AssembleuseArbre *assembleuse);

    EMPECHE_COPIE(Copieuse);

    /* L'implémentation de cette fonction est générée par l'ADN. */
    NoeudExpression *copie_noeud(const NoeudExpression *racine, NoeudBloc *bloc_parent);

  private:
    NoeudExpression *trouve_copie(const NoeudExpression *racine);

    void insere_copie(const NoeudExpression *racine, NoeudExpression *copie);
};

NoeudExpression *copie_noeud(AssembleuseArbre *assem,
                             const NoeudExpression *racine,
                             NoeudBloc *bloc_parent);
