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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <iostream>

#include "biblinternes/outils/definitions.h"

namespace KNB {

enum class TypeÉvènement : int {
    /* Category. */
    DESSIN = (1 << 0),
    CALQUE = (2 << 0),
    RAFRAICHISSEMENT = (3 << 0),
    PROJET = (4 << 0),

    /* Action. */
    AJOUTÉ = (1 << 8),
    SÉLECTIONNÉ = (2 << 8),
    FINI = (4 << 8),
    SUPPRIMÉ = (5 << 8),
    CHARGÉ = (6 << 8),
};

DEFINIS_OPERATEURS_DRAPEAU(TypeÉvènement)

constexpr TypeÉvènement action_evenement(TypeÉvènement evenement)
{
    return evenement & 0x0000ff00;
}

constexpr auto categorie_evenement(TypeÉvènement evenement)
{
    return evenement & 0x000000ff;
}

template <typename TypeChar>
std::basic_ostream<TypeChar> &operator<<(std::basic_ostream<TypeChar> &os, TypeÉvènement evenement)
{
    switch (categorie_evenement(evenement)) {
        case TypeÉvènement::DESSIN:
            os << "dessin, ";
            break;
        case TypeÉvènement::CALQUE:
            os << "calque, ";
            break;
        case TypeÉvènement::RAFRAICHISSEMENT:
            os << "rafraichissement, ";
            break;
        case TypeÉvènement::PROJET:
            os << "projet, ";
            break;
        default:
            break;
    }

    switch (action_evenement(evenement)) {
        case TypeÉvènement::AJOUTÉ:
            os << "ajouté";
            break;
        case TypeÉvènement::SÉLECTIONNÉ:
            os << "sélectionné";
            break;
        case TypeÉvènement::FINI:
            os << "fini";
            break;
        case TypeÉvènement::SUPPRIMÉ:
            os << "supprimé";
            break;
        case TypeÉvènement::CHARGÉ:
            os << "chargé";
            break;
        default:
            break;
    }

    return os;
}

}  // namespace KNB
