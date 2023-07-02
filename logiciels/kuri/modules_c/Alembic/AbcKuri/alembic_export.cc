/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "alembic_export.hh"

#include <variant>

#include "../../InterfaceCKuri/contexte_kuri.hh"

#include "alembic_archive.hh"
#include "alembic_import.hh"
#include "utilitaires.hh"

#include "../alembic.h"
#include "../alembic_types.h"

using namespace Alembic;

/* ------------------------------------------------------------------------- */
/** \nom Écriture des objets Alembic depuis leurs convertisseuse.
 * \{ */

namespace AbcKuri {

static void écris_données(AbcGeom::OXform &o_xform, ConvertisseuseExportXform *convertisseuse)
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

static void écris_données(AbcGeom::OPolyMesh &o_poly_mesh,
                          ConvertisseuseExportPolyMesh *convertisseuse)
{
    const size_t nombre_de_points = convertisseuse->nombre_de_points(convertisseuse);

    if (nombre_de_points == 0) {
        return;
    }

    /* Exporte les positions. */
    std::vector<Imath::V3f> positions;
    positions.resize(nombre_de_points);

    for (size_t i = 0; i < nombre_de_points; i++) {
        Imath::V3f &pos = positions[i];
        convertisseuse->point_pour_index(convertisseuse, i, &pos.x, &pos.y, &pos.z);
    }

    /* Exporte les polygones. */
    const size_t nombre_de_polygones = convertisseuse->nombre_de_polygones(convertisseuse);
    std::vector<int> face_counts;
    face_counts.resize(nombre_de_polygones);

    size_t nombre_de_coins = 0;
    for (size_t i = 0; i < nombre_de_polygones; i++) {
        face_counts[i] = convertisseuse->nombre_de_coins_polygone(convertisseuse, i);
        nombre_de_coins += static_cast<size_t>(face_counts[i]);
    }

    std::vector<int> face_indices;
    face_indices.resize(nombre_de_coins);

    size_t decalage = 0;
    for (size_t i = 0; i < nombre_de_polygones; i++) {
        const int face_count = face_counts[i];
        convertisseuse->coins_pour_polygone(convertisseuse, i, &face_indices[decalage]);
        decalage += static_cast<size_t>(face_count);
    }

    /* Exporte vers Alembic */
    auto &schema = o_poly_mesh.getSchema();

    AbcGeom::OPolyMeshSchema::Sample sample;
    sample.setPositions(positions);
    sample.setFaceCounts(face_counts);
    sample.setFaceIndices(face_indices);

    schema.set(sample);
}

static void écris_données(AbcGeom::OSubD & /*o_points*/,
                          ConvertisseuseExportSubD * /*convertisseuse*/)
{
    // À FAIRE
}

static void écris_données(AbcMaterial::OMaterial &omateriau,
                          ConvertisseuseExportMateriau *convertisseuse)
{
    auto &schema = omateriau.getSchema();

    const auto cible = string_depuis_rappel(convertisseuse, convertisseuse->nom_cible);
    const auto type_nuanceur = string_depuis_rappel(convertisseuse, convertisseuse->type_nuanceur);
    const auto nom_nuanceur = string_depuis_rappel(convertisseuse, convertisseuse->nom_nuanceur);

    schema.setShader(cible, type_nuanceur, nom_nuanceur);

    /* Crée les noeuds. */
    const auto nombre_de_noeuds = convertisseuse->nombre_de_noeuds(convertisseuse);

    for (size_t i = 0; i < nombre_de_noeuds; i++) {
        const auto nom_noeud = string_depuis_rappel(convertisseuse, i, convertisseuse->nom_noeud);
        const auto type_noeud = string_depuis_rappel(
            convertisseuse, i, convertisseuse->type_noeud);
        schema.addNetworkNode(nom_noeud, cible, type_noeud);
    }

    /* Crée les connexions entre les noeuds. */
    for (size_t i = 0; i < nombre_de_noeuds; i++) {
        const auto nom_noeud = string_depuis_rappel(convertisseuse, i, convertisseuse->nom_noeud);

        const auto nombre_entree = convertisseuse->nombre_entrees_noeud(convertisseuse, i);

        for (size_t e = 0; e < nombre_entree; e++) {
            const auto nom_entree = string_depuis_rappel(
                convertisseuse, i, e, convertisseuse->nom_entree_noeud);

            const auto nombre_de_connexion = convertisseuse->nombre_de_connexions(
                convertisseuse, i, e);

            for (size_t c = 0; c < nombre_de_connexion; c++) {
                const auto nom_noeud_connecte = string_depuis_rappel(
                    convertisseuse, i, e, c, convertisseuse->nom_noeud_connexion);
                const auto nom_sortie = string_depuis_rappel(
                    convertisseuse, i, e, c, convertisseuse->nom_connexion_entree);
                schema.setNetworkNodeConnection(
                    nom_noeud, nom_entree, nom_noeud_connecte, nom_sortie);
            }
        }
    }

    schema.setNetworkTerminal(
        cible,
        type_nuanceur,
        string_depuis_rappel(convertisseuse, convertisseuse->nom_sortie_graphe));

    // À FAIRE: paramètre du noeud
}

static void écris_données(AbcGeom::OCamera & /*o_camera*/,
                          ConvertisseuseExportCamera * /*convertisseuse*/)
{
    // À FAIRE
}

static void écris_données(AbcGeom::OCurves & /*o_curves*/,
                          ConvertisseuseExportCourbes * /*convertisseuse*/)
{
    // À FAIRE
}

static void écris_données(AbcGeom::OFaceSet & /*o_faceset*/,
                          ConvertisseuseExportFaceSet * /*convertisseuse*/)
{
    // À FAIRE
}

static void écris_données(AbcGeom::OLight & /*o_light*/,
                          ConvertisseuseExportLumiere * /*convertisseuse*/)
{
    // À FAIRE
}

static void écris_données(AbcGeom::ONuPatch & /*o_nupatch*/,
                          ConvertisseuseExportNurbs * /*convertisseuse*/)
{
    // À FAIRE
}

static void écris_données(AbcGeom::OPoints & /*o_points*/,
                          ConvertisseuseExportPoints * /*convertisseuse*/)
{
    // À FAIRE
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

    EcrivainType(AutriceArchive *autrice,
                 EcrivainCache *parent,
                 void *données,
                 const std::string &nom)
    {
        o_schema_object = type_objet_alembic(parent->oobject(), nom);
        définit_parent(parent);
        données_écrivain::init_convertisseuse(autrice, &convertisseuse);
        convertisseuse.donnees = données;
    }

    void écris_données() override
    {
        AbcKuri::écris_données(o_schema_object, &convertisseuse);
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
        résultat->définit_parent(parent);
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
