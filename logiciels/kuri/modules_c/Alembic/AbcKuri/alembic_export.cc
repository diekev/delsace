/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "alembic_export.hh"

#include <numeric>
#include <optional>
#include <variant>

#include "../../InterfaceCKuri/contexte_kuri.hh"

#include "alembic_archive.hh"
#include "alembic_export_attributs.hh"
#include "alembic_import.hh"
#include "utilitaires.hh"

#include "../alembic.h"
#include "../alembic_types.h"

using namespace Alembic;

/* ------------------------------------------------------------------------- */
/** \nom Écriture des objets Alembic depuis leurs convertisseuse.
 * \{ */

namespace AbcKuri {

#define ENUMERE_TYPE_PARAMS(O)                                                                    \
    O(OBoolGeomParam, OBoolGeomParam)                                                             \
    O(OUcharGeomParam, OUcharGeomParam)                                                           \
    O(OCharGeomParam, OCharGeomParam)                                                             \
    O(OUInt16GeomParam, OUInt16GeomParam)                                                         \
    O(OInt16GeomParam, OInt16GeomParam)                                                           \
    O(OUInt32GeomParam, OUInt32GeomParam)                                                         \
    O(OInt32GeomParam, OInt32GeomParam)                                                           \
    O(OUInt64GeomParam, OUInt64GeomParam)                                                         \
    O(OInt64GeomParam, OInt64GeomParam)                                                           \
    O(OHalfGeomParam, OHalfGeomParam)                                                             \
    O(OFloatGeomParam, OFloatGeomParam)                                                           \
    O(ODoubleGeomParam, ODoubleGeomParam)                                                         \
    O(OStringGeomParam, OStringGeomParam)                                                         \
    O(OWstringGeomParam, OWstringGeomParam)                                                       \
    O(OV2sGeomParam, OV2sGeomParam)                                                               \
    O(OV2iGeomParam, OV2iGeomParam)                                                               \
    O(OV2fGeomParam, OV2fGeomParam)                                                               \
    O(OV2dGeomParam, OV2dGeomParam)                                                               \
    O(OV3sGeomParam, OV3sGeomParam)                                                               \
    O(OV3iGeomParam, OV3iGeomParam)                                                               \
    O(OV3fGeomParam, OV3fGeomParam)                                                               \
    O(OV3dGeomParam, OV3dGeomParam)                                                               \
    O(OP2sGeomParam, OP2sGeomParam)                                                               \
    O(OP2iGeomParam, OP2iGeomParam)                                                               \
    O(OP2fGeomParam, OP2fGeomParam)                                                               \
    O(OP2dGeomParam, OP2dGeomParam)                                                               \
    O(OP3sGeomParam, OP3sGeomParam)                                                               \
    O(OP3iGeomParam, OP3iGeomParam)                                                               \
    O(OP3fGeomParam, OP3fGeomParam)                                                               \
    O(OP3dGeomParam, OP3dGeomParam)                                                               \
    O(OBox2sGeomParam, OBox2sGeomParam)                                                           \
    O(OBox2iGeomParam, OBox2iGeomParam)                                                           \
    O(OBox2fGeomParam, OBox2fGeomParam)                                                           \
    O(OBox2dGeomParam, OBox2dGeomParam)                                                           \
    O(OBox3sGeomParam, OBox3sGeomParam)                                                           \
    O(OBox3iGeomParam, OBox3iGeomParam)                                                           \
    O(OBox3fGeomParam, OBox3fGeomParam)                                                           \
    O(OBox3dGeomParam, OBox3dGeomParam)                                                           \
    O(OM33fGeomParam, OM33fGeomParam)                                                             \
    O(OM33dGeomParam, OM33dGeomParam)                                                             \
    O(OM44fGeomParam, OM44fGeomParam)                                                             \
    O(OM44dGeomParam, OM44dGeomParam)                                                             \
    O(OQuatfGeomParam, OQuatfGeomParam)                                                           \
    O(OQuatdGeomParam, OQuatdGeomParam)                                                           \
    O(OC3hGeomParam, OC3hGeomParam)                                                               \
    O(OC3fGeomParam, OC3fGeomParam)                                                               \
    O(OC3cGeomParam, OC3cGeomParam)                                                               \
    O(OC4hGeomParam, OC4hGeomParam)                                                               \
    O(OC4fGeomParam, OC4fGeomParam)                                                               \
    O(OC4cGeomParam, OC4cGeomParam)                                                               \
    O(ON2fGeomParam, ON2fGeomParam)                                                               \
    O(ON2dGeomParam, ON2dGeomParam)                                                               \
    O(ON3fGeomParam, ON3fGeomParam)                                                               \
    O(ON3dGeomParam, ON3dGeomParam)

/* La table d'attributs exportés est utilisée pour se souvenir des attributs exportés d'image à
 * image lorsque nous exportons une animation. */
struct TableAttributsExportés {
    AbcGeom::OCompoundProperty prop{};

#define DEFINIS_MAP_PARAM(type_param, nom_map)                                                    \
    std::map<std::string, AbcGeom::type_param> nom_map##_map{};

    ENUMERE_TYPE_PARAMS(DEFINIS_MAP_PARAM)

#undef DEFINIS_MAP_PARAM

#define DEFINIS_FONCTION_INITIALISATION_PARAM(type_param, nom_map)                                \
    void init_param(                                                                              \
        AbcGeom::type_param &param, const std::string &name, AbcGeom::GeometryScope scope)        \
    {                                                                                             \
        param = nom_map##_map[name];                                                              \
        if (!param.valid()) {                                                                     \
            param = AbcGeom::type_param(prop, name, false, scope, 1);                             \
            nom_map##_map[name] = param;                                                          \
        }                                                                                         \
    }

    ENUMERE_TYPE_PARAMS(DEFINIS_FONCTION_INITIALISATION_PARAM)

#undef DEFINIS_FONCTION_INITIALISATION_PARAM
};

static void écris_données(AbcGeom::OXform &o_xform,
                          TableAttributsExportés *&table_attributs,
                          ConvertisseuseExportXform *convertisseuse)
{
    auto &schema = o_xform.getSchema();

    double données[4][4];
    double *matrice = &données[0][0];
    convertisseuse->donne_matrice(convertisseuse, matrice);

    Abc::M44d matrice_export = Abc::M44d(données);

    AbcGeom::XformSample sample;
    sample.setMatrix(matrice_export);
    sample.setInheritsXforms(convertisseuse->doit_hériter_matrice_parent(convertisseuse));
    schema.set(sample);
}

struct InformationsDomaines {
    eTypeObjetAbc type_objet{};
    bool supporte_domaine[NOMBRE_DE_DOMAINES] = {};
    int taille_domaine[NOMBRE_DE_DOMAINES] = {};
    std::optional<std::vector<int>> indexage_domaine[NOMBRE_DE_DOMAINES] = {};
};

struct DonnéesÉcritureAttribut {
    void *attribut = nullptr;
    std::string nom = "";
    eAbcDomaineAttribut domaine{};
};

static AbcGeom::GeometryScope donne_domaine_pour_alembic(const eAbcDomaineAttribut domaine,
                                                         const eTypeObjetAbc type_objet)
{
    switch (domaine) {
        case AUCUNE:
            return AbcGeom::kUnknownScope;
        case OBJET:
            return AbcGeom::kConstantScope;
        case POINT:
            return AbcGeom::kVertexScope;
        case PRIMITIVE:
            return AbcGeom::kUniformScope;
        case POINT_PRIMITIVE:
            if (type_objet == COURBES || type_objet == NURBS) {
                return AbcGeom::kVaryingScope;
            }
            return AbcGeom::kFacevaryingScope;
    }

    return AbcGeom::kUnknownScope;
}

#define IMPRIME_VALEUR_ERR(valeur) std::cerr << #valeur << " : " << valeur << '\n'

template <eTypeDoneesAttributAbc type>
static auto extrait_données_attribut(AbcExportriceAttribut *exportrice,
                                     InformationsDomaines const &informations_domaines,
                                     DonnéesÉcritureAttribut const &données_attribut)
{
    using ConvertisseuseValeur = ConvertisseuseTypeValeurExport<type>;
    using type_abc = typename ConvertisseuseValeur::type_abc;
    using type_param = typename ConvertisseuseValeur::type_param_abc;
    using sample_type = typename type_param::Sample;
    std::vector<type_abc> résultat;

    if (!ConvertisseuseValeur::export_valeur_est_supporté(exportrice)) {
        return résultat;
    }

    const int taille_domaine = informations_domaines.taille_domaine[données_attribut.domaine];

    auto const opt_indexage_domaine =
        informations_domaines.indexage_domaine[données_attribut.domaine];
    if (opt_indexage_domaine.has_value()) {
        auto const &indexage = opt_indexage_domaine.value();
        résultat.resize(indexage.size());

        for (size_t i = 0; i < indexage.size(); i++) {
            auto const index_source = indexage[i];
            résultat[i] = ConvertisseuseValeur::donne_valeur(
                exportrice, données_attribut.attribut, index_source);
        }
    }
    else {
        résultat.resize(static_cast<size_t>(taille_domaine));
        for (size_t i = 0; i < static_cast<size_t>(taille_domaine); i++) {
            résultat[i] = ConvertisseuseValeur::donne_valeur(
                exportrice, données_attribut.attribut, static_cast<int>(i));
        }
    }

    return résultat;
}

template <eTypeDoneesAttributAbc type>
static void écris_attribut(AbcExportriceAttribut *exportrice,
                           TableAttributsExportés *table_attributs,
                           InformationsDomaines const &informations_domaines,
                           DonnéesÉcritureAttribut const &données_attribut)
{
    if (!informations_domaines.supporte_domaine[données_attribut.domaine]) {
        return;
    }

    const int taille_domaine = informations_domaines.taille_domaine[données_attribut.domaine];
    if (taille_domaine == 0) {
        return;
    }

    using ConvertisseuseValeur = ConvertisseuseTypeValeurExport<type>;
    using type_abc = typename ConvertisseuseValeur::type_abc;
    using type_param = typename ConvertisseuseValeur::type_param_abc;
    using sample_type = typename type_param::Sample;

    std::vector<type_abc> résultat = extrait_données_attribut<type>(
        exportrice, informations_domaines, données_attribut);

    if (résultat.empty()) {
        return;
    }

    try {
        auto const scope = donne_domaine_pour_alembic(données_attribut.domaine,
                                                      informations_domaines.type_objet);

        type_param param;
        table_attributs->init_param(param, données_attribut.nom, scope);

        sample_type sample(résultat, scope);
        param.set(sample);
    }
    catch (const Util::Exception &e) {
        std::cerr << e.what() << '\n';
    }
}

static void écris_attribut(AbcExportriceAttribut *exportrice,
                           TableAttributsExportés *table_attributs,
                           InformationsDomaines const &informations_domaines,
                           DonnéesÉcritureAttribut const &données_attribut,
                           eTypeDoneesAttributAbc type_données)
{
#define GERE_CAS(type_de_données)                                                                 \
    case type_de_données:                                                                         \
        écris_attribut<type_de_données>(                                                          \
            exportrice, table_attributs, informations_domaines, données_attribut);                \
        return

    switch (type_données) {
        GERE_CAS(ATTRIBUT_TYPE_BOOL);
        GERE_CAS(ATTRIBUT_TYPE_N8);
        GERE_CAS(ATTRIBUT_TYPE_Z8);
        GERE_CAS(ATTRIBUT_TYPE_N16);
        GERE_CAS(ATTRIBUT_TYPE_Z16);
        GERE_CAS(ATTRIBUT_TYPE_N32);
        GERE_CAS(ATTRIBUT_TYPE_Z32);
        GERE_CAS(ATTRIBUT_TYPE_N64);
        GERE_CAS(ATTRIBUT_TYPE_Z64);
        GERE_CAS(ATTRIBUT_TYPE_R32);
        GERE_CAS(ATTRIBUT_TYPE_R64);
        GERE_CAS(ATTRIBUT_TYPE_CHAINE);
        GERE_CAS(ATTRIBUT_TYPE_VEC2_Z16);
        GERE_CAS(ATTRIBUT_TYPE_VEC2_Z32);
        GERE_CAS(ATTRIBUT_TYPE_VEC2_R32);
        GERE_CAS(ATTRIBUT_TYPE_VEC2_R64);
        GERE_CAS(ATTRIBUT_TYPE_VEC3_Z16);
        GERE_CAS(ATTRIBUT_TYPE_VEC3_Z32);
        GERE_CAS(ATTRIBUT_TYPE_VEC3_R32);
        GERE_CAS(ATTRIBUT_TYPE_VEC3_R64);
        GERE_CAS(ATTRIBUT_TYPE_POINT2_Z16);
        GERE_CAS(ATTRIBUT_TYPE_POINT2_Z32);
        GERE_CAS(ATTRIBUT_TYPE_POINT2_R32);
        GERE_CAS(ATTRIBUT_TYPE_POINT2_R64);
        GERE_CAS(ATTRIBUT_TYPE_POINT3_Z16);
        GERE_CAS(ATTRIBUT_TYPE_POINT3_Z32);
        GERE_CAS(ATTRIBUT_TYPE_POINT3_R32);
        GERE_CAS(ATTRIBUT_TYPE_POINT3_R64);
        GERE_CAS(ATTRIBUT_TYPE_BOITE2_Z16);
        GERE_CAS(ATTRIBUT_TYPE_BOITE2_Z32);
        GERE_CAS(ATTRIBUT_TYPE_BOITE2_R32);
        GERE_CAS(ATTRIBUT_TYPE_BOITE2_R64);
        GERE_CAS(ATTRIBUT_TYPE_BOITE3_Z16);
        GERE_CAS(ATTRIBUT_TYPE_BOITE3_Z32);
        GERE_CAS(ATTRIBUT_TYPE_BOITE3_R32);
        GERE_CAS(ATTRIBUT_TYPE_BOITE3_R64);
        GERE_CAS(ATTRIBUT_TYPE_MAT3X3_R32);
        GERE_CAS(ATTRIBUT_TYPE_MAT3X3_R64);
        GERE_CAS(ATTRIBUT_TYPE_MAT4X4_R32);
        GERE_CAS(ATTRIBUT_TYPE_MAT4X4_R64);
        GERE_CAS(ATTRIBUT_TYPE_QUAT_R32);
        GERE_CAS(ATTRIBUT_TYPE_QUAT_R64);
        GERE_CAS(ATTRIBUT_TYPE_COULEUR_RGB_R32);
        GERE_CAS(ATTRIBUT_TYPE_COULEUR_RGB_N8);
        GERE_CAS(ATTRIBUT_TYPE_COULEUR_RGBA_R32);
        GERE_CAS(ATTRIBUT_TYPE_COULEUR_RGBA_N8);
        GERE_CAS(ATTRIBUT_TYPE_NORMAL2_R32);
        GERE_CAS(ATTRIBUT_TYPE_NORMAL2_R64);
        GERE_CAS(ATTRIBUT_TYPE_NORMAL3_R32);
        GERE_CAS(ATTRIBUT_TYPE_NORMAL3_R64);
        case ATTRIBUT_TYPE_R16:
            return;
        case ATTRIBUT_TYPE_WSTRING:
            return;
        case ATTRIBUT_TYPE_COULEUR_RGB_R16:
            return;
        case ATTRIBUT_TYPE_COULEUR_RGBA_R16:
            return;
    }

#undef GERE_CAS
}

static bool est_attribut_standard(std::vector<DonnéesÉcritureAttribut> const &attributs_standards,
                                  DonnéesÉcritureAttribut const &attribut)
{
    for (auto const &attribut_standard : attributs_standards) {
        if (attribut_standard.attribut == attribut.attribut) {
            return true;
        }
    }

    return false;
}

static void écris_attributs(AbcExportriceAttribut *exportrice,
                            void *donnees_utilisateur,
                            TableAttributsExportés *table_attributs,
                            InformationsDomaines const &informations_domaines,
                            std::vector<DonnéesÉcritureAttribut> const &attributs_standards)
{
    const int nombre_d_attributs = exportrice->donne_nombre_attributs_a_exporter(
        donnees_utilisateur);

    if (nombre_d_attributs == 0) {
        return;
    }

    for (int i = 0; i < nombre_d_attributs; i++) {
        char *nom;
        int64_t taille_nom;
        eAbcDomaineAttribut domaine;
        eTypeDoneesAttributAbc type_données;
        void *attribut = exportrice->donne_attribut_pour_index(
            donnees_utilisateur, i, &nom, &taille_nom, &domaine, &type_données);

        if (!attribut) {
            continue;
        }

        DonnéesÉcritureAttribut données_attribut;
        données_attribut.nom = std::string(nom, static_cast<size_t>(taille_nom));
        données_attribut.domaine = domaine;
        données_attribut.attribut = attribut;

        /* Les attributs standards sont écrits séparément. */
        if (est_attribut_standard(attributs_standards, données_attribut)) {
            continue;
        }

        écris_attribut(
            exportrice, table_attributs, informations_domaines, données_attribut, type_données);
    }
}

template <typename TypeConvertisseuse>
static std::optional<DonnéesÉcritureAttribut> donne_attribut_standard(
    eTypeDoneesAttributAbc const type_attendu,
    TypeConvertisseuse *convertisseuse,
    void *(*fonction)(
        TypeConvertisseuse *, char **, int64_t *, eAbcDomaineAttribut *, eTypeDoneesAttributAbc *))
{
    if (!fonction) {
        return {};
    }

    char *nom;
    int64_t taille_nom;
    eAbcDomaineAttribut domaine;
    eTypeDoneesAttributAbc type_données;
    void *attribut = fonction(convertisseuse, &nom, &taille_nom, &domaine, &type_données);

    if (!attribut) {
        return {};
    }

    if (type_attendu != type_données) {
        return {};
    }

    DonnéesÉcritureAttribut données_attribut;
    données_attribut.nom = std::string(nom, static_cast<size_t>(taille_nom));
    données_attribut.domaine = domaine;
    données_attribut.attribut = attribut;

    return données_attribut;
}

struct AttributsStandardPolyMesh {
    DonnéesÉcritureAttribut données_uvs{};
    std::vector<Imath::V2f> uvs{};
    DonnéesÉcritureAttribut données_vélocité{};
    std::vector<Imath::V3f> vélocité{};
    DonnéesÉcritureAttribut données_normaux{};
    std::vector<Imath::V3f> normaux{};
};

template <typename TypeConvertisseuse, typename TypeAttributsStandard>
static void donne_attribut_standard_uv(AbcExportriceAttribut *exportrice,
                                       InformationsDomaines const &informations_domaines,
                                       TypeAttributsStandard &attributs_standard_poly_mesh,
                                       std::vector<DonnéesÉcritureAttribut> &attributs_standards,
                                       TypeConvertisseuse *convertisseuse)
{
    auto opt_uv = donne_attribut_standard(
        ATTRIBUT_TYPE_VEC2_R32, convertisseuse, convertisseuse->donne_attribut_standard_uv);

    if (!opt_uv.has_value()) {
        return;
    }

    attributs_standard_poly_mesh.uvs = extrait_données_attribut<ATTRIBUT_TYPE_VEC2_R32>(
        exportrice, informations_domaines, opt_uv.value());

    if (attributs_standard_poly_mesh.uvs.empty()) {
        return;
    }

    attributs_standard_poly_mesh.données_uvs = opt_uv.value();

    attributs_standards.push_back(opt_uv.value());
}

static void donne_attribut_standard_normaux(
    AbcExportriceAttribut *exportrice,
    InformationsDomaines const &informations_domaines,
    AttributsStandardPolyMesh &attributs_standard_poly_mesh,
    std::vector<DonnéesÉcritureAttribut> &attributs_standards,
    ConvertisseuseExportPolyMesh *convertisseuse)
{
    auto opt_normaux = donne_attribut_standard(ATTRIBUT_TYPE_NORMAL3_R32,
                                               convertisseuse,
                                               convertisseuse->donne_attribut_standard_normaux);

    if (!opt_normaux.has_value()) {
        return;
    }

    attributs_standard_poly_mesh.normaux = extrait_données_attribut<ATTRIBUT_TYPE_NORMAL3_R32>(
        exportrice, informations_domaines, opt_normaux.value());

    if (attributs_standard_poly_mesh.normaux.empty()) {
        return;
    }

    attributs_standard_poly_mesh.données_normaux = opt_normaux.value();

    attributs_standards.push_back(opt_normaux.value());
}

template <typename TypeConvertisseuse, typename TypeAttributsStandard>
static void donne_attribut_standard_vélocité(
    AbcExportriceAttribut *exportrice,
    InformationsDomaines const &informations_domaines,
    TypeAttributsStandard &attributs_standard_poly_mesh,
    std::vector<DonnéesÉcritureAttribut> &attributs_standards,
    TypeConvertisseuse *convertisseuse)
{
    auto opt_vélocité = donne_attribut_standard(
        ATTRIBUT_TYPE_VEC3_R32, convertisseuse, convertisseuse->donne_attribut_standard_velocite);

    if (!opt_vélocité.has_value()) {
        return;
    }

    if (opt_vélocité->domaine != POINT) {
        return;
    }

    attributs_standard_poly_mesh.vélocité = extrait_données_attribut<ATTRIBUT_TYPE_VEC3_R32>(
        exportrice, informations_domaines, opt_vélocité.value());

    if (attributs_standard_poly_mesh.vélocité.empty()) {
        return;
    }

    attributs_standard_poly_mesh.données_vélocité = opt_vélocité.value();

    attributs_standards.push_back(opt_vélocité.value());
}

template <typename TypeConvertisseuse, typename TypeAttributsStandard>
static void donne_attribut_standard_rayons(
    AbcExportriceAttribut *exportrice,
    InformationsDomaines const &informations_domaines,
    TypeAttributsStandard &attributs_standard_poly_mesh,
    std::vector<DonnéesÉcritureAttribut> &attributs_standards,
    TypeConvertisseuse *convertisseuse)
{
    auto opt_vélocité = donne_attribut_standard(
        ATTRIBUT_TYPE_R32, convertisseuse, convertisseuse->donne_attribut_standard_velocite);

    if (!opt_vélocité.has_value()) {
        return;
    }

    if (opt_vélocité->domaine != POINT) {
        return;
    }

    attributs_standard_poly_mesh.rayons = extrait_données_attribut<ATTRIBUT_TYPE_R32>(
        exportrice, informations_domaines, opt_vélocité.value());

    if (attributs_standard_poly_mesh.rayons.empty()) {
        return;
    }

    attributs_standard_poly_mesh.données_rayons = opt_vélocité.value();

    attributs_standards.push_back(opt_vélocité.value());
}

static std::optional<AttributsStandardPolyMesh> écris_attributs(
    AbcGeom::OPolyMesh &o_poly_mesh,
    TableAttributsExportés *&table_attributs,
    ConvertisseuseExportPolyMesh *convertisseuse,
    InformationsDomaines const &informations_domaines)
{
    if (!convertisseuse->initialise_exportrice_attribut) {
        return {};
    }

    AbcExportriceAttribut exportrice;
    convertisseuse->initialise_exportrice_attribut(convertisseuse, &exportrice);

    auto &schema = o_poly_mesh.getSchema();

    if (!table_attributs) {
        table_attributs = new TableAttributsExportés;
        table_attributs->prop = schema.getArbGeomParams();
    }

    AttributsStandardPolyMesh résultat{};
    std::vector<DonnéesÉcritureAttribut> attributs_standards;

    donne_attribut_standard_uv(
        &exportrice, informations_domaines, résultat, attributs_standards, convertisseuse);
    donne_attribut_standard_normaux(
        &exportrice, informations_domaines, résultat, attributs_standards, convertisseuse);
    donne_attribut_standard_vélocité(
        &exportrice, informations_domaines, résultat, attributs_standards, convertisseuse);

    écris_attributs(&exportrice,
                    convertisseuse->donnees,
                    table_attributs,
                    informations_domaines,
                    attributs_standards);

    return résultat;
}

template <typename TypeConvertisseuse>
static std::vector<Imath::V3f> donne_positions(TypeConvertisseuse *convertisseuse)
{
    if (!convertisseuse->nombre_de_points || !convertisseuse->point_pour_index) {
        return {};
    }

    const size_t nombre_de_points = convertisseuse->nombre_de_points(convertisseuse);
    if (nombre_de_points == 0) {
        return {};
    }

    std::vector<Imath::V3f> positions;
    positions.resize(nombre_de_points);

    for (size_t i = 0; i < nombre_de_points; i++) {
        Imath::V3f &pos = positions[i];
        convertisseuse->point_pour_index(convertisseuse, i, &pos.x, &pos.y, &pos.z);
    }

    return positions;
}

template <typename TypeConvertisseuse>
static std::vector<int> donne_compte_de_sommets_par_polygones(TypeConvertisseuse *convertisseuse,
                                                              size_t &r_nombre_de_coins)
{
    if (!convertisseuse->nombre_de_coins_polygone || !convertisseuse->nombre_de_polygones) {
        r_nombre_de_coins = 0;
        return {};
    }

    const size_t nombre_de_polygones = convertisseuse->nombre_de_polygones(convertisseuse);
    std::vector<int> face_counts;
    face_counts.resize(nombre_de_polygones);

    size_t nombre_de_coins = 0;
    for (size_t i = 0; i < nombre_de_polygones; i++) {
        face_counts[i] = convertisseuse->nombre_de_coins_polygone(convertisseuse, i);
        nombre_de_coins += static_cast<size_t>(face_counts[i]);
    }

    r_nombre_de_coins = nombre_de_coins;
    return face_counts;
}

template <typename TypeConvertisseuse>
static std::vector<int> donne_index_points_par_polygone(TypeConvertisseuse *convertisseuse,
                                                        std::vector<int> const &face_counts,
                                                        const size_t nombre_de_coins)
{
    if (!convertisseuse->coins_pour_polygone) {
        return {};
    }

    std::vector<int> face_indices;
    face_indices.resize(nombre_de_coins);

    size_t decalage = 0;
    for (size_t i = 0; i < face_counts.size(); i++) {
        const int face_count = face_counts[i];
        convertisseuse->coins_pour_polygone(convertisseuse, i, &face_indices[decalage]);
        decalage += static_cast<size_t>(face_count);
    }

    return face_indices;
}

/* Construit l'indexage de correction pour que les données des coins sont dans la bon ordre
 * pour Alembic.
 */
static std::vector<int> construit_indexage_coins_polygones(std::vector<int> const &face_counts,
                                                           const size_t nombre_de_coins,
                                                           eAbcIndexagePolygone const indexage)
{
    std::vector<int> indexage_coins_polygones;
    indexage_coins_polygones.resize(nombre_de_coins);

    if (indexage == ABC_INDEXAGE_POLYGONE_HORAIRE) {
        auto index_alembic = 0;
        for (size_t i = 0; i < face_counts.size(); i++) {
            auto nombre_de_coins_polygone = face_counts[i];

            auto index_source = index_alembic;
            for (size_t j = 0; j < nombre_de_coins_polygone; j++) {
                indexage_coins_polygones[static_cast<size_t>(index_alembic++)] = index_source++;
            }
        }
    }
    else {
        auto index_alembic = 0;
        for (size_t i = 0; i < face_counts.size(); i++) {
            auto nombre_de_coins_polygone = face_counts[i];

            auto index_source = index_alembic + nombre_de_coins_polygone - 1;
            for (size_t j = 0; j < nombre_de_coins_polygone; j++) {
                indexage_coins_polygones[static_cast<size_t>(index_alembic++)] = index_source--;
            }
        }
    }

    return indexage_coins_polygones;
}

/* Retourne les limites géométriques depuis la fonction de rappel si renseignée dans la
 * convertisseuse, ou calcule-les depuis les positions. */
template <typename TypeConvertisseuse>
static Imath::Box3d donne_limites_géométrique(TypeConvertisseuse *convertisseuse,
                                              std::vector<Imath::V3f> const &positions)
{
    float min[3] = {std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max(),
                    std::numeric_limits<float>::max()};

    float max[3] = {-std::numeric_limits<float>::max(),
                    -std::numeric_limits<float>::max(),
                    -std::numeric_limits<float>::max()};

    if (convertisseuse->donne_limites_geometriques) {
        convertisseuse->donne_limites_geometriques(convertisseuse, min, max);
        return Imath::Box3d(Imath::V3d(double(min[0]), double(min[1]), double(min[2])),
                            Imath::V3d(double(max[0]), double(max[1]), double(max[2])));
    }

    for (auto const &point : positions) {
        min[0] = std::min(min[0], point.x);
        min[1] = std::min(min[1], point.y);
        min[2] = std::min(min[2], point.z);
        max[0] = std::max(max[0], point.x);
        max[1] = std::max(max[1], point.y);
        max[2] = std::max(max[2], point.z);
    }

    return Imath::Box3d(Imath::V3d(double(min[0]), double(min[1]), double(min[2])),
                        Imath::V3d(double(max[0]), double(max[1]), double(max[2])));
}

template <typename TypeConvertisseuse>
static eAbcIndexagePolygone donne_indexage_polygone(TypeConvertisseuse *convertisseuse)
{
    if (!convertisseuse->donne_indexage_polygone) {
        return ABC_INDEXAGE_POLYGONE_ANTIHORAIRE;
    }

    return convertisseuse->donne_indexage_polygone(convertisseuse);
}

static void écris_données(AbcGeom::OPolyMesh &o_poly_mesh,
                          TableAttributsExportés *&table_attributs,
                          ConvertisseuseExportPolyMesh *convertisseuse)
{
    std::vector<Imath::V3f> positions = donne_positions(convertisseuse);
    if (positions.empty()) {
        return;
    }

    size_t nombre_de_coins = 0;
    const std::vector<int> face_counts = donne_compte_de_sommets_par_polygones(convertisseuse,
                                                                               nombre_de_coins);

    const std::vector<int> indexage_coins_polygones = construit_indexage_coins_polygones(
        face_counts, nombre_de_coins, donne_indexage_polygone(convertisseuse));

    const std::vector<int> face_indices = donne_index_points_par_polygone(
        convertisseuse, face_counts, nombre_de_coins);

    InformationsDomaines informations_domaines;
    informations_domaines.type_objet = eTypeObjetAbc::POLY_MESH;
    informations_domaines.supporte_domaine[OBJET] = true;
    informations_domaines.supporte_domaine[POINT] = true;
    informations_domaines.supporte_domaine[PRIMITIVE] = true;
    informations_domaines.supporte_domaine[POINT_PRIMITIVE] = true;
    informations_domaines.indexage_domaine[POINT_PRIMITIVE] = indexage_coins_polygones;
    informations_domaines.taille_domaine[OBJET] = 1;
    informations_domaines.taille_domaine[POINT] = int(positions.size());
    informations_domaines.taille_domaine[PRIMITIVE] = int(face_counts.size());
    informations_domaines.taille_domaine[POINT_PRIMITIVE] = int(nombre_de_coins);

    auto opt_attr_std = écris_attributs(
        o_poly_mesh, table_attributs, convertisseuse, informations_domaines);

    /* Exporte vers Alembic */
    auto &schema = o_poly_mesh.getSchema();

    AbcGeom::OPolyMeshSchema::Sample sample;
    sample.setPositions(positions);
    sample.setFaceCounts(face_counts);
    sample.setFaceIndices(face_indices);

    if (opt_attr_std.has_value()) {
        if (!opt_attr_std->vélocité.empty()) {
            sample.setVelocities(opt_attr_std->vélocité);
        }

        if (!opt_attr_std->uvs.empty()) {
            AbcGeom::OV2fGeomParam::Sample uvs_sample;
            uvs_sample.setVals(opt_attr_std->uvs);
            uvs_sample.setScope(
                donne_domaine_pour_alembic(opt_attr_std->données_uvs.domaine, POLY_MESH));

            sample.setUVs(uvs_sample);
        }

        if (!opt_attr_std->normaux.empty()) {
            AbcGeom::ON3fGeomParam::Sample normals_sample;
            normals_sample.setVals(opt_attr_std->normaux);
            normals_sample.setScope(
                donne_domaine_pour_alembic(opt_attr_std->données_normaux.domaine, POLY_MESH));

            sample.setNormals(normals_sample);
        }
    }

    sample.setSelfBounds(donne_limites_géométrique(convertisseuse, positions));

    schema.set(sample);
}

struct AttributsStandardSubD {
    DonnéesÉcritureAttribut données_uvs{};
    std::vector<Imath::V2f> uvs{};
    DonnéesÉcritureAttribut données_vélocité{};
    std::vector<Imath::V3f> vélocité{};
};

static std::optional<AttributsStandardSubD> écris_attributs(
    AbcGeom::OSubD &o_subd,
    TableAttributsExportés *&table_attributs,
    ConvertisseuseExportSubD *convertisseuse,
    InformationsDomaines const &informations_domaines)
{
    if (!convertisseuse->initialise_exportrice_attribut) {
        return {};
    }

    AbcExportriceAttribut exportrice;
    convertisseuse->initialise_exportrice_attribut(convertisseuse, &exportrice);

    auto &schema = o_subd.getSchema();

    if (!table_attributs) {
        table_attributs = new TableAttributsExportés;
        table_attributs->prop = schema.getArbGeomParams();
    }

    AttributsStandardSubD résultat{};
    std::vector<DonnéesÉcritureAttribut> attributs_standards;

    /* À FAIRE(subd) : creases, etc. */
    donne_attribut_standard_uv(
        &exportrice, informations_domaines, résultat, attributs_standards, convertisseuse);
    donne_attribut_standard_vélocité(
        &exportrice, informations_domaines, résultat, attributs_standards, convertisseuse);

    écris_attributs(&exportrice,
                    convertisseuse->donnees,
                    table_attributs,
                    informations_domaines,
                    attributs_standards);

    return résultat;
}

static void écris_données(AbcGeom::OSubD &o_subd,
                          TableAttributsExportés *&table_attributs,
                          ConvertisseuseExportSubD *convertisseuse)
{
    std::vector<Imath::V3f> positions = donne_positions(convertisseuse);
    if (positions.empty()) {
        return;
    }

    size_t nombre_de_coins = 0;
    const std::vector<int> face_counts = donne_compte_de_sommets_par_polygones(convertisseuse,
                                                                               nombre_de_coins);

    const std::vector<int> indexage_coins_polygones = construit_indexage_coins_polygones(
        face_counts, nombre_de_coins, donne_indexage_polygone(convertisseuse));

    const std::vector<int> face_indices = donne_index_points_par_polygone(
        convertisseuse, face_counts, nombre_de_coins);

    InformationsDomaines informations_domaines;
    informations_domaines.type_objet = eTypeObjetAbc::POLY_MESH;
    informations_domaines.supporte_domaine[OBJET] = true;
    informations_domaines.supporte_domaine[POINT] = true;
    informations_domaines.supporte_domaine[PRIMITIVE] = true;
    informations_domaines.supporte_domaine[POINT_PRIMITIVE] = true;
    informations_domaines.indexage_domaine[POINT_PRIMITIVE] = indexage_coins_polygones;
    informations_domaines.taille_domaine[OBJET] = 1;
    informations_domaines.taille_domaine[POINT] = int(positions.size());
    informations_domaines.taille_domaine[PRIMITIVE] = int(face_counts.size());
    informations_domaines.taille_domaine[POINT_PRIMITIVE] = int(nombre_de_coins);

    auto opt_attr_std = écris_attributs(
        o_subd, table_attributs, convertisseuse, informations_domaines);

    /* Exporte vers Alembic */
    auto &schema = o_subd.getSchema();

    AbcGeom::OSubDSchema::Sample sample;
    sample.setPositions(positions);
    sample.setFaceCounts(face_counts);
    sample.setFaceIndices(face_indices);

    if (opt_attr_std.has_value()) {
        if (!opt_attr_std->vélocité.empty()) {
            sample.setVelocities(opt_attr_std->vélocité);
        }

        if (!opt_attr_std->uvs.empty()) {
            AbcGeom::OV2fGeomParam::Sample uvs_sample;
            uvs_sample.setVals(opt_attr_std->uvs);
            uvs_sample.setScope(
                donne_domaine_pour_alembic(opt_attr_std->données_uvs.domaine, POLY_MESH));

            sample.setUVs(uvs_sample);
        }
    }

    sample.setSelfBounds(donne_limites_géométrique(convertisseuse, positions));

    schema.set(sample);
}

#define RETOURNE_SI_NUL(valeur)                                                                   \
    do {                                                                                          \
        if (valeur == nullptr) {                                                                  \
            return;                                                                               \
        }                                                                                         \
    } while (0)

struct AbcExportriceGrapheMateriauImpl : public AbcExportriceGrapheMateriau {
  private:
    AbcMaterial::OMaterialSchema &m_schema;
    std::string m_cible = "";
    std::string m_type_nuanceur = "";
    std::vector<std::string> m_noms_noeuds{};

    static void ajoute_noeud_impl(AbcExportriceGrapheMateriau *exportrice,
                                  AbcChaine *nom,
                                  AbcChaine *type)
    {
        RETOURNE_SI_NUL(nom);
        RETOURNE_SI_NUL(type);

        auto impl = static_cast<AbcExportriceGrapheMateriauImpl *>(exportrice);
        impl->m_schema.addNetworkNode(
            nom->vers_std_string(), impl->m_cible, type->vers_std_string());
        impl->m_noms_noeuds.push_back(nom->vers_std_string());
    }

    static void ajoute_connexion_impl(AbcExportriceGrapheMateriau *exportrice,
                                      AbcChaine *nom_noeud_entrée,
                                      AbcChaine *nom_entrée,
                                      AbcChaine *nom_noeud_sortie,
                                      AbcChaine *nom_sortie)
    {
        RETOURNE_SI_NUL(nom_noeud_entrée);
        RETOURNE_SI_NUL(nom_entrée);
        RETOURNE_SI_NUL(nom_noeud_sortie);
        RETOURNE_SI_NUL(nom_sortie);

        auto impl = static_cast<AbcExportriceGrapheMateriauImpl *>(exportrice);
        impl->m_schema.setNetworkNodeConnection(nom_noeud_entrée->vers_std_string(),
                                                nom_entrée->vers_std_string(),
                                                nom_noeud_sortie->vers_std_string(),
                                                nom_sortie->vers_std_string());
    }

    static void definis_noeud_sortie_graphe_impl(AbcExportriceGrapheMateriau *exportrice,
                                                 AbcChaine *nom_sortie)
    {
        RETOURNE_SI_NUL(nom_sortie);
        auto impl = static_cast<AbcExportriceGrapheMateriauImpl *>(exportrice);
        impl->m_schema.setNetworkTerminal(
            impl->m_cible, impl->m_type_nuanceur, nom_sortie->vers_std_string());
    }

  public:
    AbcExportriceGrapheMateriauImpl(AbcMaterial::OMaterialSchema &schema,
                                    std::string const &cible,
                                    std::string const &type_nuanceur)
        : m_schema(schema), m_cible(cible), m_type_nuanceur(type_nuanceur)
    {
        ajoute_noeud = ajoute_noeud_impl;
        ajoute_connexion = ajoute_connexion_impl;
    }

    const std::vector<std::string> &donne_noms_noeuds_créés() const
    {
        return m_noms_noeuds;
    }
};

static void écris_données(AbcMaterial::OMaterial &omateriau,
                          TableAttributsExportés *& /*table_attributs*/,
                          ConvertisseuseExportMateriau *convertisseuse)
{
    auto &schema = omateriau.getSchema();

    const auto cible = string_depuis_rappel(convertisseuse, convertisseuse->nom_cible);
    const auto type_nuanceur = string_depuis_rappel(convertisseuse, convertisseuse->type_nuanceur);
    const auto nom_nuanceur = string_depuis_rappel(convertisseuse, convertisseuse->nom_nuanceur);

    schema.setShader(cible, type_nuanceur, nom_nuanceur);

    // À FAIRE: paramètre du nuanceur et des noeuds.

    AbcExportriceGrapheMateriauImpl exportrice_graphe(schema, cible, type_nuanceur);
    convertisseuse->remplis_graphe(convertisseuse, &exportrice_graphe);

    auto const &noms_noeuds = exportrice_graphe.donne_noms_noeuds_créés();
    if (noms_noeuds.empty()) {
        return;
    }

    //    for (const auto &nom_noeud : noms_noeuds) {
    //        auto param = schema.getNetworkNodeParameters(nom_noeud);
    //    }
}

struct AbcExportriceEchantillonCameraImpl : public AbcExportriceEchantillonCamera {
  private:
    AbcGeom::CameraSample &m_échantillon;

    static void definis_longueur_focale_impl(AbcExportriceEchantillonCamera *exportrice,
                                             AbcMillimetre *longueur)
    {
        auto impl = static_cast<AbcExportriceEchantillonCameraImpl *>(exportrice);
        if (longueur) {
            impl->m_échantillon.setFocalLength(longueur->valeur);
        }
    }

    static void definis_ouverture_impl(AbcExportriceEchantillonCamera *exportrice,
                                       AbcCentimetre *horizontale,
                                       AbcCentimetre *verticale)
    {
        auto impl = static_cast<AbcExportriceEchantillonCameraImpl *>(exportrice);
        if (horizontale) {
            impl->m_échantillon.setHorizontalAperture(horizontale->valeur);
        }
        if (verticale) {
            impl->m_échantillon.setVerticalAperture(verticale->valeur);
        }
    }

    static void definis_decalage_senseur_impl(AbcExportriceEchantillonCamera *exportrice,
                                              AbcCentimetre *horizontal,
                                              AbcCentimetre *vertical)
    {
        auto impl = static_cast<AbcExportriceEchantillonCameraImpl *>(exportrice);
        if (horizontal) {
            impl->m_échantillon.setHorizontalFilmOffset(horizontal->valeur);
        }
        if (vertical) {
            impl->m_échantillon.setVerticalFilmOffset(vertical->valeur);
        }
    }

    static void definis_aspect_horizontal_sur_vertical_impl(
        AbcExportriceEchantillonCamera *exportrice, double aspect)
    {
        auto impl = static_cast<AbcExportriceEchantillonCameraImpl *>(exportrice);
        impl->m_échantillon.setLensSqueezeRatio(aspect);
    }

    /* Définis l'extension (overscan), en pourcentage relatif, de l'image. Les valeurs sont à
     * donner dans l'ordre : gauche, droite, haut, bas. */
    static void definis_extension_image_impl(AbcExportriceEchantillonCamera *exportrice,
                                             AbcPourcentage *gauche,
                                             AbcPourcentage *droite,
                                             AbcPourcentage *haut,
                                             AbcPourcentage *bas)
    {
        auto impl = static_cast<AbcExportriceEchantillonCameraImpl *>(exportrice);
        if (gauche) {
            impl->m_échantillon.setOverScanLeft(gauche->valeur);
        }
        if (droite) {
            impl->m_échantillon.setOverScanRight(droite->valeur);
        }
        if (haut) {
            impl->m_échantillon.setOverScanTop(haut->valeur);
        }
        if (bas) {
            impl->m_échantillon.setOverScanBottom(bas->valeur);
        }
    }

    /* Définis l'aspect (largeur / hauteur). */
    static void definis_fstop_impl(AbcExportriceEchantillonCamera *exportrice, double fstop)
    {
        auto impl = static_cast<AbcExportriceEchantillonCameraImpl *>(exportrice);
        impl->m_échantillon.setFStop(fstop);
    }

    /* Définis la distance de la cible de la caméra, ce qui est focalisé. */
    static void definis_distance_de_la_cible_impl(AbcExportriceEchantillonCamera *exportrice,
                                                  AbcCentimetre *distance)
    {
        auto impl = static_cast<AbcExportriceEchantillonCameraImpl *>(exportrice);
        if (distance) {
            impl->m_échantillon.setFocusDistance(distance->valeur);
        }
    }

    /* Définis le temps, relatif à l'image, de l'ouverture et de la fermeture de l'obturateur. */
    static void definis_temps_obturation_impl(AbcExportriceEchantillonCamera *exportrice,
                                              AbcTempsSeconde *ouverture,
                                              AbcTempsSeconde *femeture)
    {
        auto impl = static_cast<AbcExportriceEchantillonCameraImpl *>(exportrice);
        if (ouverture) {
            impl->m_échantillon.setShutterOpen(ouverture->valeur);
        }
        if (femeture) {
            impl->m_échantillon.setShutterClose(femeture->valeur);
        }
    }

    /* Définis la distance de visibilté du premier et de l'arrière plan respectivement. */
    static void definis_avant_arriere_plan_impl(AbcExportriceEchantillonCamera *exportrice,
                                                AbcCentimetre *premier_plan,
                                                AbcCentimetre *arriere_plan)
    {
        auto impl = static_cast<AbcExportriceEchantillonCameraImpl *>(exportrice);
        if (premier_plan) {
            impl->m_échantillon.setNearClippingPlane(premier_plan->valeur);
        }
        if (arriere_plan) {
            impl->m_échantillon.setFarClippingPlane(arriere_plan->valeur);
        }
    }

  public:
    AbcExportriceEchantillonCameraImpl(AbcGeom::CameraSample &échantillon)
        : m_échantillon(échantillon)
    {
        definis_longueur_focale = definis_longueur_focale_impl;
        definis_ouverture = definis_ouverture_impl;
        definis_decalage_senseur = definis_decalage_senseur_impl;
        definis_aspect_horizontal_sur_vertical = definis_aspect_horizontal_sur_vertical_impl;
        definis_extension_image = definis_extension_image_impl;
        definis_fstop = definis_fstop_impl;
        definis_distance_de_la_cible = definis_distance_de_la_cible_impl;
        definis_temps_obturation = definis_temps_obturation_impl;
        definis_avant_arriere_plan = definis_avant_arriere_plan_impl;
    }
};

struct AbcExportriceOperationSenseurImpl : public AbcExportriceOperationSenseur {
  private:
    AbcGeom::CameraSample &m_échantillon;

    static void ajoute_translation_impl(AbcExportriceOperationSenseur *exportrice,
                                        double *translation,
                                        char *indice,
                                        int64_t taille_indice)
    {
        auto impl = static_cast<AbcExportriceOperationSenseurImpl *>(exportrice);

        std::string hint;
        if (indice) {
            hint = std::string(indice, size_t(taille_indice));
        }

        auto op = AbcGeom::FilmBackXformOp(AbcGeom::kTranslateFilmBackOperation, hint);
        op.setTranslate(Abc::V2d(translation[0], translation[1]));

        impl->m_échantillon.addOp(op);
    }

    static void ajoute_taille_impl(AbcExportriceOperationSenseur *exportrice,
                                   double *taille,
                                   char *indice,
                                   int64_t taille_indice)
    {
        auto impl = static_cast<AbcExportriceOperationSenseurImpl *>(exportrice);

        std::string hint;
        if (indice) {
            hint = std::string(indice, size_t(taille_indice));
        }

        auto op = AbcGeom::FilmBackXformOp(AbcGeom::kScaleFilmBackOperation, hint);
        op.setScale(Abc::V2d(taille[0], taille[1]));

        impl->m_échantillon.addOp(op);
    }

    static void ajoute_matrice_impl(AbcExportriceOperationSenseur *exportrice,
                                    double *m,
                                    char *indice,
                                    int64_t taille_indice)
    {
        auto impl = static_cast<AbcExportriceOperationSenseurImpl *>(exportrice);

        std::string hint;
        if (indice) {
            hint = std::string(indice, size_t(taille_indice));
        }

        auto op = AbcGeom::FilmBackXformOp(AbcGeom::kMatrixFilmBackOperation, hint);
        op.setMatrix(Abc::M33d(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8]));

        impl->m_échantillon.addOp(op);
    }

  public:
    AbcExportriceOperationSenseurImpl(AbcGeom::CameraSample &échantillon)
        : m_échantillon(échantillon)
    {
        ajoute_matrice = ajoute_matrice_impl;
        ajoute_taille = ajoute_taille_impl;
        ajoute_translation = ajoute_translation_impl;
    }
};

template <typename TypeConvertisseuse>
static AbcGeom::CameraSample donne_échantillon_caméra(TypeConvertisseuse *convertisseuse)
{
    double fenêtre_haut = 1.0;
    double fenêtre_bas = -1.0;
    double fenêtre_gauche = -1.0;
    double fenêtre_droite = 1.0;
    if (convertisseuse->donne_taille_fenetre) {
        convertisseuse->donne_taille_fenetre(
            convertisseuse, &fenêtre_haut, &fenêtre_bas, &fenêtre_gauche, &fenêtre_droite);
    }

    auto échantillon = AbcGeom::CameraSample(
        fenêtre_haut, fenêtre_bas, fenêtre_gauche, fenêtre_droite);

    /* Données principales. */
    AbcExportriceEchantillonCameraImpl exportrice_échantillon(échantillon);
    convertisseuse->remplis_donnees_echantillon(convertisseuse, &exportrice_échantillon);

    /* Limites de la hiérarchie fille. */
    if (convertisseuse->donne_limites_geometriques_enfant) {
        float min[3] = {std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max(),
                        std::numeric_limits<float>::max()};

        float max[3] = {-std::numeric_limits<float>::max(),
                        -std::numeric_limits<float>::max(),
                        -std::numeric_limits<float>::max()};

        convertisseuse->donne_limites_geometriques_enfant(convertisseuse, min, max);
        auto limites = Imath::Box3d(Imath::V3d(double(min[0]), double(min[1]), double(min[2])),
                                    Imath::V3d(double(max[0]), double(max[1]), double(max[2])));
        échantillon.setChildBounds(limites);
    }

    /* Opérations de transformation du senseur. */
    if (convertisseuse->ajoute_operations_senseur) {
        auto exportrice = AbcExportriceOperationSenseurImpl(échantillon);
        convertisseuse->ajoute_operations_senseur(convertisseuse, &exportrice);
    }

    return échantillon;
}

static void écris_données(AbcGeom::OCamera &o_camera,
                          TableAttributsExportés *& /*table_attributs*/,
                          ConvertisseuseExportCamera *convertisseuse)
{
    if (!convertisseuse->remplis_donnees_echantillon) {
        return;
    }

    auto échantillon = donne_échantillon_caméra(convertisseuse);

    /* Écriture de l'échantillon. */
    auto &schema = o_camera.getSchema();
    schema.set(échantillon);
}

static void écris_données(AbcGeom::OCurves & /*o_curves*/,
                          TableAttributsExportés *& /*table_attributs*/,
                          ConvertisseuseExportCourbes * /*convertisseuse*/)
{
    // À FAIRE
}

static std::vector<int> donne_groupe_polygone(ConvertisseuseExportFaceSet *convertisseuse)
{
    if (!convertisseuse->nombre_de_polygones || !convertisseuse->donne_index_polygone) {
        return {};
    }

    auto taille = convertisseuse->nombre_de_polygones(convertisseuse);
    std::vector<int> résultat(taille);

    if (convertisseuse->remplis_index_polygones) {
        convertisseuse->remplis_index_polygones(convertisseuse, résultat.data());
    }
    else {
        for (auto i = 0ul; i < taille; ++i) {
            résultat[i] = convertisseuse->donne_index_polygone(convertisseuse, int(i));
        }
    }

    return résultat;
}

static AbcGeom::FaceSetExclusivity donne_exclusivité_groupe(
    ConvertisseuseExportFaceSet *convertisseuse)
{
    if (!convertisseuse->donne_exclusivite_polygones) {
        /* Valeur défaut Alembic. */
        return AbcGeom::FaceSetExclusivity::kFaceSetNonExclusive;
    }

    auto exclusité = convertisseuse->donne_exclusivite_polygones(convertisseuse);
    switch (exclusité) {
        case ABC_EXCLUSIVITE_POLYGONE_EXCLUSIVE:
        {
            return AbcGeom::FaceSetExclusivity::kFaceSetExclusive;
        }
        case ABC_EXCLUSIVITE_POLYGONE_NON_EXCLUSIVE:
        {
            return AbcGeom::FaceSetExclusivity::kFaceSetNonExclusive;
        }
    }
    /* Valeur défaut Alembic. */
    return AbcGeom::FaceSetExclusivity::kFaceSetNonExclusive;
}

static void écris_données(AbcGeom::OFaceSet &o_faceset,
                          TableAttributsExportés *& /*table_attributs*/,
                          ConvertisseuseExportFaceSet *convertisseuse)
{
    const std::vector<int> face_indices = donne_groupe_polygone(convertisseuse);
    if (face_indices.empty()) {
        return;
    }

    AbcGeom::OFaceSetSchema::Sample sample;
    sample.setFaces(face_indices);

    auto &schema = o_faceset.getSchema();
    schema.setFaceExclusivity(donne_exclusivité_groupe(convertisseuse));
    schema.set(sample);
}

static void écris_données(AbcGeom::OLight &o_light,
                          TableAttributsExportés *& /*table_attributs*/,
                          ConvertisseuseExportLumiere *convertisseuse)
{
    if (!convertisseuse->remplis_donnees_echantillon) {
        return;
    }

    auto échantillon = donne_échantillon_caméra(convertisseuse);

    /* Écriture de l'échantillon. */
    auto &schema = o_light.getSchema();
    schema.setCameraSample(échantillon);
}

static void écris_données(AbcGeom::ONuPatch & /*o_nupatch*/,
                          TableAttributsExportés *& /*table_attributs*/,
                          ConvertisseuseExportNurbs * /*convertisseuse*/)
{
    // À FAIRE
}

struct AttributsStandarsPoints {
    DonnéesÉcritureAttribut données_vélocité{};
    std::vector<Imath::V3f> vélocité{};

    DonnéesÉcritureAttribut données_rayons{};
    std::vector<float> rayons{};

    DonnéesÉcritureAttribut données_ids{};
    std::vector<uint64_t> ids{};
};

static void donne_attribut_standard_ids(AbcExportriceAttribut *exportrice,
                                        InformationsDomaines const &informations_domaines,
                                        AttributsStandarsPoints &attributs_standard_poly_mesh,
                                        std::vector<DonnéesÉcritureAttribut> &attributs_standards,
                                        ConvertisseuseExportPoints *convertisseuse)
{
    auto opt_attribut = donne_attribut_standard(
        ATTRIBUT_TYPE_N64, convertisseuse, convertisseuse->donne_attribut_standard_velocite);

    if (!opt_attribut.has_value()) {
        return;
    }

    if (opt_attribut->domaine != POINT) {
        return;
    }

    attributs_standard_poly_mesh.ids = extrait_données_attribut<ATTRIBUT_TYPE_N64>(
        exportrice, informations_domaines, opt_attribut.value());

    if (attributs_standard_poly_mesh.ids.empty()) {
        return;
    }

    attributs_standard_poly_mesh.données_ids = opt_attribut.value();

    attributs_standards.push_back(opt_attribut.value());
}

static std::optional<AttributsStandarsPoints> écris_attributs(
    AbcGeom::OPoints &o_points,
    TableAttributsExportés *&table_attributs,
    ConvertisseuseExportPoints *convertisseuse,
    InformationsDomaines const &informations_domaines)
{
    if (!convertisseuse->initialise_exportrice_attribut) {
        return {};
    }

    AbcExportriceAttribut exportrice;
    convertisseuse->initialise_exportrice_attribut(convertisseuse, &exportrice);

    auto &schema = o_points.getSchema();

    if (!table_attributs) {
        table_attributs = new TableAttributsExportés;
        table_attributs->prop = schema.getArbGeomParams();
    }

    AttributsStandarsPoints résultat{};
    std::vector<DonnéesÉcritureAttribut> attributs_standards;

    donne_attribut_standard_rayons(
        &exportrice, informations_domaines, résultat, attributs_standards, convertisseuse);
    donne_attribut_standard_ids(
        &exportrice, informations_domaines, résultat, attributs_standards, convertisseuse);
    donne_attribut_standard_vélocité(
        &exportrice, informations_domaines, résultat, attributs_standards, convertisseuse);

    écris_attributs(&exportrice,
                    convertisseuse->donnees,
                    table_attributs,
                    informations_domaines,
                    attributs_standards);

    return résultat;
}

static void écris_données(AbcGeom::OPoints &o_points,
                          TableAttributsExportés *&table_attributs,
                          ConvertisseuseExportPoints *convertisseuse)
{
    std::vector<Imath::V3f> positions = donne_positions(convertisseuse);
    if (positions.empty()) {
        return;
    }

    InformationsDomaines informations_domaines{};
    informations_domaines.type_objet = eTypeObjetAbc::POINTS;
    informations_domaines.supporte_domaine[OBJET] = true;
    informations_domaines.supporte_domaine[POINT] = true;
    informations_domaines.supporte_domaine[PRIMITIVE] = false;
    informations_domaines.supporte_domaine[POINT_PRIMITIVE] = false;
    informations_domaines.taille_domaine[OBJET] = 1;
    informations_domaines.taille_domaine[POINT] = int(positions.size());
    informations_domaines.taille_domaine[PRIMITIVE] = 0;
    informations_domaines.taille_domaine[POINT_PRIMITIVE] = 0;

    auto opt_attr_std = écris_attributs(
        o_points, table_attributs, convertisseuse, informations_domaines);

    /* Exporte vers Alembic */
    auto &schema = o_points.getSchema();

    AbcGeom::OPointsSchema::Sample sample;
    sample.setPositions(positions);

    if (opt_attr_std.has_value()) {
        if (!opt_attr_std->vélocité.empty()) {
            sample.setVelocities(opt_attr_std->vélocité);
        }

        if (!opt_attr_std->ids.empty()) {
            sample.setIds(opt_attr_std->ids);
        }
        else {
            std::vector<uint64_t> ids(positions.size());
            std::iota(ids.begin(), ids.end(), uint64_t(0));
            sample.setIds(ids);
        }

        if (!opt_attr_std->rayons.empty()) {
            AbcGeom::OFloatGeomParam::Sample widths_sample;
            widths_sample.setVals(opt_attr_std->rayons);
            widths_sample.setScope(
                donne_domaine_pour_alembic(opt_attr_std->données_rayons.domaine, POINTS));

            sample.setWidths(widths_sample);
        }
    }

    sample.setSelfBounds(donne_limites_géométrique(convertisseuse, positions));

    schema.set(sample);
}

}  // namespace AbcKuri

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom ÉcrivainCache.
 * \{ */

RacineAutriceCache::RacineAutriceCache(AutriceArchive &autrice) : m_autrice(&autrice)
{
}

Abc::OObject RacineAutriceCache::oobject()
{
    return m_autrice->racine_ecriture();
}

Abc::OObject RacineAutriceCache::oobject() const
{
    return m_autrice->racine_ecriture();
}

template <typename TypeObjetAlembic>
struct DonnéesEcrivainCache;

#define DEFINIS_DONNEES_ECRIVAIN(TypeObjet, TypeConvertisseuse, fonction_init)                    \
    template <>                                                                                   \
    struct DonnéesEcrivainCache<TypeObjet> {                                                      \
        using type_objet_alembic = TypeObjet;                                                     \
        using type_convertisseuse = TypeConvertisseuse;                                           \
        static void init_convertisseuse(AutriceArchive *autrice,                                  \
                                        type_convertisseuse *convertisseuse)                      \
        {                                                                                         \
            autrice->ctx_écriture.fonction_init(convertisseuse);                                  \
        }                                                                                         \
    }

DEFINIS_DONNEES_ECRIVAIN(AbcGeom::OCamera,
                         ConvertisseuseExportCamera,
                         initialise_convertisseuse_camera);
DEFINIS_DONNEES_ECRIVAIN(AbcGeom::OCurves,
                         ConvertisseuseExportCourbes,
                         initialise_convertisseuse_courbes);
DEFINIS_DONNEES_ECRIVAIN(AbcGeom::OFaceSet,
                         ConvertisseuseExportFaceSet,
                         initialise_convertisseuse_face_set);
DEFINIS_DONNEES_ECRIVAIN(AbcGeom::OLight,
                         ConvertisseuseExportLumiere,
                         initialise_convertisseuse_lumiere);
DEFINIS_DONNEES_ECRIVAIN(AbcGeom::ONuPatch,
                         ConvertisseuseExportNurbs,
                         initialise_convertisseuse_nurbs);
DEFINIS_DONNEES_ECRIVAIN(AbcGeom::OPoints,
                         ConvertisseuseExportPoints,
                         initialise_convertisseuse_points);
DEFINIS_DONNEES_ECRIVAIN(AbcGeom::OPolyMesh,
                         ConvertisseuseExportPolyMesh,
                         initialise_convertisseuse_polymesh);
DEFINIS_DONNEES_ECRIVAIN(AbcGeom::OSubD, ConvertisseuseExportSubD, initialise_convertisseuse_subd);
DEFINIS_DONNEES_ECRIVAIN(AbcGeom::OXform,
                         ConvertisseuseExportXform,
                         initialise_convertisseuse_xform);
DEFINIS_DONNEES_ECRIVAIN(AbcMaterial::OMaterial,
                         ConvertisseuseExportMateriau,
                         initialise_convertisseuse_materiau);

#undef DEFINIS_DONNEES_ECRIVAIN

template <typename TypeObjetAlembic>
struct EcrivainType : public EcrivainCache {
    using données_écrivain = DonnéesEcrivainCache<TypeObjetAlembic>;
    using type_objet_alembic = typename données_écrivain::type_objet_alembic;
    using type_convertisseuse = typename données_écrivain::type_convertisseuse;

    type_objet_alembic o_schema_object{};
    type_convertisseuse convertisseuse{};
    AbcKuri::TableAttributsExportés *table_attributs = nullptr;

    EcrivainType(AutriceArchive *autrice,
                 EcrivainCache *parent,
                 void *données,
                 const std::string &nom)
    {
        o_schema_object = type_objet_alembic(parent->oobject(), nom);
        définis_parent(parent);
        données_écrivain::init_convertisseuse(autrice, &convertisseuse);
        convertisseuse.donnees = données;
    }

    EcrivainType(const EcrivainType &) = delete;
    EcrivainType &operator=(const EcrivainType &) = delete;

    ~EcrivainType()
    {
        delete table_attributs;
    }

    void écris_données() override
    {
        AbcKuri::écris_données(o_schema_object, table_attributs, &convertisseuse);
    }

    AbcGeom::OObject oobject() override
    {
        return o_schema_object;
    }

    AbcGeom::OObject oobject() const override
    {
        return o_schema_object;
    }
};

template <typename T>
EcrivainCache *EcrivainCache::cree(ContexteKuri *ctx,
                                   AutriceArchive *autrice,
                                   EcrivainCache *parent,
                                   void *données,
                                   const std::string &nom)
{
    auto ecrivain = kuri_loge<EcrivainType<T>>(ctx, autrice, parent, données, nom);
    return ecrivain;
}

#define DONNES_OOBJECT(TypeAlembic, ecrivain)                                                     \
    static_cast<EcrivainType<TypeAlembic> *>(ecrivain)->o_schema_object

/** \} */

namespace AbcKuri {

EcrivainCache *cree_ecrivain_cache_depuis_ref(ContexteKuri *ctx,
                                              AutriceArchive *autrice,
                                              LectriceCache *lectrice,
                                              EcrivainCache *parent,
                                              void *données)
{
    if (parent == nullptr) {
        parent = autrice->racine;
    }

#define GERE_CAS(TypeClasseEntree, TypeClasseSortie)                                              \
    if (TypeClasseEntree::matches(lectrice->iobject.getHeader())) {                               \
        return EcrivainCache::cree<TypeClasseSortie>(                                             \
            ctx, autrice, parent, données, lectrice->iobject.getName());                          \
    }

    GERE_CAS(AbcGeom::IPolyMesh, AbcGeom::OPolyMesh)
    GERE_CAS(AbcGeom::ISubD, AbcGeom::OSubD)
    GERE_CAS(AbcGeom::IPoints, AbcGeom::OPoints)
    GERE_CAS(AbcGeom::INuPatch, AbcGeom::ONuPatch)
    GERE_CAS(AbcGeom::ICurves, AbcGeom::OCurves)
    GERE_CAS(AbcGeom::ICamera, AbcGeom::OCamera)
    GERE_CAS(AbcGeom::IFaceSet, AbcGeom::OFaceSet)
    GERE_CAS(AbcGeom::ILight, AbcGeom::OLight)
    GERE_CAS(AbcMaterial::IMaterial, AbcMaterial::OMaterial)

    {
        /* Instance. */
    }

#undef GERE_CAS

    return nullptr;
}

EcrivainCache *cree_ecrivain_cache(ContexteKuri *ctx,
                                   AutriceArchive *autrice,
                                   EcrivainCache *parent,
                                   const char *nom,
                                   uint64_t taille_nom,
                                   void *données,
                                   eTypeObjetAbc type_objet)
{
    if (parent == nullptr) {
        parent = autrice->racine;
    }

#define GERE_CAS(TypeObjet, TypeClasse)                                                           \
    case eTypeObjetAbc::TypeObjet:                                                                \
    {                                                                                             \
        return EcrivainCache::cree<TypeClasse>(ctx, autrice, parent, données, {nom, taille_nom}); \
    }

    switch (type_objet) {
        GERE_CAS(CAMERA, AbcGeom::OCamera)
        GERE_CAS(COURBES, AbcGeom::OCurves)
        GERE_CAS(FACE_SET, AbcGeom::OFaceSet)
        GERE_CAS(LUMIERE, AbcGeom::OLight)
        GERE_CAS(MATERIAU, AbcMaterial::OMaterial)
        GERE_CAS(NURBS, AbcGeom::ONuPatch)
        GERE_CAS(POINTS, AbcGeom::OPoints)
        GERE_CAS(POLY_MESH, AbcGeom::OPolyMesh)
        GERE_CAS(SUBD, AbcGeom::OSubD)
        GERE_CAS(XFORM, AbcGeom::OXform)
    }

#undef GERE_CAS

    return nullptr;
}

class ÉcrivainInstance final : public EcrivainCache {
  public:
    static ÉcrivainInstance *crée(ContexteKuri *ctx_kuri,
                                  EcrivainCache *parent,
                                  EcrivainCache *origine,
                                  std::string const &nom)
    {
        auto oobject_parent = parent->oobject();
        auto oobject_origine = origine->oobject();

        if (!oobject_parent.addChildInstance(oobject_origine, nom)) {
            return nullptr;
        }

        auto résultat = kuri_loge<ÉcrivainInstance>(ctx_kuri);
        résultat->définis_parent(parent);
        return résultat;
    }

    void écris_données() override
    {
        /* Rien à faire, simplement créer les instances suffit. */
    }

    AbcGeom::OObject oobject() override
    {
        /* Il n'y a pas d'OObject pour les instances. */
        return {};
    }

    AbcGeom::OObject oobject() const override
    {
        /* Il n'y a pas d'OObject pour les instances. */
        return {};
    }
};

EcrivainCache *crée_instance(ContexteKuri *ctx,
                             AutriceArchive *autrice,
                             EcrivainCache *parent,
                             EcrivainCache *origine,
                             const char *nom,
                             size_t taille_nom)
{
    if (parent == nullptr) {
        parent = autrice->racine;
    }

    return ÉcrivainInstance::crée(ctx, parent, origine, {nom, taille_nom});
}

void detruit_ecrivain(ContexteKuri *ctx_kuri, EcrivainCache *ecrivain)
{
    kuri_deloge(ctx_kuri, ecrivain);
}

}  // namespace AbcKuri
