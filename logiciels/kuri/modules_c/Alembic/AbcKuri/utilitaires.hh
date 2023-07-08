/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include <string>

#include "../alembic_types.h"

template <typename Objet>
std::string string_depuis_rappel(Objet *objet, void (*rappel)(Objet *, const char **, size_t *))
{
    if (!rappel) {
        return "";
    }
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
    if (!rappel) {
        return "";
    }
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
    if (!rappel) {
        return "";
    }
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
