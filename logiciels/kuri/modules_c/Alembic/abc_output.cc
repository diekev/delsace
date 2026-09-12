/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020-2026 Kévin Dietrich. */

#include "abc_common.hh"

#include "../InterfaceCKuri/contexte_kuri.hh"

struct Abc_Output_Object;
struct Abc_Output_Geom_Param;

/* ------------------------------------------------------------------------- */
/** \nom Array_Sample
 * \{ */

template <typename Abc_Sample_Type, typename IPA_Type>
auto make_array_sample(IPA_Type sample, Array_Sample_Data *)
{
    return Abc_Sample_Type(sample.values, sample.num_values);
}

template <>
auto make_array_sample<AbcGeom::StringArraySample>(Abc_String_Array_Sample sample,
                                                   Array_Sample_Data *sample_data)
{
    sample_data->strings.resize(sample.num_values);
    for (uint64_t i = 0; i < sample.num_values; i++) {
        sample_data->strings[i] = vers_std_string(*sample.values++);
    }
    return AbcGeom::StringArraySample(sample_data->strings.data(), sample.num_values);
}

#define MAKE_TYPED_ARRAY_SAMPLE(type_geom, type_abc_value, type_c, nom_court)                     \
    template <>                                                                                   \
    auto make_array_sample<AbcGeom::type_geom##ArraySample>(                                      \
        Abc_##type_geom##_Array_Sample sample, Array_Sample_Data *)                               \
    {                                                                                             \
        return AbcGeom::type_geom##ArraySample(reinterpret_cast<type_abc_value *>(sample.values), \
                                               sample.num_values);                                \
    }

ENUMERATE_ABC_ATTRIBUTE_SPECIAL(MAKE_TYPED_ARRAY_SAMPLE)

#undef MAKE_TYPED_ARRAY_SAMPLE

#define MAKE_TYPED_SAMPLE_FROM_ARRAY_SAMPLE(type_geom, type_abc_value, type_c, nom_court)         \
    static AbcGeom::type_geom##ArraySample make_typed_sample(                                     \
        Abc_##type_geom##_Array_Sample sample, Array_Sample_Data *sample_data)                    \
    {                                                                                             \
        return make_array_sample<AbcGeom::type_geom##ArraySample>(sample, sample_data);           \
    }                                                                                             \
    static AbcGeom::O##type_geom##GeomParam::Sample make_typed_sample(                            \
        Abc_Output_##type_geom##_Geom_Param_Sample param_sample, Array_Sample_Data *sample_data)  \
    {                                                                                             \
        auto array_sample = make_array_sample<AbcGeom::type_geom##ArraySample>(                   \
            param_sample.values, sample_data);                                                    \
        auto abc_scope = static_cast<AbcGeom::GeometryScope>(param_sample.scope);                 \
        if (param_sample.indices.values) {                                                        \
            auto indices = make_array_sample<AbcGeom::UInt32ArraySample>(param_sample.indices,    \
                                                                         sample_data);            \
            return AbcGeom::O##type_geom##GeomParam::Sample(array_sample, indices, abc_scope);    \
        }                                                                                         \
        auto sample = AbcGeom::O##type_geom##GeomParam::Sample(array_sample, abc_scope);          \
        return sample;                                                                            \
    }

ENUMERATE_ABC_ATTRIBUTE_TYPES(MAKE_TYPED_SAMPLE_FROM_ARRAY_SAMPLE)

#undef MAKE_TYPED_SAMPLE_FROM_ARRAY_SAMPLE

/** \} */

struct Abc_Output_Archive {
    ContexteKuri *ctx_kuri = nullptr;
    Abc::OArchive *archive = nullptr;

    Abc_Output_Xform *racine = nullptr;

    Abc_Output_Object *objects = nullptr;

    Abc_Output_Compound_Property *compound_props = nullptr;
    Abc_Output_Scalar_Property *scalar_props = nullptr;
    Abc_Output_Array_Property *array_props = nullptr;
    Abc_Output_Geom_Param *geom_params = nullptr;
};

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Compound_Property
 * \{ */

struct Abc_Output_Compound_Property {
    Abc_Output_Archive *archive = nullptr;
    AbcGeom::OCompoundProperty prop{};
    Abc_Output_Compound_Property *next = nullptr;
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

struct Abc_Output_Geom_Param {
    Abc_Output_Archive *archive = nullptr;
    Abc_Output_Geom_Param *next = nullptr;

    virtual ~Abc_Output_Geom_Param() = default;
};

template <typename T>
T *make_output_geom_param(Abc_Output_Archive *archive)
{
    auto résultat = kuri_loge<T>(archive->ctx_kuri);
    liste_ajoute(&archive->geom_params, static_cast<Abc_Output_Geom_Param *>(résultat));
    résultat->archive = archive;
    return résultat;
}

#define DEFINE_ABC_OUTPUT_GEOM_PARAMS(type_geom, type_abc_value, type_c, nom_court)               \
    struct Abc_Output_##type_geom##_Geom_Param : public Abc_Output_Geom_Param {                   \
        AbcGeom::O##type_geom##GeomParam param{};                                                 \
        Array_Sample_Data sample_data{};                                                          \
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
        auto résultat = make_output_geom_param<Abc_Output_##type_geom##_Geom_Param>(              \
            parent->archive);                                                                     \
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
        if (sample->values.values) {                                                              \
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
        kuri_deloge_liste(archive->ctx_kuri, archive->scalar_props);
        kuri_deloge_liste(archive->ctx_kuri, archive->array_props);
        kuri_deloge_liste(archive->ctx_kuri, archive->geom_params);
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

Abc_Xform_Sample *abc_output_xform_sample_create(Abc_Output_Xform *xform)
{
    auto résultat = kuri_loge<Abc_Xform_Sample>(xform->archive->ctx_kuri);
    résultat->ctx_kuri = xform->archive->ctx_kuri;
    return résultat;
}

void abc_output_xform_sample_set(Abc_Output_Xform *xform, Abc_Xform_Sample *sample)
{
    xform->set_sample(sample->sample);
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
    AbcGeom::OFaceSet object{};

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

struct Abc_Camera_Sample *abc_output_camera_create_sample(struct Abc_Output_Camera *camera)
{
    auto résultat = kuri_loge<Abc_Camera_Sample>(camera->archive->ctx_kuri);
    résultat->ctx_kuri = camera->archive->ctx_kuri;
    résultat->sample = AbcGeom::CameraSample();
    return résultat;
}

struct Abc_Camera_Sample *abc_output_camera_sample_create_window(
    struct Abc_Output_Archive *archive, double top, double bottom, double left, double right)
{
    auto résultat = kuri_loge<Abc_Camera_Sample>(archive->ctx_kuri);
    résultat->ctx_kuri = archive->ctx_kuri;
    résultat->sample = AbcGeom::CameraSample(top, bottom, left, right);
    return résultat;
}

void abc_output_camera_sample_set(struct Abc_Output_Camera *camera,
                                  struct Abc_Camera_Sample *sample)
{
    camera->set_sample(sample->sample);
}

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
                                        struct Abc_Camera_Sample *sample)
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
