/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include <cstdint>

#include "structures/tableau_page.hh"

#include "lexemes.hh"

enum class GenreLexème : uint32_t;
struct Lexème;

struct PositionLexème {
    int64_t indice_ligne = 0;
    int64_t numéro_ligne = 0;
    int64_t pos = 0;
};

PositionLexème position_lexeme(Lexème const &lexeme);

GenreLexème operateur_pour_assignation_composee(GenreLexème type);

/* Utilisée pour créer des lexèmes pour les noeuds générés par la compilation. */
struct LexèmesExtra {
  private:
    kuri::tableau_page<Lexème> lexèmes_extra{};

  public:
    Lexème *crée_lexème(Lexème const *référence, GenreLexème genre, kuri::chaine_statique texte);

    Lexème *crée_lexème(GenreLexème genre, IdentifiantCode *ident);

    int64_t mémoire_utilisée() const
    {
        return lexèmes_extra.mémoire_utilisée();
    }
};
