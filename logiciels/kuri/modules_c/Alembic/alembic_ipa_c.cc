/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "abc_common.hh"

#include <fstream>
#include <string_view>

#include "alembic_ipa_c.h"

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

Abc_String::operator std::string()
{
    return vers_std_string(*this);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_String
 * \{ */

static Abc_New_String make_new_string(std::string_view str)
{
    auto characters = new char[str.size()];
    memcpy(characters, str.data(), str.size());
    Abc_New_String résultat;
    résultat.characters = characters;
    résultat.size = str.size();
    return résultat;
}

void abc_new_string_destroy(struct Abc_New_String *new_string)
{
    delete[] new_string->characters;
    new_string->characters = nullptr;
    new_string->size = 0;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Array_Sample
 * \{ */

template <typename IPA_Type, typename Alembic_Type>
auto make_input_array_sample(std::shared_ptr<Alembic_Type> ptr, Array_Sample_Data &)
{
    using value_type = typename Alembic_Type::value_type;
    auto values = const_cast<value_type *>((*ptr).get());
    return IPA_Type{reinterpret_cast<decltype(IPA_Type::values)>(values), (*ptr).size()};
}

template <>
auto make_input_array_sample<Abc_String_Array_Sample, AbcGeom::StringArraySample>(
    AbcGeom::StringArraySamplePtr ptr, Array_Sample_Data &data)
{
    data.input_strings.resize(ptr->size());

    for (auto i = 0ul; i < ptr->size(); i++) {
        auto string = ptr->get() + i;
        data.input_strings[i].characters = string->c_str();
        data.input_strings[i].size = string->size();
    }

    return Abc_String_Array_Sample{data.input_strings.data(), data.input_strings.size()};
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
    Abc_Input_Geom_Param *geom_params = nullptr;
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

    return Abc::ISampleSelector(selector.requested_index);
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
    Abc_Input_Geom_Param *next = nullptr;
    Array_Sample_Data sample_data{};

    virtual ~Abc_Input_Geom_Param() = default;
};

template <typename T>
T *make_input_geom_param(Abc_Input_Archive *archive)
{
    auto résultat = kuri_loge<T>(archive->ctx_kuri);
    liste_ajoute(&archive->geom_params, static_cast<Abc_Input_Geom_Param *>(résultat));
    résultat->archive = archive;
    return résultat;
}

#define DEFINE_INPUT_GEOM_PARAM(type_geom, type_abc_value, type_c, nom_court)                     \
    struct Abc_Input_##type_geom##_Geom_Param : public Abc_Input_Geom_Param {                     \
        AbcGeom::I##type_geom##GeomParam param{};                                                 \
        AbcGeom::I##type_geom##GeomParam::Sample sample{};                                        \
    };                                                                                            \
    bool abc_input_##nom_court##_geom_param_matches(Abc_Property_Header *prop_header)             \
    {                                                                                             \
        return AbcGeom::I##type_geom##GeomParam::matches(prop_header->header);                    \
    }                                                                                             \
    Abc_Input_##type_geom##_Geom_Param *abc_input_##nom_court##_geom_param_get(                   \
        Abc_Input_Compound_Property *prop, Abc_String name)                                       \
    {                                                                                             \
        auto résultat = make_input_geom_param<Abc_Input_##type_geom##_Geom_Param>(prop->archive); \
        résultat->param = AbcGeom::I##type_geom##GeomParam(prop->prop, name);                     \
        return résultat;                                                                          \
    }                                                                                             \
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
        if (sample) {                                                                             \
            sample->values = make_input_array_sample<Abc_##type_geom##_Array_Sample>(             \
                param->sample.getVals(), param->sample_data);                                     \
            sample->indices = make_input_array_sample<Abc_UInt32_Array_Sample>(                   \
                param->sample.getIndices(), param->sample_data);                                  \
            sample->scope = static_cast<Abc_Geometry_Scope>(param->sample.getScope());            \
        }                                                                                         \
    }                                                                                             \
    void abc_input_##nom_court##_geom_param_get_expanded(                                         \
        struct Abc_Input_##type_geom##_Geom_Param *param,                                         \
        struct Abc_Input_##type_geom##_Geom_Param_Sample *sample,                                 \
        struct Abc_Sample_Selector selector)                                                      \
    {                                                                                             \
        param->param.getExpanded(param->sample, get_sample_selector(selector));                   \
        if (sample) {                                                                             \
            sample->values = make_input_array_sample<Abc_##type_geom##_Array_Sample>(             \
                param->sample.getVals(), param->sample_data);                                     \
            sample->scope = static_cast<Abc_Geometry_Scope>(param->sample.getScope());            \
        }                                                                                         \
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

    Abc_MetaData metadata_{};

    bool metadata_initialized = false;
};

struct Abc_Input_Schema {
    Abc_Input_Archive *archive = nullptr;
    std::vector<std::string> face_set_names_std_string{};
    std::vector<Abc_String> face_set_names_abc_string{};

    Abc_Input_Compound_Property arb_geom_params{};
    Abc_Input_Compound_Property user_properties{};

    bool arb_geom_params_initialized = false;
    bool user_properties_initialized = false;
};

#define DECLARE_TYPED_INPUT_OBJECTS(type_abc, type_kuri, lname)                                   \
    struct Abc_Input_##type_kuri##_Schema : public Abc_Input_Schema {                             \
        type_abc##Schema *impl = nullptr;                                                         \
    };                                                                                            \
    struct Abc_Input_##type_kuri : public Abc_Input_Object {                                      \
        type_abc typed_object{};                                                                  \
        Abc_Input_##type_kuri##_Schema schema{};                                                  \
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

struct Abc_Object_Header *abc_input_object_get_header(Abc_Generic_Input_Object object)
{
    const AbcGeom::ObjectHeader &header = object.object->untyped_object.getHeader();
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

#define DECLARE_TYPED_INPUT_OBJECTS(type_abc, type_kuri, lname)                                   \
    Abc_Input_##type_kuri *abc_input_##lname##_get(Abc_Generic_Input_Object parent,               \
                                                   Abc_String name)                               \
    {                                                                                             \
        auto résultat = make_object<Abc_Input_##type_kuri>(parent.object->archive);               \
        résultat->typed_object = type_abc(parent.object->untyped_object, name);                   \
        résultat->untyped_object = résultat->typed_object;                                        \
        return résultat;                                                                          \
    }

ENUMERATE_INPUT_OBJECT_TYPES(DECLARE_TYPED_INPUT_OBJECTS)

#undef DECLARE_TYPED_INPUT_OBJECTS

/** \} */

#define DEFINE_INPUT_SAMPLE_SCALAR_GET_FUNCTION(uname, lname, snake_name, method, sample_type)    \
    sample_type abc_input_##lname##_schema_sample_##snake_name(                                   \
        struct Abc_Input_##uname##_Schema_Sample *sample)                                         \
    {                                                                                             \
        return sample->sample.method();                                                           \
    }

#define DEFINE_INPUT_SAMPLE_ARRAY_GET_FUNCTION(uname, lname, snake_name, method, sample_type)     \
    sample_type abc_input_##lname##_schema_sample_##snake_name(                                   \
        struct Abc_Input_##uname##_Schema_Sample *sample)                                         \
    {                                                                                             \
        auto ptr = sample->sample.method();                                                       \
        return make_input_array_sample<sample_type>(ptr, sample->sample_data);                    \
    }

#define DEFINE_COMMON_INPUT_SCHEMA_FUNCTIONS(uname, lname)                                        \
    struct Abc_Input_##uname##_Schema *abc_input_##lname##_get_schema(                            \
        struct Abc_Input_##uname *lname)                                                          \
    {                                                                                             \
        if (lname->schema.impl == nullptr) {                                                      \
            lname->schema.impl = &lname->typed_object.getSchema();                                \
            lname->schema.archive = lname->archive;                                               \
        }                                                                                         \
        return &lname->schema;                                                                    \
    }                                                                                             \
    struct Abc_Time_Sampling *abc_input_##lname##_schema_get_time_sampling(                       \
        struct Abc_Input_##uname##_Schema *schema)                                                \
    {                                                                                             \
        return make_time_sampling(schema->archive, schema->impl->getTimeSampling());              \
    }                                                                                             \
    bool abc_input_##lname##_schema_is_constant(struct Abc_Input_##uname##_Schema *schema)        \
    {                                                                                             \
        return schema->impl->isConstant();                                                        \
    }                                                                                             \
    uint64_t abc_input_##lname##_schema_get_num_samples(                                          \
        struct Abc_Input_##uname##_Schema *schema)                                                \
    {                                                                                             \
        auto résultat = schema->impl->getNumSamples();                                            \
        return résultat;                                                                          \
    }                                                                                             \
    void abc_input_##lname##_schema_reset(struct Abc_Input_##uname##_Schema *schema)              \
    {                                                                                             \
        schema->impl->reset();                                                                    \
    }                                                                                             \
    bool abc_input_##lname##_schema_valid(struct Abc_Input_##uname##_Schema *schema)              \
    {                                                                                             \
        return schema->impl->valid();                                                             \
    }                                                                                             \
    Abc_Input_Compound_Property *abc_input_##lname##_schema_get_arb_geom_params(                  \
        Abc_Input_##uname##_Schema *schema)                                                       \
    {                                                                                             \
        if (schema->arb_geom_params_initialized == false) {                                       \
            schema->arb_geom_params.prop = schema->impl->getArbGeomParams();                      \
            schema->arb_geom_params.archive = schema->archive;                                    \
            schema->arb_geom_params_initialized = true;                                           \
        }                                                                                         \
        return &schema->arb_geom_params;                                                          \
    }                                                                                             \
    Abc_Input_Compound_Property *abc_input_##lname##_schema_get_user_properties(                  \
        Abc_Input_##uname##_Schema *schema)                                                       \
    {                                                                                             \
        if (schema->user_properties_initialized == false) {                                       \
            schema->user_properties.prop = schema->impl->getUserProperties();                     \
            schema->user_properties.archive = schema->archive;                                    \
            schema->user_properties_initialized = true;                                           \
        }                                                                                         \
        return &schema->user_properties;                                                          \
    }

#define DEFINE_COMMON_INPUT_SAMPLE_FUNCTIONS(uname, lname)                                        \
    struct Abc_Input_##uname##_Schema_Sample *abc_input_##lname##_schema_get(                     \
        struct Abc_Input_##uname##_Schema *schema, struct Abc_Sample_Selector selector)           \
    {                                                                                             \
        auto résultat = kuri_loge<Abc_Input_##uname##_Schema_Sample>(schema->archive->ctx_kuri);  \
        résultat->archive = schema->archive;                                                      \
        schema->impl->get(résultat->sample, get_sample_selector(selector));                       \
        return résultat;                                                                          \
    }                                                                                             \
    void abc_input_##lname##_schema_get_value(struct Abc_Input_##uname##_Schema *schema,          \
                                              struct Abc_Input_##uname##_Schema_Sample *sample,   \
                                              struct Abc_Sample_Selector selector)                \
    {                                                                                             \
        schema->impl->get(sample->sample, get_sample_selector(selector));                         \
    }                                                                                             \
    void abc_input_##lname##_schema_sample_destroy(                                               \
        struct Abc_Input_##uname##_Schema_Sample *sample)                                         \
    {                                                                                             \
        if (sample) {                                                                             \
            kuri_deloge(sample->archive->ctx_kuri, sample);                                       \
        }                                                                                         \
    }                                                                                             \
    void abc_input_##lname##_schema_sample_get_self_bounds(                                       \
        struct Abc_Input_##uname##_Schema_Sample *sample, Abc_Box3d *r_box)                       \
    {                                                                                             \
        auto résultat = sample->sample.getSelfBounds();                                           \
        *r_box = import_value_converter<Abc::Box3d>::convert_value(&résultat);                    \
    }                                                                                             \
    bool abc_input_##lname##_schema_sample_valid(                                                 \
        struct Abc_Input_##uname##_Schema_Sample *sample)                                         \
    {                                                                                             \
        return sample->sample.valid();                                                            \
    }                                                                                             \
    void abc_input_##lname##_schema_sample_reset(                                                 \
        struct Abc_Input_##uname##_Schema_Sample *sample)                                         \
    {                                                                                             \
        sample->sample.reset();                                                                   \
    }

/* ------------------------------------------------------------------------- */
/** \nom Face sets extraction.
 * \{ */

template <typename Input_Schema_Object_Type>
static void abc_input_object_schema_get_face_set_names(Input_Schema_Object_Type *schema,
                                                       Abc_String **r_names,
                                                       uint64_t *r_count)
{
    if (schema->face_set_names_std_string.empty()) {
        schema->impl->getFaceSetNames(schema->face_set_names_std_string);

        schema->face_set_names_abc_string.resize(schema->face_set_names_std_string.size());

        auto strings = schema->face_set_names_abc_string.data();
        auto num_strings = schema->face_set_names_abc_string.size();
        for (auto i = 0ul; i < num_strings; i++) {
            vers_abc_string(strings++, schema->face_set_names_std_string[i]);
        }
    }

    *r_names = schema->face_set_names_abc_string.data();
    *r_count = schema->face_set_names_abc_string.size();
}

template <typename Input_Schema_Object_Type>
struct Abc_Input_FaceSet *abc_input_object_schema_get_face_set(Input_Schema_Object_Type *schema,
                                                               Abc_String face_set_name)
{
    Abc_Input_FaceSet *résultat = make_object<Abc_Input_FaceSet>(schema->archive);
    résultat->typed_object = schema->impl->getFaceSet(face_set_name);
    résultat->untyped_object = résultat->typed_object;
    return résultat;
}

template <typename Input_Schema_Object_Type>
static bool abc_input_object_schema_has_face_set(Input_Schema_Object_Type *schema,
                                                 Abc_String face_set_name)
{
    return schema->impl->hasFaceSet(face_set_name);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Schema_Sample
 * \{ */

struct Abc_Input_Schema_Sample {
    Abc_Input_Archive *archive = nullptr;
    Array_Sample_Data sample_data{};
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_PolyMesh_Schema_Sample
 * À FAIRE
 *   IV2fGeomParam getUVsParam() const
 *   IN3fGeomParam getNormalsParam() const
 *   Abc::IInt32ArrayProperty getFaceCountsProperty() const
 *   Abc::IInt32ArrayProperty getFaceIndicesProperty() const
 *   Abc::IP3fArrayProperty getPositionsProperty() const
 *   Abc::IV3fArrayProperty getVelocitiesProperty() const
 * \{ */

struct Abc_Input_PolyMesh_Schema_Sample : public Abc_Input_Schema_Sample {
    AbcGeom::IPolyMeshSchema::Sample sample{};
};

DEFINE_COMMON_INPUT_SCHEMA_FUNCTIONS(PolyMesh, polymesh)
DEFINE_COMMON_INPUT_SAMPLE_FUNCTIONS(PolyMesh, polymesh)

void abc_input_polymesh_schema_get_face_set_names(struct Abc_Input_PolyMesh_Schema *schema,
                                                  Abc_String **r_names,
                                                  uint64_t *r_count)
{
    abc_input_object_schema_get_face_set_names(schema, r_names, r_count);
}

struct Abc_Input_FaceSet *abc_input_polymesh_schema_get_face_set(
    struct Abc_Input_PolyMesh_Schema *schema, Abc_String face_set_name)
{
    return abc_input_object_schema_get_face_set(schema, face_set_name);
}

bool abc_input_polymesh_schema_has_face_set(struct Abc_Input_PolyMesh_Schema *schema,
                                            Abc_String face_set_name)
{
    return abc_input_object_schema_has_face_set(schema, face_set_name);
}

DEFINE_POLYMESH_SAMPLE_ARRAY_GET_FUNCTIONS(DEFINE_INPUT_SAMPLE_ARRAY_GET_FUNCTION)

Abc_Mesh_Topology_Variance abc_input_polymesh_schema_get_topology_variance(
    Abc_Input_PolyMesh_Schema *schema)
{
    return static_cast<Abc_Mesh_Topology_Variance>(schema->impl->getTopologyVariance());
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_SubD_Schema_Sample
 * À FAIRE
 *   MeshTopologyVariance getTopologyVariance() const;
 *   Abc::IInt32ArrayProperty getFaceCountsProperty() const
 *   Abc::IInt32ArrayProperty getFaceIndicesProperty() const
 *   Abc::IP3fArrayProperty getPositionsProperty() const
 *   Abc::IInt32Property getFaceVaryingInterpolateBoundaryProperty() const
 *   Abc::IInt32Property getFaceVaryingPropagateCornersProperty() const
 *   Abc::IInt32Property getInterpolateBoundaryProperty() const
 *   Abc::IInt32ArrayProperty getCreaseIndicesProperty() const
 *   Abc::IInt32ArrayProperty getCreaseLengthsProperty() const
 *   Abc::IFloatArrayProperty getCreaseSharpnessesProperty() const
 *   Abc::IInt32ArrayProperty getCornerIndicesProperty() const
 *   Abc::IFloatArrayProperty getCornerSharpnessesProperty() const
 *   Abc::IInt32ArrayProperty getHolesProperty() const { return m_holesProperty; }
 *   Abc::IStringProperty getSubdivisionSchemeProperty() const
 *   Abc::IV3fArrayProperty getVelocitiesProperty() const
 *   IV2fGeomParam getUVsParam() const
 * \{ */

struct Abc_Input_SubD_Schema_Sample : public Abc_Input_Schema_Sample {
    AbcGeom::ISubDSchema::Sample sample{};
    std::string subdivision_scheme{};
};

DEFINE_COMMON_INPUT_SCHEMA_FUNCTIONS(SubD, subd)
DEFINE_COMMON_INPUT_SAMPLE_FUNCTIONS(SubD, subd)

void abc_input_subd_schema_get_face_set_names(struct Abc_Input_SubD_Schema *schema,
                                              Abc_String **r_names,
                                              uint64_t *r_count)
{
    abc_input_object_schema_get_face_set_names(schema, r_names, r_count);
}

struct Abc_Input_FaceSet *abc_input_subd_schema_get_face_set(struct Abc_Input_SubD_Schema *schema,
                                                             Abc_String face_set_name)
{
    return abc_input_object_schema_get_face_set(schema, face_set_name);
}

bool abc_input_subd_schema_has_face_set(struct Abc_Input_SubD_Schema *schema,
                                        Abc_String face_set_name)
{
    return abc_input_object_schema_has_face_set(schema, face_set_name);
}

DEFINE_SUBD_SAMPLE_SCALAR_GET_FUNCTION(DEFINE_INPUT_SAMPLE_SCALAR_GET_FUNCTION)
DEFINE_SUBD_SAMPLE_ARRAY_GET_FUNCTIONS(DEFINE_INPUT_SAMPLE_ARRAY_GET_FUNCTION)

Abc_String abc_input_subd_schema_sample_get_subdivision_scheme(
    struct Abc_Input_SubD_Schema_Sample *sample)
{
    sample->subdivision_scheme = sample->sample.getSubdivisionScheme();

    Abc_String résultat;
    vers_abc_string(&résultat, sample->subdivision_scheme);
    return résultat;
}

Abc_Mesh_Topology_Variance abc_input_subd_schema_get_topology_variance(
    Abc_Input_SubD_Schema *schema)
{
    return static_cast<Abc_Mesh_Topology_Variance>(schema->impl->getTopologyVariance());
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_FaceSet_Schema_Sample
 *  À FAIRE:
 *    FaceSetExclusivity getFaceExclusivity() const;
 *    Abc::IInt32ArrayProperty getFacesProperty() const
 * \{ */

struct Abc_Input_FaceSet_Schema_Sample : public Abc_Input_Schema_Sample {
    AbcGeom::IFaceSetSchema::Sample sample{};
};

DEFINE_COMMON_INPUT_SCHEMA_FUNCTIONS(FaceSet, face_set)
DEFINE_COMMON_INPUT_SAMPLE_FUNCTIONS(FaceSet, face_set)

DEFINE_FACE_SET_SAMPLE_ARRAY_GET_FUNCTIONS(DEFINE_INPUT_SAMPLE_ARRAY_GET_FUNCTION)

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Points_Schema_Sample
 *  À FAIRE:
 *    Abc::IP3fArrayProperty getPositionsProperty() const
 *    Abc::IV3fArrayProperty getVelocitiesProperty() const
 *    Abc::IUInt64ArrayProperty getIdsProperty() const
 *    IFloatGeomParam getWidthsParam() const
 * \{ */

struct Abc_Input_Points_Schema_Sample : public Abc_Input_Schema_Sample {
    AbcGeom::IPointsSchema::Sample sample{};
};

DEFINE_COMMON_INPUT_SCHEMA_FUNCTIONS(Points, points)
DEFINE_COMMON_INPUT_SAMPLE_FUNCTIONS(Points, points)

DEFINE_POINTS_SAMPLE_ARRAY_GET_FUNCTIONS(DEFINE_INPUT_SAMPLE_ARRAY_GET_FUNCTION)

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Curves_Schema_Sample
 *  À FAIRE: complète
 * \{ */

struct Abc_Input_Curves_Schema_Sample : public Abc_Input_Schema_Sample {
    AbcGeom::ICurvesSchema::Sample sample{};
};

DEFINE_COMMON_INPUT_SCHEMA_FUNCTIONS(Curves, curves)
DEFINE_COMMON_INPUT_SAMPLE_FUNCTIONS(Curves, curves)

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Xform_Op
 * \{ */

static_assert(sizeof(Abc_Xform_Op) == sizeof(AbcGeom::XformOp));
static_assert(alignof(Abc_Xform_Op) == alignof(AbcGeom::XformOp));

void abc_xform_op_init(struct Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    new (abc_op) AbcGeom::XformOp;
}

void abc_xform_op_init_type_hint(struct Abc_Xform_Op *op,
                                 enum Abc_Xform_Operation_Type type,
                                 uint8_t hint)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    new (abc_op) AbcGeom::XformOp(static_cast<AbcGeom::XformOperationType>(type), hint);
}

enum Abc_Xform_Operation_Type abc_xform_op_get_type(struct Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return static_cast<Abc_Xform_Operation_Type>(abc_op->getType());
}

void abc_xform_op_set_type(Abc_Xform_Op *op, Abc_Xform_Operation_Type type)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    abc_op->setType(static_cast<AbcGeom::XformOperationType>(type));
}

uint8_t abc_xform_op_get_hint(struct Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getHint();
}

void abc_xform_op_set_hint(Abc_Xform_Op *op, uint8_t hint)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    abc_op->setHint(hint);
}

bool abc_xform_op_is_x_animated(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isXAnimated();
}

bool abc_xform_op_is_y_animated(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isYAnimated();
}

bool abc_xform_op_is_z_animated(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isZAnimated();
}

bool abc_xform_op_is_angle_animated(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isAngleAnimated();
}

bool abc_xform_op_is_channel_animated(Abc_Xform_Op *op, uint64_t index)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isChannelAnimated(index);
}

uint64_t abc_xform_op_get_num_channels(struct Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getNumChannels();
}

double abc_xform_op_get_default_channel_value(struct Abc_Xform_Op *op, uint64_t index)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getDefaultChannelValue(index);
}

double abc_xform_op_get_channel_value(struct Abc_Xform_Op *op, uint64_t index)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getChannelValue(index);
}

void abc_xform_op_set_channel_value(Abc_Xform_Op *op, uint64_t index, double val)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    abc_op->setChannelValue(index, val);
}

void abc_xform_op_set_vector(Abc_Xform_Op *op, Abc_V3d *vec)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto abc_vec = *reinterpret_cast<Abc::V3d *>(vec);
    abc_op->setVector(abc_vec);
}

void abc_xform_op_set_translate(Abc_Xform_Op *op, Abc_V3d *trans)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto abc_trans = *reinterpret_cast<Abc::V3d *>(trans);
    abc_op->setTranslate(abc_trans);
}

void abc_xform_op_set_scale(Abc_Xform_Op *op, Abc_V3d *scale)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto abc_scale = *reinterpret_cast<Abc::V3d *>(scale);
    abc_op->setScale(abc_scale);
}

void abc_xform_op_set_axis(Abc_Xform_Op *op, Abc_V3d *axis)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto abc_axis = *reinterpret_cast<Abc::V3d *>(axis);
    abc_op->setAxis(abc_axis);
}

void abc_xform_op_set_angle(Abc_Xform_Op *op, double angle)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    abc_op->setAngle(angle);
}

void abc_xform_op_set_matrix(Abc_Xform_Op *op, Abc_M44d *matrix)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto abc_matrix = *reinterpret_cast<Abc::M44d *>(matrix);
    abc_op->setMatrix(abc_matrix);
}

void abc_xform_op_get_vector(Abc_Xform_Op *op, Abc_V3d *r_vec)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto vec = abc_op->getVector();
    *r_vec = *reinterpret_cast<Abc_V3d *>(&vec);
}

void abc_xform_op_get_translate(Abc_Xform_Op *op, Abc_V3d *r_trans)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto trans = abc_op->getTranslate();
    *r_trans = *reinterpret_cast<Abc_V3d *>(&trans);
}

void abc_xform_op_get_scale(Abc_Xform_Op *op, Abc_V3d *r_scale)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto scale = abc_op->getScale();
    *r_scale = *reinterpret_cast<Abc_V3d *>(&scale);
}

void abc_xform_op_get_axis(Abc_Xform_Op *op, Abc_V3d *r_axis)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto axis = abc_op->getAxis();
    *r_axis = *reinterpret_cast<Abc_V3d *>(&axis);
}

double abc_xform_op_get_angle(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getAngle();
}

void abc_xform_op_get_matrix(Abc_Xform_Op *op, Abc_M44d *r_matrix)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto matrix = abc_op->getMatrix();
    *r_matrix = *reinterpret_cast<Abc_M44d *>(&matrix);
}

double abc_xform_op_get_x_rotation(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getXRotation();
}

double abc_xform_op_get_y_rotation(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getYRotation();
}

double abc_xform_op_get_z_rotation(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getZRotation();
}

bool abc_xform_op_is_translate_op(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isTranslateOp();
}

bool abc_xform_op_is_scale_op(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isScaleOp();
}

bool abc_xform_op_is_rotate_op(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isRotateOp();
}

bool abc_xform_op_is_matrix_op(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isMatrixOp();
}

bool abc_xform_op_is_rotate_x_op(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isRotateXOp();
}

bool abc_xform_op_is_rotate_y_op(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isRotateYOp();
}

bool abc_xform_op_is_rotate_z_op(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isRotateZOp();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Xform_Schema
 *  À FAIRE: complète
 * \{ */

static Abc::V2d convertis_vers_abc(Abc_V2d *v)
{
    static_assert(sizeof(Abc::V2d) == sizeof(Abc_V2d));
    return *reinterpret_cast<Abc::V2d *>(v);
}

static void convertis_vers_kuri(Abc_V2d *résultat, Abc::V2d v)
{
    static_assert(sizeof(Abc::V2d) == sizeof(Abc_V2d));
    *reinterpret_cast<Abc::V2d *>(résultat) = v;
}

static Abc::V3d convertis_vers_abc(Abc_V3d *v)
{
    static_assert(sizeof(Abc::V3d) == sizeof(Abc_V3d));
    return *reinterpret_cast<Abc::V3d *>(v);
}

static void convertis_vers_kuri(Abc_V3d *résultat, Abc::V3d v)
{
    static_assert(sizeof(Abc::V3d) == sizeof(Abc_V3d));
    *reinterpret_cast<Abc::V3d *>(résultat) = v;
}

static Abc::M33d convertis_vers_abc(Abc_M33d *v)
{
    static_assert(sizeof(Abc::M33d) == sizeof(Abc_M33d));
    return *reinterpret_cast<Abc::M33d *>(v);
}

static void convertis_vers_kuri(Abc_M33d *résultat, Abc::M33d v)
{
    static_assert(sizeof(Abc::M33d) == sizeof(Abc_M33d));
    *reinterpret_cast<Abc::M33d *>(résultat) = v;
}

static Abc::M44d convertis_vers_abc(Abc_M44d *v)
{
    static_assert(sizeof(Abc::M44d) == sizeof(Abc_M44d));
    return *reinterpret_cast<Abc::M44d *>(v);
}

static void convertis_vers_kuri(Abc_M44d *résultat, Abc::M44d v)
{
    static_assert(sizeof(Abc::M44d) == sizeof(Abc_M44d));
    *reinterpret_cast<Abc::M44d *>(résultat) = v;
}

static Abc::Box3d convertis_vers_abc(Abc_Box3d *v)
{
    static_assert(sizeof(Abc::Box3d) == sizeof(Abc_Box3d));
    return *reinterpret_cast<Abc::Box3d *>(v);
}

static void convertis_vers_kuri(Abc_Box3d *résultat, Abc::Box3d v)
{
    static_assert(sizeof(Abc::Box3d) == sizeof(Abc_Box3d));
    *reinterpret_cast<Abc::Box3d *>(résultat) = v;
}

void abc_xform_sample_destroy(Abc_Xform_Sample *sample)
{
    if (sample) {
        kuri_deloge(sample->ctx_kuri, sample);
    }
}

uint64_t abc_xform_sample_add_traslate_or_scale_op(Abc_Xform_Sample *sample,
                                                   Abc_Xform_Op *translate_or_scale_op,
                                                   Abc_V3d *val)
{
    auto abc_op = *reinterpret_cast<AbcGeom::XformOp *>(translate_or_scale_op);
    auto abc_val = convertis_vers_abc(val);
    return sample->sample.addOp(abc_op, abc_val);
}

uint64_t abc_xform_sample_add_rotate_op(Abc_Xform_Sample *sample,
                                        Abc_Xform_Op *rotate_op,
                                        Abc_V3d *axis,
                                        Abc_Degrees degrees)
{
    auto abc_op = *reinterpret_cast<AbcGeom::XformOp *>(rotate_op);
    auto abc_axis = convertis_vers_abc(axis);
    return sample->sample.addOp(abc_op, abc_axis, degrees);
}

uint64_t abc_xform_sample_add_matrix_op(Abc_Xform_Sample *sample,
                                        Abc_Xform_Op *matrix_op,
                                        Abc_M44d *matrix)
{
    auto abc_op = *reinterpret_cast<AbcGeom::XformOp *>(matrix_op);
    auto abc_matrix = convertis_vers_abc(matrix);
    return sample->sample.addOp(abc_op, abc_matrix);
}

uint64_t abc_xform_sample_add_single_rotate_op(Abc_Xform_Sample *sample,
                                               Abc_Xform_Op *single_rotate_op,
                                               Abc_Degrees single_axis_rotation)
{
    auto abc_op = *reinterpret_cast<AbcGeom::XformOp *>(single_rotate_op);
    return sample->sample.addOp(abc_op, single_axis_rotation);
}

uint64_t abc_xform_sample_add_op(Abc_Xform_Sample *sample, Abc_Xform_Op *op)
{
    auto abc_op = *reinterpret_cast<AbcGeom::XformOp *>(op);
    return sample->sample.addOp(abc_op);
}

void abc_xform_sample_get_op(struct Abc_Xform_Sample *sample,
                             uint64_t index,
                             struct Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    *abc_op = sample->sample.getOp(index);
}

uint64_t abc_xform_sample_get_num_ops(struct Abc_Xform_Sample *sample)
{
    return sample->sample.getNumOps();
}

uint64_t abc_xform_sample_get_num_op_channels(struct Abc_Xform_Sample *sample)
{
    return sample->sample.getNumOpChannels();
}

void abc_xform_sample_set_inherits_xforms(Abc_Xform_Sample *sample, bool inherits)
{
    sample->sample.setInheritsXforms(inherits);
}

bool abc_xform_sample_get_inherits_xforms(Abc_Xform_Sample *sample)
{
    return sample->sample.getInheritsXforms();
}

void abc_xform_sample_set_translation(Abc_Xform_Sample *sample, Abc_V3d *trans)
{
    auto abc_trans = convertis_vers_abc(trans);
    sample->sample.setTranslation(abc_trans);
}

void abc_xform_sample_get_translation(Abc_Xform_Sample *sample, Abc_V3d *r_trans)
{
    auto trans = sample->sample.getTranslation();
    convertis_vers_kuri(r_trans, trans);
}

void abc_xform_sample_set_rotation(Abc_Xform_Sample *sample, Abc_V3d *axis, Abc_Degrees degrees)
{
    auto abc_axis = convertis_vers_abc(axis);
    sample->sample.setRotation(abc_axis, degrees);
}

void abc_xform_sample_et_axis(Abc_Xform_Sample *sample, Abc_V3d *r_axis)
{
    auto axis = sample->sample.getAxis();
    convertis_vers_kuri(r_axis, axis);
}

double abc_xform_sample_get_angle(Abc_Xform_Sample *sample)
{
    return sample->sample.getAngle();
}

void abc_xform_sample_set_x_rotation(Abc_Xform_Sample *sample, Abc_Degrees degrees)
{
    return sample->sample.setXRotation(degrees);
}

void abc_xform_sample_get_x_rotation(Abc_Xform_Sample *sample, Abc_Degrees *r_degrees)
{
    *r_degrees = sample->sample.getXRotation();
}

void abc_xform_sample_set_y_rotation(Abc_Xform_Sample *sample, Abc_Degrees degrees)
{
    return sample->sample.setYRotation(degrees);
}

void abc_xform_sample_get_y_rotation(Abc_Xform_Sample *sample, Abc_Degrees *r_degrees)
{
    *r_degrees = sample->sample.getYRotation();
}

void abc_xform_sample_set_z_rotation(Abc_Xform_Sample *sample, Abc_Degrees degrees)
{
    return sample->sample.setZRotation(degrees);
}

void abc_xform_sample_get_z_rotation(Abc_Xform_Sample *sample, Abc_Degrees *r_degrees)
{
    *r_degrees = sample->sample.getZRotation();
}

void abc_xform_sample_set_scale(Abc_Xform_Sample *sample, Abc_V3d *scale)
{
    auto abc_scale = convertis_vers_abc(scale);
    sample->sample.setScale(abc_scale);
}

void abc_xform_sample_get_scale(Abc_Xform_Sample *sample, Abc_V3d *r_scale)
{
    auto scale = sample->sample.getScale();
    convertis_vers_kuri(r_scale, scale);
}

void abc_xform_sample_set_matrix(Abc_Xform_Sample *sample, Abc_M44d *matrix)
{
    auto abc_matrix = convertis_vers_abc(matrix);
    sample->sample.setMatrix(abc_matrix);
}

void abc_xform_sample_get_matrix(Abc_Xform_Sample *sample, Abc_M44d *r_matrix)
{
    auto matrix = sample->sample.getMatrix();
    convertis_vers_kuri(r_matrix, matrix);
}

bool abc_xform_sample_is_topology_equal(Abc_Xform_Sample *sample, Abc_Xform_Sample *other)
{
    return sample->sample.isTopologyEqual(other->sample);
}

bool abc_xform_sample_get_is_topology_frozen(Abc_Xform_Sample *sample)
{
    return sample->sample.getIsTopologyFrozen();
}

void abc_xform_sample_reset(Abc_Xform_Sample *sample)
{
    sample->sample.reset();
}

DEFINE_COMMON_INPUT_SCHEMA_FUNCTIONS(Xform, xform)

Abc_Xform_Sample *abc_input_xform_schema_get_value(Abc_Input_Xform_Schema *schema,
                                                   Abc_Sample_Selector selector)
{
    auto résultat = kuri_loge<Abc_Xform_Sample>(schema->archive->ctx_kuri);
    résultat->ctx_kuri = schema->archive->ctx_kuri;
    résultat->sample = schema->impl->getValue(get_sample_selector(selector));
    return résultat;
}

void abc_input_xform_schema_get(Abc_Input_Xform_Schema *schema,
                                Abc_Xform_Sample *sample,
                                Abc_Sample_Selector selector)
{
    schema->impl->get(sample->sample, get_sample_selector(selector));
}

bool abc_input_xform_schema_is_constant_identity(Abc_Input_Xform_Schema *schema)
{
    return schema->impl->isConstantIdentity();
}

bool abc_input_xform_schema_get_inherits_xform(Abc_Input_Xform_Schema *schema,
                                               Abc_Sample_Selector selector)
{
    return schema->impl->getInheritsXforms(get_sample_selector(selector));
}

uint64_t abc_input_xform_schema_get_num_ops(Abc_Input_Xform_Schema *schema)
{
    return schema->impl->getNumOps();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Film_Back_Xform_Op
 * \{ */

static_assert(sizeof(Abc_Film_Back_Xform_Op) == sizeof(AbcGeom::FilmBackXformOp));
static_assert(alignof(Abc_Film_Back_Xform_Op) == alignof(AbcGeom::FilmBackXformOp));

void abc_film_back_xform_op_init(struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    new (abc_op) AbcGeom::FilmBackXformOp();
}

void abc_film_back_xform_op_init_type_hint(struct Abc_Film_Back_Xform_Op *op,
                                           enum Abc_Film_Back_Xform_Operation_Type type,
                                           struct Abc_String hint)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto abc_type = static_cast<AbcGeom::FilmBackXformOperationType>(type);
    new (abc_op) AbcGeom::FilmBackXformOp(abc_type, hint);
}

Abc_Film_Back_Xform_Operation_Type abc_film_back_xform_op_get_type(
    struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return static_cast<Abc_Film_Back_Xform_Operation_Type>(abc_op->getType());
}

Abc_New_String abc_film_back_xform_op_get_hint(struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto hint = abc_op->getHint();
    return make_new_string(hint);
}

Abc_New_String abc_film_back_xform_op_get_type_and_hint(struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto type_and_hint = abc_op->getTypeAndHint();
    return make_new_string(type_and_hint);
}

uint64_t abc_film_back_xform_op_get_num_channels(struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return abc_op->getNumChannels();
}

double abc_film_back_xform_op_get_channel_value(struct Abc_Film_Back_Xform_Op *op, uint64_t index)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return abc_op->getChannelValue(index);
}

void abc_film_back_xform_op_set_channel_value(struct Abc_Film_Back_Xform_Op *op,
                                              uint64_t index,
                                              double val)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return abc_op->setChannelValue(index, val);
}

void abc_film_back_xform_op_setTranslate(struct Abc_Film_Back_Xform_Op *op, Abc_V2d *trans)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto abc_trans = convertis_vers_abc(trans);
    abc_op->setTranslate(abc_trans);
}

void abc_film_back_xform_op_setScale(struct Abc_Film_Back_Xform_Op *op, Abc_V2d *scale)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto abc_scale = convertis_vers_abc(scale);
    abc_op->setScale(abc_scale);
}

void abc_film_back_xform_op_setMatrix(struct Abc_Film_Back_Xform_Op *op, Abc_M33d *matrix)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto abc_matrix = convertis_vers_abc(matrix);
    abc_op->setMatrix(abc_matrix);
}

void abc_film_back_xform_op_getTranslate(struct Abc_Film_Back_Xform_Op *op, Abc_V2d *result)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto abc_trans = abc_op->getTranslate();
    convertis_vers_kuri(result, abc_trans);
}

void abc_film_back_xform_op_getScale(struct Abc_Film_Back_Xform_Op *op, Abc_V2d *result)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto abc_scale = abc_op->getScale();
    convertis_vers_kuri(result, abc_scale);
}

void abc_film_back_xform_op_getMatrix(struct Abc_Film_Back_Xform_Op *op, Abc_M33d *result)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto abc_matrix = abc_op->getMatrix();
    convertis_vers_kuri(result, abc_matrix);
}

bool abc_film_back_xform_op_isTranslateOp(struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return abc_op->isTranslateOp();
}

bool abc_film_back_xform_op_isScaleOp(struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return abc_op->isScaleOp();
}

bool abc_film_back_xform_op_isMatrixOp(struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return abc_op->isMatrixOp();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Camera_Sample
 * \{ */

void abc_camera_sample_get_screen_window(struct Abc_Camera_Sample *sample,
                                         double *r_top,
                                         double *r_bottom,
                                         double *r_left,
                                         double *r_right)
{
    double top;
    double bottom;
    double left;
    double right;
    sample->sample.getScreenWindow(top, bottom, left, right);
    *r_top = top;
    *r_bottom = bottom;
    *r_left = left;
    *r_right = right;
}

#define DEFINE_CAMERA_SAMPLE_GET_SET(nom_ipa, nom_method, type_ipa)                               \
    type_ipa abc_camera_sample_get_##nom_ipa(struct Abc_Camera_Sample *sample)                    \
    {                                                                                             \
        return sample->sample.get##nom_method();                                                  \
    }                                                                                             \
    void abc_camera_sample_set_##nom_ipa(struct Abc_Camera_Sample *sample, type_ipa value)        \
    {                                                                                             \
        sample->sample.set##nom_method(value);                                                    \
    }

ENUMERATE_CAMERA_SAMPLE_PROPERTIES_SIMPLE(DEFINE_CAMERA_SAMPLE_GET_SET);

#undef DEFINE_CAMERA_SAMPLE_GET_SET

#define DEFINE_CAMERA_SAMPLE_GET_SET(nom_ipa, nom_method, type_ipa)                               \
    void abc_camera_sample_get_##nom_ipa(struct Abc_Camera_Sample *sample, type_ipa *value)       \
    {                                                                                             \
        *value = sample->sample.get##nom_method();                                                \
    }                                                                                             \
    void abc_camera_sample_set_##nom_ipa(struct Abc_Camera_Sample *sample, type_ipa *value)       \
    {                                                                                             \
        sample->sample.set##nom_method(*value);                                                   \
    }

ENUMERATE_CAMERA_SAMPLE_PROPERTIES_COMPLEX(DEFINE_CAMERA_SAMPLE_GET_SET);

void abc_camera_sample_get_child_bounds(struct Abc_Camera_Sample *sample, Abc_Box3d *r_value)
{
    auto value = sample->sample.getChildBounds();
    convertis_vers_kuri(r_value, value);
}

void abc_camera_sample_set_child_bounds(struct Abc_Camera_Sample *sample, Abc_Box3d *value)
{
    auto abc_value = convertis_vers_abc(value);
    sample->sample.setChildBounds(abc_value);
}

#undef DEFINE_CAMERA_SAMPLE_GET_SET

void abc_camera_sample_reset(struct Abc_Camera_Sample *sample)
{
    sample->sample.reset();
}

void abc_camera_sample_destroy(struct Abc_Camera_Sample *sample)
{
    if (sample) {
        kuri_deloge(sample->ctx_kuri, sample);
    }
}

double abc_camera_sample_get_core_value(struct Abc_Camera_Sample *sample, uint64_t index)
{
    return sample->sample.getCoreValue(index);
}

double abc_camera_sample_get_field_of_view(struct Abc_Camera_Sample *sample)
{
    return sample->sample.getFieldOfView();
}

uint64_t abc_camera_sample_add_op(Abc_Camera_Sample *sample, Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return sample->sample.addOp(*abc_op);
}

void abc_camera_sample_get_op(Abc_Camera_Sample *sample,
                              uint64_t index,
                              Abc_Film_Back_Xform_Op *r_op)
{
    auto op = sample->sample.getOp(index);
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(r_op);
    *abc_op = op;
}

void abc_camera_sample_get_film_back_matrix(Abc_Camera_Sample *sample, Abc_M33d *r_matrix)
{
    auto matrix = sample->sample.getFilmBackMatrix();
    convertis_vers_kuri(r_matrix, matrix);
}

uint64_t abc_camera_sample_get_num_ops(Abc_Camera_Sample *sample)
{
    return sample->sample.getNumOps();
}

uint64_t abc_camera_sample_get_num_op_channels(Abc_Camera_Sample *sample)
{
    return sample->sample.getNumOpChannels();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Camera
 * \{ */

DEFINE_COMMON_INPUT_SCHEMA_FUNCTIONS(Camera, camera)

struct Abc_Camera_Sample *abc_input_camera_schema_get_value(struct Abc_Input_Camera_Schema *schema,
                                                            struct Abc_Sample_Selector selector)
{
    auto résultat = kuri_loge<Abc_Camera_Sample>(schema->archive->ctx_kuri);
    résultat->ctx_kuri = schema->archive->ctx_kuri;
    résultat->sample = schema->impl->getValue(get_sample_selector(selector));
    return résultat;
}

void abc_input_camera_schema_get(struct Abc_Input_Camera_Schema *schema,
                                 struct Abc_Camera_Sample *sample,
                                 struct Abc_Sample_Selector selector)
{
    schema->impl->get(sample->sample, get_sample_selector(selector));
}

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
        kuri_deloge_liste(archive->ctx_kuri, archive->geom_params);
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
