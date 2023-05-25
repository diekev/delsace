/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include <cstdint>

enum class GenreLexeme : unsigned int;
struct Lexeme;

struct PositionLexeme {
    int64_t index_ligne = 0;
    int64_t numero_ligne = 0;
    int64_t pos = 0;
};

PositionLexeme position_lexeme(Lexeme const &lexeme);

GenreLexeme operateur_pour_assignation_composee(GenreLexeme type);
