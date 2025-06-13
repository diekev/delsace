/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2025 Kévin Dietrich. */

#pragma once

#include <cstdint>

/**
 * Retourne vrai si le premier élément est à l'un ou l'autre des éléments
 * suivants.
 */
template <typename T, typename... Ts>
inline auto est_élément(T &&a, Ts &&...ts)
{
    return ((a == ts) || ...);
}

inline char char_depuis_hex(char hex)
{
    return "0123456789ABCDEF"[static_cast<int>(hex)];
}

int nombre_de_chiffres(int64_t nombre);
