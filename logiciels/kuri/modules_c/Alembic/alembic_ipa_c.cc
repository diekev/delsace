/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "alembic.h"

#include <string_view>
#include <variant>

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

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom MetaData
 * \{ */

struct Abc_MetaData {
    ContexteKuri *ctx_kuri = nullptr;
    Abc::MetaData metadata{};
};

void abc_metadata_destroy(struct Abc_MetaData *metadata)
{
    if (metadata) {
        kuri_deloge(metadata->ctx_kuri, metadata);
    }
}

struct Abc_MetaData_Iterator_Impl : public Abc_MetaData_Iterator {
    Abc::MetaData::const_iterator current{};
    Abc::MetaData::const_iterator end{};
};

static bool abc_metadata_iterator_next(Abc_MetaData_Iterator *iterator,
                                       Abc_String *key,
                                       Abc_String *value)
{
    auto iterator_impl = static_cast<Abc_MetaData_Iterator_Impl *>(iterator);

    if (iterator_impl->current == iterator_impl->end) {
        return false;
    }

    if (key) {
        key->characters = iterator_impl->current->first.data();
        key->size = iterator_impl->current->first.size();
    }
    if (value) {
        value->characters = iterator_impl->current->second.data();
        value->size = iterator_impl->current->second.size();
    }

    iterator_impl->current++;
    return true;
}

struct Abc_MetaData_Iterator *abc_metadata_get_iterator(struct Abc_MetaData *metadata)
{
    if (!metadata) {
        return nullptr;
    }

    auto résultat = kuri_loge<Abc_MetaData_Iterator_Impl>(metadata->ctx_kuri);
    résultat->metadata = metadata;
    résultat->next = abc_metadata_iterator_next;
    résultat->current = résultat->metadata->metadata.begin();
    résultat->end = résultat->metadata->metadata.end();
    return résultat;
}

void abc_metadata_iterator_destroy(struct Abc_MetaData_Iterator *iterator)
{
    if (iterator) {
        auto iterator_impl = static_cast<Abc_MetaData_Iterator_Impl *>(iterator);
        kuri_deloge(iterator->metadata->ctx_kuri, iterator_impl);
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Input_Archive
 * \{ */

struct Abc_Input_Archive {
    ContexteKuri *ctx_kuri = nullptr;
    Abc::IArchive iarchive{};
};

struct Abc_Input_Archive *abc_input_archive_create(ContexteKuri *ctx_kuri,
                                                   struct Abc_String *chemins,
                                                   uint64_t nombre_de_chemins)
{
    if (nombre_de_chemins == 0) {
        return nullptr;
    }

    std::vector<std::string> strings_chemins;
    for (size_t i = 0; i < nombre_de_chemins; ++i) {
        strings_chemins.push_back(std::string(chemins->characters, chemins->size));
        chemins += 1;
    }

    // À FAIRE : paramétrage
    Alembic::AbcCoreFactory::IFactory factory;

    Abc::IArchive iarchive = factory.getArchive(strings_chemins);
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
        kuri_deloge(archive->ctx_kuri, archive);
    }
}

Abc_MetaData *abc_input_archive_get_metadata(Abc_Input_Archive *archive)
{
    auto résultat = kuri_loge<Abc_MetaData>(archive->ctx_kuri);
    résultat->ctx_kuri = archive->ctx_kuri;
    résultat->metadata = archive->iarchive.getTop().getMetaData();
    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Compound_Property
 * \{ */

struct Abc_Output_Compound_Property {
    ContexteKuri *ctx_kuri;
    AbcGeom::OCompoundProperty prop;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Typed_Geom_Param
 * \{ */

template <typename Abc_Sample_Type, typename T>
auto make_array_sample(T *values, uint64_t num_values)
{
    return Abc_Sample_Type(values, num_values);
}

template <>
auto make_array_sample<AbcGeom::StringArraySample>(Abc_String *values, uint64_t num_values)
{
    std::vector<std::string> strings;
    strings.resize(num_values);
    for (uint64_t i = 0; i < num_values; i++) {
        strings[i] = vers_std_string(*values++);
    }
    return AbcGeom::StringArraySample(strings.data(), num_values);
}

#define MAKE_TYPED_ARRAY_SAMPLE(type_geom, type_abc_value, type_c, nom_court)                     \
    template <>                                                                                   \
    auto make_array_sample<AbcGeom::type_geom##ArraySample>(type_c * values, uint64_t num_values) \
    {                                                                                             \
        return AbcGeom::type_geom##ArraySample(reinterpret_cast<Abc::type_abc_value *>(values),   \
                                               num_values);                                       \
    }

ENUMERATE_ABC_ATTRIBUTE_SPECIAL(MAKE_TYPED_ARRAY_SAMPLE)

#undef MAKE_TYPED_ARRAY_SAMPLE

#define MAKE_TYPED_SAMPLE_FROM_ARRAY_SAMPLE(type_geom, type_abc_value, type_c, nom_court)         \
    static AbcGeom::type_geom##ArraySample make_typed_sample(                                     \
        Abc_##type_geom##_Array_Sample sample)                                                    \
    {                                                                                             \
        return make_array_sample<AbcGeom::type_geom##ArraySample>(sample.values,                  \
                                                                  sample.num_values);             \
    }                                                                                             \
    static AbcGeom::O##type_geom##GeomParam::Sample make_typed_sample(                            \
        Abc_Output_##type_geom##_Geom_Param_Sample param_sample)                                  \
    {                                                                                             \
        auto array_sample = make_array_sample<AbcGeom::type_geom##ArraySample>(                   \
            param_sample.values, param_sample.num_values);                                        \
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

#define DEFINE_ABC_OUTPUT_GEOM_PARAMS(type_geom, type_abc_value, type_c, nom_court)               \
    struct Abc_Output_##type_geom##_Geom_Param {                                                  \
        AbcGeom::O##type_geom##GeomParam param;                                                   \
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
        auto résultat = kuri_loge<Abc_Output_##type_geom##_Geom_Param>(parent->ctx_kuri);         \
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
            auto param_sample = make_typed_sample(*sample);                                       \
            param->param.set(param_sample);                                                       \
        }                                                                                         \
    }

ENUMERATE_ABC_ATTRIBUTE_TYPES(DEFINE_ABC_OUTPUT_GEOM_PARAMS)

#undef DECLARE_ABC_OUTPUT_GEOM_PARAMS

#define DEFINE_OUTPUT_SAMPLE_FUNCTIONS(uname, lname, snake_name, method, sample_type)             \
    void abc_output_##lname##_sample_##snake_name(                                                \
        struct Abc_Output_##uname##_Sample *lname##_sample, struct sample_type sample)            \
    {                                                                                             \
        auto typed_sample = make_typed_sample(sample);                                            \
        lname##_sample->sample.method(typed_sample);                                              \
    }

#define DEFINE_OUTPUT_SAMPLE_SCALAR_FUNCTIONS(uname, lname, snake_name, method, sample_type)      \
    void abc_output_##lname##_sample_##snake_name(                                                \
        struct Abc_Output_##uname##_Sample *lname##_sample, sample_type sample)                   \
    {                                                                                             \
        lname##_sample->sample.method(sample);                                                    \
    }

#define DEFINE_COMMON_OBJECT_FUNCTIONS(uname, lname)                                              \
    Abc_Output_Compound_Property *abc_output_##lname##_arb_geom_params_get(                       \
        struct Abc_Output_##uname *lname)                                                         \
    {                                                                                             \
        if (!lname->arb_geom_params_initialized) {                                                \
            lname->get_arb_geom_params(&lname->arb_geom_params);                                  \
            lname->arb_geom_params_initialized = true;                                            \
        }                                                                                         \
        return &lname->arb_geom_params;                                                           \
    }                                                                                             \
    Abc_Output_Compound_Property *abc_output_##lname##_user_properties_get(                       \
        struct Abc_Output_##uname *lname)                                                         \
    {                                                                                             \
        if (!lname->user_properties_initialized) {                                                \
            lname->get_user_properties(&lname->user_properties);                                  \
            lname->user_properties_initialized = true;                                            \
        }                                                                                         \
        return &lname->user_properties;                                                           \
    }

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Export
 * \{ */

using namespace Alembic;

struct Abc_Output_Object_Base {
    Abc_Output_Object_Base *next = nullptr;
    Abc_Output_Archive *archive = nullptr;

    Abc_Output_Compound_Property arb_geom_params{};
    Abc_Output_Compound_Property user_properties{};

    bool arb_geom_params_initialized = false;
    bool user_properties_initialized = false;

    virtual ~Abc_Output_Object_Base() = default;
};

struct Abc_Output_Xform : public Abc_Output_Object_Base {
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
};

struct Abc_Output_Points : public Abc_Output_Object_Base {
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
};

struct Abc_Output_Curves : public Abc_Output_Object_Base {
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
};

struct Abc_Output_Archive {
    ContexteKuri *ctx_kuri = nullptr;
    Abc::OArchive *archive = nullptr;

    Abc_Output_Xform *racine = nullptr;

    Abc_Output_Object_Base *objects = nullptr;
};

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
        while (object != nullptr) {
            auto next_object = object->next;
            kuri_deloge(archive->ctx_kuri, object);
            object = next_object;
        }
        kuri_deloge(archive->ctx_kuri, archive->archive);
        kuri_deloge(archive->ctx_kuri, archive);
    }
}

template <typename T>
T *crée_objet_sortie(Abc_Output_Archive *archive)
{
    auto résultat = kuri_loge<T>(archive->ctx_kuri);
    résultat->next = archive->objects;
    archive->objects = résultat;
    résultat->archive = archive;
    return résultat;
}

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

DEFINE_COMMON_OBJECT_FUNCTIONS(Xform, xform)

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
        Abc_Output_Archive *archive)                                                              \
    {                                                                                             \
        auto résultat = kuri_loge<Abc_Output_##uppercase_name##_Sample>(archive->ctx_kuri);       \
        résultat->ctx_kuri = archive->ctx_kuri;                                                   \
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
    }                                                                                             \
    void abc_output_##lowercase_name##_sample_set_from_previous(                                  \
        Abc_Output_##uppercase_name *lowercase_name)                                              \
    {                                                                                             \
        lowercase_name->set_from_previous();                                                      \
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

DEFINE_COMMON_OBJECT_FUNCTIONS(Points, points)

struct Abc_Output_Points_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::OPointsSchema::Sample sample{};
};

DEFINE_COMMON_SAMPLE_FONCTIONS(Points, points)

ENUMERATE_POINTS_SAMPLE_INTERFACE(DEFINE_OUTPUT_SAMPLE_FUNCTIONS)

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_Curves
 * \{ */

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

DEFINE_COMMON_OBJECT_FUNCTIONS(Curves, curves)

struct Abc_Output_Curves_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::OCurvesSchema::Sample sample{};
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

struct Abc_Output_FaceSet : public Abc_Output_Object_Base {
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
};

DEFINE_COMMON_OBJECT_FUNCTIONS(FaceSet, faceset)

struct Abc_Output_FaceSet_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::OFaceSetSchema::Sample sample{};
};

DEFINE_COMMON_SAMPLE_FONCTIONS(FaceSet, faceset)

ENUMERATE_FACESET_SAMPLE_INTERFACE(DEFINE_OUTPUT_SAMPLE_FUNCTIONS)

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_PolyMesh
 * \{ */

struct Abc_Output_PolyMesh : public Abc_Output_Object_Base {
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
};

Abc_Output_PolyMesh *abc_output_poly_mesh_create(Abc_Output_Xform *parent,
                                                 Abc_String nom,
                                                 Abc_Time_Sample_Index time_sample_index)
{
    auto archive = parent->archive;
    auto résultat = crée_objet_sortie<Abc_Output_PolyMesh>(archive);
    résultat->object = AbcGeom::OPolyMesh(
        parent->object, vers_std_string(nom), time_sample_index.value);
    return résultat;
}

DEFINE_COMMON_OBJECT_FUNCTIONS(PolyMesh, polymesh)

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
};

DEFINE_COMMON_SAMPLE_FONCTIONS(PolyMesh, polymesh)

ENUMERATE_POLYMESH_SAMPLE_INTERFACE(DEFINE_OUTPUT_SAMPLE_FUNCTIONS)

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Output_SubD
 * \{ */

struct Abc_Output_SubD : public Abc_Output_Object_Base {
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

DEFINE_COMMON_OBJECT_FUNCTIONS(SubD, subd)

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

struct Abc_Output_Camera : public Abc_Output_Object_Base {
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

DEFINE_COMMON_OBJECT_FUNCTIONS(Camera, camera)

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
