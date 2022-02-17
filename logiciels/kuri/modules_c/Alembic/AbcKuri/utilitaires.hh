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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <string>

#include "../alembic_types.h"

template <typename Objet>
std::string string_depuis_rappel(Objet *objet, void (*rappel)(Objet *, const char **, size_t *))
{
    const char *pointeur = nullptr;
    size_t taille = 0;

    rappel(objet, &pointeur, &taille);

    if (pointeur == nullptr || taille == 0) {
        return "";
    }

    return std::string(pointeur, taille);
}

template <typename Objet>
std::string string_depuis_rappel(Objet *objet,
                                 size_t index,
                                 void (*rappel)(Objet *, size_t, const char **, size_t *))
{
    const char *pointeur = nullptr;
    size_t taille = 0;

    rappel(objet, index, &pointeur, &taille);

    if (pointeur == nullptr || taille == 0) {
        return "";
    }

    return std::string(pointeur, taille);
}

template <typename Objet>
std::string string_depuis_rappel(Objet *objet,
                                 size_t index0,
                                 size_t index1,
                                 void (*rappel)(Objet *, size_t, size_t, const char **, size_t *))
{
    const char *pointeur = nullptr;
    size_t taille = 0;

    rappel(objet, index0, index1, &pointeur, &taille);

    if (pointeur == nullptr || taille == 0) {
        return "";
    }

    return std::string(pointeur, taille);
}

template <typename Objet>
std::string string_depuis_rappel(
    Objet *objet,
    size_t index0,
    size_t index1,
    size_t index2,
    void (*rappel)(Objet *, size_t, size_t, size_t, const char **, size_t *))
{
    if (!rappel) {
        return "";
    }

    const char *pointeur = nullptr;
    size_t taille = 0;

    rappel(objet, index0, index1, index2, &pointeur, &taille);

    if (pointeur == nullptr || taille == 0) {
        return "";
    }

    return std::string(pointeur, taille);
}
