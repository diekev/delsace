/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "alembic.h"

#include <fstream>
#include <string_view>

#include "alembic_ipa_c.h"
#include "alembic_types.h"

#include "AbcKuri/alembic_archive.hh"
#include "AbcKuri/alembic_export.hh"
#include "AbcKuri/alembic_import.hh"

#include "../InterfaceCKuri/contexte_kuri.hh"

/*
    - [x] Alembic
    - [ ] OpenColorIO (reprend code de Sergey)
    - [x] OpenImageIO
    - [ ] OpenSubDiv (reprend code de Sergey)
    - [ ] PTex

    - [x] Alembic
    - [ ] Bullet
    - [ ] Cycles
    - [ ] CUDA
    - [ ] OpenColorIO (reprend code de Sergey)
    - [ ] OpenCV (ou ffmpeg)
    - [x] OpenImageIO
    - [ ] OpenSubDiv (reprend code de Sergey)
    - [ ] OpenVDB
    - [ ] OptiX
    - [ ] OSL (avec Cycles)
    - [ ] PTex
    - [ ] USD
    - [ ] Vulkan

    Pipeline rendu :
    - charge les objets
    - construction des tampons
    - déforme les positions selon les armatures
    - applique quelconque algorithme de sous-division
    - applique quelconque déplacement
    - recalcule les normaux
 */

template <typename T>
void liste_ajoute(T **tête, T *élément)
{
    élément->next = *tête;
    *tête = élément;
}

template <typename T>
void kuri_deloge_liste(ContexteKuri *ctx_kuri, T *liste)
{
    while (liste != nullptr) {
        auto next = liste->next;
        kuri_deloge(ctx_kuri, liste);
        liste = next;
    }
}

#define ABC_STRING_FROM_C_STRING(x) Abc_String{x, sizeof(x) - 1}

Abc_Attribute_Type_Descriptor *abc_get_attribute_type_descriptors(uint64_t *r_len)
{
#define DECLARE_DESCRIPTOR(type_geom, type_abc, type_c, nom_court)                                \
    {ABC_STRING_FROM_C_STRING(#type_geom),                                                        \
     ABC_STRING_FROM_C_STRING(#type_c),                                                           \
     ABC_STRING_FROM_C_STRING(#nom_court)},

    static Abc_Attribute_Type_Descriptor descriptors[] = {
        ENUMERATE_ABC_ATTRIBUTE_TYPES(DECLARE_DESCRIPTOR)};

#undef DECLARE_DESCRIPTOR

    if (r_len) {
        *r_len = sizeof(descriptors) / sizeof(descriptors[0]);
    }

    return descriptors;
}

Abc_Attribute_Type_Descriptor *abc_get_pod_type_descriptors(uint64_t *r_len)
{
#define DECLARE_DESCRIPTOR(type_geom, type_abc, type_c, nom_court)                                \
    {ABC_STRING_FROM_C_STRING(#type_geom),                                                        \
     ABC_STRING_FROM_C_STRING(#type_c),                                                           \
     ABC_STRING_FROM_C_STRING(#nom_court)},

    static Abc_Attribute_Type_Descriptor descriptors[] = {
        ENUMERATE_ABC_POD_TYPE(DECLARE_DESCRIPTOR)};

#undef DECLARE_DESCRIPTOR

    if (r_len) {
        *r_len = sizeof(descriptors) / sizeof(descriptors[0]);
    }

    return descriptors;
}

/* ------------------------------------------------------------------------- */
/** \nom Abc_String
 * \{ */

static std::string vers_std_string(struct Abc_String string)
{
    if (string.characters == nullptr) {
        return "";
    }
    return std::string(string.characters, string.size);
}

static std::string vers_std_string_ou_défaut(struct Abc_String string, std::string_view défaut)
{
    if (string.characters == nullptr) {
        return std::string(défaut.data(), défaut.size());
    }
    return std::string(string.characters, string.size);
}

Abc_String::operator std::string()
{
    return vers_std_string(*this);
}

static void vers_abc_string(Abc_String *result, const std::string &name)
{
    if (result) {
        result->characters = name.c_str();
        result->size = name.size();
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom value_converter
 * \{ */

template <typename Type_IPA>
struct value_converter {
    using Type_Abc = Type_IPA;

    static Type_Abc convert_value(Type_IPA *ptr)
    {
        return *ptr;
    }
};

template <>
struct value_converter<Abc_String> {
    using Type_Abc = std::string;

    static Type_Abc convert_value(Abc_String *ptr)
    {
        return *ptr;
    }
};

#define DECLARE_VALUE_CONVERTER(type_geom, type_abc, type_c, nom_court)                           \
    template <>                                                                                   \
    struct value_converter<type_c> {                                                              \
        using Type_Abc = type_abc;                                                                \
        static Type_Abc convert_value(type_c *ptr)                                                \
        {                                                                                         \
            return *reinterpret_cast<type_abc *>(ptr);                                            \
        }                                                                                         \
    };

ENUMERATE_ABC_ATTRIBUTE_SPECIAL_UNIQUE(DECLARE_VALUE_CONVERTER)

#undef DECLARE_VALUE_CONVERTER

template <typename Type_Abc>
struct import_value_converter {
    using Type_IPA = Type_Abc;

    static Type_IPA convert_value(Type_Abc *ptr)
    {
        return *ptr;
    }
};

template <>
struct import_value_converter<std::string> {
    using Type_IPA = Abc_String;

    static Type_IPA convert_value(std::string *ptr)
    {
        Abc_String résultat;
        vers_abc_string(&résultat, *ptr);
        return résultat;
    }
};

#define DECLARE_VALUE_CONVERTER(type_geom, type_abc, type_c, nom_court)                           \
    template <>                                                                                   \
    struct import_value_converter<type_abc> {                                                     \
        using Type_IPA = type_c;                                                                  \
        static Type_IPA convert_value(type_abc *ptr)                                              \
        {                                                                                         \
            return *reinterpret_cast<Type_IPA *>(ptr);                                            \
        }                                                                                         \
    };

ENUMERATE_ABC_ATTRIBUTE_SPECIAL_UNIQUE(DECLARE_VALUE_CONVERTER)

#undef DECLARE_VALUE_CONVERTER

/** \} */

extern "C" {

ArchiveCache *ABC_cree_archive(ContexteKuri *ctx_kuri, ContexteOuvertureArchive *ctx)
{
    return AbcKuri::cree_archive(ctx_kuri, ctx);
}

void ABC_detruit_archive(ContexteKuri *ctx, ArchiveCache *archive)
{
    AbcKuri::detruit_archive(ctx, archive);
}

void ABC_traverse_archive(ContexteKuri *ctx_kuri,
                          ArchiveCache *archive,
                          ContexteTraverseArchive *ctx)
{
    AbcKuri::traverse_archive(ctx_kuri, archive, ctx);
}

LectriceCache *ABC_cree_lectrice_cache(ContexteKuri *ctx_kuri,
                                       ArchiveCache *archive,
                                       const char *ptr_nom,
                                       size_t taille_nom)
{
    return AbcKuri::cree_lectrice_cache(ctx_kuri, archive, ptr_nom, taille_nom);
}

void ABC_detruit_lectrice(ContexteKuri *ctx_kuri, LectriceCache *lectrice)
{
    AbcKuri::detruit_lectrice(ctx_kuri, lectrice);
}

void ABC_lectrice_ajourne_donnees(LectriceCache *lectrice, void *donnees)
{
    AbcKuri::lectrice_ajourne_donnees(lectrice, donnees);
}

void ABC_lis_objet(ContexteKuri *ctx_kuri,
                   ContexteLectureCache *contexte,
                   LectriceCache *lectrice,
                   double temps)
{
    AbcKuri::lis_objet(ctx_kuri, contexte, lectrice, temps);
}

// ABC_lis_transformation

/* ABC_lis_attributs
 * - rappel_lis_tous_les_attributs
 * - rappel_nombre_attributs_requis
 * - rappel_nom_attribut_requis_index
 * - rappel_information_portée
 * - reserve_attribut_point
 * - reserve_attribut_polygone
 * - reserve_attribut_point_polygone
 *
 * machine à état pour remplir l'attribut courant ?
 *
 * ajoute_bool
 * ajoute_r32
 * ajoute_r64
 * ajoute_z8
 * ajoute_z16
 * ajoute_z32
 * ajoute_z64
 * ajoute_n8
 * ajoute_n16
 * ajoute_n32
 * ajoute_n64
 * ajoute_matrice_r64
 * ajoute_chaine
 */

// ABC_informations_temporelles_archive

/* ABC_topologie_a_change
 */

// ABC_est_constant

AutriceArchive *ABC_cree_autrice_archive(ContexteKuri *ctx_kuri,
                                         ContexteCreationArchive *ctx,
                                         ContexteEcritureCache *ctx_écriture)
{
    return AbcKuri::crée_autrice_archive(ctx_kuri, ctx, ctx_écriture);
}

void ABC_detruit_autrice(ContexteKuri *ctx, AutriceArchive *autrice)
{
    AbcKuri::détruit_autrice(ctx, autrice);
}

EcrivainCache *ABC_cree_ecrivain_cache_depuis_ref(ContexteKuri *ctx,
                                                  AutriceArchive *autrice,
                                                  LectriceCache *lectrice,
                                                  EcrivainCache *parent,
                                                  void *données)
{
    return AbcKuri::cree_ecrivain_cache_depuis_ref(ctx, autrice, lectrice, parent, données);
}

EcrivainCache *ABC_cree_ecrivain_cache(ContexteKuri *ctx,
                                       AutriceArchive *autrice,
                                       EcrivainCache *parent,
                                       const char *nom,
                                       uint64_t taille_nom,
                                       void *données,
                                       eTypeObjetAbc type_objet)
{
    return AbcKuri::cree_ecrivain_cache(
        ctx, autrice, parent, nom, taille_nom, données, type_objet);
}

EcrivainCache *ABC_cree_instance(ContexteKuri *ctx,
                                 AutriceArchive *autrice,
                                 EcrivainCache *parent,
                                 EcrivainCache *origine,
                                 const char *nom,
                                 uint64_t taille_nom)
{
    return AbcKuri::crée_instance(ctx, autrice, parent, origine, nom, taille_nom);
}

void ABC_ecris_donnees(AutriceArchive *autrice)
{
    AbcKuri::écris_données(autrice);
}

void ABC_lis_attributs(ContexteKuri *ctx_kuri,
                       LectriceCache *lectrice,
                       ConvertisseuseImportAttributs *convertisseuse,
                       double temps)
{
    AbcKuri::lis_attributs(ctx_kuri, lectrice, convertisseuse, temps);
}
}

/* ------------------------------------------------------------------------- */
/** \nom Array_Sample
 * \{ */

struct Array_Sample_Data {
    std::vector<std::string> strings{};
};

template <typename Abc_Sample_Type, typename T>
auto make_array_sample(T *values, uint64_t num_values, Array_Sample_Data *)
{
    return Abc_Sample_Type(values, num_values);
}

template <>
auto make_array_sample<AbcGeom::StringArraySample>(Abc_String *values,
                                                   uint64_t num_values,
                                                   Array_Sample_Data *sample_data)
{
    sample_data->strings.resize(num_values);
    for (uint64_t i = 0; i < num_values; i++) {
        sample_data->strings[i] = vers_std_string(*values++);
    }
    return AbcGeom::StringArraySample(sample_data->strings.data(), num_values);
}

#define MAKE_TYPED_ARRAY_SAMPLE(type_geom, type_abc_value, type_c, nom_court)                     \
    template <>                                                                                   \
    auto make_array_sample<AbcGeom::type_geom##ArraySample>(                                      \
        type_c * values, uint64_t num_values, Array_Sample_Data *)                                \
    {                                                                                             \
        return AbcGeom::type_geom##ArraySample(reinterpret_cast<type_abc_value *>(values),        \
                                               num_values);                                       \
    }

ENUMERATE_ABC_ATTRIBUTE_SPECIAL(MAKE_TYPED_ARRAY_SAMPLE)

#undef MAKE_TYPED_ARRAY_SAMPLE

#define MAKE_TYPED_SAMPLE_FROM_ARRAY_SAMPLE(type_geom, type_abc_value, type_c, nom_court)         \
    static AbcGeom::type_geom##ArraySample make_typed_sample(                                     \
        Abc_##type_geom##_Array_Sample sample, Array_Sample_Data *sample_data)                    \
    {                                                                                             \
        return make_array_sample<AbcGeom::type_geom##ArraySample>(                                \
            sample.values, sample.num_values, sample_data);                                       \
    }                                                                                             \
    static AbcGeom::O##type_geom##GeomParam::Sample make_typed_sample(                            \
        Abc_Output_##type_geom##_Geom_Param_Sample param_sample, Array_Sample_Data *sample_data)  \
    {                                                                                             \
        auto array_sample = make_array_sample<AbcGeom::type_geom##ArraySample>(                   \
            param_sample.values, param_sample.num_values, sample_data);                           \
        auto abc_scope = static_cast<AbcGeom::GeometryScope>(param_sample.scope);                 \
        if (param_sample.indices) {                                                               \
            auto indices = AbcGeom::UInt32ArraySample(param_sample.indices,                       \
                                                      param_sample.num_indices);                  \
            return AbcGeom::O##type_geom##GeomParam::Sample(array_sample, indices, abc_scope);    \
        }                                                                                         \
        auto sample = AbcGeom::O##type_geom##GeomParam::Sample(array_sample, abc_scope);          \
        return sample;                                                                            \
    }

ENUMERATE_ABC_ATTRIBUTE_TYPES(MAKE_TYPED_SAMPLE_FROM_ARRAY_SAMPLE)

#undef MAKE_TYPED_SAMPLE_FROM_ARRAY_SAMPLE

static Abc_P3f_Array_Sample get_input_array_sample(const Abc::P3fArraySamplePtr &ptr)
{
    auto résultat = Abc_P3f_Array_Sample();
    if (ptr) {
        résultat.values = reinterpret_cast<Abc_V3f *>(const_cast<Abc::V3f *>(ptr->get()));
        résultat.num_values = ptr->size();
    }
    else {
        résultat.values = nullptr;
        résultat.num_values = 0;
    }
    return résultat;
}

static Abc_V3f_Array_Sample get_input_array_sample(const Abc::V3fArraySamplePtr &ptr)
{
    auto résultat = Abc_V3f_Array_Sample();
    if (ptr) {
        résultat.values = reinterpret_cast<Abc_V3f *>(const_cast<Abc::V3f *>(ptr->get()));
        résultat.num_values = ptr->size();
    }
    else {
        résultat.values = nullptr;
        résultat.num_values = 0;
    }
    return résultat;
}

static Abc_Int32_Array_Sample get_input_array_sample(const Abc::Int32ArraySamplePtr &ptr)
{
    auto résultat = Abc_Int32_Array_Sample();
    if (ptr) {
        résultat.values = reinterpret_cast<int32_t *>(const_cast<int *>(ptr->get()));
        résultat.num_values = ptr->size();
    }
    else {
        résultat.values = nullptr;
        résultat.num_values = 0;
    }
    return résultat;
}

static Abc_Float_Array_Sample get_input_array_sample(const Abc::FloatArraySamplePtr &ptr)
{
    auto résultat = Abc_Float_Array_Sample();
    if (ptr) {
        résultat.values = reinterpret_cast<float *>(const_cast<float *>(ptr->get()));
        résultat.num_values = ptr->size();
    }
    else {
        résultat.values = nullptr;
        résultat.num_values = 0;
    }
    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom MetaData
 * \{ */

struct Abc_MetaData_Iterator {
    struct Abc_MetaData *metadata = nullptr;
    /* Pour la liste libre. */
    Abc_MetaData_Iterator *next = nullptr;
    Abc::MetaData::const_iterator current{};
    Abc::MetaData::const_iterator end{};
};

struct Abc_MetaData {
    ContexteKuri *ctx_kuri = nullptr;
    Abc::MetaData metadata{};
    Abc_MetaData_Iterator *iterators = nullptr;
};

static Abc_MetaData *make_metadata(ContexteKuri *ctx_kuri, const Abc::MetaData &metadata)
{
    auto résultat = kuri_loge<Abc_MetaData>(ctx_kuri);
    résultat->ctx_kuri = ctx_kuri;
    résultat->metadata = metadata;
    return résultat;
}

void abc_metadata_destroy(struct Abc_MetaData *metadata)
{
    if (metadata) {
        kuri_deloge_liste(metadata->ctx_kuri, metadata->iterators);
        kuri_deloge(metadata->ctx_kuri, metadata);
    }
}

void abc_metadata_set(struct Abc_MetaData *metadata, Abc_String key, Abc_String data)
{
    metadata->metadata.set(key, data);
}

void abc_metadata_set_unique(struct Abc_MetaData *metadata, Abc_String key, Abc_String data)
{
    metadata->metadata.setUnique(key, data);
}

// À FAIRE : get, getRequired

void abc_metadata_append(struct Abc_MetaData *metadata, struct Abc_MetaData *source)
{
    metadata->metadata.append(source->metadata);
}

void abc_metadata_append_only_unique(struct Abc_MetaData *metadata, struct Abc_MetaData *source)
{
    metadata->metadata.appendOnlyUnique(source->metadata);
}

void abc_metadata_append_unique(struct Abc_MetaData *metadata, struct Abc_MetaData *source)
{
    metadata->metadata.appendUnique(source->metadata);
}

struct Abc_MetaData_Iterator *abc_metadata_get_iterator(struct Abc_MetaData *metadata)
{
    if (!metadata) {
        return nullptr;
    }

    Abc_MetaData_Iterator *résultat;
    if (metadata->iterators) {
        résultat = metadata->iterators;
        metadata->iterators = metadata->iterators->next;
    }
    else {
        résultat = kuri_loge<Abc_MetaData_Iterator>(metadata->ctx_kuri);
    }
    résultat->metadata = metadata;
    résultat->next = nullptr;
    résultat->current = résultat->metadata->metadata.begin();
    résultat->end = résultat->metadata->metadata.end();
    return résultat;
}

bool abc_metadata_iterator_next(Abc_MetaData_Iterator *iterator,
                                Abc_String *key,
                                Abc_String *value)
{
    if (iterator->current == iterator->end) {
        liste_ajoute(&iterator->metadata->iterators, iterator);
        return false;
    }

    if (key) {
        key->characters = iterator->current->first.data();
        key->size = iterator->current->first.size();
    }
    if (value) {
        value->characters = iterator->current->second.data();
        value->size = iterator->current->second.size();
    }

    iterator->current++;
    return true;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Time_Sampling
 * \{ */

struct Abc_Time_Sampling {
    Abc_Time_Sampling *next = nullptr;
    Abc::TimeSamplingPtr ptr = nullptr;
};

uint64_t abc_time_sampling_get_num_stored_times(struct Abc_Time_Sampling *time_sampling)
{
    return time_sampling->ptr->getNumStoredTimes();
}

void abc_time_sampling_get_stored_times(struct Abc_Time_Sampling *time_sampling,
                                        double **r_times,
                                        uint64_t *r_num_times)
{
    if (r_times && r_num_times) {
        const std::vector<Abc::chrono_t> &stored_times = time_sampling->ptr->getStoredTimes();
        *r_times = const_cast<double *>(stored_times.data());
        *r_num_times = stored_times.size();
    }
}

void abc_time_sampling_get_time_sampling_type(struct Abc_Time_Sampling *time_sampling,
                                              struct Abc_Time_Sampling_Type *r_type)
{
    if (r_type) {
        auto type = time_sampling->ptr->getTimeSamplingType();
        r_type->time_per_cycle = type.getTimePerCycle();
        r_type->num_samples_per_cycle = type.getNumSamplesPerCycle();
    }
}

double abc_time_sampling_get_sample_time(struct Abc_Time_Sampling *time_sampling, int64_t index)
{
    return time_sampling->ptr->getSampleTime(index);
}

void abc_time_sampling_get_floor_index(struct Abc_Time_Sampling *time_sampling,
                                       double time,
                                       int64_t num_samples,
                                       struct Abc_Sample_Time_Index *r_index)
{
    if (r_index) {
        auto result = time_sampling->ptr->getFloorIndex(time, num_samples);
        r_index->index = result.first;
        r_index->time = result.second;
    }
}

void abc_time_sampling_get_ceil_index(struct Abc_Time_Sampling *time_sampling,
                                      double time,
                                      int64_t num_samples,
                                      struct Abc_Sample_Time_Index *r_index)
{
    if (r_index) {
        auto result = time_sampling->ptr->getCeilIndex(time, num_samples);
        r_index->index = result.first;
        r_index->time = result.second;
    }
}

void abc_time_sampling_get_near_index(struct Abc_Time_Sampling *time_sampling,
                                      double time,
                                      int64_t num_samples,
                                      struct Abc_Sample_Time_Index *r_index)
{
    if (r_index) {
        auto result = time_sampling->ptr->getNearIndex(time, num_samples);
        r_index->index = result.first;
        r_index->time = result.second;
    }
}

/** \} */

struct Abc_Property_Header;

struct Abc_Input_Archive {
    ContexteKuri *ctx_kuri = nullptr;
    Abc::IArchive iarchive{};

    Abc_Object_Header *headers = nullptr;
    Abc_Input_Object *objects = nullptr;
    Abc_Property_Header *prop_headers = nullptr;
    Abc_Input_Scalar_Property *scalar_props = nullptr;
    Abc_Input_Array_Property *array_props = nullptr;
    Abc_Time_Sampling *time_samplings = nullptr;
};

struct Abc_Time_Sampling *make_time_sampling(struct Abc_Input_Archive *archive,
                                             Abc::TimeSamplingPtr ptr)
{
    auto résultat = kuri_loge<Abc_Time_Sampling>(archive->ctx_kuri);
    résultat->ptr = ptr;
    liste_ajoute(&archive->time_samplings, résultat);
    return résultat;
}

/* ------------------------------------------------------------------------- */
/** \nom Abc_Sample_Selector
 * \{ */

static Abc::ISampleSelector get_sample_selector(Abc_Sample_Selector selector)
{
    // Notre version de l'énumération possède `NearIndex` à 0 pour qu'une valeur à défaut
    // possède la même valeur que Alembic.
    // Pour convertir, nous pouvons simplement faire (v + 2) % 3
    //
    // Nom   | Alembic | IPA
    // ------+---------+----
    // Floor |       0 |   1
    // Ceil  |       1 |   2
    // Near  |       2 |   0
    auto time_index_type = static_cast<Abc::ISampleSelector::TimeIndexType>(
        (selector.requested_time_index_type + 2) % 3);

    if (selector.requested_index == -1) {
        return Abc::ISampleSelector(selector.requested_time, time_index_type);
    }

    return Abc::ISampleSelector(selector.requested_index, time_index_type);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Property_Header
 * \{ */

struct Abc_Property_Header {
    const Alembic::AbcCoreAbstract::PropertyHeader &header;
    Abc_Property_Header *next;
    ContexteKuri *ctx_kuri;
};

void abc_property_header_get_name(struct Abc_Property_Header *header, struct Abc_String *name)
{
    vers_abc_string(name, header->header.getName());
}

enum Abc_Property_Type abc_property_header_get_property_type(struct Abc_Property_Header *header)
{
    return static_cast<Abc_Property_Type>(header->header.getPropertyType());
}

bool abc_property_header_is_scalar(struct Abc_Property_Header *header)
{
    return header->header.isScalar();
}

bool abc_property_header_is_array(struct Abc_Property_Header *header)
{
    return header->header.isArray();
}

bool abc_property_header_is_compound(struct Abc_Property_Header *header)
{
    return header->header.isCompound();
}

bool abc_property_header_is_simple(struct Abc_Property_Header *header)
{
    return header->header.isSimple();
}

Abc_MetaData *abc_property_header_get_metadata(struct Abc_Property_Header *header)
{
    return make_metadata(header->ctx_kuri, header->header.getMetaData());
}

static void make_abc_data_type(const AbcGeom::DataType &abc_data_type,
                               struct Abc_Data_Type *r_data_type)
{
    if (r_data_type) {
        r_data_type->pod_type = static_cast<Abc_Plain_Old_Data_Type>(abc_data_type.getPod());
        r_data_type->extent = abc_data_type.getExtent();
    }
}

void abc_property_header_get_data_type(struct Abc_Property_Header *header,
                                       struct Abc_Data_Type *r_data_type)
{
    make_abc_data_type(header->header.getDataType(), r_data_type);
}

// À FAIRE TimeSamplingPtr getTimeSampling() const

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Compound_Property
 * \{ */

struct Abc_Input_Compound_Property {
    Abc_Input_Archive *archive = nullptr;
    AbcGeom::ICompoundProperty prop{};
};

bool abc_input_compound_property_valid(struct Abc_Input_Compound_Property *props)
{
    return props && props->prop.valid();
}

uint64_t abc_input_compound_property_get_num_properties(struct Abc_Input_Compound_Property *prop)
{
    return prop->prop.getNumProperties();
}

struct Abc_Property_Header *abc_input_compound_property_get_property_header(
    struct Abc_Input_Compound_Property *prop, uint64_t i)
{
    const Alembic::AbcCoreAbstract::PropertyHeader &header = prop->prop.getPropertyHeader(i);
    auto résultat = kuri_loge<Abc_Property_Header>(prop->archive->ctx_kuri, header);
    résultat->ctx_kuri = prop->archive->ctx_kuri;
    liste_ajoute(&prop->archive->prop_headers, résultat);
    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Scalar_Property
 * \{ */

struct Abc_Input_Scalar_Property {
    AbcGeom::IScalarProperty prop{};
    Abc_Input_Scalar_Property *next = nullptr;
    std::string tampon_pour_get{};

    template <typename T>
    static T *make(Abc_Input_Archive *archive)
    {
        auto résultat = kuri_loge<T>(archive->ctx_kuri);
        liste_ajoute(&archive->scalar_props, static_cast<Abc_Input_Scalar_Property *>(résultat));
        return résultat;
    }
};

template <typename T>
void get_property_value_impl(struct Abc_Input_Scalar_Property *prop,
                             T *result,
                             struct Abc_Sample_Selector selector)
{
    prop->prop.get(result, get_sample_selector(selector));
}

template <>
void get_property_value_impl<bool>(struct Abc_Input_Scalar_Property *prop,
                                   bool *result,
                                   struct Abc_Sample_Selector selector)
{
    Abc::bool_t tmp_result;
    prop->prop.get(&tmp_result, get_sample_selector(selector));
    *result = tmp_result;
}

template <>
void get_property_value_impl<Abc_String>(struct Abc_Input_Scalar_Property *prop,
                                         Abc_String *result,
                                         struct Abc_Sample_Selector selector)
{
    prop->prop.get(&prop->tampon_pour_get, get_sample_selector(selector));
    vers_abc_string(result, prop->tampon_pour_get);
}

#define DECLARE_ABC_TYPED_SCALAR_PROPERTY(type_geom, type_abc_value, type_c, nom_court)           \
    struct Abc_Input_##type_geom##_Property : public Abc_Input_Scalar_Property {};                \
    struct Abc_Input_##type_geom##_Property *abc_input_##nom_court##_property(                    \
        struct Abc_Input_Compound_Property *parent, Abc_String name)                              \
    {                                                                                             \
        auto résultat = Abc_Input_Scalar_Property::make<Abc_Input_##type_geom##_Property>(        \
            parent->archive);                                                                     \
        résultat->prop = AbcGeom::IScalarProperty(parent->prop, name);                            \
        return résultat;                                                                          \
    }                                                                                             \
    void abc_input_##nom_court##_property_get(struct Abc_Input_##type_geom##_Property *prop,      \
                                              type_c *result,                                     \
                                              struct Abc_Sample_Selector selector)                \
    {                                                                                             \
        return get_property_value_impl<type_c>(prop, result, selector);                           \
    }

ENUMERATE_ABC_ATTRIBUTE_TYPES(DECLARE_ABC_TYPED_SCALAR_PROPERTY)

#undef DECLARE_ABC_TYPED_SCALAR_PROPERTY

uint64_t abc_input_scalar_property_get_num_samples(union Abc_Generic_Input_Scalar_Property prop)
{
    return prop.prop->prop.getNumSamples();
}

bool abc_input_scalar_property_is_constant(union Abc_Generic_Input_Scalar_Property prop)
{
    return prop.prop->prop.isConstant();
}

bool abc_input_scalar_property_valid(union Abc_Generic_Input_Scalar_Property prop)
{
    return prop.prop->prop.valid();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Array_Property
 * \{ */

struct Abc_Input_Array_Property {
    Abc_Input_Compound_Property *parent = nullptr;
    Abc_Input_Array_Property *next = nullptr;
    AbcGeom::IArrayProperty prop{};
    Array_Sample_Data sample_data{};

    virtual ~Abc_Input_Array_Property() = default;
};

template <typename T>
T *make_input_array_prop(Abc_Input_Archive *archive)
{
    auto résultat = kuri_loge<T>(archive->ctx_kuri);
    liste_ajoute(&archive->array_props, static_cast<Abc_Input_Array_Property *>(résultat));
    return résultat;
}

#define DEFINE_ABC_TYPED_ARRAY_PROPERTY(type_geom, type_abc_value, type_c, nom_court)             \
    struct Abc_Input_##type_geom##_Array_Property : public Abc_Input_Array_Property {             \
        Abc::I##type_geom##ArrayProperty typed_prop{};                                            \
    };

ENUMERATE_ABC_ATTRIBUTE_TYPES(DEFINE_ABC_TYPED_ARRAY_PROPERTY)

#undef DEFINE_ABC_TYPED_ARRAY_PROPERTY

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Geom_Param
 * \{ */

struct Abc_Input_Geom_Param {
    Abc_Input_Archive *archive = nullptr;
};

#define DEFINE_INPUT_GEOM_PARAM(type_geom, type_abc_value, type_c, nom_court)                     \
    struct Abc_Input_##type_geom##_Geom_Param : public Abc_Input_Geom_Param {                     \
        AbcGeom::I##type_geom##GeomParam param{};                                                 \
        AbcGeom::I##type_geom##GeomParam::Sample sample{};                                        \
    };                                                                                            \
    uint64_t abc_input_##nom_court##_geom_param_get_num_samples(                                  \
        struct Abc_Input_##type_geom##_Geom_Param *param)                                         \
    {                                                                                             \
        return param->param.getNumSamples();                                                      \
    }                                                                                             \
    void abc_input_##nom_court##_geom_param_get_data_type(                                        \
        struct Abc_Input_##type_geom##_Geom_Param *param, struct Abc_Data_Type *r_data_type)      \
    {                                                                                             \
        make_abc_data_type(param->param.getDataType(), r_data_type);                              \
    }                                                                                             \
    uint64_t abc_input_##nom_court##_geom_param_get_array_extent(                                 \
        struct Abc_Input_##type_geom##_Geom_Param *param)                                         \
    {                                                                                             \
        return param->param.getArrayExtent();                                                     \
    }                                                                                             \
    bool abc_input_##nom_court##_geom_param_is_indexed(                                           \
        struct Abc_Input_##type_geom##_Geom_Param *param)                                         \
    {                                                                                             \
        return param->param.isIndexed();                                                          \
    }                                                                                             \
    enum Abc_Geometry_Scope abc_input_##nom_court##_geom_param_get_scope(                         \
        struct Abc_Input_##type_geom##_Geom_Param *param)                                         \
    {                                                                                             \
        return static_cast<Abc_Geometry_Scope>(param->param.getScope());                          \
    }                                                                                             \
    void abc_input_##nom_court##_geom_param_get_name(                                             \
        struct Abc_Input_##type_geom##_Geom_Param *param, struct Abc_String *r_name)              \
    {                                                                                             \
        vers_abc_string(r_name, param->param.getName());                                          \
    }                                                                                             \
    struct Abc_Time_Sampling *abc_input_##nom_court##_geom_param_get_time_sampling(               \
        struct Abc_Input_##type_geom##_Geom_Param *param)                                         \
    {                                                                                             \
        return make_time_sampling(param->archive, param->param.getTimeSampling());                \
    }                                                                                             \
    struct Abc_MetaData *abc_input_##nom_court##_geom_param_get_metadata(                         \
        Abc_Input_##type_geom##_Geom_Param *param)                                                \
    {                                                                                             \
        return make_metadata(param->archive->ctx_kuri, param->param.getMetaData());               \
    }                                                                                             \
    void abc_input_##nom_court##_geom_param_get_indexed(                                          \
        struct Abc_Input_##type_geom##_Geom_Param *param,                                         \
        struct Abc_Input_##type_geom##_Geom_Param_Sample *sample,                                 \
        struct Abc_Sample_Selector selector)                                                      \
    {                                                                                             \
        param->param.getIndexed(param->sample, get_sample_selector(selector));                    \
    }                                                                                             \
    void abc_input_##nom_court##_geom_param_get_expanded(                                         \
        struct Abc_Input_##type_geom##_Geom_Param *param,                                         \
        struct Abc_Input_##type_geom##_Geom_Param_Sample *sample,                                 \
        struct Abc_Sample_Selector selector)                                                      \
    {                                                                                             \
        param->param.getExpanded(param->sample, get_sample_selector(selector));                   \
    }

ENUMERATE_ABC_ATTRIBUTE_TYPES(DEFINE_INPUT_GEOM_PARAM)

#undef DEFINE_INPUT_GEOM_PARAM

/*

 Abc::ICompoundProperty getParent() const;

 const AbcA::PropertyHeader &getHeader() const;

 */

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Object_Header
 * \{ */

struct Abc_Object_Header {
    const AbcGeom::ObjectHeader &header;
    Abc_Object_Header *next = nullptr;
    ContexteKuri *ctx_kuri = nullptr;
};

void abc_object_header_get_name(struct Abc_Object_Header *header, Abc_String *name)
{
    vers_abc_string(name, header->header.getName());
}

void abc_object_header_get_full_name(struct Abc_Object_Header *header, Abc_String *name)
{
    vers_abc_string(name, header->header.getFullName());
}

Abc_MetaData *abc_object_header_get_metadata(struct Abc_Object_Header *header)
{
    return make_metadata(header->ctx_kuri, header->header.getMetaData());
}

#define DECLARE_OBJECT_MATCHES_FUNCTIONS(type_abc, type_kuri, lname)                              \
    bool abc_object_header_matches_##lname(struct Abc_Object_Header *header)                      \
    {                                                                                             \
        return type_abc::matches(header->header);                                                 \
    }

ENUMERATE_INPUT_OBJECT_TYPES(DECLARE_OBJECT_MATCHES_FUNCTIONS)

#undef DECLARE_OBJECT_MATCHES_FUNCTIONS

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Object
 * \{ */

struct Abc_Input_Object {
    Abc_Input_Archive *archive = nullptr;
    Abc_Input_Object *next = nullptr;
    AbcGeom::IObject untyped_object{};

    Abc_Input_Compound_Property arb_geom_params{};
    Abc_Input_Compound_Property user_properties{};
    Abc_MetaData metadata_{};

    bool arb_geom_params_initialized = false;
    bool user_properties_initialized = false;
    bool metadata_initialized = false;

    std::vector<std::string> face_set_names_std_string{};
    std::vector<Abc_String> face_set_names_abc_string{};
};

#define DECLARE_TYPED_INPUT_OBJECTS(type_abc, type_kuri, lname)                                   \
    struct Abc_Input_##type_kuri : public Abc_Input_Object {                                      \
        type_abc typed_object{};                                                                  \
    };

ENUMERATE_INPUT_OBJECT_TYPES(DECLARE_TYPED_INPUT_OBJECTS)

#undef DECLARE_TYPED_INPUT_OBJECTS
template <typename T>
T *make_object(Abc_Input_Archive *archive)
{
    auto résultat = kuri_loge<T>(archive->ctx_kuri);
    résultat->archive = archive;
    liste_ajoute(&résultat->archive->objects, static_cast<Abc_Input_Object *>(résultat));
    return résultat;
}

bool abc_input_object_valid(Abc_Generic_Input_Object object)
{
    return object.object && object.object->untyped_object.valid();
}

uint64_t abc_input_object_get_num_children(Abc_Generic_Input_Object object)
{
    return object.object->untyped_object.getNumChildren();
}

struct Abc_Object_Header *abc_input_object_get_child_header(Abc_Generic_Input_Object object,
                                                            uint64_t i)
{
    const AbcGeom::ObjectHeader &header = object.object->untyped_object.getChildHeader(i);
    auto résultat = kuri_loge<Abc_Object_Header>(object.object->archive->ctx_kuri, header);
    résultat->ctx_kuri = object.object->archive->ctx_kuri;
    liste_ajoute(&object.object->archive->headers, résultat);
    return résultat;
}

struct Abc_Input_Object *abc_input_object_get_child(Abc_Generic_Input_Object object,
                                                    struct Abc_String name)
{
    auto résultat = make_object<Abc_Input_Object>(object.object->archive);
    résultat->untyped_object = object.object->untyped_object.getChild(name);
    return résultat;
}

void abc_input_object_get_full_name(Abc_Generic_Input_Object object, struct Abc_String *name)
{
    vers_abc_string(name, object.object->untyped_object.getFullName());
}

bool abc_input_object_is_instance_root(struct Abc_Input_Object *object)
{
    return object->untyped_object.isInstanceRoot();
}

struct Abc_Input_Visibility_Property : public Abc_Input_Scalar_Property {};

Abc_Input_Visibility_Property *abc_input_object_get_visibility_property(
    Abc_Generic_Input_Object object)
{
    auto résultat = Abc_Input_Scalar_Property::make<Abc_Input_Visibility_Property>(
        object.object->archive);
    résultat->prop = Alembic::AbcGeom::GetVisibilityProperty(object.object->untyped_object);
    return résultat;
}

Abc_Object_Visibility abc_input_visibility_property_get(Abc_Input_Visibility_Property *prop,
                                                        Abc_Sample_Selector selector)
{
    int8_t valeur;
    prop->prop.get(&valeur, get_sample_selector(selector));
    return static_cast<Abc_Object_Visibility>(valeur);
}

template <typename TypedObject>
Abc_Input_Compound_Property *get_arb_geom_params(TypedObject *object)
{
    if (object->arb_geom_params_initialized == false) {
        object->arb_geom_params.prop = object->typed_object.getSchema().getArbGeomParams();
        object->arb_geom_params.archive = object->archive;
        object->arb_geom_params_initialized = true;
    }
    return &object->arb_geom_params;
}

static Abc_Input_Compound_Property *get_arb_geom_params(Abc_Input_Material *object)
{
    return &object->arb_geom_params;
}

template <typename TypedObject>
Abc_Input_Compound_Property *get_user_properties(TypedObject *object)
{
    if (object->user_properties_initialized == false) {
        object->user_properties.prop = object->typed_object.getSchema().getUserProperties();
        object->user_properties.archive = object->archive;
        object->user_properties_initialized = true;
    }
    return &object->user_properties;
}

static Abc_Input_Compound_Property *get_user_properties(Abc_Input_Material *object)
{
    return &object->user_properties;
}

#define DECLARE_TYPED_INPUT_OBJECTS(type_abc, type_kuri, lname)                                   \
    Abc_Input_##type_kuri *abc_input_##lname##_get(Abc_Generic_Input_Object parent,               \
                                                   Abc_String name)                               \
    {                                                                                             \
        auto résultat = make_object<Abc_Input_##type_kuri>(parent.object->archive);               \
        résultat->typed_object = type_abc(parent.object->untyped_object, name);                   \
        résultat->untyped_object = résultat->typed_object;                                        \
        return résultat;                                                                          \
    }                                                                                             \
    Abc_Input_Compound_Property *abc_input_##lname##_get_arb_geom_params(                         \
        Abc_Input_##type_kuri *object)                                                            \
    {                                                                                             \
        return get_arb_geom_params(object);                                                       \
    }                                                                                             \
    Abc_Input_Compound_Property *abc_input_##lname##_get_user_properties(                         \
        Abc_Input_##type_kuri *object)                                                            \
    {                                                                                             \
        return get_user_properties(object);                                                       \
    }

ENUMERATE_INPUT_OBJECT_TYPES(DECLARE_TYPED_INPUT_OBJECTS)

#undef DECLARE_TYPED_INPUT_OBJECTS

/** \} */

#define DEFINE_INPUT_SAMPLE_SCALAR_GET_FUNCTION(uname, lname, snake_name, method, sample_type)    \
    sample_type abc_input_##lname##_sample_##snake_name(                                          \
        struct Abc_Input_##uname##_Sample *lname##_sample)                                        \
    {                                                                                             \
        return lname##_sample->sample.method();                                                   \
    }

#define DEFINE_INPUT_SAMPLE_ARRAY_GET_FUNCTION(uname, lname, snake_name, method, sample_type)     \
    sample_type abc_input_##lname##_sample_##snake_name(                                          \
        struct Abc_Input_##uname##_Sample *lname##_sample)                                        \
    {                                                                                             \
        auto ptr = lname##_sample->sample.method();                                               \
        return get_input_array_sample(ptr);                                                       \
    }

#define DEFINE_COMMON_INPUT_SAMPLE_FUNCTIONS(uname, lname)                                        \
    struct Abc_Time_Sampling *abc_input_##lname##_get_time_sampling(                              \
        struct Abc_Input_##uname *lname)                                                          \
    {                                                                                             \
        return make_time_sampling(lname->archive,                                                 \
                                  lname->typed_object.getSchema().getTimeSampling());             \
    }                                                                                             \
    uint64_t abc_input_##lname##_get_num_samples(struct Abc_Input_##uname *lname)                 \
    {                                                                                             \
        auto résultat = lname->typed_object.getSchema().getNumSamples();                          \
        return résultat;                                                                          \
    }                                                                                             \
    struct Abc_Input_##uname##_Sample *abc_input_##lname##_get_sample(                            \
        struct Abc_Input_##uname *lname, struct Abc_Sample_Selector selector)                     \
    {                                                                                             \
        auto résultat = kuri_loge<Abc_Input_##uname##_Sample>(lname->archive->ctx_kuri);          \
        résultat->archive = lname->archive;                                                       \
        lname->typed_object.getSchema().get(résultat->sample, get_sample_selector(selector));     \
        return résultat;                                                                          \
    }                                                                                             \
    void abc_input_##lname##_sample_destroy(struct Abc_Input_##uname##_Sample *sample)            \
    {                                                                                             \
        if (sample) {                                                                             \
            kuri_deloge(sample->archive->ctx_kuri, sample);                                       \
        }                                                                                         \
    }                                                                                             \
    void abc_input_##lname##_sample_get_self_bounds(                                              \
        struct Abc_Input_##uname##_Sample *lname##_sample, Abc_Box3d *r_box)                      \
    {                                                                                             \
        auto résultat = lname##_sample->sample.getSelfBounds();                                   \
        *r_box = import_value_converter<Abc::Box3d>::convert_value(&résultat);                    \
    }

/* ------------------------------------------------------------------------- */
/** \nom Face sets extraction.
 * \{ */

template <typename Input_Schema_Object_Type>
static void abc_input_object_schema_get_face_set_names(Input_Schema_Object_Type *object,
                                                       Abc_String **r_names,
                                                       uint64_t *r_count)
{
    if (object->face_set_names_std_string.empty()) {
        object->typed_object.getSchema().getFaceSetNames(object->face_set_names_std_string);

        object->face_set_names_abc_string.resize(object->face_set_names_std_string.size());

        auto strings = object->face_set_names_abc_string.data();
        auto num_strings = object->face_set_names_abc_string.size();
        for (auto i = 0ul; i < num_strings; i++) {
            vers_abc_string(strings++, object->face_set_names_std_string[i]);
        }
    }

    *r_names = object->face_set_names_abc_string.data();
    *r_count = object->face_set_names_abc_string.size();
}

template <typename Input_Schema_Object_Type>
struct Abc_Input_FaceSet *abc_input_object_schema_get_face_set(Input_Schema_Object_Type *object,
                                                               Abc_String face_set_name)
{
    Abc_Input_FaceSet *résultat = make_object<Abc_Input_FaceSet>(object->archive);
    résultat->typed_object = object->typed_object.getSchema().getFaceSet(face_set_name);
    résultat->untyped_object = résultat->typed_object;
    return résultat;
}

template <typename Input_Schema_Object_Type>
static bool abc_input_object_schema_has_face_set(Input_Schema_Object_Type *object,
                                                 Abc_String face_set_name)
{
    return object->typed_object.getSchema().hasFaceSet(face_set_name);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_PolyMesh_Sample
 * \{ */

struct Abc_Input_PolyMesh_Sample {
    Abc_Input_Archive *archive = nullptr;
    AbcGeom::IPolyMeshSchema::Sample sample{};
};

DEFINE_COMMON_INPUT_SAMPLE_FUNCTIONS(PolyMesh, polymesh)

void abc_input_polymesh_schema_get_face_set_names(struct Abc_Input_PolyMesh *polymesh,
                                                  Abc_String **r_names,
                                                  uint64_t *r_count)
{
    abc_input_object_schema_get_face_set_names(polymesh, r_names, r_count);
}

struct Abc_Input_FaceSet *abc_input_polymesh_schema_get_face_set(
    struct Abc_Input_PolyMesh *polymesh, Abc_String face_set_name)
{
    return abc_input_object_schema_get_face_set(polymesh, face_set_name);
}

bool abc_input_polymesh_schema_has_face_set(struct Abc_Input_PolyMesh *polymesh,
                                            Abc_String face_set_name)
{
    return abc_input_object_schema_has_face_set(polymesh, face_set_name);
}

DEFINE_POLYMESH_SAMPLE_ARRAY_GET_FUNCTIONS(DEFINE_INPUT_SAMPLE_ARRAY_GET_FUNCTION)

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_SubD_Sample
 * \{ */

struct Abc_Input_SubD_Sample {
    Abc_Input_Archive *archive = nullptr;
    AbcGeom::ISubDSchema::Sample sample{};
    std::string subdivision_scheme{};
};

DEFINE_COMMON_INPUT_SAMPLE_FUNCTIONS(SubD, subd)

void abc_input_subd_schema_get_face_set_names(struct Abc_Input_SubD *subd,
                                              Abc_String **r_names,
                                              uint64_t *r_count)
{
    abc_input_object_schema_get_face_set_names(subd, r_names, r_count);
}

struct Abc_Input_FaceSet *abc_input_subd_schema_get_face_set(struct Abc_Input_SubD *subd,
                                                             Abc_String face_set_name)
{
    return abc_input_object_schema_get_face_set(subd, face_set_name);
}

bool abc_input_subd_schema_has_face_set(struct Abc_Input_SubD *subd, Abc_String face_set_name)
{
    return abc_input_object_schema_has_face_set(subd, face_set_name);
}

DEFINE_SUBD_SAMPLE_SCALAR_GET_FUNCTION(DEFINE_INPUT_SAMPLE_SCALAR_GET_FUNCTION)
DEFINE_SUBD_SAMPLE_ARRAY_GET_FUNCTIONS(DEFINE_INPUT_SAMPLE_ARRAY_GET_FUNCTION)

Abc_String abc_input_subd_sample_get_subdivision_scheme(struct Abc_Input_SubD_Sample *subd_sample)
{
    subd_sample->subdivision_scheme = subd_sample->sample.getSubdivisionScheme();

    Abc_String résultat;
    vers_abc_string(&résultat, subd_sample->subdivision_scheme);
    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_FaceSet_Sample
 * \{ */

struct Abc_Input_FaceSet_Sample {
    Abc_Input_Archive *archive = nullptr;
    AbcGeom::IFaceSetSchema::Sample sample{};
};

DEFINE_COMMON_INPUT_SAMPLE_FUNCTIONS(FaceSet, face_set)

DEFINE_FACE_SET_SAMPLE_ARRAY_GET_FUNCTIONS(DEFINE_INPUT_SAMPLE_ARRAY_GET_FUNCTION)

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Archive
 * \{ */

struct Abc_Input_Archive *abc_input_archive_create(ContexteKuri *ctx_kuri,
                                                   struct Abc_String *chemins,
                                                   uint64_t nombre_de_chemins)
{
    if (nombre_de_chemins == 0) {
        return nullptr;
    }

    std::vector<std::string> strings_chemins;
    std::string filename;
    for (size_t i = 0; i < nombre_de_chemins; ++i) {
        filename = *chemins++;
        strings_chemins.push_back(filename);
    }

    Abc::IArchive iarchive;

    try {
        Alembic::AbcCoreOgawa::ReadArchive archive_reader;
        iarchive = Abc::IArchive(
            archive_reader(filename), Abc::kWrapExisting, Abc::ErrorHandler::kThrowPolicy);
    }
    catch (const Abc::Exception &e) {
        std::cerr << e.what() << '\n';

        /* Inspect the file to see whether it's actually a HDF5 file. */
        char header[4]; /* char(0x89) + "HDF" */
        std::ifstream the_file(filename.c_str(), std::ios::in | std::ios::binary);
        if (!the_file) {
            std::cerr << "Unable to open " << filename << std::endl;
        }
        else if (!the_file.read(header, sizeof(header))) {
            std::cerr << "Unable to read from " << filename << std::endl;
        }
        else if (strncmp(header + 1, "HDF", 3) != 0) {
            std::cerr << filename << " has an unknown file format, unable to read." << std::endl;
        }
        else {
            std::cerr << filename << " is in the obsolete HDF5 format, unable to read."
                      << std::endl;
        }

        if (the_file.is_open()) {
            the_file.close();
        }
    }

    if (!iarchive.valid()) {
        return nullptr;
    }

    auto résultat = kuri_loge<Abc_Input_Archive>(ctx_kuri);
    résultat->iarchive = iarchive;
    résultat->ctx_kuri = ctx_kuri;
    return résultat;
}

void abc_input_archive_destroy(struct Abc_Input_Archive *archive)
{
    if (archive) {
        kuri_deloge_liste(archive->ctx_kuri, archive->headers);
        kuri_deloge_liste(archive->ctx_kuri, archive->objects);
        kuri_deloge_liste(archive->ctx_kuri, archive->prop_headers);
        kuri_deloge_liste(archive->ctx_kuri, archive->scalar_props);
        kuri_deloge_liste(archive->ctx_kuri, archive->array_props);
        kuri_deloge(archive->ctx_kuri, archive);
    }
}

Abc_MetaData *abc_input_archive_get_metadata(Abc_Input_Archive *archive)
{
    return make_metadata(archive->ctx_kuri, archive->iarchive.getTop().getMetaData());
}

Abc_Input_Object *abc_input_archive_get_top(Abc_Input_Archive *archive)
{
    auto résultat = make_object<Abc_Input_Object>(archive);
    résultat->untyped_object = archive->iarchive.getTop();
    return résultat;
}

void abc_input_archive_get_start_and_end_time(struct Abc_Input_Archive *archive,
                                              double *r_start_time,
                                              double *r_end_time)
{
    double start_time;
    double end_time;
    Alembic::Abc::GetArchiveStartAndEndTime(archive->iarchive, start_time, end_time);
    if (r_start_time) {
        *r_start_time = start_time;
    }
    if (r_end_time) {
        *r_end_time = end_time;
    }
}

uint32_t abc_input_archive_get_num_time_sampling(struct Abc_Input_Archive *archive)
{
    return archive->iarchive.getNumTimeSamplings();
}

/** \} */

struct Abc_Output_Object;

struct Abc_Output_Archive {
    ContexteKuri *ctx_kuri = nullptr;
    Abc::OArchive *archive = nullptr;

    Abc_Output_Xform *racine = nullptr;

    Abc_Output_Object *objects = nullptr;

    Abc_Output_Compound_Property *compound_props = nullptr;
    Abc_Output_Scalar_Property *scalar_props = nullptr;
    Abc_Output_Array_Property *array_props = nullptr;
};

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Compound_Property
 * \{ */

struct Abc_Output_Compound_Property {
    Abc_Output_Archive *archive;
    AbcGeom::OCompoundProperty prop;
    Abc_Output_Compound_Property *next;
};

Abc_Output_Compound_Property *make_output_compound_property(Abc_Output_Archive *archive)
{
    auto résultat = kuri_loge<Abc_Output_Compound_Property>(archive->ctx_kuri);
    résultat->archive = archive;
    liste_ajoute(&résultat->archive->compound_props, résultat);
    return résultat;
}

Abc_Output_Compound_Property *abc_output_compound_property_create(
    Abc_Output_Compound_Property *parent, Abc_String name)
{
    auto résultat = make_output_compound_property(parent->archive);
    résultat->prop = AbcGeom::OCompoundProperty(parent->prop, name);
    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Scalar_Property
 * \{ */

struct Abc_Output_Scalar_Property {
    Abc_Output_Compound_Property *parent = nullptr;
    Abc_Output_Scalar_Property *next = nullptr;
    AbcGeom::OScalarProperty prop{};

    virtual ~Abc_Output_Scalar_Property() = default;
};

template <typename T>
T *make_output_scalar_prop(Abc_Output_Archive *archive, Abc_Output_Compound_Property *parent)
{
    auto résultat = kuri_loge<T>(archive->ctx_kuri);
    résultat->parent = parent;
    liste_ajoute(&archive->scalar_props, static_cast<Abc_Output_Scalar_Property *>(résultat));
    return résultat;
}

void abc_output_property_set_time_sample_index(union Abc_Generic_Output_Scalar_Property prop,
                                               struct Abc_Time_Sample_Index index)
{
    prop.prop->prop.setTimeSampling(index.value);
}

void abc_output_property_set_from_previous(union Abc_Generic_Output_Scalar_Property prop)
{
    prop.prop->prop.setFromPrevious();
}

#define DEFINE_ABC_TYPED_SCALAR_PROPERTY(type_geom, type_abc_value, type_c, nom_court)            \
    struct Abc_Output_##type_geom##_Property : public Abc_Output_Scalar_Property {                \
        Abc::O##type_geom##Property typed_prop{};                                                 \
    };                                                                                            \
    Abc_Output_##type_geom##_Property *abc_output_##nom_court##_property_create(                  \
        Abc_Output_Compound_Property *parent, Abc_String name)                                    \
    {                                                                                             \
        auto résultat = make_output_scalar_prop<Abc_Output_##type_geom##_Property>(               \
            parent->archive, parent);                                                             \
        résultat->typed_prop = Abc::O##type_geom##Property(parent->prop, name);                   \
        résultat->prop = résultat->typed_prop;                                                    \
        return résultat;                                                                          \
    }                                                                                             \
    void abc_output_##nom_court##_property_set(Abc_Output_##type_geom##_Property *prop,           \
                                               type_c *value)                                     \
    {                                                                                             \
        using value_conv = value_converter<type_c>;                                               \
        type_abc_value sample_value = value_conv::convert_value(value);                           \
        prop->typed_prop.set(sample_value);                                                       \
    }

ENUMERATE_ABC_ATTRIBUTE_TYPES(DEFINE_ABC_TYPED_SCALAR_PROPERTY)

#undef DEFINE_ABC_TYPED_SCALAR_PROPERTY

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Array_Property
 * \{ */

struct Abc_Output_Array_Property {
    Abc_Output_Compound_Property *parent = nullptr;
    Abc_Output_Array_Property *next = nullptr;
    AbcGeom::OArrayProperty prop{};
    Array_Sample_Data sample_data{};

    virtual ~Abc_Output_Array_Property() = default;
};

template <typename T>
T *make_output_array_prop(Abc_Output_Compound_Property *parent)
{
    auto résultat = kuri_loge<T>(parent->archive->ctx_kuri);
    résultat->parent = parent;
    liste_ajoute(&parent->archive->array_props,
                 static_cast<Abc_Output_Array_Property *>(résultat));
    return résultat;
}

void abc_output_array_property_set_from_previous(Abc_Generic_Output_Array_Property prop)
{
    prop.prop->prop.setFromPrevious();
}

void abc_output_array_property_set_time_sample_index(Abc_Generic_Output_Array_Property prop,
                                                     Abc_Time_Sample_Index index)
{
    prop.prop->prop.setTimeSampling(index.value);
}

#define DEFINE_ABC_TYPED_ARRAY_PROPERTY(type_geom, type_abc_value, type_c, nom_court)             \
    struct Abc_Output_##type_geom##_Array_Property : public Abc_Output_Array_Property {           \
        Abc::O##type_geom##ArrayProperty typed_prop{};                                            \
    };                                                                                            \
    Abc_Output_##type_geom##_Array_Property *abc_output_##nom_court##_array_property_create(      \
        Abc_Output_Compound_Property *parent, Abc_String name)                                    \
    {                                                                                             \
        auto résultat = make_output_array_prop<Abc_Output_##type_geom##_Array_Property>(parent);  \
        résultat->typed_prop = Abc::O##type_geom##ArrayProperty(parent->prop, name);              \
        résultat->prop = résultat->typed_prop;                                                    \
        return résultat;                                                                          \
    }                                                                                             \
    void abc_output_##nom_court##_array_property_set(                                             \
        Abc_Output_##type_geom##_Array_Property *prop, Abc_##type_geom##_Array_Sample sample)     \
    {                                                                                             \
        auto array_sample = make_typed_sample(sample, &prop->sample_data);                        \
        prop->typed_prop.set(array_sample);                                                       \
    }

ENUMERATE_ABC_ATTRIBUTE_TYPES(DEFINE_ABC_TYPED_ARRAY_PROPERTY)

#undef DEFINE_ABC_TYPED_ARRAY_PROPERTY

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Typed_Geom_Param
 * \{ */

#define DEFINE_ABC_OUTPUT_GEOM_PARAMS(type_geom, type_abc_value, type_c, nom_court)               \
    struct Abc_Output_##type_geom##_Geom_Param {                                                  \
        AbcGeom::O##type_geom##GeomParam param;                                                   \
        Array_Sample_Data sample_data;                                                            \
        using ABC_ARRAY_SAMPLE_TYPE = AbcGeom::type_geom##ArraySample;                            \
        using KURI_ARRAY_SAMPLE_TYPE = Abc_Output_##type_geom##_Geom_Param_Sample;                \
    };                                                                                            \
    struct Abc_Output_##type_geom##_Geom_Param *abc_output_##nom_court##_geom_param_create(       \
        struct Abc_Output_Compound_Property *parent,                                              \
        struct Abc_String name,                                                                   \
        bool is_indexed,                                                                          \
        enum Abc_Geometry_Scope scope,                                                            \
        uint64_t array_extent)                                                                    \
    {                                                                                             \
        if (parent == nullptr) {                                                                  \
            return nullptr;                                                                       \
        }                                                                                         \
        auto résultat = kuri_loge<Abc_Output_##type_geom##_Geom_Param>(                           \
            parent->archive->ctx_kuri);                                                           \
        résultat->param = AbcGeom::O##type_geom##GeomParam(                                       \
            parent->prop,                                                                         \
            vers_std_string(name),                                                                \
            is_indexed,                                                                           \
            static_cast<AbcGeom::GeometryScope>(scope),                                           \
            array_extent);                                                                        \
        return résultat;                                                                          \
    }                                                                                             \
    void abc_output_##nom_court##_geom_param_set_time_sampling(                                   \
        struct Abc_Output_##type_geom##_Geom_Param *param, struct Abc_Time_Sample_Index index)    \
    {                                                                                             \
        param->param.setTimeSampling(index.value);                                                \
    }                                                                                             \
    void abc_output_##nom_court##_geom_param_sample_set(                                          \
        struct Abc_Output_##type_geom##_Geom_Param *param,                                        \
        struct Abc_Output_##type_geom##_Geom_Param_Sample *sample)                                \
    {                                                                                             \
        if (sample->values) {                                                                     \
            auto param_sample = make_typed_sample(*sample, &param->sample_data);                  \
            param->param.set(param_sample);                                                       \
        }                                                                                         \
    }

ENUMERATE_ABC_ATTRIBUTE_TYPES(DEFINE_ABC_OUTPUT_GEOM_PARAMS)

#undef DECLARE_ABC_OUTPUT_GEOM_PARAMS

#define DEFINE_OUTPUT_SAMPLE_FUNCTIONS(uname, lname, snake_name, method, sample_type)             \
    void abc_output_##lname##_sample_##snake_name(                                                \
        struct Abc_Output_##uname##_Sample *lname##_sample, struct sample_type sample)            \
    {                                                                                             \
        auto typed_sample = make_typed_sample(sample, &lname##_sample->sample_data);              \
        lname##_sample->sample.method(typed_sample);                                              \
    }

#define DEFINE_OUTPUT_SAMPLE_SCALAR_FUNCTIONS(uname, lname, snake_name, method, sample_type)      \
    void abc_output_##lname##_sample_##snake_name(                                                \
        struct Abc_Output_##uname##_Sample *lname##_sample, sample_type sample)                   \
    {                                                                                             \
        lname##_sample->sample.method(sample);                                                    \
    }

#define DEFINE_COMMON_OUTPUT_OBJECT_FUNCTIONS(uname, lname)                                       \
    Abc_Output_Compound_Property *abc_output_##lname##_arb_geom_params_get(                       \
        struct Abc_Output_##uname *lname)                                                         \
    {                                                                                             \
        if (!lname->arb_geom_params_initialized) {                                                \
            lname->get_arb_geom_params(&lname->arb_geom_params);                                  \
            lname->arb_geom_params.archive = lname->archive;                                      \
            lname->arb_geom_params_initialized = true;                                            \
        }                                                                                         \
        return &lname->arb_geom_params;                                                           \
    }                                                                                             \
    Abc_Output_Compound_Property *abc_output_##lname##_user_properties_get(                       \
        struct Abc_Output_##uname *lname)                                                         \
    {                                                                                             \
        if (!lname->user_properties_initialized) {                                                \
            lname->get_user_properties(&lname->user_properties);                                  \
            lname->user_properties.archive = lname->archive;                                      \
            lname->user_properties_initialized = true;                                            \
        }                                                                                         \
        return &lname->user_properties;                                                           \
    }                                                                                             \
    Abc_MetaData *abc_output_##lname##_metadata_get(struct Abc_Output_##uname *lname)             \
    {                                                                                             \
        if (!lname->metadata_initialized) {                                                       \
            lname->get_metadata(&lname->metadata_);                                               \
            lname->metadata_.ctx_kuri = lname->archive->ctx_kuri;                                 \
            lname->metadata_initialized = true;                                                   \
        }                                                                                         \
        return &lname->metadata_;                                                                 \
    }                                                                                             \
    void abc_output_##lname##_sample_set_from_previous(Abc_Output_##uname *lname)                 \
    {                                                                                             \
        lname->set_from_previous();                                                               \
    }

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Export
 * \{ */

using namespace Alembic;

struct Abc_Output_Object {
    Abc_Output_Object *next = nullptr;
    Abc_Output_Archive *archive = nullptr;

    Abc_Output_Compound_Property arb_geom_params{};
    Abc_Output_Compound_Property user_properties{};
    Abc_MetaData metadata_{};

    bool arb_geom_params_initialized = false;
    bool user_properties_initialized = false;
    bool metadata_initialized = false;

    virtual ~Abc_Output_Object() = default;

    virtual AbcGeom::OObject &get_object() = 0;
};

struct Abc_Output_Visibility_Property : public Abc_Output_Scalar_Property {};

Abc_Output_Visibility_Property *abc_output_object_create_visibility_property(
    Abc_Generic_Output_Object object, Abc_Time_Sample_Index index)
{
    auto résultat = make_output_scalar_prop<Abc_Output_Visibility_Property>(object.object->archive,
                                                                            nullptr);
    résultat->prop = Alembic::AbcGeom::CreateVisibilityProperty(object.object->get_object(),
                                                                index.value);
    return résultat;
}

void abc_output_visibility_property_set(Abc_Output_Visibility_Property *prop,
                                        Abc_Object_Visibility visibility)
{
    int8_t ovisibility = int8_t(visibility);
    prop->prop.set(&ovisibility);
}

static void initialise_metadonnées(struct Abc_Output_Archive_Metadata *metadata,
                                   Abc::MetaData &abc_metadata)
{
    auto nom_application = vers_std_string_ou_défaut(metadata->application_name, "unknown");
    abc_metadata.set(Abc::kApplicationNameKey, nom_application);

    auto description = vers_std_string_ou_défaut(metadata->user_description, "unknown");
    abc_metadata.set(Abc::kUserDescriptionKey, description);

    if (metadata->fps > 0.0) {
        abc_metadata.set("FramesPerTimeUnit", std::to_string(metadata->fps));
    }

    time_t raw_time;
    time(&raw_time);
    char buffer[128];

#if defined _WIN32 || defined _WIN64
    ctime_s(buffer, 128, &raw_time);
#else
    ctime_r(&raw_time, buffer);
#endif

    const std::size_t buffer_len = strlen(buffer);
    if (buffer_len > 0 && buffer[buffer_len - 1] == '\n') {
        buffer[buffer_len - 1] = '\0';
    }

    abc_metadata.set(Alembic::Abc::kDateWrittenKey, buffer);
}

struct Abc_Output_Archive *abc_output_archive_create(ContexteKuri *ctx_kuri,
                                                     struct Abc_String path,
                                                     struct Abc_Output_Archive_Metadata *metadata)
{
    auto str_chemin = vers_std_string(path);

    Abc::MetaData abc_metadata;
    initialise_metadonnées(metadata, abc_metadata);

    AbcCoreOgawa::WriteArchive archive_writer;
    Abc::ErrorHandler::Policy policy = Abc::ErrorHandler::kThrowPolicy;

    auto oarchive = kuri_loge<Abc::OArchive>(
        ctx_kuri, AbcCoreOgawa::WriteArchive(), str_chemin, abc_metadata, policy);

    auto résultat = kuri_loge<Abc_Output_Archive>(ctx_kuri);
    résultat->ctx_kuri = ctx_kuri;
    résultat->archive = oarchive;
    résultat->racine = nullptr;
    résultat->objects = nullptr;
    return résultat;
}

struct Abc_Time_Sample_Index abc_output_archive_default_time_sampling(
    struct Abc_Output_Archive * /*archive*/)
{
    return Abc_Time_Sample_Index{0};
}

struct Abc_Time_Sample_Index abc_output_archive_create_time_sampling(
    struct Abc_Output_Archive *archive,
    double *echantillons,
    uint64_t nombre_d_echantillons,
    double temps_par_cycle)
{
    Abc::TimeSamplingPtr time_sampling_ptr;

    if (nombre_d_echantillons == 0 || echantillons == nullptr) {
        time_sampling_ptr = Abc::TimeSamplingPtr(new Abc::TimeSampling());
    }
    else {
        std::vector<double> samples(echantillons, echantillons + nombre_d_echantillons);
        Abc::TimeSamplingType ts(uint32_t(nombre_d_echantillons), temps_par_cycle);
        time_sampling_ptr = Abc::TimeSamplingPtr(new Abc::TimeSampling(ts, samples));
    }

    struct Abc_Time_Sample_Index résultat;
    résultat.value = archive->archive->addTimeSampling(*time_sampling_ptr);
    return résultat;
}

void abc_output_archive_destroy(struct Abc_Output_Archive *archive)
{
    if (archive) {
        auto object = archive->objects;
        kuri_deloge_liste(archive->ctx_kuri, archive->scalar_props);
        kuri_deloge_liste(archive->ctx_kuri, archive->array_props);
        kuri_deloge_liste(archive->ctx_kuri, archive->compound_props);
        kuri_deloge_liste(archive->ctx_kuri, archive->objects);
        kuri_deloge(archive->ctx_kuri, archive->archive);
        kuri_deloge(archive->ctx_kuri, archive);
    }
}

template <typename T>
T *crée_objet_sortie(Abc_Output_Archive *archive)
{
    auto résultat = kuri_loge<T>(archive->ctx_kuri);
    liste_ajoute(&archive->objects, static_cast<Abc_Output_Object *>(résultat));
    résultat->archive = archive;
    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Xform
 * \{ */

struct Abc_Output_Xform : public Abc_Output_Object {
    Abc::OObject object{};
    AbcGeom::OXformSchema schema{};

    void set_sample(AbcGeom::OXformSchema::sample_type &sample)
    {
        schema.set(sample);
    }

    void set_from_previous()
    {
        schema.setFromPrevious();
    }

    void get_arb_geom_params(Abc_Output_Compound_Property *prop)
    {
        prop->prop = schema.getArbGeomParams();
    }

    void get_user_properties(Abc_Output_Compound_Property *prop)
    {
        prop->prop = schema.getUserProperties();
    }

    void get_metadata(Abc_MetaData *metadata)
    {
        metadata->metadata = object.getMetaData();
    }

    AbcGeom::OObject &get_object() override
    {
        return object;
    }
};

Abc_Output_Xform *abc_output_archive_root_object_get(Abc_Output_Archive *archive)
{
    if (archive->racine) {
        return archive->racine;
    }

    auto racine = crée_objet_sortie<Abc_Output_Xform>(archive);
    racine->object = archive->archive->getTop();
    archive->racine = racine;
    return racine;
}

DEFINE_COMMON_OUTPUT_OBJECT_FUNCTIONS(Xform, xform)

Abc_Output_Xform *abc_output_xform_create(Abc_Output_Xform *parent,
                                          Abc_String nom,
                                          Abc_Time_Sample_Index time_sample_index)
{
    auto archive = parent->archive;
    auto résultat = crée_objet_sortie<Abc_Output_Xform>(archive);
    auto oxform = AbcGeom::OXform(parent->object, vers_std_string(nom));
    résultat->object = oxform;
    résultat->schema = oxform.getSchema();
    résultat->schema.setTimeSampling(time_sample_index.value);
    return résultat;
}

#define DEFINE_COMMON_SAMPLE_FONCTIONS(uppercase_name, lowercase_name)                            \
    Abc_Output_##uppercase_name##_Sample *abc_output_##lowercase_name##_sample_create(            \
        Abc_Output_##uppercase_name *lowercase_name)                                              \
    {                                                                                             \
        auto résultat = kuri_loge<Abc_Output_##uppercase_name##_Sample>(                          \
            lowercase_name->archive->ctx_kuri);                                                   \
        résultat->ctx_kuri = lowercase_name->archive->ctx_kuri;                                   \
        return résultat;                                                                          \
    }                                                                                             \
    void abc_output_##lowercase_name##_sample_reset(Abc_Output_##uppercase_name##_Sample *sample) \
    {                                                                                             \
        sample->sample.reset();                                                                   \
    }                                                                                             \
    void abc_output_##lowercase_name##_sample_destroy(                                            \
        Abc_Output_##uppercase_name##_Sample *sample)                                             \
    {                                                                                             \
        if (sample) {                                                                             \
            kuri_deloge(sample->ctx_kuri, sample);                                                \
        }                                                                                         \
    }                                                                                             \
    void abc_output_##lowercase_name##_sample_set(Abc_Output_##uppercase_name *lowercase_name,    \
                                                  Abc_Output_##uppercase_name##_Sample *sample)   \
    {                                                                                             \
        lowercase_name->set_sample(sample->sample);                                               \
    }

struct Abc_Output_Xform_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::XformSample sample{};
};

DEFINE_COMMON_SAMPLE_FONCTIONS(Xform, xform)

void abc_output_xform_sample_set_matrix(Abc_Output_Xform_Sample *sample, Abc_M44d *matrix)
{
    // À FAIRE : généralise les assertions
    static_assert(sizeof(Abc_M44d) == sizeof(Abc::M44d));
    sample->sample.setMatrix(*reinterpret_cast<Abc::M44d *>(matrix));
}

void abc_output_xform_sample_set_inherits_xform(Abc_Output_Xform_Sample *sample, bool inherits)
{
    sample->sample.setInheritsXforms(inherits);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Points
 * \{ */

struct Abc_Output_Points : public Abc_Output_Object {
    AbcGeom::OPoints object{};

    void set_sample(AbcGeom::OPointsSchema::Sample &sample)
    {
        object.getSchema().set(sample);
    }

    void set_from_previous()
    {
        object.getSchema().setFromPrevious();
    }

    void get_arb_geom_params(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getArbGeomParams();
    }

    void get_user_properties(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getUserProperties();
    }

    void get_metadata(Abc_MetaData *metadata)
    {
        metadata->metadata = object.getMetaData();
    }

    AbcGeom::OObject &get_object() override
    {
        return object;
    }
};

Abc_Output_Points *abc_output_points_create(Abc_Output_Xform *parent,
                                            Abc_String nom,
                                            Abc_Time_Sample_Index time_sample_index)
{
    auto archive = parent->archive;
    auto résultat = crée_objet_sortie<Abc_Output_Points>(archive);
    résultat->object = AbcGeom::OPoints(
        parent->object, vers_std_string(nom), time_sample_index.value);
    return résultat;
}

DEFINE_COMMON_OUTPUT_OBJECT_FUNCTIONS(Points, points)

struct Abc_Output_Points_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::OPointsSchema::Sample sample{};
    Array_Sample_Data sample_data{};
};

DEFINE_COMMON_SAMPLE_FONCTIONS(Points, points)

ENUMERATE_POINTS_SAMPLE_INTERFACE(DEFINE_OUTPUT_SAMPLE_FUNCTIONS)

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Curves
 * \{ */

struct Abc_Output_Curves : public Abc_Output_Object {
    AbcGeom::OCurves object{};

    void set_sample(AbcGeom::OCurvesSchema::Sample &sample)
    {
        object.getSchema().set(sample);
    }

    void set_from_previous()
    {
        object.getSchema().setFromPrevious();
    }

    void get_arb_geom_params(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getArbGeomParams();
    }

    void get_user_properties(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getUserProperties();
    }

    void get_metadata(Abc_MetaData *metadata)
    {
        metadata->metadata = object.getMetaData();
    }

    AbcGeom::OObject &get_object() override
    {
        return object;
    }
};

Abc_Output_Curves *abc_output_curves_create(Abc_Output_Xform *parent,
                                            Abc_String nom,
                                            Abc_Time_Sample_Index time_sample_index)
{
    auto archive = parent->archive;
    auto résultat = crée_objet_sortie<Abc_Output_Curves>(archive);
    résultat->object = AbcGeom::OCurves(
        parent->object, vers_std_string(nom), time_sample_index.value);
    return résultat;
}

DEFINE_COMMON_OUTPUT_OBJECT_FUNCTIONS(Curves, curves)

struct Abc_Output_Curves_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::OCurvesSchema::Sample sample{};
    Array_Sample_Data sample_data{};
};

DEFINE_COMMON_SAMPLE_FONCTIONS(Curves, curves)

ENUMERATE_CURVES_SAMPLE_INTERFACE(DEFINE_OUTPUT_SAMPLE_FUNCTIONS)

void abc_output_curves_sample_type_set(Abc_Output_Curves_Sample *sample, Abc_Curve_Type type)
{
    AbcGeom::CurveType itype = static_cast<AbcGeom::CurveType>(type);
    sample->sample.setType(itype);
}

void abc_output_curves_sample_wrap_set(Abc_Output_Curves_Sample *sample,
                                       Abc_Curve_Periodicity wrap)
{
    AbcGeom::CurvePeriodicity iwrap = static_cast<AbcGeom::CurvePeriodicity>(wrap);
    sample->sample.setWrap(iwrap);
}

void abc_output_curves_sample_basis_set(Abc_Output_Curves_Sample *sample, Abc_Basis_Type basis)
{
    AbcGeom::BasisType ibasis = static_cast<AbcGeom::BasisType>(basis);
    sample->sample.setBasis(ibasis);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_FaceSet
 * \{ */

struct Abc_Output_FaceSet : public Abc_Output_Object {
    AbcGeom::OFaceSet object;

    void set_sample(AbcGeom::OFaceSetSchema::Sample &sample)
    {
        object.getSchema().set(sample);
    }

    void set_from_previous()
    {
    }

    void get_arb_geom_params(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getArbGeomParams();
    }

    void get_user_properties(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getUserProperties();
    }

    void get_metadata(Abc_MetaData *metadata)
    {
        metadata->metadata = object.getMetaData();
    }

    AbcGeom::OObject &get_object() override
    {
        return object;
    }
};

DEFINE_COMMON_OUTPUT_OBJECT_FUNCTIONS(FaceSet, faceset)

struct Abc_Output_FaceSet_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::OFaceSetSchema::Sample sample{};
    Array_Sample_Data sample_data{};
};

DEFINE_COMMON_SAMPLE_FONCTIONS(FaceSet, faceset)

ENUMERATE_FACESET_SAMPLE_INTERFACE(DEFINE_OUTPUT_SAMPLE_FUNCTIONS)

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_PolyMesh
 * \{ */

struct Abc_Output_PolyMesh : public Abc_Output_Object {
    AbcGeom::OPolyMesh object{};

    void set_sample(AbcGeom::OPolyMeshSchema::Sample &sample)
    {
        object.getSchema().set(sample);
    }

    void set_from_previous()
    {
        object.getSchema().setFromPrevious();
    }

    void get_arb_geom_params(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getArbGeomParams();
    }

    void get_user_properties(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getUserProperties();
    }

    void get_metadata(Abc_MetaData *metadata)
    {
        metadata->metadata = object.getMetaData();
    }

    AbcGeom::OObject &get_object() override
    {
        return object;
    }
};

Abc_Output_PolyMesh *abc_output_polymesh_create(Abc_Output_Xform *parent,
                                                Abc_String nom,
                                                Abc_Time_Sample_Index time_sample_index)
{
    auto archive = parent->archive;
    auto résultat = crée_objet_sortie<Abc_Output_PolyMesh>(archive);
    résultat->object = AbcGeom::OPolyMesh(
        parent->object, vers_std_string(nom), time_sample_index.value);
    return résultat;
}

DEFINE_COMMON_OUTPUT_OBJECT_FUNCTIONS(PolyMesh, polymesh)

Abc_Output_FaceSet *abc_output_polymesh_create_face_set(Abc_Output_PolyMesh *mesh, Abc_String name)
{
    auto result = crée_objet_sortie<Abc_Output_FaceSet>(mesh->archive);
    result->object = mesh->object.getSchema().createFaceSet(vers_std_string(name));
    return result;
}

void abc_output_polymesh_set_uv_source_name(Abc_Output_PolyMesh *mesh, Abc_String name)
{
    mesh->object.getSchema().setUVSourceName(vers_std_string(name));
}

struct Abc_Output_PolyMesh_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::OPolyMeshSchema::Sample sample{};
    Array_Sample_Data sample_data{};
};

DEFINE_COMMON_SAMPLE_FONCTIONS(PolyMesh, polymesh)

ENUMERATE_POLYMESH_SAMPLE_INTERFACE(DEFINE_OUTPUT_SAMPLE_FUNCTIONS)

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_SubD
 * \{ */

struct Abc_Output_SubD : public Abc_Output_Object {
    AbcGeom::OSubD object{};

    void set_sample(AbcGeom::OSubDSchema::Sample &sample)
    {
        object.getSchema().set(sample);
    }

    void set_from_previous()
    {
        object.getSchema().setFromPrevious();
    }

    void get_arb_geom_params(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getArbGeomParams();
    }

    void get_user_properties(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getUserProperties();
    }

    void get_metadata(Abc_MetaData *metadata)
    {
        metadata->metadata = object.getMetaData();
    }

    AbcGeom::OObject &get_object() override
    {
        return object;
    }
};

Abc_Output_SubD *abc_output_subd_create(Abc_Output_Xform *parent,
                                        Abc_String nom,
                                        Abc_Time_Sample_Index time_sample_index)
{
    auto archive = parent->archive;
    auto résultat = crée_objet_sortie<Abc_Output_SubD>(archive);
    résultat->object = AbcGeom::OSubD(
        parent->object, vers_std_string(nom), time_sample_index.value);
    return résultat;
}

DEFINE_COMMON_OUTPUT_OBJECT_FUNCTIONS(SubD, subd)

Abc_Output_FaceSet *abc_output_subd_create_face_set(Abc_Output_SubD *subd, Abc_String name)
{
    auto result = crée_objet_sortie<Abc_Output_FaceSet>(subd->archive);
    result->object = subd->object.getSchema().createFaceSet(vers_std_string(name));
    return result;
}

void abc_output_subd_set_uv_source_name(Abc_Output_SubD *subd, Abc_String name)
{
    subd->object.getSchema().setUVSourceName(vers_std_string(name));
}

struct Abc_Output_SubD_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::OSubDSchema::Sample sample{};
    Array_Sample_Data sample_data{};
};

DEFINE_COMMON_SAMPLE_FONCTIONS(SubD, subd)

ENUMERATE_SUBD_SAMPLE_INTERFACE(DEFINE_OUTPUT_SAMPLE_FUNCTIONS)

void abc_output_subd_sample_face_varying_interpolate_boundary_set(
    struct Abc_Output_SubD_Sample *sample, int value)
{
    sample->sample.setFaceVaryingInterpolateBoundary(value);
}

void abc_output_subd_sample_face_varying_propagate_corners_set(
    struct Abc_Output_SubD_Sample *sample, int value)
{
    sample->sample.setFaceVaryingPropagateCorners(value);
}

void abc_output_subd_sample_interpolate_boundary_set(struct Abc_Output_SubD_Sample *sample,
                                                     int value)
{
    sample->sample.setInterpolateBoundary(value);
}

void abc_output_subd_sample_subdivision_scheme_set(struct Abc_Output_SubD_Sample *sample,
                                                   Abc_String value)
{
    sample->sample.setSubdivisionScheme(vers_std_string(value));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Camera
 * \{ */

struct Abc_Output_Camera : public Abc_Output_Object {
    AbcGeom::OCamera object{};

    void set_sample(AbcGeom::CameraSample &sample)
    {
        object.getSchema().set(sample);
    }

    void set_from_previous()
    {
        object.getSchema().setFromPrevious();
    }

    void get_arb_geom_params(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getArbGeomParams();
    }

    void get_user_properties(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getUserProperties();
    }

    void get_metadata(Abc_MetaData *metadata)
    {
        metadata->metadata = object.getMetaData();
    }

    AbcGeom::OObject &get_object() override
    {
        return object;
    }
};

Abc_Output_Camera *abc_output_camera_create(Abc_Output_Xform *parent,
                                            Abc_String nom,
                                            Abc_Time_Sample_Index time_sample_index)
{
    auto archive = parent->archive;
    auto résultat = crée_objet_sortie<Abc_Output_Camera>(archive);
    résultat->object = AbcGeom::OCamera(
        parent->object, vers_std_string(nom), time_sample_index.value);
    return résultat;
}

DEFINE_COMMON_OUTPUT_OBJECT_FUNCTIONS(Camera, camera)

struct Abc_Output_Camera_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::CameraSample sample{};
};

DEFINE_COMMON_SAMPLE_FONCTIONS(Camera, camera)

struct Abc_Output_Camera_Sample *abc_output_camera_sample_create_window(
    struct Abc_Output_Archive *archive, double top, double bottom, double left, double right)
{
    auto résultat = kuri_loge<Abc_Output_Camera_Sample>(archive->ctx_kuri);
    résultat->ctx_kuri = archive->ctx_kuri;
    résultat->sample = AbcGeom::CameraSample(top, bottom, left, right);
    return résultat;
}

ENUMERATE_OUTPUT_CAMERA_SAMPLE_SCALAR_INTERFACE(DEFINE_OUTPUT_SAMPLE_SCALAR_FUNCTIONS)

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_NuPatch
 * \{ */

struct Abc_Output_NuPatch : public Abc_Output_Object {
    AbcGeom::ONuPatch object{};

    void set_sample(AbcGeom::ONuPatchSchema::Sample &sample)
    {
        object.getSchema().set(sample);
    }

    void set_from_previous()
    {
        object.getSchema().setFromPrevious();
    }

    void get_arb_geom_params(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getArbGeomParams();
    }

    void get_user_properties(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getUserProperties();
    }

    void get_metadata(Abc_MetaData *metadata)
    {
        metadata->metadata = object.getMetaData();
    }

    AbcGeom::OObject &get_object() override
    {
        return object;
    }
};

Abc_Output_NuPatch *abc_output_nupatch_create(Abc_Output_Xform *parent,
                                              Abc_String nom,
                                              Abc_Time_Sample_Index time_sample_index)
{
    auto archive = parent->archive;
    auto résultat = crée_objet_sortie<Abc_Output_NuPatch>(archive);
    résultat->object = AbcGeom::ONuPatch(
        parent->object, vers_std_string(nom), time_sample_index.value);
    return résultat;
}

DEFINE_COMMON_OUTPUT_OBJECT_FUNCTIONS(NuPatch, nupatch)

struct Abc_Output_NuPatch_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::ONuPatchSchema::Sample sample{};
    Array_Sample_Data sample_data{};
};

DEFINE_COMMON_SAMPLE_FONCTIONS(NuPatch, nupatch)

ENUMERATE_OUTPUT_NUPATCH_SAMPLE_SCALAR_INTERFACE(DEFINE_OUTPUT_SAMPLE_SCALAR_FUNCTIONS)
ENUMERATE_OUTPUT_NUPATCH_SAMPLE_INTERFACE(DEFINE_OUTPUT_SAMPLE_FUNCTIONS)

void abc_output_nupatch_sample_trim_curve_set(Abc_Output_NuPatch_Sample *sample,
                                              int32_t trim_n_loops,
                                              Abc_Int32_Array_Sample trim_n_curves,
                                              Abc_Int32_Array_Sample trim_n,
                                              Abc_Int32_Array_Sample trim_order,
                                              Abc_Float_Array_Sample trim_knot,
                                              Abc_Float_Array_Sample trim_min,
                                              Abc_Float_Array_Sample trim_max,
                                              Abc_Float_Array_Sample trim_u,
                                              Abc_Float_Array_Sample trim_v,
                                              Abc_Float_Array_Sample trim_w)
{
    auto i_trim_n_curves = make_typed_sample(trim_n_curves, &sample->sample_data);
    auto i_trim_n = make_typed_sample(trim_n, &sample->sample_data);
    auto i_trim_order = make_typed_sample(trim_order, &sample->sample_data);
    auto i_trim_knot = make_typed_sample(trim_knot, &sample->sample_data);
    auto i_trim_min = make_typed_sample(trim_min, &sample->sample_data);
    auto i_trim_max = make_typed_sample(trim_max, &sample->sample_data);
    auto i_trim_u = make_typed_sample(trim_u, &sample->sample_data);
    auto i_trim_v = make_typed_sample(trim_v, &sample->sample_data);
    auto i_trim_w = make_typed_sample(trim_w, &sample->sample_data);

    sample->sample.setTrimCurve(trim_n_loops,
                                i_trim_n_curves,
                                i_trim_n,
                                i_trim_order,
                                i_trim_knot,
                                i_trim_min,
                                i_trim_max,
                                i_trim_u,
                                i_trim_v,
                                i_trim_w);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Light
 * \{ */

struct Abc_Output_Light : public Abc_Output_Object {
    AbcGeom::OLight object{};

    void set_from_previous()
    {
        object.getSchema().setFromPrevious();
    }

    void get_arb_geom_params(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getArbGeomParams();
    }

    void get_user_properties(Abc_Output_Compound_Property *prop)
    {
        prop->prop = object.getSchema().getUserProperties();
    }

    void get_metadata(Abc_MetaData *metadata)
    {
        metadata->metadata = object.getMetaData();
    }

    AbcGeom::OObject &get_object() override
    {
        return object;
    }
};

Abc_Output_Light *abc_output_light_create(Abc_Output_Xform *parent,
                                          Abc_String nom,
                                          Abc_Time_Sample_Index time_sample_index)
{
    auto archive = parent->archive;
    auto résultat = crée_objet_sortie<Abc_Output_Light>(archive);
    résultat->object = AbcGeom::OLight(
        parent->object, vers_std_string(nom), time_sample_index.value);
    return résultat;
}

DEFINE_COMMON_OUTPUT_OBJECT_FUNCTIONS(Light, light)

void abc_output_light_set_camera_sample(struct Abc_Output_Light *light,
                                        struct Abc_Output_Camera_Sample *sample)
{
    light->object.getSchema().setCameraSample(sample->sample);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Material
 * \{ */

struct Abc_Output_Material : public Abc_Output_Object {
    AbcMaterial::OMaterial object{};

    void get_metadata(Abc_MetaData *metadata)
    {
        metadata->metadata = object.getMetaData();
    }

    AbcGeom::OObject &get_object() override
    {
        return object;
    }
};

Abc_Output_Material *abc_output_material_create(Abc_Output_Xform *parent, Abc_String nom)
{
    auto archive = parent->archive;
    auto résultat = crée_objet_sortie<Abc_Output_Material>(archive);
    résultat->object = AbcMaterial::OMaterial(parent->object, nom);
    return résultat;
}

Abc_MetaData *abc_output_material_metadata_get(struct Abc_Output_Material *metarial)
{
    if (!metarial->metadata_initialized) {
        metarial->get_metadata(&metarial->metadata_);
        metarial->metadata_.ctx_kuri = metarial->archive->ctx_kuri;
        metarial->metadata_initialized = true;
    }
    return &metarial->metadata_;
}

void abc_output_material_set_shader(Abc_Output_Material *material,
                                    Abc_String target,
                                    Abc_String shader_type,
                                    Abc_String shader_name)
{
    material->object.getSchema().setShader(target, shader_type, shader_name);
}

Abc_Output_Compound_Property *abc_output_material_get_shader_parameters(
    Abc_Output_Material *material, Abc_String target, Abc_String shader_type)
{
    auto résultat = make_output_compound_property(material->archive);
    résultat->prop = material->object.getSchema().getShaderParameters(target, shader_type);
    return résultat;
}

void abc_output_material_add_network_node(Abc_Output_Material *material,
                                          Abc_String node_name,
                                          Abc_String target,
                                          Abc_String node_type)
{
    material->object.getSchema().addNetworkNode(node_name, target, node_type);
}

void abc_output_material_set_network_node_connection(Abc_Output_Material *material,
                                                     Abc_String node_name,
                                                     Abc_String input_name,
                                                     Abc_String connected_node_name,
                                                     Abc_String connected_output_name)
{
    material->object.getSchema().setNetworkNodeConnection(
        node_name, input_name, connected_node_name, connected_output_name);
}

Abc_Output_Compound_Property *abc_output_material_get_network_node_parameters(
    Abc_Output_Material *material, Abc_String node_name)
{
    auto résultat = make_output_compound_property(material->archive);
    résultat->prop = material->object.getSchema().getNetworkNodeParameters(node_name);
    return résultat;
}

void abc_output_material_set_network_terminal(Abc_Output_Material *material,
                                              Abc_String target,
                                              Abc_String shader_type,
                                              Abc_String node_name,
                                              Abc_String output_name)
{
    material->object.getSchema().setNetworkTerminal(target, shader_type, node_name, output_name);
}

void abc_output_material_set_network_interface_parameter_mapping(Abc_Output_Material *material,
                                                                 Abc_String interface_param_name,
                                                                 Abc_String map_to_node_name,
                                                                 Abc_String map_to_param_name)
{
    material->object.getSchema().setNetworkInterfaceParameterMapping(
        interface_param_name, map_to_node_name, map_to_param_name);
}

Abc_Output_Compound_Property *abc_output_material_get_network_interface_parameters(
    Abc_Output_Material *material)
{
    auto résultat = make_output_compound_property(material->archive);
    résultat->prop = material->object.getSchema().getNetworkInterfaceParameters();
    return résultat;
}

/** \} */
