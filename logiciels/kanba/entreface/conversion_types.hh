/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "biblinternes/math/matrice.hh"
#include "biblinternes/math/vecteur.hh"
#include "biblinternes/phys/couleur.hh"

#include "coeur/kanba.h"

template <typename TypeCible, typename TypeOrig>
static TypeCible convertis_type(TypeOrig val)
{
    static_assert(sizeof(TypeCible) == sizeof(TypeOrig));
    return *reinterpret_cast<TypeCible *>(&val);
}

#define DEFINIS_FONCTION_CONVERSION_TYPE(nom, type_dls, type_knb)                                 \
    inline type_dls convertis_##nom(type_knb valeur)                                              \
    {                                                                                             \
        return convertis_type<type_dls>(valeur);                                                  \
    }                                                                                             \
    inline type_knb convertis_##nom(type_dls valeur)                                              \
    {                                                                                             \
        return convertis_type<type_knb>(valeur);                                                  \
    }

DEFINIS_FONCTION_CONVERSION_TYPE(matrice, dls::math::mat4x4f, KNB::Mat4r)
DEFINIS_FONCTION_CONVERSION_TYPE(vecteur, dls::math::vec2f, KNB::Vec2)
DEFINIS_FONCTION_CONVERSION_TYPE(vecteur, dls::math::vec3f, KNB::Vec3)
// DEFINIS_FONCTION_CONVERSION_TYPE(vecteur, dls::math::vec4f, KNB::Vec4)
DEFINIS_FONCTION_CONVERSION_TYPE(couleur, dls::phys::couleur32, KNB::CouleurRVBA)
DEFINIS_FONCTION_CONVERSION_TYPE(couleur_vec4, dls::math::vec4f, KNB::CouleurRVBA)
// DEFINIS_FONCTION_CONVERSION_TYPE(point, dls::math::vec3f, KNB::Point3D)

#undef DEFINIS_FONCTION_CONVERSION_TYPE
