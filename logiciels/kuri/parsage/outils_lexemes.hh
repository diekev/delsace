/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include <cstdint>

enum class GenreLexème : uint32_t;
struct Lexème;

struct PositionLexème {
    int64_t index_ligne = 0;
    int64_t numero_ligne = 0;
    int64_t pos = 0;
};

PositionLexème position_lexeme(Lexème const &lexeme);

GenreLexème operateur_pour_assignation_composee(GenreLexème type);
