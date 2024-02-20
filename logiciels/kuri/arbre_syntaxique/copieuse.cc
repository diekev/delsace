/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "copieuse.hh"

Copieuse::Copieuse(AssembleuseArbre *assembleuse, OptionsCopieNoeud options)
    : assem(assembleuse), m_options(options)
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

void Copieuse::copie_membres_de_bases_et_insère(const NoeudExpression *racine,
                                                NoeudExpression *nracine)
{
    insere_copie(racine, nracine);
    nracine->ident = racine->ident;
    nracine->type = racine->type;
    nracine->drapeaux = racine->drapeaux & ~DrapeauxNoeud::FUT_APLATIS;
    /* Les paramètres des copies des opérateurs « pour » ne sont pas revalidés. */
    nracine->genre_valeur = racine->genre_valeur;
    if ((m_options & OptionsCopieNoeud::PRÉSERVE_DRAPEAUX_VALIDATION) == OptionsCopieNoeud(0)) {
        nracine->drapeaux &= ~DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    }
}

void Copieuse::insere_copie(const NoeudExpression *racine, NoeudExpression *copie)
{
    noeuds_copies.insère(racine, copie);
}

NoeudExpression *copie_noeud(AssembleuseArbre *assem,
                             const NoeudExpression *racine,
                             NoeudBloc *bloc_parent,
                             OptionsCopieNoeud options)
{
    Copieuse copieuse(assem, options);
    /* Pour simplifier la copie et la gestion des blocs, les blocs parents sont copiés.
     * Par contre, nous ne devons pas copier le bloc parent de la racine. */
    copieuse.insere_copie(bloc_parent, bloc_parent);
    return copieuse.copie_noeud(racine);
}
