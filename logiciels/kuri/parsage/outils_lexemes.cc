/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#include "outils_lexemes.hh"

#include "lexemes.hh"

PositionLexème position_lexème(Lexème const &lexème)
{
    auto pos = PositionLexème{};
    pos.pos = lexème.colonne;
    pos.numéro_ligne = lexème.ligne + 1;
    pos.indice_ligne = lexème.ligne;
    return pos;
}

GenreLexème operateur_pour_assignation_composee(GenreLexème type)
{
    switch (type) {
        default:
        {
            return type;
        }
        case GenreLexème::MOINS_ÉGAL:
        {
            return GenreLexème::MOINS;
        }
        case GenreLexème::PLUS_ÉGAL:
        {
            return GenreLexème::PLUS;
        }
        case GenreLexème::MULTIPLIE_ÉGAL:
        {
            return GenreLexème::FOIS;
        }
        case GenreLexème::DIVISE_ÉGAL:
        {
            return GenreLexème::DIVISE;
        }
        case GenreLexème::MODULO_ÉGAL:
        {
            return GenreLexème::POURCENT;
        }
        case GenreLexème::ET_ÉGAL:
        {
            return GenreLexème::ESPERLUETTE;
        }
        case GenreLexème::OU_ÉGAL:
        {
            return GenreLexème::BARRE;
        }
        case GenreLexème::OUX_ÉGAL:
        {
            return GenreLexème::CHAPEAU;
        }
        case GenreLexème::DEC_DROITE_ÉGAL:
        {
            return GenreLexème::DECALAGE_DROITE;
        }
        case GenreLexème::DEC_GAUCHE_ÉGAL:
        {
            return GenreLexème::DECALAGE_GAUCHE;
        }
        case GenreLexème::ESP_ESP_ÉGAL:
        {
            return GenreLexème::ESP_ESP;
        }
        case GenreLexème::BARRE_BARRE_ÉGAL:
        {
            return GenreLexème::BARRE_BARRE;
        }
    }
}

Lexème *LexèmesExtra::crée_lexème(Lexème const *référence,
                                  GenreLexème genre,
                                  kuri::chaine_statique texte)
{
    auto résultat = lexèmes_extra.ajoute_élément();
    *résultat = *référence;
    résultat->genre = genre;
    résultat->chaine = texte;
    return résultat;
}

Lexème *LexèmesExtra::crée_lexème(GenreLexème genre, IdentifiantCode *ident)
{
    auto résultat = lexèmes_extra.ajoute_élément();
    résultat->genre = genre;
    résultat->ident = ident;
    return résultat;
}
