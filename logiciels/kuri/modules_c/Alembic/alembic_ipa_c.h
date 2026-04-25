/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "alembic_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Crée une archive pour lire des objets Alembic.
 *
 * Si l'archive ne peut être ouverte, retourne nul, et rapporte une erreur via le ctx.
 */
struct ArchiveCache *ABC_cree_archive(struct ContexteKuri *ctx_kuri,
                                      struct ContexteOuvertureArchive *ctx);

void ABC_detruit_archive(struct ContexteKuri *ctx, struct ArchiveCache *archive);

void ABC_traverse_archive(struct ContexteKuri *ctx_kuri,
                          struct ArchiveCache *archive,
                          struct ContexteTraverseArchive *ctx);

struct LectriceCache *ABC_cree_lectrice_cache(struct ContexteKuri *ctx_kuri,
                                              struct ArchiveCache *archive,
                                              const char *ptr_nom,
                                              uint64_t taille_nom);
void ABC_detruit_lectrice(struct ContexteKuri *ctx_kuri, struct LectriceCache *lectrice);
void ABC_lectrice_ajourne_donnees(struct LectriceCache *lectrice, void *donnees);
void ABC_lis_objet(struct ContexteKuri *ctx_kuri,
                   struct ContexteLectureCache *contexte,
                   struct LectriceCache *lectrice,
                   double temps);

void ABC_lis_attributs(struct ContexteKuri *ctx_kuri,
                       struct LectriceCache *lectrice,
                       struct ConvertisseuseImportAttributs *convertisseuse,
                       double temps);

struct AutriceArchive *ABC_cree_autrice_archive(struct ContexteKuri *ctx_kuri,
                                                struct ContexteCreationArchive *ctx,
                                                struct ContexteEcritureCache *ctx_ecriture);

void ABC_detruit_autrice(struct ContexteKuri *ctx, struct AutriceArchive *autrice);

struct EcrivainCache *ABC_cree_ecrivain_cache(struct ContexteKuri *ctx,
                                              struct AutriceArchive *archive,
                                              struct EcrivainCache *parent,
                                              const char *nom,
                                              uint64_t taille_nom,
                                              void *données,
                                              enum eTypeObjetAbc type_objet);

/** Crée une instance de `origine` comme enfant de `parent`. Retourne nul s'il est impossible de
 * créer une telle instance. */
struct EcrivainCache *ABC_cree_instance(struct ContexteKuri *ctx,
                                        struct AutriceArchive *archive,
                                        struct EcrivainCache *parent,
                                        struct EcrivainCache *origine,
                                        const char *nom,
                                        uint64_t taille_nom);

void ABC_ecris_donnees(struct AutriceArchive *autrice);

typedef struct Abc_String {
    const char *characters;
    uint64_t size;
} Abc_String;

/* ------------------------------------------------------------------------- */
/** \nom MetaData
 * \{ */

struct Abc_MetaData;

void abc_metadata_destroy(struct Abc_MetaData *metadata);

struct Abc_MetaData_Iterator {
    struct Abc_MetaData *metadata;

    /* Retourne vrai si une valeur fut renseignée dans key et value.
     * Retourne faux si l'itérateur est à la fin. */
    bool (*next)(struct Abc_MetaData_Iterator *iterator,
                 struct Abc_String *key,
                 struct Abc_String *value);
};

struct Abc_MetaData_Iterator *abc_metadata_get_iterator(struct Abc_MetaData *metadata);

void abc_metadata_iterator_destroy(struct Abc_MetaData_Iterator *iterator);

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Geometry_Scope
 * \{ */

enum Abc_Geometry_Scope {
    CONSTANT = 0,
    UNIFORM = 1,
    VARYING = 2,
    VERTEX = 3,
    FACE_VARYING = 4,

    UNKNOWN = 127
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Archive
 * \{ */

struct Abc_Input_Archive;

struct Abc_Input_Archive *abc_input_archive_create(struct ContexteKuri *ctx_kuri,
                                                   struct Abc_String *chemins,
                                                   uint64_t nombre_de_chemins);

void abc_input_archive_destroy(struct Abc_Input_Archive *archive);

struct Abc_MetaData *abc_input_archive_get_metadata(struct Abc_Input_Archive *archive);

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Export
 * \{ */

struct Abc_Output_Archive_Metadata {
    struct Abc_String application_name;
    struct Abc_String user_description;
    double fps;
};

struct Abc_Output_Archive;

struct Abc_Output_Xform;
struct Abc_Output_Points;

union Abc_Output_Object {
    struct Abc_Output_Xform *xform;
    struct Abc_Output_Points *points;
};

/**
 * @brief abc_output_archive_create Crée une archive Alembic pour y écrire des objets.
 * @param ctx_kuri Le contexte Kuri utilisé pour toutes les allocations.
 * @param path Le chemin où sera écris le fichier. Si un fichier existe déjà à ce chemin, il sera
 * surécrit.
 * @param metadata Les métadonnées de l'archive. DOIT être non-nul.
 * @return Une instance de Abc_Output_Archive.
 */
struct Abc_Output_Archive *abc_output_archive_create(struct ContexteKuri *ctx_kuri,
                                                     struct Abc_String path,
                                                     struct Abc_Output_Archive_Metadata *metadata);

/**
 * @brief abc_output_archive_destroy Détruit l'archive. Rien ne sera écris tant que ceci n'est pas
 * appelé.
 * @param archive L'archive à détruire. Peut être nulle.
 */
void abc_output_archive_destroy(struct Abc_Output_Archive *archive);

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Time_Sample_Index
 * \{ */

struct Abc_Time_Sample_Index {
    uint32_t value;
};

/**
 * @brief abc_output_archive_default_time_sampling Retourne le Abc_Time_Sample_Index utilisé par
 * défaut.
 */
struct Abc_Time_Sample_Index abc_output_archive_default_time_sampling(
    struct Abc_Output_Archive *archive);

/**
 * @brief abc_output_archive_create_time_sampling Ajoute un TimeSampling à l'archive et retourne
 * son indice.
 * @param archive
 * @param echantillons Les échantillons pour le TimeSampling. Chaque échantillons est le temps
 * depuis le début de l'animation.
 * @param nombre_d_echantillons Le nombre d'échantillons. Peut être 0, dans ce cas les échantillons
 * sont ignorés.
 * @param temps_par_cycle La durée d'un cycle d'échantillon.
 * @return L'indice du TimeSampling dans la poule de TimeSampling de l'archive.
 */
struct Abc_Time_Sample_Index abc_output_archive_create_time_sampling(
    struct Abc_Output_Archive *archive,
    double *echantillons,
    uint64_t nombre_d_echantillons,
    double temps_par_cycle);

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Compound_Property
 * \{ */

struct Abc_Output_Compound_Property;

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Compound_Property
 * \{ */

#define ENUMERATE_VEC_TYPES(X)                                                                    \
    X(s, short)                                                                                   \
    X(i, int)                                                                                     \
    X(f, float)                                                                                   \
    X(d, double)

#define DECLARE_VEC_TYPES(vec_suffix, type)                                                       \
    typedef struct Abc_V2##vec_suffix {                                                           \
        type x;                                                                                   \
        type y;                                                                                   \
    } Abc_V2##vec_suffix;                                                                         \
    typedef struct Abc_V3##vec_suffix {                                                           \
        type x;                                                                                   \
        type y;                                                                                   \
        type z;                                                                                   \
    } Abc_V3##vec_suffix;

ENUMERATE_VEC_TYPES(DECLARE_VEC_TYPES)

#undef DECLARE_VEC_TYPES

#define ENUMERATE_BOX_TYPES(X)                                                                    \
    X(Box2s, V2s)                                                                                 \
    X(Box2i, V2i)                                                                                 \
    X(Box2f, V2f)                                                                                 \
    X(Box2d, V2d)                                                                                 \
    X(Box3s, V3s)                                                                                 \
    X(Box3i, V3i)                                                                                 \
    X(Box3f, V3f)                                                                                 \
    X(Box3d, V3d)

#define DECLARE_BOX_TYPES(box_name, vec_type)                                                     \
    typedef struct Abc_##box_name {                                                               \
        Abc_##vec_type min;                                                                       \
        Abc_##vec_type max;                                                                       \
    } Abc_##box_name;

ENUMERATE_BOX_TYPES(DECLARE_BOX_TYPES)

#undef DECLARE_BOX_TYPES

// À FAIRE :
/*

typedef OTypedGeomParam<Float16TPTraits>         OHalfGeomParam;
typedef OTypedGeomParam<WstringTPTraits>         OWstringGeomParam;

typedef OTypedGeomParam<P2sTPTraits>             OP2sGeomParam;
typedef OTypedGeomParam<P2iTPTraits>             OP2iGeomParam;
typedef OTypedGeomParam<P2fTPTraits>             OP2fGeomParam;
typedef OTypedGeomParam<P2dTPTraits>             OP2dGeomParam;

typedef OTypedGeomParam<P3sTPTraits>             OP3sGeomParam;
typedef OTypedGeomParam<P3iTPTraits>             OP3iGeomParam;
typedef OTypedGeomParam<P3fTPTraits>             OP3fGeomParam;
typedef OTypedGeomParam<P3dTPTraits>             OP3dGeomParam;

typedef OTypedGeomParam<M33fTPTraits>            OM33fGeomParam;
typedef OTypedGeomParam<M33dTPTraits>            OM33dGeomParam;
typedef OTypedGeomParam<M44fTPTraits>            OM44fGeomParam;
typedef OTypedGeomParam<M44dTPTraits>            OM44dGeomParam;

typedef OTypedGeomParam<QuatfTPTraits>           OQuatfGeomParam;
typedef OTypedGeomParam<QuatdTPTraits>           OQuatdGeomParam;

typedef OTypedGeomParam<C3hTPTraits>             OC3hGeomParam;
typedef OTypedGeomParam<C3fTPTraits>             OC3fGeomParam;
typedef OTypedGeomParam<C3cTPTraits>             OC3cGeomParam;

typedef OTypedGeomParam<C4hTPTraits>             OC4hGeomParam;
typedef OTypedGeomParam<C4fTPTraits>             OC4fGeomParam;
typedef OTypedGeomParam<C4cTPTraits>             OC4cGeomParam;

typedef OTypedGeomParam<N2fTPTraits>             ON2fGeomParam;
typedef OTypedGeomParam<N2dTPTraits>             ON2dGeomParam;

typedef OTypedGeomParam<N3fTPTraits>             ON3fGeomParam;
typedef OTypedGeomParam<N3dTPTraits>             ON3dGeomParam;


 */
// X(nom pour type, type c primitif)
#define ENUMERATE_ABC_ATTRIBUTE_TYPES(X)                                                          \
    X(Bool, bool)                                                                                 \
    X(Uchar, uint8_t)                                                                             \
    X(Char, int8_t)                                                                               \
    X(UInt16, uint16_t)                                                                           \
    X(Int16, int16_t)                                                                             \
    X(UInt32, uint32_t)                                                                           \
    X(Int32, int32_t)                                                                             \
    X(UInt64, uint64_t)                                                                           \
    X(Int64, int64_t)                                                                             \
    X(Float, float)                                                                               \
    X(Double, double)                                                                             \
    X(String, Abc_String)                                                                         \
    X(V2s, Abc_V2s)                                                                               \
    X(V2i, Abc_V2i)                                                                               \
    X(V2f, Abc_V2f)                                                                               \
    X(V2d, Abc_V2d)                                                                               \
    X(V3s, Abc_V3s)                                                                               \
    X(V3i, Abc_V3i)                                                                               \
    X(V3f, Abc_V3f)                                                                               \
    X(V3d, Abc_V3d)                                                                               \
    X(Box2s, Abc_Box2s)                                                                           \
    X(Box2i, Abc_Box2i)                                                                           \
    X(Box2f, Abc_Box2f)                                                                           \
    X(Box2d, Abc_Box2d)                                                                           \
    X(Box3s, Abc_Box3s)                                                                           \
    X(Box3i, Abc_Box3i)                                                                           \
    X(Box3f, Abc_Box3f)                                                                           \
    X(Box3d, Abc_Box3d)

#define DECLARE_ABC_OUTPUT_GEOM_PARAMS(nom_abc, type_c)                                           \
    struct Abc_Output_##nom_abc##_Geom_Param;                                                     \
    struct Abc_Output_##nom_abc##_Geom_Param_Sample {                                             \
        type_c *values;                                                                           \
        uint64_t num_values;                                                                      \
        uint32_t *indices;                                                                        \
        uint64_t num_indices;                                                                     \
        enum Abc_Geometry_Scope scope;                                                            \
    };                                                                                            \
    struct Abc_Output_##nom_abc##_Geom_Param *abc_output_##type_c##_geom_param_create(            \
        struct Abc_Output_Compound_Property *parent,                                              \
        struct Abc_String name,                                                                   \
        bool is_indexed,                                                                          \
        enum Abc_Geometry_Scope scope,                                                            \
        uint64_t array_extent);                                                                   \
    void abc_output_##type_c##_geom_param_set_time_sampling(                                      \
        struct Abc_Output_##nom_abc##_Geom_Param *param, struct Abc_Time_Sample_Index index);     \
    void abc_output_##type_c##_geom_param_sample_set(                                             \
        struct Abc_Output_##nom_abc##_Geom_Param *param,                                          \
        struct Abc_Output_##nom_abc##_Geom_Param_Sample *sample);

ENUMERATE_ABC_ATTRIBUTE_TYPES(DECLARE_ABC_OUTPUT_GEOM_PARAMS)

#undef DECLARE_ABC_OUTPUT_GEOM_PARAMS

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Xform
 * \{ */

/**
 * @brief abc_output_archive_root_object_get Retourne l'objet racine de l'archive.
 */
struct Abc_Output_Xform *abc_output_archive_root_object_get(struct Abc_Output_Archive *archive);

/**
 * @brief abc_output_xform_create Crée un object de type Xform.
 * @param parent Le parent de l'objet.
 * @param nom Le nom de l'objet. Doit être unique au sein du parent.
 * @return L'objet xform créé.
 */
struct Abc_Output_Xform *abc_output_xform_create(struct Abc_Output_Xform *parent,
                                                 struct Abc_String nom,
                                                 struct Abc_Time_Sample_Index time_sample_index);

struct Abc_Output_Xform_Sample;
struct Abc_Output_Xform_Sample *abc_output_xform_sample_create(struct Abc_Output_Archive *archive);
void abc_output_xform_sample_reset(struct Abc_Output_Xform_Sample *sample);
void abc_output_xform_sample_destroy(struct Abc_Output_Xform_Sample *sample);
void abc_output_xform_sample_set_matrix(struct Abc_Output_Xform_Sample *sample, float *matrix);
void abc_output_xform_sample_set_inherits_xform(struct Abc_Output_Xform_Sample *sample,
                                                bool inherits);
void abc_output_xform_sample_set(struct Abc_Output_Xform *xform,
                                 struct Abc_Output_Xform_Sample *sample);

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Points
 * \{ */

/**
 * @brief abc_output_points_create Crée un object de type Points.
 * @param parent Le parent de l'objet.
 * @param nom Le nom de l'objet. Doit être unique au sein du parent.
 * @return L'objet points créé.
 */
struct Abc_Output_Points *abc_output_points_create(struct Abc_Output_Xform *parent,
                                                   struct Abc_String nom,
                                                   struct Abc_Time_Sample_Index time_sample_index);

struct Abc_Output_Points_Sample;
struct Abc_Output_Points_Sample *abc_output_points_sample_create(
    struct Abc_Output_Archive *archive);
void abc_output_points_sample_reset(struct Abc_Output_Points_Sample *sample);
void abc_output_points_sample_destroy(struct Abc_Output_Points_Sample *sample);
void abc_output_points_sample_positions_set(struct Abc_Output_Points_Sample *sample,
                                            float *positions,
                                            uint64_t num_positions);
void abc_output_points_sample_velocities_set(struct Abc_Output_Points_Sample *sample,
                                             float *velocities,
                                             uint64_t num_velocities);
void abc_output_points_sample_widths_set(struct Abc_Output_Points_Sample *sample,
                                         float *widths,
                                         uint64_t num_widths,
                                         enum Abc_Geometry_Scope scope);
void abc_output_points_sample_ids_set(struct Abc_Output_Points_Sample *sample,
                                      uint64_t *ids,
                                      uint64_t num_ids);
void abc_output_points_sample_set(struct Abc_Output_Points *points,
                                  struct Abc_Output_Points_Sample *sample);
void abc_output_points_sample_set_from_previous(struct Abc_Output_Points *points);

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Curves
 * \{ */

enum Abc_Curve_Periodicity {
    ABC_CURVE_PERIODICITY_NON_PERIODIC = 0,
    ABC_CURVE_PERIODICITY_PERIODIC = 1,
};

enum Abc_Curve_Type {
    ABC_CURVE_TYPE_CUBIC = 0,
    ABC_CURVE_TYPE_LINEAR = 1,
    ABC_CURVE_TYPE_VARIABLE_ORDER = 2,
};

enum Abc_Basis_Type {
    ABC_BASIS_TYPE_NO_BASIS = 0,
    ABC_BASIS_TYPE_BEZIER_BASIS = 1,
    ABC_BASIS_TYPE_B_SPLINE_BASIS = 2,
    ABC_BASIS_TYPE_CATMULLROM_BASIS = 3,
    ABC_BASIS_TYPE_HERMITE_BASIS = 4,
    ABC_BASIS_TYPE_POWER_BASIS = 5,
};

struct Abc_Output_Curves;

struct Abc_Output_Curves *abc_output_curves_create(struct Abc_Output_Xform *parent,
                                                   struct Abc_String nom,
                                                   struct Abc_Time_Sample_Index time_sample_index);

struct Abc_Output_Curves_Sample;

struct Abc_Output_Curves_Sample *abc_output_curves_sample_create(
    struct Abc_Output_Archive *archive);
void abc_output_curves_sample_reset(struct Abc_Output_Curves_Sample *sample);
void abc_output_curves_sample_destroy(struct Abc_Output_Curves_Sample *sample);
void abc_output_curves_sample_type_set(struct Abc_Output_Curves_Sample *sample,
                                       enum Abc_Curve_Type type);
void abc_output_curves_sample_wrap_set(struct Abc_Output_Curves_Sample *sample,
                                       enum Abc_Curve_Periodicity wrap);
void abc_output_curves_sample_basis_set(struct Abc_Output_Curves_Sample *sample,
                                        enum Abc_Basis_Type basis);
void abc_output_curves_sample_positions_set(struct Abc_Output_Curves_Sample *sample,
                                            float *positions,
                                            uint64_t num_positions);
void abc_output_curves_sample_position_weights_set(struct Abc_Output_Curves_Sample *sample,
                                                   float *weights,
                                                   uint64_t num_weights);
void abc_output_curves_sample_velocities_set(struct Abc_Output_Curves_Sample *sample,
                                             float *velocities,
                                             uint64_t num_velocities);
void abc_output_curves_sample_widths_set(struct Abc_Output_Curves_Sample *sample,
                                         float *widths,
                                         uint64_t num_widths,
                                         enum Abc_Geometry_Scope scope);
void abc_output_curves_sample_curves_num_vertices_set(struct Abc_Output_Curves_Sample *sample,
                                                      int *num_vertices,
                                                      uint64_t num_nun_vertices);
void abc_output_curves_sample_orders_set(struct Abc_Output_Curves_Sample *sample,
                                         uint8_t *values,
                                         uint64_t num_values);
void abc_output_curves_sample_knots_set(struct Abc_Output_Curves_Sample *sample,
                                        float *values,
                                        uint64_t num_values);
void abc_output_curves_sample_uvs_set(struct Abc_Output_Curves_Sample *sample,
                                      float *values,
                                      uint64_t num_values,
                                      enum Abc_Geometry_Scope scope);
void abc_output_curves_sample_normals_set(struct Abc_Output_Curves_Sample *sample,
                                          float *values,
                                          uint64_t num_values,
                                          enum Abc_Geometry_Scope scope);
void abc_output_curves_sample_set(struct Abc_Output_Curves *curves,
                                  struct Abc_Output_Curves_Sample *sample);
void abc_output_curves_sample_set_from_previous(struct Abc_Output_Curves *curves);

/** \} */

#ifdef __cplusplus
}
#endif
