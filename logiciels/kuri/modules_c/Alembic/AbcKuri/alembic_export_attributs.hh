/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "../alembic.h"
#include "../alembic_types.h"

/* ------------------------------------------------------------------------- */
/** \name Conversions des types de valeurs des attributs pour l'export.
 * \{ */

template <eTypeDoneesAttributAbc type>
struct ConvertisseuseTypeValeurExport;

#define DEFINIS_CONVERTISSEUSE_TYPE_SIMPLE(                                                       \
    type_attribut, type_param, type_cpp, nom_type_pour_rappel)                                    \
    template <>                                                                                   \
    struct ConvertisseuseTypeValeurExport<type_attribut> {                                        \
        using type_param_abc = Alembic::AbcGeom::type_param;                                      \
        using type_abc = type_param_abc::value_type;                                              \
        static bool export_valeur_est_supporté(AbcExportriceAttribut *exportrice)                 \
        {                                                                                         \
            return exportrice->donne_valeur_##nom_type_pour_rappel != nullptr;                    \
        }                                                                                         \
        static type_abc donne_valeur(AbcExportriceAttribut *exportrice,                           \
                                     void *attribut,                                              \
                                     int index)                                                   \
        {                                                                                         \
            type_cpp résultat;                                                                    \
            exportrice->donne_valeur_##nom_type_pour_rappel(attribut, index, &résultat);          \
            return résultat;                                                                      \
        }                                                                                         \
    }

DEFINIS_CONVERTISSEUSE_TYPE_SIMPLE(ATTRIBUT_TYPE_BOOL, OBoolGeomParam, bool, bool);
DEFINIS_CONVERTISSEUSE_TYPE_SIMPLE(ATTRIBUT_TYPE_N8, OUcharGeomParam, uint8_t, n8);
DEFINIS_CONVERTISSEUSE_TYPE_SIMPLE(ATTRIBUT_TYPE_Z8, OCharGeomParam, int8_t, z8);
DEFINIS_CONVERTISSEUSE_TYPE_SIMPLE(ATTRIBUT_TYPE_N16, OUInt16GeomParam, uint16_t, n16);
DEFINIS_CONVERTISSEUSE_TYPE_SIMPLE(ATTRIBUT_TYPE_Z16, OInt16GeomParam, int16_t, z16);
DEFINIS_CONVERTISSEUSE_TYPE_SIMPLE(ATTRIBUT_TYPE_N32, OUInt32GeomParam, uint32_t, n32);
DEFINIS_CONVERTISSEUSE_TYPE_SIMPLE(ATTRIBUT_TYPE_Z32, OInt32GeomParam, int32_t, z32);
DEFINIS_CONVERTISSEUSE_TYPE_SIMPLE(ATTRIBUT_TYPE_N64, OUInt64GeomParam, uint64_t, n64);
DEFINIS_CONVERTISSEUSE_TYPE_SIMPLE(ATTRIBUT_TYPE_Z64, OInt64GeomParam, int64_t, z64);
/* À FAIRE : r16 */
// DEFINIS_CONVERTISSEUSE_TYPE_SIMPLE(ATTRIBUT_TYPE_R16, OHalfGeomParam, uint16_t, r16);
DEFINIS_CONVERTISSEUSE_TYPE_SIMPLE(ATTRIBUT_TYPE_R32, OFloatGeomParam, float, r32);
DEFINIS_CONVERTISSEUSE_TYPE_SIMPLE(ATTRIBUT_TYPE_R64, ODoubleGeomParam, double, r64);

template <>
struct ConvertisseuseTypeValeurExport<ATTRIBUT_TYPE_CHAINE> {
    using type_param_abc = Alembic::AbcGeom::OStringGeomParam;
    using type_abc = std::string;

    static bool export_valeur_est_supporté(AbcExportriceAttribut *exportrice)
    {
        return exportrice->donne_valeur_chaine != nullptr;
    }

    static std::string donne_valeur(AbcExportriceAttribut *exportrice, void *attribut, int index)
    {
        char *pointeur;
        int64_t taille;
        exportrice->donne_valeur_chaine(attribut, index, &pointeur, &taille);
        return {pointeur, static_cast<size_t>(taille)};
    }
};

#define DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(                                                      \
    type_attribut, type_param, type_cpp, nom_type_pour_rappel)                                    \
    template <>                                                                                   \
    struct ConvertisseuseTypeValeurExport<type_attribut> {                                        \
        using type_param_abc = Alembic::AbcGeom::type_param;                                      \
        using type_abc = type_param_abc::value_type;                                              \
        static bool export_valeur_est_supporté(AbcExportriceAttribut *exportrice)                 \
        {                                                                                         \
            return exportrice->donne_valeur_##nom_type_pour_rappel != nullptr;                    \
        }                                                                                         \
        static type_abc donne_valeur(AbcExportriceAttribut *exportrice,                           \
                                     void *attribut,                                              \
                                     int index)                                                   \
        {                                                                                         \
            type_abc résultat;                                                                    \
            exportrice->donne_valeur_##nom_type_pour_rappel(attribut, index, &résultat.x);        \
            return résultat;                                                                      \
        }                                                                                         \
    }

DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_VEC2_Z16, OV2sGeomParam, int16_t, vec2_z16);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_VEC2_Z32, OV2iGeomParam, int32_t, vec2_z32);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_VEC2_R32, OV2fGeomParam, float, vec2_r32);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_VEC2_R64, OV2dGeomParam, double, vec2_r64);

DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_VEC3_Z16, OV3sGeomParam, int16_t, vec3_z16);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_VEC3_Z32, OV3iGeomParam, int32_t, vec3_z32);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_VEC3_R32, OV3fGeomParam, float, vec3_r32);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_VEC3_R64, OV3dGeomParam, double, vec3_r64);

DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_POINT2_Z16, OP2sGeomParam, int16_t, point2_z16);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_POINT2_Z32, OP2iGeomParam, int32_t, point2_z32);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_POINT2_R32, OP2fGeomParam, float, point2_r32);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_POINT2_R64, OP2dGeomParam, double, point2_r64);

DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_POINT3_Z16, OP3sGeomParam, int16_t, point3_z16);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_POINT3_Z32, OP3iGeomParam, int32_t, point3_z32);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_POINT3_R32, OP3fGeomParam, float, point3_r32);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_POINT3_R64, OP3dGeomParam, double, point3_r64);

DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_NORMAL2_R32, ON2fGeomParam, float, normal2_r32);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_NORMAL2_R64, ON2dGeomParam, double, normal2_r64);

DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_NORMAL3_R32, ON3fGeomParam, float, normal3_r32);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_NORMAL3_R64, ON3dGeomParam, double, normal3_r64);

#define DEFINIS_CONVERTISSEUSE_TYPE_BOITE2(                                                       \
    type_attribut, type_param, type_cpp, nom_type_pour_rappel)                                    \
    template <>                                                                                   \
    struct ConvertisseuseTypeValeurExport<type_attribut> {                                        \
        using type_param_abc = Alembic::AbcGeom::type_param;                                      \
        using type_abc = type_param_abc::value_type;                                              \
        static bool export_valeur_est_supporté(AbcExportriceAttribut *exportrice)                 \
        {                                                                                         \
            return exportrice->donne_valeur_##nom_type_pour_rappel != nullptr;                    \
        }                                                                                         \
        static type_abc donne_valeur(AbcExportriceAttribut *exportrice,                           \
                                     void *attribut,                                              \
                                     int index)                                                   \
        {                                                                                         \
            type_cpp valeur[4];                                                                   \
            exportrice->donne_valeur_##nom_type_pour_rappel(attribut, index, valeur);             \
            type_abc résultat({valeur[0], valeur[1]}, {valeur[2], valeur[3]});                    \
            return résultat;                                                                      \
        }                                                                                         \
    }

DEFINIS_CONVERTISSEUSE_TYPE_BOITE2(ATTRIBUT_TYPE_BOITE2_Z16, OBox2sGeomParam, int16_t, boite2_z16);
DEFINIS_CONVERTISSEUSE_TYPE_BOITE2(ATTRIBUT_TYPE_BOITE2_Z32, OBox2iGeomParam, int32_t, boite2_z32);
DEFINIS_CONVERTISSEUSE_TYPE_BOITE2(ATTRIBUT_TYPE_BOITE2_R32, OBox2fGeomParam, float, boite2_r32);
DEFINIS_CONVERTISSEUSE_TYPE_BOITE2(ATTRIBUT_TYPE_BOITE2_R64, OBox2dGeomParam, double, boite2_r64);

#define DEFINIS_CONVERTISSEUSE_TYPE_BOITE3(                                                       \
    type_attribut, type_param, type_cpp, nom_type_pour_rappel)                                    \
    template <>                                                                                   \
    struct ConvertisseuseTypeValeurExport<type_attribut> {                                        \
        using type_param_abc = Alembic::AbcGeom::type_param;                                      \
        using type_abc = type_param_abc::value_type;                                              \
        static bool export_valeur_est_supporté(AbcExportriceAttribut *exportrice)                 \
        {                                                                                         \
            return exportrice->donne_valeur_##nom_type_pour_rappel != nullptr;                    \
        }                                                                                         \
        static type_abc donne_valeur(AbcExportriceAttribut *exportrice,                           \
                                     void *attribut,                                              \
                                     int index)                                                   \
        {                                                                                         \
            type_cpp valeur[6];                                                                   \
            exportrice->donne_valeur_##nom_type_pour_rappel(attribut, index, valeur);             \
            type_abc résultat({valeur[0], valeur[1], valeur[2]},                                  \
                              {valeur[3], valeur[4], valeur[5]});                                 \
            return résultat;                                                                      \
        }                                                                                         \
    }

DEFINIS_CONVERTISSEUSE_TYPE_BOITE3(ATTRIBUT_TYPE_BOITE3_Z16, OBox3sGeomParam, int16_t, boite3_z16);
DEFINIS_CONVERTISSEUSE_TYPE_BOITE3(ATTRIBUT_TYPE_BOITE3_Z32, OBox3iGeomParam, int32_t, boite3_z32);
DEFINIS_CONVERTISSEUSE_TYPE_BOITE3(ATTRIBUT_TYPE_BOITE3_R32, OBox3fGeomParam, float, boite3_r32);
DEFINIS_CONVERTISSEUSE_TYPE_BOITE3(ATTRIBUT_TYPE_BOITE3_R64, OBox3dGeomParam, double, boite3_r64);

/* À FAIRE : r16 */
// DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_COULEUR_RGB_R16, OC3hGeomParam, int16_t,
// couleur_rgb_r16);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_COULEUR_RGB_R32,
                                    OC3fGeomParam,
                                    float,
                                    couleur_rgb_r32);
DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR(ATTRIBUT_TYPE_COULEUR_RGB_N8,
                                    OC3cGeomParam,
                                    uint8_t,
                                    couleur_rgb_n8);

#define DEFINIS_CONVERTISSEUSE_TYPE_COULEUR(                                                      \
    type_attribut, type_param, type_cpp, nom_type_pour_rappel)                                    \
    template <>                                                                                   \
    struct ConvertisseuseTypeValeurExport<type_attribut> {                                        \
        using type_param_abc = Alembic::AbcGeom::type_param;                                      \
        using type_abc = type_param_abc::value_type;                                              \
        static bool export_valeur_est_supporté(AbcExportriceAttribut *exportrice)                 \
        {                                                                                         \
            return exportrice->donne_valeur_##nom_type_pour_rappel != nullptr;                    \
        }                                                                                         \
        static type_abc donne_valeur(AbcExportriceAttribut *exportrice,                           \
                                     void *attribut,                                              \
                                     int index)                                                   \
        {                                                                                         \
            type_abc résultat;                                                                    \
            exportrice->donne_valeur_##nom_type_pour_rappel(attribut, index, &résultat.r);        \
            return résultat;                                                                      \
        }                                                                                         \
    }

// DEFINIS_CONVERTISSEUSE_TYPE_COULEUR(ATTRIBUT_TYPE_COULEUR_RGBA_R16, OC4hGeomParam, int16_t,
// couleur_rgba_r16);
DEFINIS_CONVERTISSEUSE_TYPE_COULEUR(ATTRIBUT_TYPE_COULEUR_RGBA_R32,
                                    OC4fGeomParam,
                                    float,
                                    couleur_rgba_r32);
DEFINIS_CONVERTISSEUSE_TYPE_COULEUR(ATTRIBUT_TYPE_COULEUR_RGBA_N8,
                                    OC4cGeomParam,
                                    uint8_t,
                                    couleur_rgba_n8);

#define DEFINIS_CONVERTISSEUSE_TYPE_MAT3X3(                                                       \
    type_attribut, type_param, type_cpp, nom_type_pour_rappel)                                    \
    template <>                                                                                   \
    struct ConvertisseuseTypeValeurExport<type_attribut> {                                        \
        using type_param_abc = Alembic::AbcGeom::type_param;                                      \
        using type_abc = type_param_abc::value_type;                                              \
        static bool export_valeur_est_supporté(AbcExportriceAttribut *exportrice)                 \
        {                                                                                         \
            return exportrice->donne_valeur_##nom_type_pour_rappel != nullptr;                    \
        }                                                                                         \
        static type_abc donne_valeur(AbcExportriceAttribut *exportrice,                           \
                                     void *attribut,                                              \
                                     int index)                                                   \
        {                                                                                         \
            type_cpp valeur[9];                                                                   \
            exportrice->donne_valeur_##nom_type_pour_rappel(attribut, index, valeur);             \
            type_abc résultat(valeur[0],                                                          \
                              valeur[1],                                                          \
                              valeur[2],                                                          \
                              valeur[3],                                                          \
                              valeur[4],                                                          \
                              valeur[5],                                                          \
                              valeur[6],                                                          \
                              valeur[7],                                                          \
                              valeur[8]);                                                         \
            return résultat;                                                                      \
        }                                                                                         \
    }

DEFINIS_CONVERTISSEUSE_TYPE_MAT3X3(ATTRIBUT_TYPE_MAT3X3_R32, OM33fGeomParam, float, mat3x3_r32);
DEFINIS_CONVERTISSEUSE_TYPE_MAT3X3(ATTRIBUT_TYPE_MAT3X3_R64, OM33dGeomParam, double, mat3x3_r64);

#define DEFINIS_CONVERTISSEUSE_TYPE_MAT4X4(                                                       \
    type_attribut, type_param, type_cpp, nom_type_pour_rappel)                                    \
    template <>                                                                                   \
    struct ConvertisseuseTypeValeurExport<type_attribut> {                                        \
        using type_param_abc = Alembic::AbcGeom::type_param;                                      \
        using type_abc = type_param_abc::value_type;                                              \
        static bool export_valeur_est_supporté(AbcExportriceAttribut *exportrice)                 \
        {                                                                                         \
            return exportrice->donne_valeur_##nom_type_pour_rappel != nullptr;                    \
        }                                                                                         \
        static type_abc donne_valeur(AbcExportriceAttribut *exportrice,                           \
                                     void *attribut,                                              \
                                     int index)                                                   \
        {                                                                                         \
            type_cpp valeur[16];                                                                  \
            exportrice->donne_valeur_##nom_type_pour_rappel(attribut, index, valeur);             \
            type_abc résultat(valeur[0],                                                          \
                              valeur[1],                                                          \
                              valeur[2],                                                          \
                              valeur[3],                                                          \
                              valeur[4],                                                          \
                              valeur[5],                                                          \
                              valeur[6],                                                          \
                              valeur[7],                                                          \
                              valeur[8],                                                          \
                              valeur[9],                                                          \
                              valeur[10],                                                         \
                              valeur[11],                                                         \
                              valeur[12],                                                         \
                              valeur[13],                                                         \
                              valeur[14],                                                         \
                              valeur[15]);                                                        \
            return résultat;                                                                      \
        }                                                                                         \
    }

DEFINIS_CONVERTISSEUSE_TYPE_MAT4X4(ATTRIBUT_TYPE_MAT4X4_R32, OM44fGeomParam, float, mat4x4_r32);
DEFINIS_CONVERTISSEUSE_TYPE_MAT4X4(ATTRIBUT_TYPE_MAT4X4_R64, OM44dGeomParam, double, mat4x4_r64);

#define DEFINIS_CONVERTISSEUSE_TYPE_QUAT(                                                         \
    type_attribut, type_param, type_cpp, nom_type_pour_rappel)                                    \
    template <>                                                                                   \
    struct ConvertisseuseTypeValeurExport<type_attribut> {                                        \
        using type_param_abc = Alembic::AbcGeom::type_param;                                      \
        using type_abc = type_param_abc::value_type;                                              \
        static bool export_valeur_est_supporté(AbcExportriceAttribut *exportrice)                 \
        {                                                                                         \
            return exportrice->donne_valeur_##nom_type_pour_rappel != nullptr;                    \
        }                                                                                         \
        static type_abc donne_valeur(AbcExportriceAttribut *exportrice,                           \
                                     void *attribut,                                              \
                                     int index)                                                   \
        {                                                                                         \
            type_cpp valeur[4];                                                                   \
            exportrice->donne_valeur_##nom_type_pour_rappel(attribut, index, valeur);             \
            type_abc résultat(valeur[0], valeur[1], valeur[2], valeur[3]);                        \
            return résultat;                                                                      \
        }                                                                                         \
    }

DEFINIS_CONVERTISSEUSE_TYPE_QUAT(ATTRIBUT_TYPE_QUAT_R32, OQuatfGeomParam, float, quat_r32);
DEFINIS_CONVERTISSEUSE_TYPE_QUAT(ATTRIBUT_TYPE_QUAT_R64, OQuatdGeomParam, double, quat_r64);

#undef DEFINIS_CONVERTISSEUSE_TYPE_QUAT
#undef DEFINIS_CONVERTISSEUSE_TYPE_MAT4X4
#undef DEFINIS_CONVERTISSEUSE_TYPE_MAT3X3
#undef DEFINIS_CONVERTISSEUSE_TYPE_COULEUR
#undef DEFINIS_CONVERTISSEUSE_TYPE_BOITE3
#undef DEFINIS_CONVERTISSEUSE_TYPE_BOITE2
#undef DEFINIS_CONVERTISSEUSE_TYPE_VECTEUR
#undef DEFINIS_CONVERTISSEUSE_TYPE_SIMPLE

/** \} */
