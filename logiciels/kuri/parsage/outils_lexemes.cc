/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#include "outils_lexemes.hh"

#include "lexemes.hh"

PositionLexème position_lexeme(Lexème const &lexeme)
{
    auto pos = PositionLexème{};
    pos.pos = lexeme.colonne;
    pos.numero_ligne = lexeme.ligne + 1;
    pos.index_ligne = lexeme.ligne;
    return pos;
}

GenreLexème operateur_pour_assignation_composee(GenreLexème type)
{
    switch (type) {
        default:
        {
            return type;
        }
        case GenreLexème::MOINS_EGAL:
        {
            return GenreLexème::MOINS;
        }
        case GenreLexème::PLUS_EGAL:
        {
            return GenreLexème::PLUS;
        }
        case GenreLexème::MULTIPLIE_EGAL:
        {
            return GenreLexème::FOIS;
        }
        case GenreLexème::DIVISE_EGAL:
        {
            return GenreLexème::DIVISE;
        }
        case GenreLexème::MODULO_EGAL:
        {
            return GenreLexème::POURCENT;
        }
        case GenreLexème::ET_EGAL:
        {
            return GenreLexème::ESPERLUETTE;
        }
        case GenreLexème::OU_EGAL:
        {
            return GenreLexème::BARRE;
        }
        case GenreLexème::OUX_EGAL:
        {
            return GenreLexème::CHAPEAU;
        }
        case GenreLexème::DEC_DROITE_EGAL:
        {
            return GenreLexème::DECALAGE_DROITE;
        }
        case GenreLexème::DEC_GAUCHE_EGAL:
        {
            return GenreLexème::DECALAGE_GAUCHE;
        }
        case GenreLexème::ESP_ESP_EGAL:
        {
            return GenreLexème::ESP_ESP;
        }
        case GenreLexème::BARRE_BARRE_EGAL:
        {
            return GenreLexème::BARRE_BARRE;
        }
    }
}
