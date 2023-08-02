/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#include "outils_lexemes.hh"

#include "lexemes.hh"

PositionLexeme position_lexeme(Lexeme const &lexeme)
{
    auto pos = PositionLexeme{};
    pos.pos = lexeme.colonne;
    pos.numero_ligne = lexeme.ligne + 1;
    pos.index_ligne = lexeme.ligne;
    return pos;
}

GenreLexeme operateur_pour_assignation_composee(GenreLexeme type)
{
    switch (type) {
        default:
        {
            return type;
        }
        case GenreLexeme::MOINS_EGAL:
        {
            return GenreLexeme::MOINS;
        }
        case GenreLexeme::PLUS_EGAL:
        {
            return GenreLexeme::PLUS;
        }
        case GenreLexeme::MULTIPLIE_EGAL:
        {
            return GenreLexeme::FOIS;
        }
        case GenreLexeme::DIVISE_EGAL:
        {
            return GenreLexeme::DIVISE;
        }
        case GenreLexeme::MODULO_EGAL:
        {
            return GenreLexeme::POURCENT;
        }
        case GenreLexeme::ET_EGAL:
        {
            return GenreLexeme::ESPERLUETTE;
        }
        case GenreLexeme::OU_EGAL:
        {
            return GenreLexeme::BARRE;
        }
        case GenreLexeme::OUX_EGAL:
        {
            return GenreLexeme::CHAPEAU;
        }
        case GenreLexeme::DEC_DROITE_EGAL:
        {
            return GenreLexeme::DECALAGE_DROITE;
        }
        case GenreLexeme::DEC_GAUCHE_EGAL:
        {
            return GenreLexeme::DECALAGE_GAUCHE;
        }
    }
}
