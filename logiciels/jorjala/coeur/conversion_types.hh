/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "biblinternes/math/matrice.hh"
#include "biblinternes/math/vecteur.hh"
#include "biblinternes/phys/couleur.hh"

#include "jorjala.hh"

template <typename TypeCible, typename TypeOrig>
static TypeCible convertis_type(TypeOrig val)
{
    static_assert(sizeof(TypeCible) == sizeof(TypeOrig));
    return *reinterpret_cast<TypeCible *>(&val);
}

#define DEFINIE_FONCTION_CONVERSION_TYPE(nom, type_dls, type_jjl)                                 \
    inline type_dls convertis_##nom(type_jjl valeur)                                              \
    {                                                                                             \
        return convertis_type<type_dls>(valeur);                                                  \
    }                                                                                             \
    inline type_jjl convertis_##nom(type_dls valeur)                                              \
    {                                                                                             \
        return convertis_type<type_jjl>(valeur);                                                  \
    }

DEFINIE_FONCTION_CONVERSION_TYPE(matrice, dls::math::mat4x4f, JJL::Mat4r);
DEFINIE_FONCTION_CONVERSION_TYPE(vecteur, dls::math::vec2f, JJL::Vec2);
DEFINIE_FONCTION_CONVERSION_TYPE(vecteur, dls::math::vec3f, JJL::Vec3);
DEFINIE_FONCTION_CONVERSION_TYPE(couleur, dls::phys::couleur32, JJL::CouleurRVBA);
DEFINIE_FONCTION_CONVERSION_TYPE(point, dls::math::vec3f, JJL::Point3D);

#undef DEFINIE_FONCTION_CONVERSION_TYPE
