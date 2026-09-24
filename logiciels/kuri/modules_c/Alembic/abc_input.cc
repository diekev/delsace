/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020-2026 Kévin Dietrich. */

#include "abc_common.hh"

#include <fstream>

#include "../InterfaceCKuri/contexte_kuri.hh"

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
/** \nom Array_Sample
 * \{ */

template <typename IPA_Type, typename Alembic_Type>
auto make_input_array_sample(std::shared_ptr<Alembic_Type> ptr, Array_Sample_Data &)
{
    using value_type = typename Alembic_Type::value_type;
    if (!ptr) {
        return IPA_Type(nullptr, 0);
    }
    auto values = const_cast<value_type *>((*ptr).get());
    return IPA_Type{reinterpret_cast<decltype(IPA_Type::values)>(values), (*ptr).size()};
}

template <>
auto make_input_array_sample<Abc_String_Array_Sample, AbcGeom::StringArraySample>(
    AbcGeom::StringArraySamplePtr ptr, Array_Sample_Data &data)
{
    if (!ptr) {
        return Abc_String_Array_Sample(nullptr, 0);
    }
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
    std::vector<std::string> faceset_names_std_string{};
    std::vector<Abc_String> faceset_names_abc_string{};

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

void abc_input_object_get_name(Abc_Generic_Input_Object object, struct Abc_String *name)
{
    vers_abc_string(name, object.object->untyped_object.getName());
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
static void abc_input_object_schema_get_faceset_names(Input_Schema_Object_Type *schema,
                                                      Abc_String **r_names,
                                                      uint64_t *r_count)
{
    if (schema->faceset_names_std_string.empty()) {
        schema->impl->getFaceSetNames(schema->faceset_names_std_string);

        schema->faceset_names_abc_string.resize(schema->faceset_names_std_string.size());

        auto strings = schema->faceset_names_abc_string.data();
        auto num_strings = schema->faceset_names_abc_string.size();
        for (auto i = 0ul; i < num_strings; i++) {
            vers_abc_string(strings++, schema->faceset_names_std_string[i]);
        }
    }

    *r_names = schema->faceset_names_abc_string.data();
    *r_count = schema->faceset_names_abc_string.size();
}

template <typename Input_Schema_Object_Type>
struct Abc_Input_FaceSet *abc_input_object_schema_get_faceset(Input_Schema_Object_Type *schema,
                                                              Abc_String faceset_name)
{
    Abc_Input_FaceSet *résultat = make_object<Abc_Input_FaceSet>(schema->archive);
    résultat->typed_object = schema->impl->getFaceSet(faceset_name);
    résultat->untyped_object = résultat->typed_object;
    return résultat;
}

template <typename Input_Schema_Object_Type>
static bool abc_input_object_schema_has_faceset(Input_Schema_Object_Type *schema,
                                                Abc_String faceset_name)
{
    return schema->impl->hasFaceSet(faceset_name);
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

void abc_input_polymesh_schema_get_faceset_names(struct Abc_Input_PolyMesh_Schema *schema,
                                                 Abc_String **r_names,
                                                 uint64_t *r_count)
{
    abc_input_object_schema_get_faceset_names(schema, r_names, r_count);
}

struct Abc_Input_FaceSet *abc_input_polymesh_schema_get_faceset(
    struct Abc_Input_PolyMesh_Schema *schema, Abc_String faceset_name)
{
    return abc_input_object_schema_get_faceset(schema, faceset_name);
}

bool abc_input_polymesh_schema_has_faceset(struct Abc_Input_PolyMesh_Schema *schema,
                                           Abc_String faceset_name)
{
    return abc_input_object_schema_has_faceset(schema, faceset_name);
}

DEFINE_POLYMESH_SAMPLE_ARRAY_GET_FUNCTIONS(DEFINE_INPUT_SAMPLE_ARRAY_GET_FUNCTION)

Abc_Mesh_Topology_Variance abc_input_polymesh_schema_get_topology_variance(
    Abc_Input_PolyMesh_Schema *schema)
{
    return static_cast<Abc_Mesh_Topology_Variance>(schema->impl->getTopologyVariance());
}

Abc_Input_V2f_Geom_Param *abc_input_polymesh_schema_get_uvs_param(
    struct Abc_Input_PolyMesh_Schema *schema)
{
    auto résultat = make_input_geom_param<Abc_Input_V2f_Geom_Param>(schema->archive);
    résultat->param = schema->impl->getUVsParam();
    return résultat;
}

Abc_Input_N3f_Geom_Param *abc_input_polymesh_schema_get_normals_param(
    struct Abc_Input_PolyMesh_Schema *schema)
{
    auto résultat = make_input_geom_param<Abc_Input_N3f_Geom_Param>(schema->archive);
    résultat->param = schema->impl->getNormalsParam();
    return résultat;
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
 * \{ */

struct Abc_Input_SubD_Schema_Sample : public Abc_Input_Schema_Sample {
    AbcGeom::ISubDSchema::Sample sample{};
    std::string subdivision_scheme{};
};

DEFINE_COMMON_INPUT_SCHEMA_FUNCTIONS(SubD, subd)
DEFINE_COMMON_INPUT_SAMPLE_FUNCTIONS(SubD, subd)

void abc_input_subd_schema_get_faceset_names(struct Abc_Input_SubD_Schema *schema,
                                             Abc_String **r_names,
                                             uint64_t *r_count)
{
    abc_input_object_schema_get_faceset_names(schema, r_names, r_count);
}

struct Abc_Input_FaceSet *abc_input_subd_schema_get_faceset(struct Abc_Input_SubD_Schema *schema,
                                                            Abc_String faceset_name)
{
    return abc_input_object_schema_get_faceset(schema, faceset_name);
}

bool abc_input_subd_schema_has_faceset(struct Abc_Input_SubD_Schema *schema,
                                       Abc_String faceset_name)
{
    return abc_input_object_schema_has_faceset(schema, faceset_name);
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

Abc_Input_V2f_Geom_Param *abc_input_subd_schema_get_uvs_param(struct Abc_Input_SubD_Schema *schema)
{
    auto résultat = make_input_geom_param<Abc_Input_V2f_Geom_Param>(schema->archive);
    résultat->param = schema->impl->getUVsParam();
    return résultat;
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

DEFINE_COMMON_INPUT_SCHEMA_FUNCTIONS(FaceSet, faceset)
DEFINE_COMMON_INPUT_SAMPLE_FUNCTIONS(FaceSet, faceset)

DEFINE_FACE_SET_SAMPLE_ARRAY_GET_FUNCTIONS(DEFINE_INPUT_SAMPLE_ARRAY_GET_FUNCTION)

enum Abc_FaceSet_Exclusivity abc_input_faceset_schema_get_face_exclusivity(
    struct Abc_Input_FaceSet_Schema *schema)
{
    return static_cast<Abc_FaceSet_Exclusivity>(schema->impl->getFaceExclusivity());
}

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
/** \nom Abc_Input_Xform_Schema
 * \{ */

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
