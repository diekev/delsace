/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <type_traits>

#include "biblinternes/outils/definitions.h"

template <typename T>
inline T valeur_defaut()
{
    if constexpr (std::is_integral_v<T>) {
        return static_cast<T>(0);
    }

    if constexpr (std::is_pointer_v<T>) {
        return static_cast<T>(nullptr);
    }

    if constexpr (std::is_floating_point_v<T>) {
        return static_cast<T>(0.0);
    }

    return T();
}

template <typename T>
struct SauvegardeuseEtat {
    T &ref;
    T valeur_precedente;

    SauvegardeuseEtat(T &valeur)
        : ref(valeur)
        , valeur_precedente(valeur)
    {
        valeur = valeur_defaut<T>();
    }

    ~SauvegardeuseEtat()
    {
        ref = valeur_precedente;
    }

    COPIE_CONSTRUCT(SauvegardeuseEtat);
};

#define SAUVEGARDE_ETAT(x) const SauvegardeuseEtat VARIABLE_ANONYME(SAUVEGARDE)(x)
