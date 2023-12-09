/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include <cstdint>

struct NoeudDeclarationType;
using Type = NoeudDeclarationType;

/**
 * Retourne si la valeur passée est une puissance de 2. 0 est exclus car nous ne supportons pas les
 * types ayant une taille de 0 ou les alignements de 0.
 */
template <typename T>
inline bool est_puissance_de_2(T x)
{
    return (x != 0) && (x & (x - 1)) == 0;
}

/**
 * Retourne vrai si la valeur ne pourrait être représentée par le, ou stockée dans une varible du,
 * type.
 */
bool est_hors_des_limites(int64_t valeur, Type *type);

/**
 * Retourne la valeur minimale pouvant être représentée par le type.
 */
int64_t valeur_min(Type *type);

/**
 * Retourne la valeur maximale pouvant être représentée par le type.
 */
uint64_t valeur_max(Type *type);

/**
 * Retourne le nombre de bits du type (8 * sa taille en octets).
 */
int nombre_de_bits_pour_type(Type *type);
