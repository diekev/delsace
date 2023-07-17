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

#include "melange.h"

#include "biblinternes/structures/chaine.hh"

namespace KNB {

dls::chaine nom_mode_fusion(TypeMelange type_melange)
{
    switch (type_melange) {
        default:
        case TypeMelange::NORMAL:
            return "normal";
        case TypeMelange::ADDITION:
            return "addition";
        case TypeMelange::SOUSTRACTION:
            return "soustraction";
        case TypeMelange::MULTIPLICATION:
            return "multiplication";
        case TypeMelange::DIVISION:
            return "division";
    }
}

TypeMelange mode_fusion_depuis_nom(dls::chaine const &nom)
{
    if (nom == "normal") {
        return TypeMelange::NORMAL;
    }

    if (nom == "addition") {
        return TypeMelange::ADDITION;
    }

    if (nom == "soustraction") {
        return TypeMelange::SOUSTRACTION;
    }

    if (nom == "multiplication") {
        return TypeMelange::MULTIPLICATION;
    }

    if (nom == "division") {
        return TypeMelange::DIVISION;
    }

    return TypeMelange::NORMAL;
}

}  // namespace KNB
