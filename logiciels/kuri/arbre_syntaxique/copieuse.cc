/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "copieuse.hh"

Copieuse::Copieuse(AssembleuseArbre *assembleuse) : assem(assembleuse)
{
}

NoeudExpression *Copieuse::trouve_copie(const NoeudExpression *racine)
{
    auto trouve = false;
    auto copie = noeuds_copies.trouve(racine, trouve);
    if (trouve) {
        return copie;
    }
    return nullptr;
}

void Copieuse::insere_copie(const NoeudExpression *racine, NoeudExpression *copie)
{
    noeuds_copies.insere(racine, copie);
}

NoeudExpression *copie_noeud(AssembleuseArbre *assem,
                             const NoeudExpression *racine,
                             NoeudBloc *bloc_parent)
{
    Copieuse copieuse(assem);
    /* Pour simplifier la copie et la gestion des blocs, les blocs parents sont copiés.
     * Par contre, nous ne devons pas copier le bloc parent de la racine. */
    copieuse.insere_copie(bloc_parent, bloc_parent);
    return copieuse.copie_noeud(racine);
}
