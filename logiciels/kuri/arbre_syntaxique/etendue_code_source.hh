/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#pragma once

#include <cstdint>
#include <limits>

struct Lexème;
struct NoeudExpression;

/* ------------------------------------------------------------------------- */
/** \name ÉtendueSourceNoeud.
 * Représente l'étendue (ligne et colonne de début et de fin) d'un noeud
 * syntaxique dans le code source originel.
 * \{ */

struct ÉtendueSourceNoeud {
    int32_t ligne_début = std::numeric_limits<int32_t>::max();
    int32_t ligne_fin = std::numeric_limits<int32_t>::min();
    int32_t colonne_début = std::numeric_limits<int32_t>::max();
    int32_t colonne_fin = std::numeric_limits<int32_t>::min();

    void fusionne(ÉtendueSourceNoeud autre);

    static ÉtendueSourceNoeud depuis_lexème(Lexème const *lexème);
};

ÉtendueSourceNoeud donne_étendue_source_noeud(NoeudExpression const *noeud);

/** \} */
