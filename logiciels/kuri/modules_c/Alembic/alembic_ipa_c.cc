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
/** \nom Export
 * \{ */

using namespace Alembic;

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

struct Abc_Output_Object_Base {
    Abc_Output_Object_Base *next = nullptr;
    Abc_Output_Archive *archive = nullptr;

    virtual ~Abc_Output_Object_Base() = default;
};

struct Abc_Output_Xform : public Abc_Output_Object_Base {
    Abc::OObject object{};
    AbcGeom::OXformSchema schema{};
};

struct Abc_Output_Points : public Abc_Output_Object_Base {
    AbcGeom::OPoints object{};
};

struct Abc_Output_Curves : public Abc_Output_Object_Base {
    AbcGeom::OCurves object{};
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

struct Abc_Output_Xform_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::XformSample sample{};
};

Abc_Output_Xform_Sample *abc_output_xform_sample_create(Abc_Output_Archive *archive)
{
    auto résultat = kuri_loge<Abc_Output_Xform_Sample>(archive->ctx_kuri);
    résultat->ctx_kuri = archive->ctx_kuri;
    return résultat;
}

void abc_output_xform_sample_reset(Abc_Output_Xform_Sample *sample)
{
    sample->sample = {};
}

void abc_output_xform_sample_destroy(Abc_Output_Xform_Sample *sample)
{
    if (sample) {
        kuri_deloge(sample->ctx_kuri, sample);
    }
}

void abc_output_xform_sample_set_matrix(Abc_Output_Xform_Sample *sample, float *matrix)
{
    double m[4][4];

    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 4; i++) {
            m[i][j] = matrix[j * 4 + i];
        }
    }

    sample->sample.setMatrix(m);
}

void abc_output_xform_sample_set_inherits_xform(Abc_Output_Xform_Sample *sample, bool inherits)
{
    sample->sample.setInheritsXforms(inherits);
}

void abc_output_xform_sample_set(Abc_Output_Xform *xform, Abc_Output_Xform_Sample *sample)
{
    xform->schema.set(sample->sample);
}

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

struct Abc_Output_Points_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::OPointsSchema::Sample sample{};
};

Abc_Output_Points_Sample *abc_output_points_sample_create(Abc_Output_Archive *archive)
{
    auto résultat = kuri_loge<Abc_Output_Points_Sample>(archive->ctx_kuri);
    résultat->ctx_kuri = archive->ctx_kuri;
    return résultat;
}

void abc_output_points_sample_reset(Abc_Output_Points_Sample *sample)
{
    sample->sample = {};
}

void abc_output_points_sample_destroy(Abc_Output_Points_Sample *sample)
{
    if (sample) {
        kuri_deloge(sample->ctx_kuri, sample);
    }
}

void abc_output_points_sample_positions_set(Abc_Output_Points_Sample *sample,
                                            float *positions,
                                            uint64_t num_positions)
{
    auto p3f_sample = AbcGeom::P3fArraySample(reinterpret_cast<AbcGeom::V3f *>(positions),
                                              num_positions);
    sample->sample.setPositions(p3f_sample);
}

void abc_output_points_sample_velocities_set(Abc_Output_Points_Sample *sample,
                                             float *velocities,
                                             uint64_t num_velocities)
{
    auto v3f_sample = AbcGeom::V3fArraySample(reinterpret_cast<AbcGeom::V3f *>(velocities),
                                              num_velocities);
    sample->sample.setVelocities(v3f_sample);
}

void abc_output_points_sample_widths_set(Abc_Output_Points_Sample *sample,
                                         float *widths,
                                         uint64_t num_widths)
{
    auto values = AbcGeom::FloatArraySample(widths, num_widths);
    auto f_sample = AbcGeom::OFloatGeomParam::Sample(values, AbcGeom::GeometryScope::kVertexScope);
    sample->sample.setWidths(f_sample);
}

void abc_output_points_sample_ids_set(Abc_Output_Points_Sample *sample,
                                      uint64_t *ids,
                                      uint64_t num_ids)
{
    auto uint64_sample = AbcGeom::UInt64ArraySample(ids, num_ids);
    sample->sample.setIds(uint64_sample);
}

void abc_output_points_sample_set(Abc_Output_Points *points, Abc_Output_Points_Sample *sample)
{
    auto &opoints = points->object;
    opoints.getSchema().set(sample->sample);
}

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

struct Abc_Output_Curves_Sample {
    ContexteKuri *ctx_kuri = nullptr;
    AbcGeom::OCurvesSchema::Sample sample{};
};

Abc_Output_Curves_Sample *abc_output_curves_sample_create(Abc_Output_Archive *archive)
{
    auto résultat = kuri_loge<Abc_Output_Curves_Sample>(archive->ctx_kuri);
    résultat->ctx_kuri = archive->ctx_kuri;
    return résultat;
}

void abc_output_curves_sample_reset(Abc_Output_Curves_Sample *sample)
{
    sample->sample = {};
}

void abc_output_curves_sample_destroy(Abc_Output_Curves_Sample *sample)
{
    if (sample) {
        kuri_deloge(sample->ctx_kuri, sample);
    }
}

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

void abc_output_curves_sample_positions_set(Abc_Output_Curves_Sample *sample,
                                            float *positions,
                                            uint64_t num_positions)
{
    auto p3f_sample = AbcGeom::P3fArraySample(reinterpret_cast<AbcGeom::V3f *>(positions),
                                              num_positions);
    sample->sample.setPositions(p3f_sample);
}

void abc_output_curves_sample_position_weights_set(Abc_Output_Curves_Sample *sample,
                                                   float *weights,
                                                   uint64_t num_weights)
{
    auto values = AbcGeom::FloatArraySample(weights, num_weights);
    sample->sample.setPositionWeights(values);
}

void abc_output_curves_sample_velocities_set(Abc_Output_Curves_Sample *sample,
                                             float *velocities,
                                             uint64_t num_velocities)
{
    auto v3f_sample = AbcGeom::V3fArraySample(reinterpret_cast<AbcGeom::V3f *>(velocities),
                                              num_velocities);
    sample->sample.setVelocities(v3f_sample);
}

void abc_output_curves_sample_widths_set(Abc_Output_Curves_Sample *sample,
                                         float *widths,
                                         uint64_t num_widths)
{
    auto values = AbcGeom::FloatArraySample(widths, num_widths);
    auto f_sample = AbcGeom::OFloatGeomParam::Sample(values, AbcGeom::GeometryScope::kVertexScope);
    sample->sample.setWidths(f_sample);
}

void abc_output_curves_sample_curves_num_vertices_set(Abc_Output_Curves_Sample *sample,
                                                      int *num_vertices,
                                                      uint64_t num_nun_vertices)
{
    auto values = AbcGeom::Int32ArraySample(num_vertices, num_nun_vertices);
    sample->sample.setCurvesNumVertices(values);
}

void abc_output_curves_sample_orders_set(Abc_Output_Curves_Sample *sample,
                                         uint8_t *values,
                                         uint64_t num_values)
{
    auto array_sample = AbcGeom::UcharArraySample(values, num_values);
    sample->sample.setKnots(array_sample);
}

void abc_output_curves_sample_knots_set(Abc_Output_Curves_Sample *sample,
                                        float *values,
                                        uint64_t num_values)
{
    auto array_sample = AbcGeom::FloatArraySample(values, num_values);
    sample->sample.setOrders(array_sample);
}

void abc_output_curves_sample_uvs_set(Abc_Output_Curves_Sample *sample,
                                      float *values,
                                      uint64_t num_values)
{
    auto array_sample = AbcGeom::V2fArraySample(reinterpret_cast<Abc::V2f *>(values), num_values);
    auto geom_param = AbcGeom::OV2fGeomParam::Sample(array_sample,
                                                     AbcGeom::GeometryScope::kUniformScope);
    sample->sample.setUVs(geom_param);
}

void abc_output_curves_sample_normals_set(Abc_Output_Curves_Sample *sample,
                                          float *values,
                                          uint64_t num_values)
{
    auto array_sample = AbcGeom::N3fArraySample(reinterpret_cast<Abc::N3f *>(values), num_values);
    auto geom_param = AbcGeom::ON3fGeomParam::Sample(array_sample,
                                                     AbcGeom::GeometryScope::kUniformScope);
    sample->sample.setNormals(geom_param);
}

void abc_output_curves_sample_set(Abc_Output_Curves *curves, Abc_Output_Curves_Sample *sample)
{
    auto &ocurves = curves->object;
    ocurves.getSchema().set(sample->sample);
}

/** \} */
