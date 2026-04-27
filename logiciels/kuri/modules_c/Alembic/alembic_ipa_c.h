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

#ifdef __cplusplus
    operator std::string();
#endif
} Abc_String;

typedef struct Abc_Milimeters {
    double value;

#ifdef __cplusplus
    operator double()
    {
        return value;
    }
#endif
} Abc_Milimeters;

typedef struct Abc_Centimeters {
    double value;

#ifdef __cplusplus
    operator double()
    {
        return value;
    }
#endif
} Abc_Centimeters;

typedef struct Abc_Seconds {
    double value;

#ifdef __cplusplus
    operator double()
    {
        return value;
    }
#endif
} Abc_Seconds;

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

#define ENUMERATE_COLOR_TYPES(X)                                                                  \
    X(c, uint8_t)                                                                                 \
    X(f, float)

#define DECLARE_COLOR_TYPES(suffix, type)                                                         \
    typedef struct Abc_C3##suffix {                                                               \
        type x;                                                                                   \
        type y;                                                                                   \
        type z;                                                                                   \
    } Abc_C3##suffix;                                                                             \
    typedef struct Abc_C4##suffix {                                                               \
        type r;                                                                                   \
        type g;                                                                                   \
        type b;                                                                                   \
        type a;                                                                                   \
    } Abc_C4##suffix;

ENUMERATE_COLOR_TYPES(DECLARE_COLOR_TYPES)

#undef DECLARE_COLOR_TYPES

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

typedef struct Abc_Quatf {
    float r;
    Abc_V3f v;
} Abc_Quatf;

typedef struct Abc_Quatd {
    double r;
    Abc_V3d v;
} Abc_Quatd;

#define ENUMERATE_ABC_MATRIX_TYPES(X)                                                             \
    X(3, f, float)                                                                                \
    X(3, d, double)                                                                               \
    X(4, f, float)                                                                                \
    X(4, d, double)

#define DEFINE_ABC_MATRIX_TYPE(dim, suffix, data_type)                                            \
    typedef struct Abc_M##dim##dim##suffix {                                                      \
        data_type x[dim][dim];                                                                    \
    } Abc_M##dim##dim##suffix;

ENUMERATE_ABC_MATRIX_TYPES(DEFINE_ABC_MATRIX_TYPE)

#undef DEFINE_ABC_MATRIX_TYPE

// À FAIRE :
/*

typedef OTypedGeomParam<Float16TPTraits>         OHalfGeomParam;
typedef OTypedGeomParam<WstringTPTraits>         OWstringGeomParam;
typedef OTypedGeomParam<C3hTPTraits>             OC3hGeomParam;
typedef OTypedGeomParam<C4hTPTraits>             OC4hGeomParam;

 */

// Ces attributs requiers des conversions spéciales
// #define X(type_geom, type_abc_value, type_c, nom_court)
#define ENUMERATE_ABC_ATTRIBUTE_SPECIAL_UNIQUE(X)                                                 \
    X(Bool, Abc::bool_t, bool, bool)                                                              \
    X(V2s, Abc::V2s, Abc_V2s, v2s)                                                                \
    X(V2i, Abc::V2i, Abc_V2i, v2i)                                                                \
    X(V2f, Abc::V2f, Abc_V2f, v2f)                                                                \
    X(V2d, Abc::V2d, Abc_V2d, v2d)                                                                \
    X(V3s, Abc::V3s, Abc_V3s, v3s)                                                                \
    X(V3i, Abc::V3i, Abc_V3i, v3i)                                                                \
    X(V3f, Abc::V3f, Abc_V3f, v3f)                                                                \
    X(V3d, Abc::V3d, Abc_V3d, v3d)                                                                \
    X(Box2s, Abc::Box2s, Abc_Box2s, box2s)                                                        \
    X(Box2i, Abc::Box2i, Abc_Box2i, box2i)                                                        \
    X(Box2f, Abc::Box2f, Abc_Box2f, box2f)                                                        \
    X(Box2d, Abc::Box2d, Abc_Box2d, box2d)                                                        \
    X(Box3s, Abc::Box3s, Abc_Box3s, box3s)                                                        \
    X(Box3i, Abc::Box3i, Abc_Box3i, box3i)                                                        \
    X(Box3f, Abc::Box3f, Abc_Box3f, box3f)                                                        \
    X(Box3d, Abc::Box3d, Abc_Box3d, box3d)                                                        \
    X(Quatf, Abc::Quatf, Abc_Quatf, quatf)                                                        \
    X(Quatd, Abc::Quatd, Abc_Quatd, quatd)                                                        \
    X(C3c, Abc::C3c, Abc_C3c, c3c)                                                                \
    X(C3f, Abc::C3f, Abc_C3f, c3f)                                                                \
    X(C4c, Abc::C4c, Abc_C4c, c4c)                                                                \
    X(C4f, Abc::C4f, Abc_C4f, c4f)                                                                \
    X(M33f, Abc::M33f, Abc_M33f, m33f)                                                            \
    X(M33d, Abc::M33d, Abc_M33d, m33d)                                                            \
    X(M44f, Abc::M44f, Abc_M44f, m44f)                                                            \
    X(M44d, Abc::M44d, Abc_M44d, m44d)

#define ENUMERATE_ABC_ATTRIBUTE_SPECIAL(X)                                                        \
    ENUMERATE_ABC_ATTRIBUTE_SPECIAL_UNIQUE(X)                                                     \
    X(P2s, Abc::V2s, Abc_V2s, p2s)                                                                \
    X(P2i, Abc::V2i, Abc_V2i, p2i)                                                                \
    X(P2f, Abc::V2f, Abc_V2f, p2f)                                                                \
    X(P2d, Abc::V2d, Abc_V2d, p2d)                                                                \
    X(P3s, Abc::V3s, Abc_V3s, p3s)                                                                \
    X(P3i, Abc::V3i, Abc_V3i, p3i)                                                                \
    X(P3f, Abc::V3f, Abc_V3f, p3f)                                                                \
    X(P3d, Abc::V3d, Abc_V3d, p3d)                                                                \
    X(N2f, Abc::V2f, Abc_V2f, n2f)                                                                \
    X(N2d, Abc::V2d, Abc_V2d, n2d)                                                                \
    X(N3f, Abc::V3f, Abc_V3f, n3f)                                                                \
    X(N3d, Abc::V3d, Abc_V3d, n3d)

// #define X(type_geom, type_abc_value, type_c, nom_court)
#define ENUMERATE_ABC_ATTRIBUTE_TYPES(X)                                                          \
    X(Uchar, uint8_t, uint8_t, uchar)                                                             \
    X(Char, int8_t, int8_t, char)                                                                 \
    X(UInt16, uint16_t, uint16_t, uint16)                                                         \
    X(Int16, int16_t, int16_t, int16)                                                             \
    X(UInt32, uint32_t, uint32_t, uint32)                                                         \
    X(Int32, int32_t, int32_t, int32)                                                             \
    X(UInt64, uint64_t, uint64_t, uint64)                                                         \
    X(Int64, int64_t, int64_t, int64)                                                             \
    X(Float, float, float, float)                                                                 \
    X(Double, double, double, double)                                                             \
    X(String, std::string, Abc_String, string)                                                    \
    ENUMERATE_ABC_ATTRIBUTE_SPECIAL(X)

#define DECLARE_ABC_TYPED_ARRAY_SAMPLE(type_geom, type_abc_value, type_c, nom_court)              \
    struct Abc_##type_geom##_Array_Sample {                                                       \
        type_c *values;                                                                           \
        uint64_t num_values;                                                                      \
    };

ENUMERATE_ABC_ATTRIBUTE_TYPES(DECLARE_ABC_TYPED_ARRAY_SAMPLE)

#undef DECLARE_ABC_TYPED_ARRAY_SAMPLE

#define DECLARE_COMMON_SAMPLE_FONCTIONS(uppercase_name, lowercase_name)                           \
    struct Abc_Output_##uppercase_name##_Sample *abc_output_##lowercase_name##_sample_create(     \
        struct Abc_Output_Archive *archive);                                                      \
    void abc_output_##lowercase_name##_sample_reset(                                              \
        struct Abc_Output_##uppercase_name##_Sample *sample);                                     \
    void abc_output_##lowercase_name##_sample_destroy(                                            \
        struct Abc_Output_##uppercase_name##_Sample *sample);                                     \
    void abc_output_##lowercase_name##_sample_set(                                                \
        struct Abc_Output_##uppercase_name *lowercase_name,                                       \
        struct Abc_Output_##uppercase_name##_Sample *sample);                                     \
    void abc_output_##lowercase_name##_sample_set_from_previous(                                  \
        struct Abc_Output_##uppercase_name *lowercase_name);

#define DECLARE_OUTPUT_SAMPLE_SET_FUNCTION(uname, lname, snake_name, method, sample_type)         \
    void abc_output_##lname##_sample_##snake_name(                                                \
        struct Abc_Output_##uname##_Sample *lname##_sample, struct sample_type sample);

#define DECLARE_OUTPUT_SAMPLE_SCALAR_FUNCTIONS(uname, lname, snake_name, method, sample_type)     \
    void abc_output_##lname##_sample_##snake_name(                                                \
        struct Abc_Output_##uname##_Sample *lname##_sample, sample_type sample);

#define DECLARE_COMMON_OBJECT_FUNCTIONS(uname, lname)                                             \
    struct Abc_Output_Compound_Property *abc_output_##lname##_arb_geom_params_get(                \
        struct Abc_Output_##uname *lname);                                                        \
    struct Abc_Output_Compound_Property *abc_output_##lname##_user_properties_get(                \
        struct Abc_Output_##uname *lname);                                                        \
    struct Abc_MetaData *abc_output_##lname##_metadata_get(struct Abc_Output_##uname *lname);

/* ------------------------------------------------------------------------- */
/** \nom MetaData
 * \{ */

struct Abc_MetaData;

void abc_metadata_destroy(struct Abc_MetaData *metadata);

void abc_metadata_set(struct Abc_MetaData *metadata, Abc_String key, Abc_String data);
void abc_metadata_set_unique(struct Abc_MetaData *metadata, Abc_String key, Abc_String data);
void abc_metadata_append(struct Abc_MetaData *metadata, struct Abc_MetaData *source);
void abc_metadata_append_only_unique(struct Abc_MetaData *metadata, struct Abc_MetaData *source);
void abc_metadata_append_unique(struct Abc_MetaData *metadata, struct Abc_MetaData *source);

struct Abc_MetaData_Iterator;

struct Abc_MetaData_Iterator *abc_metadata_get_iterator(struct Abc_MetaData *metadata);

/* Retourne vrai si une valeur fut renseignée dans key et value.
 * Retourne faux si l'itérateur est à la fin. */
bool abc_metadata_iterator_next(struct Abc_MetaData_Iterator *iterator,
                                struct Abc_String *key,
                                struct Abc_String *value);

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
/** \nom Abc_Plain_Old_Data_Type
 * \{ */

enum Abc_Plain_Old_Data_Type {
    ABC_PLAIN_OLD_DATA_TYPE_BOOLEAN,
    ABC_PLAIN_OLD_DATA_TYPE_UINT8,
    ABC_PLAIN_OLD_DATA_TYPE_INT8,
    ABC_PLAIN_OLD_DATA_TYPE_UINT16,
    ABC_PLAIN_OLD_DATA_TYPE_INT16,
    ABC_PLAIN_OLD_DATA_TYPE_UINT32,
    ABC_PLAIN_OLD_DATA_TYPE_INT32,
    ABC_PLAIN_OLD_DATA_TYPE_UINT64,
    ABC_PLAIN_OLD_DATA_TYPE_INT64,
    ABC_PLAIN_OLD_DATA_TYPE_FLOAT16,
    ABC_PLAIN_OLD_DATA_TYPE_FLOAT32,
    ABC_PLAIN_OLD_DATA_TYPE_FLOAT64,
    ABC_PLAIN_OLD_DATA_TYPE_STRING,
    ABC_PLAIN_OLD_DATA_TYPE_WSTRING,
    ABC_PLAIN_OLD_DATA_TYPE_UNKNOWN = 127,
};

// #define X(type_geom, type_abc_value, type_c, nom_court)
#define ENUMERATE_ABC_POD_TYPE(X)                                                                 \
    X(Bool, Abc::bool_t, bool, bool)                                                              \
    X(Uchar, uint8_t, uint8_t, uchar)                                                             \
    X(Char, int8_t, int8_t, char)                                                                 \
    X(UInt16, uint16_t, uint16_t, uint16)                                                         \
    X(Int16, int16_t, int16_t, int16)                                                             \
    X(UInt32, uint32_t, uint32_t, uint32)                                                         \
    X(Int32, int32_t, int32_t, int32)                                                             \
    X(UInt64, uint64_t, uint64_t, uint64)                                                         \
    X(Int64, int64_t, int64_t, int64)                                                             \
    X(Float, float, float, float)                                                                 \
    X(Double, double, double, double)                                                             \
    X(String, std::string, Abc_String, string)

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Data_Type
 * \{ */

struct Abc_Data_Type {
    enum Abc_Plain_Old_Data_Type pod_type;
    uint8_t extent;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Property_Type
 * \{ */

enum Abc_Property_Type {
    ABC_PROPERTY_TYPE_COMPOUND = 0,
    ABC_PROPERTY_TYPE_SCALAR = 1,
    ABC_PROPERTY_TYPE_ARRAY = 2,
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Sample_Selector
 * \{ */

enum Abc_Time_Index_Type {
    ABC_TIME_INDEX_TYPE_NEAR_INDEX,
    ABC_TIME_INDEX_TYPE_FLOOR_INDEX,
    ABC_TIME_INDEX_TYPE_CEIL_INDEX,
};

struct Abc_Sample_Selector {
    int64_t requested_index;
    double requested_time;
    enum Abc_Time_Index_Type requested_time_index_type;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Property_Header
 * \{ */

struct Abc_Property_Header;
void abc_property_header_get_name(struct Abc_Property_Header *header, struct Abc_String *name);
enum Abc_Property_Type abc_property_header_get_property_type(struct Abc_Property_Header *header);
bool abc_property_header_is_scalar(struct Abc_Property_Header *header);
bool abc_property_header_is_array(struct Abc_Property_Header *header);
bool abc_property_header_is_compound(struct Abc_Property_Header *header);
bool abc_property_header_is_simple(struct Abc_Property_Header *header);
struct Abc_MetaData *abc_property_header_get_metadata(struct Abc_Property_Header *header);
void abc_property_header_get_data_type(struct Abc_Property_Header *header,
                                       struct Abc_Data_Type *r_data_type);

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Compound_Property
 * \{ */

struct Abc_Input_Compound_Property;

bool abc_input_compound_property_valid(struct Abc_Input_Compound_Property *props);

uint64_t abc_input_compound_property_get_num_properties(struct Abc_Input_Compound_Property *prop);

struct Abc_Property_Header *abc_input_compound_property_get_property_header(
    struct Abc_Input_Compound_Property *prop, uint64_t i);

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Scalar_Property
 * \{ */

struct Abc_Input_Scalar_Property;

struct Abc_Input_Scalar_Property *abc_input_compound_property_get_scalar(
    struct Abc_Input_Compound_Property *props, struct Abc_String name);
uint64_t abc_input_scalar_property_get_num_samples(struct Abc_Input_Scalar_Property *prop);
bool abc_input_scalar_property_is_constant(struct Abc_Input_Scalar_Property *prop);

#define DECLARE_SCALAR_PROPERTY_GETTER(type_geom, type_abc_value, type_c, nom_court)              \
    type_c abc_input_scalar_property_##nom_court##_get(struct Abc_Input_Scalar_Property *prop,    \
                                                       struct Abc_Sample_Selector selector);

ENUMERATE_ABC_POD_TYPE(DECLARE_SCALAR_PROPERTY_GETTER)

#undef DECLARE_SCALAR_PROPERTY_GETTER

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Object_Header
 * \{ */

struct Abc_Object_Header;

void abc_object_header_get_name(struct Abc_Object_Header *header, Abc_String *name);

void abc_object_header_get_full_name(struct Abc_Object_Header *header, Abc_String *name);

struct Abc_MetaData *abc_object_header_get_metadata(struct Abc_Object_Header *header);

#define ENUMERATE_INPUT_OBJECT_TYPES(X)                                                           \
    X(AbcGeom::IXform, Xform, xform)                                                              \
    X(AbcGeom::ISubD, SubD, subd)                                                                 \
    X(AbcGeom::IPolyMesh, PolyMesh, polymesh)                                                     \
    X(AbcGeom::ICurves, Curves, curves)                                                           \
    X(AbcGeom::IFaceSet, FaceSet, faceset)                                                        \
    X(AbcGeom::IPoints, Points, points)                                                           \
    X(AbcGeom::ILight, Light, light)                                                              \
    X(AbcGeom::INuPatch, NuPatch, nupatch)                                                        \
    X(AbcGeom::ICamera, Camera, camera)                                                           \
    X(Alembic::AbcMaterial::IMaterial, Material, material)

#define DECLARE_OBJECT_MATCHES_FUNCTIONS(type_abc, type_kuri, lname)                              \
    bool abc_object_header_matches_##lname(struct Abc_Object_Header *header);

ENUMERATE_INPUT_OBJECT_TYPES(DECLARE_OBJECT_MATCHES_FUNCTIONS)

#undef DECLARE_OBJECT_MATCHES_FUNCTIONS

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Object
 * \{ */

struct Abc_Input_Object;

union Abc_Generic_Input_Object {
    struct Abc_Input_Object *object;

#define DECLARE_TYPED_INPUT_OBJECTS(type_abc, type_kuri, lname)                                   \
    struct Abc_Input_##type_kuri *lname;

    ENUMERATE_INPUT_OBJECT_TYPES(DECLARE_TYPED_INPUT_OBJECTS)

#undef DECLARE_TYPED_INPUT_OBJECTS
};

bool abc_input_object_valid(union Abc_Generic_Input_Object object);

uint64_t abc_input_object_get_num_children(union Abc_Generic_Input_Object object);

struct Abc_Object_Header *abc_input_object_get_child_header(union Abc_Generic_Input_Object object,
                                                            uint64_t i);

struct Abc_Input_Object *abc_input_object_get_child(union Abc_Generic_Input_Object object,
                                                    struct Abc_String name);

void abc_input_object_get_full_name(union Abc_Generic_Input_Object object,
                                    struct Abc_String *name);

bool abc_input_object_is_instance_root(struct Abc_Input_Object *object);

#define DECLARE_TYPED_INPUT_OBJECTS(type_abc, type_kuri, lname)                                   \
    struct Abc_Input_##type_kuri;                                                                 \
    struct Abc_Input_##type_kuri *abc_input_##lname##_get(union Abc_Generic_Input_Object parent,  \
                                                          struct Abc_String name);                \
    struct Abc_Input_Compound_Property *abc_input_##lname##_get_arb_geom_params(                  \
        struct Abc_Input_##type_kuri *object);                                                    \
    struct Abc_Input_Compound_Property *abc_input_##lname##_get_user_properties(                  \
        struct Abc_Input_##type_kuri *object);

ENUMERATE_INPUT_OBJECT_TYPES(DECLARE_TYPED_INPUT_OBJECTS)

#undef DECLARE_TYPED_INPUT_OBJECTS

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

struct Abc_Input_Object *abc_input_archive_get_top(struct Abc_Input_Archive *archive);

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
/** \nom Abc_Output_Scalar_Property
 * \{ */

struct Abc_Output_Scalar_Property;

struct Abc_Output_Scalar_Property *abc_output_scalar_property_create(
    struct Abc_Output_Compound_Property *parent,
    struct Abc_String name,
    struct Abc_Data_Type data_type,
    struct Abc_Time_Sample_Index ts_index);

void abc_output_scalar_property_set_from_previous(struct Abc_Output_Scalar_Property *prop);

#define DECLARE_SCALAR_PROPERTY_SETTER(type_geom, type_abc_value, type_c, nom_court)              \
    void abc_output_scalar_property_##nom_court##_set(struct Abc_Output_Scalar_Property *prop,    \
                                                      type_c sample);

ENUMERATE_ABC_POD_TYPE(DECLARE_SCALAR_PROPERTY_SETTER)

#undef DECLARE_SCALAR_PROPERTY_SETTER

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Typed Abc_Output_Scalar_Property
 * \{ */

#define DECLARE_ABC_TYPED_SCALAR_PROPERTY(type_geom, type_abc_value, type_c, nom_court)           \
    struct Abc_Output_##type_geom##_Scalar_Property;                                              \
    struct Abc_Output_##type_geom##_Scalar_Property                                               \
        *abc_output_##nom_court##_scalar_property_create(                                         \
            struct Abc_Output_Compound_Property *parent, Abc_String name);                        \
    void abc_output_##nom_court##_scalar_property_set_time_sample_index(                          \
        struct Abc_Output_##type_geom##_Scalar_Property *prop,                                    \
        struct Abc_Time_Sample_Index index);                                                      \
    void abc_output_##nom_court##_scalar_property_set(                                            \
        struct Abc_Output_##type_geom##_Scalar_Property *prop, type_c value);

ENUMERATE_ABC_ATTRIBUTE_TYPES(DECLARE_ABC_TYPED_SCALAR_PROPERTY)

#undef DECLARE_ABC_TYPED_SCALAR_PROPERTY

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Array_Property
 * \{ */

struct Abc_Output_Array_Property;

struct Abc_Output_Array_Property *abc_output_array_property_create(
    struct Abc_Output_Compound_Property *parent,
    struct Abc_String name,
    struct Abc_Data_Type data_type,
    struct Abc_Time_Sample_Index ts_index);

void abc_output_array_property_set_from_previous(struct Abc_Output_Scalar_Property *prop);

#define DECLARE_ARRAY_PROPERTY_SETTER(type_geom, type_abc_value, type_c, nom_court)               \
    void abc_output_array_property_##nom_court##_set(                                             \
        struct Abc_Output_Array_Property *prop, struct Abc_##type_geom##_Array_Sample sample);

ENUMERATE_ABC_POD_TYPE(DECLARE_ARRAY_PROPERTY_SETTER)

#undef DECLARE_ARRAY_PROPERTY_SETTER

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_TYPE_Geom_Param
 * \{ */

#define DECLARE_ABC_OUTPUT_GEOM_PARAMS(type_geom, type_abc_value, type_c, nom_court)              \
    struct Abc_Output_##type_geom##_Geom_Param;                                                   \
    struct Abc_Output_##type_geom##_Geom_Param_Sample {                                           \
        type_c *values;                                                                           \
        uint64_t num_values;                                                                      \
        uint32_t *indices;                                                                        \
        uint64_t num_indices;                                                                     \
        enum Abc_Geometry_Scope scope;                                                            \
    };                                                                                            \
    struct Abc_Output_##type_geom##_Geom_Param *abc_output_##nom_court##_geom_param_create(       \
        struct Abc_Output_Compound_Property *parent,                                              \
        struct Abc_String name,                                                                   \
        bool is_indexed,                                                                          \
        enum Abc_Geometry_Scope scope,                                                            \
        uint64_t array_extent);                                                                   \
    void abc_output_##nom_court##_geom_param_set_time_sampling(                                   \
        struct Abc_Output_##type_geom##_Geom_Param *param, struct Abc_Time_Sample_Index index);   \
    void abc_output_##nom_court##_geom_param_sample_set(                                          \
        struct Abc_Output_##type_geom##_Geom_Param *param,                                        \
        struct Abc_Output_##type_geom##_Geom_Param_Sample *sample);

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

DECLARE_COMMON_OBJECT_FUNCTIONS(Xform, xform)

struct Abc_Output_Xform_Sample;
DECLARE_COMMON_SAMPLE_FONCTIONS(Xform, xform)
void abc_output_xform_sample_set_matrix(struct Abc_Output_Xform_Sample *sample, Abc_M44d *matrix);
void abc_output_xform_sample_set_inherits_xform(struct Abc_Output_Xform_Sample *sample,
                                                bool inherits);

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

DECLARE_COMMON_OBJECT_FUNCTIONS(Points, points)

// X(uname, lname, snake_name, method, sample_type)
#define ENUMERATE_POINTS_SAMPLE_INTERFACE(X)                                                      \
    X(Points, points, positions_set, setPositions, Abc_P3f_Array_Sample)                          \
    X(Points, points, velocities_set, setVelocities, Abc_V3f_Array_Sample)                        \
    X(Points, points, ids_set, setIds, Abc_UInt64_Array_Sample)                                   \
    X(Points, points, widths_set, setWidths, Abc_Output_Float_Geom_Param_Sample)

struct Abc_Output_Points_Sample;
DECLARE_COMMON_SAMPLE_FONCTIONS(Points, points)
ENUMERATE_POINTS_SAMPLE_INTERFACE(DECLARE_OUTPUT_SAMPLE_SET_FUNCTION)

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

DECLARE_COMMON_OBJECT_FUNCTIONS(Curves, curves)

// X(uname, lname, snake_name, method, sample_type)
#define ENUMERATE_CURVES_SAMPLE_INTERFACE(X)                                                      \
    X(Curves, curves, positions_set, setPositions, Abc_P3f_Array_Sample)                          \
    X(Curves, curves, velocities_set, setVelocities, Abc_V3f_Array_Sample)                        \
    X(Curves, curves, position_weights_set, setPositionWeights, Abc_Float_Array_Sample)           \
    X(Curves, curves, curves_num_vertices_set, setCurvesNumVertices, Abc_Int32_Array_Sample)      \
    X(Curves, curves, orders_set, setOrders, Abc_Uchar_Array_Sample)                              \
    X(Curves, curves, knots_set, setKnots, Abc_Float_Array_Sample)                                \
    X(Curves, curves, widths_set, setWidths, Abc_Output_Float_Geom_Param_Sample)                  \
    X(Curves, curves, uvs_set, setUVs, Abc_Output_V2f_Geom_Param_Sample)                          \
    X(Curves, curves, normals_set, setNormals, Abc_Output_N3f_Geom_Param_Sample)

struct Abc_Output_Curves_Sample;
DECLARE_COMMON_SAMPLE_FONCTIONS(Curves, curves)
ENUMERATE_CURVES_SAMPLE_INTERFACE(DECLARE_OUTPUT_SAMPLE_SET_FUNCTION)

void abc_output_curves_sample_type_set(struct Abc_Output_Curves_Sample *sample,
                                       enum Abc_Curve_Type type);
void abc_output_curves_sample_wrap_set(struct Abc_Output_Curves_Sample *sample,
                                       enum Abc_Curve_Periodicity wrap);
void abc_output_curves_sample_basis_set(struct Abc_Output_Curves_Sample *sample,
                                        enum Abc_Basis_Type basis);

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_FaceSet
 * \{ */

struct Abc_Output_FaceSet;

DECLARE_COMMON_OBJECT_FUNCTIONS(FaceSet, faceset)

// X(uname, lname, snake_name, method, sample_type)
#define ENUMERATE_FACESET_SAMPLE_INTERFACE(X)                                                     \
    X(FaceSet, faceset, faces_set, setFaces, Abc_Int32_Array_Sample)

struct Abc_Output_FaceSet_Sample;
DECLARE_COMMON_SAMPLE_FONCTIONS(FaceSet, faceset)
ENUMERATE_FACESET_SAMPLE_INTERFACE(DECLARE_OUTPUT_SAMPLE_SET_FUNCTION)

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_PolyMesh
 * \{ */

struct Abc_Output_PolyMesh;

struct Abc_Output_PolyMesh *abc_output_poly_mesh_create(
    struct Abc_Output_Xform *parent,
    struct Abc_String nom,
    struct Abc_Time_Sample_Index time_sample_index);

DECLARE_COMMON_OBJECT_FUNCTIONS(PolyMesh, polymesh)

struct Abc_Output_FaceSet *abc_output_polymesh_create_face_set(struct Abc_Output_PolyMesh *mesh,
                                                               struct Abc_String name);

void abc_output_polymesh_set_uv_source_name(struct Abc_Output_PolyMesh *mesh,
                                            struct Abc_String name);

// X(uname, lname, snake_name, method, sample_type)
#define ENUMERATE_POLYMESH_SAMPLE_INTERFACE(X)                                                    \
    X(PolyMesh, polymesh, positions_set, setPositions, Abc_P3f_Array_Sample)                      \
    X(PolyMesh, polymesh, velocities_set, setVelocities, Abc_V3f_Array_Sample)                    \
    X(PolyMesh, polymesh, face_indices_set, setFaceIndices, Abc_Int32_Array_Sample)               \
    X(PolyMesh, polymesh, face_counts_set, setFaceCounts, Abc_Int32_Array_Sample)                 \
    X(PolyMesh, polymesh, uvs_set, setUVs, Abc_Output_V2f_Geom_Param_Sample)                      \
    X(PolyMesh, polymesh, normals_set, setNormals, Abc_Output_N3f_Geom_Param_Sample)

struct Abc_Output_PolyMesh_Sample;

DECLARE_COMMON_SAMPLE_FONCTIONS(PolyMesh, polymesh)

ENUMERATE_POLYMESH_SAMPLE_INTERFACE(DECLARE_OUTPUT_SAMPLE_SET_FUNCTION)

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_SubD
 * \{ */

struct Abc_Output_SubD;

struct Abc_Output_SubD *abc_output_subd_create(struct Abc_Output_Xform *parent,
                                               struct Abc_String nom,
                                               struct Abc_Time_Sample_Index time_sample_index);

DECLARE_COMMON_OBJECT_FUNCTIONS(SubD, subd)

struct Abc_Output_FaceSet *abc_output_subd_create_face_set(struct Abc_Output_SubD *subd,
                                                           struct Abc_String name);

void abc_output_subd_set_uv_source_name(struct Abc_Output_SubD *subd, struct Abc_String name);

#define ENUMERATE_SUBD_SAMPLE_INTERFACE(X)                                                        \
    X(SubD, subd, positions_set, setPositions, Abc_P3f_Array_Sample)                              \
    X(SubD, subd, velocities_set, setVelocities, Abc_V3f_Array_Sample)                            \
    X(SubD, subd, face_indices_set, setFaceIndices, Abc_Int32_Array_Sample)                       \
    X(SubD, subd, face_counts_set, setFaceCounts, Abc_Int32_Array_Sample)                         \
    X(SubD, subd, uvs_set, setUVs, Abc_Output_V2f_Geom_Param_Sample)                              \
    X(SubD, subd, crease_indices_set, setCreaseIndices, Abc_Int32_Array_Sample)                   \
    X(SubD, subd, crease_lenghts_set, setCreaseLengths, Abc_Int32_Array_Sample)                   \
    X(SubD, subd, crease_sharpnesses_set, setCreaseSharpnesses, Abc_Float_Array_Sample)           \
    X(SubD, subd, corner_indices_set, setCornerIndices, Abc_Int32_Array_Sample)                   \
    X(SubD, subd, corner_sharpnesses_set, setCornerSharpnesses, Abc_Float_Array_Sample)           \
    X(SubD, subd, holes_set, setHoles, Abc_Int32_Array_Sample)

struct Abc_Output_SubD_Sample;

DECLARE_COMMON_SAMPLE_FONCTIONS(SubD, subd)

ENUMERATE_SUBD_SAMPLE_INTERFACE(DECLARE_OUTPUT_SAMPLE_SET_FUNCTION)

void abc_output_subd_sample_face_varying_interpolate_boundary_set(
    struct Abc_Output_SubD_Sample *sample, int value);

void abc_output_subd_sample_face_varying_propagate_corners_set(
    struct Abc_Output_SubD_Sample *sample, int value);

void abc_output_subd_sample_interpolate_boundary_set(struct Abc_Output_SubD_Sample *sample,
                                                     int value);

void abc_output_subd_sample_subdivision_scheme_set(struct Abc_Output_SubD_Sample *sample,
                                                   struct Abc_String value);

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Camera
 * \{ */

struct Abc_Output_Camera;

struct Abc_Output_Camera *abc_output_camera_create(struct Abc_Output_Xform *parent,
                                                   struct Abc_String nom,
                                                   struct Abc_Time_Sample_Index time_sample_index);

DECLARE_COMMON_OBJECT_FUNCTIONS(Camera, camera)

#define ENUMERATE_OUTPUT_CAMERA_SAMPLE_SCALAR_INTERFACE(X)                                        \
    X(Camera, camera, focal_length_set, setFocalLength, Abc_Milimeters)                           \
    X(Camera, camera, horizontal_aperture_set, setHorizontalAperture, Abc_Centimeters)            \
    X(Camera, camera, horizontal_film_offset_set, setHorizontalFilmOffset, Abc_Centimeters)       \
    X(Camera, camera, vertical_aperture_set, setVerticalAperture, Abc_Centimeters)                \
    X(Camera, camera, vertical_film_offset_set, setVerticalFilmOffset, Abc_Centimeters)           \
    X(Camera, camera, lens_squeeze_ratio_set, setLensSqueezeRatio, double)                        \
    X(Camera, camera, overscan_left_set, setOverScanLeft, double)                                 \
    X(Camera, camera, overscan_right_set, setOverScanRight, double)                               \
    X(Camera, camera, overscan_top_set, setOverScanTop, double)                                   \
    X(Camera, camera, overscan_bottom_set, setOverScanBottom, double)                             \
    X(Camera, camera, fstop_set, setFStop, double)                                                \
    X(Camera, camera, focus_distance_set, setFocusDistance, Abc_Centimeters)                      \
    X(Camera, camera, shutter_open_set, setShutterOpen, Abc_Seconds)                              \
    X(Camera, camera, shutter_close_set, setShutterClose, Abc_Seconds)                            \
    X(Camera, camera, near_clipping_plane_set, setNearClippingPlane, Abc_Centimeters)             \
    X(Camera, camera, far_clipping_plane_set, setFarClippingPlane, Abc_Centimeters)

struct Abc_Output_Camera_Sample;

DECLARE_COMMON_SAMPLE_FONCTIONS(Camera, camera)

struct Abc_Output_Camera_Sample *abc_output_camera_sample_create_window(
    struct Abc_Output_Archive *archive, double top, double bottom, double left, double right);

ENUMERATE_OUTPUT_CAMERA_SAMPLE_SCALAR_INTERFACE(DECLARE_OUTPUT_SAMPLE_SCALAR_FUNCTIONS)

/** \} */

#ifdef __cplusplus
}
#endif
