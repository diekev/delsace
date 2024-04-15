/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#include "etendue_code_source.hh"

#include "parsage/outils_lexemes.hh"

#include "noeud_expression.hh"

static void corrige_étendue_pour_chaine_lexème(ÉtendueSourceNoeud &étendue, Lexème const *lexème)
{
    POUR (lexème->chaine) {
        if (it == '\n') {
            étendue.ligne_fin += 1;
        }
    }
}

static void corrige_étendue_pour_bloc(ÉtendueSourceNoeud &étendue, NoeudBloc const *bloc)
{
    /* Les blocs de corps de fonctions générées par des #corps_texte n'ont pas d'accolades. */
    if (bloc && bloc->lexème_accolade_finale) {
        étendue.ligne_fin = std::max(étendue.ligne_fin, bloc->lexème_accolade_finale->ligne);
    }
}

void ÉtendueSourceNoeud::fusionne(ÉtendueSourceNoeud autre)
{
    if (autre.ligne_début < ligne_début) {
        ligne_début = autre.ligne_début;
        colonne_début = autre.colonne_début;
    }

    if (autre.ligne_fin > ligne_fin) {
        ligne_fin = autre.ligne_fin;
        colonne_fin = autre.colonne_fin;
    }
}

ÉtendueSourceNoeud ÉtendueSourceNoeud::depuis_lexème(Lexème const *lexème)
{
    auto const pos = position_lexeme(*lexème);
    ÉtendueSourceNoeud résultat;
    résultat.ligne_début = int32_t(pos.index_ligne);
    résultat.colonne_début = int32_t(pos.pos);
    résultat.ligne_fin = int32_t(pos.index_ligne);
    résultat.colonne_fin = int32_t(pos.pos + lexème->chaine.taille());
    return résultat;
}

ÉtendueSourceNoeud donne_étendue_source_noeud(NoeudExpression const *noeud)
{
    auto résultat = ÉtendueSourceNoeud{};

    visite_noeud(noeud,
                 PreferenceVisiteNoeud::ORIGINAL,
                 false,
                 [&](NoeudExpression const *noeud_visité) -> DecisionVisiteNoeud {
                     auto lexème = noeud->lexème;
                     auto étendue_noeud_visité = ÉtendueSourceNoeud::depuis_lexème(lexème);

                     if (noeud_visité->est_commentaire() || noeud_visité->est_littérale_chaine()) {
                         corrige_étendue_pour_chaine_lexème(étendue_noeud_visité, lexème);
                     }
                     else if (noeud_visité->est_bloc()) {
                         corrige_étendue_pour_bloc(étendue_noeud_visité,
                                                   noeud_visité->comme_bloc());
                     }

                     résultat.fusionne(étendue_noeud_visité);

                     return DecisionVisiteNoeud::CONTINUE;
                 });

    return résultat;
}
