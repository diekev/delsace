/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "alembic_import.hh"

#include <codecvt>

#include "../../InterfaceCKuri/contexte_kuri.hh"

#include "../alembic_types.h"

#include "alembic_archive.hh"
#include "utilitaires.hh"

using namespace Alembic;

namespace AbcKuri {

// --------------------------------------------------------------
// Utilitaires.

template <typename Convertisseuse>
void convertis_positions(Convertisseuse *convertisseuse, AbcGeom::P3fArraySamplePtr positions)
{
    if (convertisseuse->reserve_points) {
        convertisseuse->reserve_points(convertisseuse->donnees, positions->size());
    }

    if (convertisseuse->ajoute_tous_les_points) {
        convertisseuse->ajoute_tous_les_points(
            convertisseuse->donnees, &positions->get()->x, positions->size());
    }
    else if (convertisseuse->ajoute_un_point) {
        for (size_t i = 0; i < positions->size(); ++i) {
            auto p = positions->get()[i];
            convertisseuse->ajoute_un_point(convertisseuse->donnees, p.x, p.y, p.z);
        }
    }
}

template <typename Convertisseuse>
void convertis_polygones(Convertisseuse *convertisseuse,
                         AbcGeom::Int32ArraySamplePtr face_counts,
                         AbcGeom::Int32ArraySamplePtr face_indices)
{
    if (convertisseuse->reserve_polygones) {
        convertisseuse->reserve_polygones(convertisseuse->donnees, face_counts->size());
    }

    if (convertisseuse->reserve_coin) {
        convertisseuse->reserve_coin(convertisseuse->donnees, face_indices->size());
    }

    if (convertisseuse->ajoute_tous_les_polygones) {
        convertisseuse->ajoute_tous_les_polygones(
            convertisseuse->donnees, face_counts->get(), face_counts->size());
    }
    else {
        if (convertisseuse->reserve_coins_polygone) {
            for (size_t i = 0; i < face_counts->size(); ++i) {
                convertisseuse->reserve_coins_polygone(
                    convertisseuse->donnees, i, face_counts->get()[i]);
            }
        }

        if (convertisseuse->ajoute_polygone) {
            auto decalage_coin = 0;

            for (size_t i = 0; i < face_counts->size(); ++i) {
                auto nombre_de_coins = face_counts->get()[i];
                auto ptr_coins = &face_indices->get()[decalage_coin];
                convertisseuse->ajoute_polygone(
                    convertisseuse->donnees, i, ptr_coins, nombre_de_coins);
                decalage_coin += nombre_de_coins;
            }
        }
    }

    if (convertisseuse->ajoute_tous_les_coins) {
        convertisseuse->ajoute_tous_les_coins(
            convertisseuse->donnees, face_indices->get(), face_indices->size());
    }
    else {
        if (convertisseuse->ajoute_coin_polygone) {
            auto decalage_coin = 0;

            for (size_t i = 0; i < face_counts->size(); ++i) {
                auto nombre_de_coins = face_counts->get()[i];
                auto ptr_coins = &face_indices->get()[decalage_coin];

                for (int j = 0; j < nombre_de_coins; ++j) {
                    convertisseuse->ajoute_coin_polygone(convertisseuse->donnees, i, ptr_coins[j]);
                }

                decalage_coin += nombre_de_coins;
            }
        }
    }
}

// --------------------------------------------------------------
// Lecture des objets.

static void convertis_polygones_trous(ConvertisseuseSubD *convertisseuse,
                                      Abc::Int32ArraySamplePtr trous)
{
    if (!trous) {
        return;
    }

    if (convertisseuse->reserve_trous) {
        convertisseuse->reserve_trous(convertisseuse->donnees, trous->size());
    }

    for (size_t i = 0; i < trous->size(); i++) {
        convertisseuse->marque_polygone_trou(convertisseuse->donnees, trous->get()[i]);
    }
}

static void convertis_plis_sommets(ConvertisseuseSubD *convertisseuse,
                                   Abc::FloatArraySamplePtr sharpnesses,
                                   Abc::Int32ArraySamplePtr indices)
{
    if (!sharpnesses || !indices) {
        return;
    }

    if (convertisseuse->reserve_plis_sommets) {
        convertisseuse->reserve_plis_sommets(convertisseuse->donnees, indices->size());
    }

    for (size_t i = 0; i < indices->size(); ++i) {
        const int index = indices->get()[i];
        const float sharpness = sharpnesses->get()[i];

        convertisseuse->marque_plis_vertex(convertisseuse->donnees, index, sharpness);
    }
}

static void convertis_plis_aretes(ConvertisseuseSubD *convertisseuse,
                                  Abc::FloatArraySamplePtr sharpnesses,
                                  Abc::Int32ArraySamplePtr /*lengths*/,
                                  Abc::Int32ArraySamplePtr indices)
{
    if (!sharpnesses || !indices) {
        return;
    }

    if (convertisseuse->reserve_plis_aretes) {
        convertisseuse->reserve_plis_aretes(convertisseuse->donnees, indices->size());
    }

    for (size_t i = 0, s = 0; i < indices->size(); i += 2, s++) {
        int v1 = (*indices)[i];
        int v2 = (*indices)[i + 1];

        if (v2 < v1) {
            /* Il est commun de stocker les arêtes avec le vertex dont l'index est le plus comme
             * origine. */
            std::swap(v1, v2);
        }

        const auto sharpness = (*sharpnesses)[s];

        convertisseuse->marque_plis_aretes(convertisseuse->donnees, v1, v2, sharpness);
    }
}

static void convertis_objet(ContexteKuri * /*ctx_kuri*/,
                            ConvertisseuseSubD *convertisseuse,
                            AbcGeom::ISubD &subd,
                            const double time)
{
    auto &schema = subd.getSchema();
    auto selector = Abc::ISampleSelector(time);

    AbcGeom::ISubDSchema::Sample sample;
    schema.get(sample, selector);

    if (!sample.valid()) {
        return;
    }

    /* Convertis les positions. */
    const auto &positions = sample.getPositions();
    convertis_positions(convertisseuse, positions);

    /* Convertis les polygones. */
    const auto &face_counts = sample.getFaceCounts();
    const auto &face_indices = sample.getFaceIndices();
    convertis_polygones(convertisseuse, face_counts, face_indices);

    convertis_polygones_trous(convertisseuse, sample.getHoles());
    convertis_plis_sommets(
        convertisseuse, sample.getCornerSharpnesses(), sample.getCornerIndices());
    convertis_plis_aretes(convertisseuse,
                          sample.getCreaseSharpnesses(),
                          sample.getCreaseLengths(),
                          sample.getCreaseIndices());

    /* Paramètres. */
    auto schema_subdivision = sample.getSubdivisionScheme();
    convertisseuse->marque_schema_subdivision(
        convertisseuse->donnees, schema_subdivision.c_str(), schema_subdivision.size());
    convertisseuse->marque_propagation_coins_face_varying(convertisseuse->donnees,
                                                          sample.getFaceVaryingPropagateCorners());
    convertisseuse->marque_interpolation_frontiere_face_varying(
        convertisseuse->donnees, sample.getFaceVaryingInterpolateBoundary());
    convertisseuse->marque_interpolation_frontiere(convertisseuse->donnees,
                                                   sample.getInterpolateBoundary());
}

static void convertis_index_points(ConvertisseusePoints *convertisseuse,
                                   AbcGeom::UInt64ArraySamplePtr indices)
{
    if (!indices) {
        return;
    }

    if (convertisseuse->reserve_index) {
        convertisseuse->reserve_index(convertisseuse->donnees, indices->size());
    }

    if (convertisseuse->ajoute_index_point) {
        for (size_t i = 0; i < indices->size(); ++i) {
            convertisseuse->ajoute_index_point(convertisseuse->donnees, i, indices->get()[i]);
        }
    }
}

static void convertis_objet(ContexteKuri * /*ctx_kuri*/,
                            ConvertisseusePoints *convertisseuse,
                            AbcGeom::IPoints &points,
                            const double time)
{
    auto &schema = points.getSchema();
    auto selector = Abc::ISampleSelector(time);

    AbcGeom::IPointsSchema::Sample sample;
    schema.get(sample, selector);

    if (!sample.valid()) {
        return;
    }

    /* Convertis les positions. */
    const auto &positions = sample.getPositions();
    convertis_positions(convertisseuse, positions);

    convertis_index_points(convertisseuse, sample.getIds());
}

static void convertis_objet(ContexteKuri * /*ctx_kuri*/,
                            ConvertisseuseCourbes *convertisseuse,
                            AbcGeom::ICurves &curves,
                            const double time)
{
    auto &schema = curves.getSchema();
    auto selector = Abc::ISampleSelector(time);

    AbcGeom::ICurvesSchema::Sample sample;
    schema.get(sample, selector);

    if (!sample.valid()) {
        return;
    }

    // À FAIRE
}

static void convertis_objet(ContexteKuri * /*ctx_kuri*/,
                            ConvertisseuseNurbs *convertisseuse,
                            AbcGeom::INuPatch &nurbs,
                            const double time)
{
    auto &schema = nurbs.getSchema();
    auto selector = Abc::ISampleSelector(time);

    AbcGeom::INuPatchSchema::Sample sample;
    schema.get(sample, selector);

    if (!sample.valid()) {
        return;
    }

    // À FAIRE
}

static void convertis_objet(ContexteKuri * /*ctx_kuri*/,
                            ConvertisseuseXform *convertisseuse,
                            AbcGeom::IXform &xform,
                            const double time)
{
    auto &schema = xform.getSchema();
    auto selector = Abc::ISampleSelector(time);

    AbcGeom::XformSample sample;
    schema.get(sample, selector);

    // À FAIRE
}

static void convertis_objet(ContexteKuri * /*ctx_kuri*/,
                            ConvertisseuseFaceSet *convertisseuse,
                            AbcGeom::IFaceSet &xform,
                            const double time)
{
    // auto &schema = xform.getSchema();
    // auto selector = Abc::ISampleSelector(time);

    // À FAIRE
}

static void convertis_objet(ContexteKuri * /*ctx_kuri*/,
                            ConvertisseuseCamera *convertisseuse,
                            AbcGeom::ICamera &xform,
                            const double time)
{
    // auto &schema = xform.getSchema();
    // auto selector = Abc::ISampleSelector(time);

    // À FAIRE
}

static void convertis_objet(ContexteKuri * /*ctx_kuri*/,
                            ConvertisseuseLumiere *convertisseuse,
                            AbcGeom::ILight &xform,
                            const double time)
{
    // auto &schema = xform.getSchema();
    // auto selector = Abc::ISampleSelector(time);

    // À FAIRE
}

static void convertis_objet(ContexteKuri * /*ctx_kuri*/,
                            ConvertisseuseMateriau *convertisseuse,
                            AbcMaterial::IMaterial &xform,
                            const double time)
{
    // auto &schema = xform.getSchema();
    // auto selector = Abc::ISampleSelector(time);
    // À FAIRE
}

static void convertis_objet(ContexteKuri * /*ctx_kuri*/,
                            ConvertisseusePolyMesh *convertisseuse,
                            AbcGeom::IPolyMesh &polymesh,
                            const double time)
{
    auto &schema = polymesh.getSchema();
    auto selector = Abc::ISampleSelector(time);

    AbcGeom::IPolyMeshSchema::Sample sample;
    schema.get(sample, selector);

    if (!sample.valid()) {
        return;
    }

    /* Convertis les positions. */
    const auto &positions = sample.getPositions();
    convertis_positions(convertisseuse, positions);

    /* Convertis les polygones. */
    const auto &face_counts = sample.getFaceCounts();
    const auto &face_indices = sample.getFaceIndices();
    convertis_polygones(convertisseuse, face_counts, face_indices);
}

struct iteratrice_chemin {
  private:
    std::string_view chemin{};
    std::string_view morceau_courant{};
    size_t pos = 0;

  public:
    iteratrice_chemin(const std::string &chemin_complet) : chemin(chemin_complet)
    {
    }

    std::string_view suivant()
    {
        if (fini()) {
            return "";
        }

        /* Saute le premier slash. */
        if (chemin[pos] == '/') {
            pos += 1;
        }

        auto nouvelle_pos = chemin.find('/', pos);
        if (nouvelle_pos == std::string::npos) {
            nouvelle_pos = chemin.size();
        }

        morceau_courant = chemin.substr(pos, nouvelle_pos - pos);
        pos = nouvelle_pos;
        return morceau_courant;
    }

    bool fini() const
    {
        return pos >= chemin.size();
    }
};

LectriceCache *cree_lectrice_cache(ContexteKuri *ctx_kuri,
                                   ArchiveCache *archive,
                                   const char *ptr_nom,
                                   size_t taille_nom)
{
    if (!archive) {
        // std::cerr << "L'archive est nulle !\n";
        return nullptr;
    }

    auto &iarchive = archive->iarchive();

    if (!iarchive.valid()) {
        // std::cerr << "L'archive est invalide !\n";
        return nullptr;
    }

    const auto nom = std::string(ptr_nom, taille_nom);

    if (nom.empty()) {
        // std::cerr << "Le nom est vide !\n";
        return nullptr;
    }

    AbcGeom::IObject courant = iarchive.getTop();

    if (!courant.valid()) {
        // std::cerr << "La racine de l'archvie est invalide !\n";
        return nullptr;
    }

    auto iteratrice = iteratrice_chemin(nom);

    while (!iteratrice.fini()) {
        const auto morceau = iteratrice.suivant();
        auto enfant = courant.getChild(std::string(morceau));

        if (!enfant.valid()) {
            // std::cerr << "L'enfant est invalide !\n";
            return nullptr;
        }

        courant = enfant;
    }

    if (!courant.valid()) {
        // std::cerr << "L'enfant final est invalide !\n";
        return nullptr;
    }

    auto lectrice = kuri_loge<LectriceCache>(ctx_kuri);
    lectrice->iobject = courant;
    return lectrice;
}

void detruit_lectrice(ContexteKuri *ctx_kuri, LectriceCache *lectrice)
{
    kuri_deloge(ctx_kuri, lectrice);
}

void lectrice_ajourne_donnees(LectriceCache *lectrice, void *donnees)
{
    lectrice->donnees = donnees;
}

template <typename TypeObjet, typename Convertisseuse>
static void convertis_objet_impl(ContexteKuri *ctx_kuri,
                                 LectriceCache *lectrice,
                                 double temps,
                                 void (*rappel_initialisation)(Convertisseuse *))
{
    if (!rappel_initialisation) {
        return;
    }

    Convertisseuse convertisseuse;
    convertisseuse.donnees = lectrice->donnees;
    rappel_initialisation(&convertisseuse);

    auto objet = lectrice->comme<TypeObjet>();
    convertis_objet(ctx_kuri, &convertisseuse, objet, temps);
}

void lis_objet(ContexteKuri *ctx_kuri,
               ContexteLectureCache *contexte,
               LectriceCache *lectrice,
               double temps)
{
    if (!lectrice || !lectrice->iobject.valid()) {
        // contexte->rapporte_erreur(contexte, )
        return;
    }

    if (lectrice->est_un<AbcGeom::IPolyMesh>()) {
        convertis_objet_impl<AbcGeom::IPolyMesh>(
            ctx_kuri, lectrice, temps, contexte->initialise_convertisseuse_polymesh);
    }
    else if (lectrice->est_un<AbcGeom::ISubD>()) {
        convertis_objet_impl<AbcGeom::ISubD>(
            ctx_kuri, lectrice, temps, contexte->initialise_convertisseuse_subd);
    }
    else if (lectrice->est_un<AbcGeom::IPoints>()) {
        convertis_objet_impl<AbcGeom::IPoints>(
            ctx_kuri, lectrice, temps, contexte->initialise_convertisseuse_points);
    }
    else if (lectrice->est_un<AbcGeom::INuPatch>()) {
        convertis_objet_impl<AbcGeom::INuPatch>(
            ctx_kuri, lectrice, temps, contexte->initialise_convertisseuse_nurbs);
    }
    else if (lectrice->est_un<AbcGeom::ICurves>()) {
        convertis_objet_impl<AbcGeom::ICurves>(
            ctx_kuri, lectrice, temps, contexte->initialise_convertisseuse_courbes);
    }
    else if (lectrice->est_un<AbcGeom::IXform>()) {
        convertis_objet_impl<AbcGeom::IXform>(
            ctx_kuri, lectrice, temps, contexte->initialise_convertisseuse_xform);
    }
    else if (lectrice->est_un<AbcGeom::ICamera>()) {
        convertis_objet_impl<AbcGeom::ICamera>(
            ctx_kuri, lectrice, temps, contexte->initialise_convertisseuse_camera);
    }
    else if (lectrice->est_un<AbcGeom::ILight>()) {
        convertis_objet_impl<AbcGeom::ILight>(
            ctx_kuri, lectrice, temps, contexte->initialise_convertisseuse_lumiere);
    }
    else if (lectrice->est_un<AbcGeom::IFaceSet>()) {
        convertis_objet_impl<AbcGeom::IFaceSet>(
            ctx_kuri, lectrice, temps, contexte->initialise_convertisseuse_face_set);
    }
    else if (lectrice->est_un<AbcMaterial::IMaterial>()) {
        convertis_objet_impl<AbcMaterial::IMaterial>(
            ctx_kuri, lectrice, temps, contexte->initialise_convertisseuse_materiau);
    }
    else {
        if (lectrice->iobject.isInstanceRoot()) {
            /* Que faire pour les instances ? */
        }
    }
}

/* À FAIRE : ceci prend en compte les propriétés de bases (.P, .N, etc.). */
static void traverse_propriete_composee(
    const AbcGeom::ICompoundProperty &prop,
    std::function<void(const AbcGeom::ICompoundProperty &, const Abc::PropertyHeader &)> rappel)
{
    if (!prop.valid()) {
        return;
    }

    for (size_t i = 0; i < prop.getNumProperties(); i++) {
        const auto &sous_prop = prop.getPropertyHeader(i);

        if (sous_prop.isArray()) {
            rappel(prop, sous_prop);
        }
        else if (sous_prop.isScalar()) {
            rappel(prop, sous_prop);
        }
        else if (sous_prop.isCompound()) {
            traverse_propriete_composee(AbcGeom::ICompoundProperty(prop, sous_prop.getName()),
                                        rappel);
        }
    }
}

static std::set<std::string> liste_attributs_requis(ConvertisseuseImportAttributs *convertisseuse,
                                                    const AbcGeom::ICompoundProperty &attributs)
{
    std::set<std::string> résultat;

    if (convertisseuse->lis_tous_les_attributs(convertisseuse)) {
        traverse_propriete_composee(attributs, [&](const auto & /*parent*/, const auto &entete) {
            résultat.insert(entete.getName());
        });
        return résultat;
    }

    const int nombre_attributs_requis = convertisseuse->nombre_attributs_requis(convertisseuse);

    if (nombre_attributs_requis == 0) {
        return {};
    }

    for (int i = 0; i < nombre_attributs_requis; i++) {
        auto nom_attribut = string_depuis_rappel(
            convertisseuse, static_cast<size_t>(i), convertisseuse->nom_attribut_requis);

        if (nom_attribut.empty()) {
            continue;
        }

        résultat.insert(nom_attribut);
    }

    return résultat;
}

enum {
    SCALAIRE,
    UV,
    VECTEUR_2D,
    VECTEUR_3D,
    POINT_2D,
    POINT_3D,
    BOITE_2D,
    BOITE_3D,
    MATRICE_3D,
    MATRICE_4D,
    QUATERNION,
    COULEUR_RVB,
    COULEUR_RVBA,
    NORMAL_2D,
    NORMAL_3D,
};

enum {
    BOOL,
    Z8,
    Z16,
    Z32,
    Z64,
    N8,
    N16,
    N32,
    N64,
    R16,
    R32,
    R64,
    CHAINE,
};

template <typename T>
struct convertisseuse_valeur;

template <>
struct convertisseuse_valeur<std::string> {
    static void convertis(ConvertisseuseImportAttributs *convertisseuse,
                          void *ptr,
                          size_t i,
                          const std::string &valeur)
    {
        convertisseuse->ajoute_chaine(ptr, i, valeur.c_str(), valeur.size());
    }
};

template <>
struct convertisseuse_valeur<std::wstring> {
    static void convertis(ConvertisseuseImportAttributs *convertisseuse,
                          void *ptr,
                          size_t i,
                          const std::wstring &valeur)
    {
        using convert_type = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_type, wchar_t> converter;
        std::string valeur_convertie = converter.to_bytes(valeur);
        convertisseuse->ajoute_chaine(ptr, i, valeur_convertie.c_str(), valeur_convertie.size());
    }
};

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
#define DEFINIS_CONVERTISSEUSE_VALEUR(TYPE_ALEMBIC, TYPE_POINTEUR, TYPE_KURI, DIMENSIONS)         \
    template <>                                                                                   \
    struct convertisseuse_valeur<TYPE_ALEMBIC> {                                                  \
        static void convertis(ConvertisseuseImportAttributs *convertisseuse,                      \
                              void *ptr,                                                          \
                              size_t i,                                                           \
                              const TYPE_ALEMBIC &valeur)                                         \
        {                                                                                         \
            convertisseuse->ajoute_##TYPE_KURI(                                                   \
                ptr, i, reinterpret_cast<const TYPE_POINTEUR *>(&valeur), DIMENSIONS);            \
        }                                                                                         \
    }

DEFINIS_CONVERTISSEUSE_VALEUR(Alembic::Abc::bool_t, bool, bool, 1);
DEFINIS_CONVERTISSEUSE_VALEUR(int8_t, int8_t, z8, 1);
DEFINIS_CONVERTISSEUSE_VALEUR(uint8_t, uint8_t, n8, 1);
DEFINIS_CONVERTISSEUSE_VALEUR(int16_t, int16_t, z16, 1);
DEFINIS_CONVERTISSEUSE_VALEUR(uint16_t, uint16_t, n16, 1);
DEFINIS_CONVERTISSEUSE_VALEUR(int32_t, int32_t, z32, 1);
DEFINIS_CONVERTISSEUSE_VALEUR(uint32_t, uint32_t, n32, 1);
DEFINIS_CONVERTISSEUSE_VALEUR(int64_t, int64_t, z64, 1);
DEFINIS_CONVERTISSEUSE_VALEUR(uint64_t, uint64_t, n64, 1);
// DEFINIS_CONVERTISSEUSE_VALEUR(half, half, r16, 1);
DEFINIS_CONVERTISSEUSE_VALEUR(float, float, r32, 1);
DEFINIS_CONVERTISSEUSE_VALEUR(double, double, r64, 1);

DEFINIS_CONVERTISSEUSE_VALEUR(Imath::V2s, int16_t, z16, 2);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::V2i, int32_t, z32, 2);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::V2f, float, r32, 2);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::V2d, double, r64, 2);

DEFINIS_CONVERTISSEUSE_VALEUR(Imath::V3s, int16_t, z16, 3);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::V3i, int32_t, z32, 3);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::V3f, float, r32, 3);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::V3d, double, r64, 3);

DEFINIS_CONVERTISSEUSE_VALEUR(Imath::Box2s, int16_t, z16, 4);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::Box2i, int32_t, z32, 4);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::Box2f, float, r32, 4);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::Box2d, double, r64, 4);

DEFINIS_CONVERTISSEUSE_VALEUR(Imath::Box3s, int16_t, z16, 6);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::Box3i, int32_t, z32, 6);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::Box3f, float, r32, 6);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::Box3d, double, r64, 6);

DEFINIS_CONVERTISSEUSE_VALEUR(Imath::M33f, float, r32, 9);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::M33d, double, r64, 9);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::M44f, float, r32, 16);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::M44d, double, r64, 16);

DEFINIS_CONVERTISSEUSE_VALEUR(Imath::Quatf, float, r32, 4);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::Quatd, double, r64, 4);

// DEFINIS_CONVERTISSEUSE_VALEUR(Imath::C3h, half, r16, 3);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::C3f, float, r32, 3);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::C3c, uint8_t, n8, 3);

// DEFINIS_CONVERTISSEUSE_VALEUR(Imath::C4h, half, r16, 4);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::C4f, float, r32, 4);
DEFINIS_CONVERTISSEUSE_VALEUR(Imath::C4c, uint8_t, n8, 4);
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

static eAbcDomaineAttribut determine_portee(ConvertisseuseImportAttributs *convertisseuse,
                                            AbcGeom::GeometryScope portee_pretendue,
                                            AbcGeom::UInt32ArraySamplePtr indices)
{
    if (!indices) {
        return eAbcDomaineAttribut::AUCUNE;
    }

    if (!convertisseuse->information_portee) {
        return eAbcDomaineAttribut::AUCUNE;
    }

    const int taille_des_donnees = static_cast<int>(indices->size());

    int nombre_de_points = 0;
    int nombre_de_primitives = 0;
    int nombre_de_points_primitives = 0;
    convertisseuse->information_portee(
        convertisseuse, &nombre_de_points, &nombre_de_primitives, &nombre_de_points_primitives);

    switch (portee_pretendue) {
        case AbcGeom::kConstantScope:
        {
            if (taille_des_donnees == 1) {
                return eAbcDomaineAttribut::OBJET;
            }
            break;
        }
        case AbcGeom::kUniformScope:
        {
            if (taille_des_donnees == 1) {
                return eAbcDomaineAttribut::OBJET;
            }
            break;
        }
        case AbcGeom::kVaryingScope:
        {
            if (taille_des_donnees == nombre_de_points) {
                return eAbcDomaineAttribut::POINT;
            }

            if (taille_des_donnees == nombre_de_points_primitives) {
                return eAbcDomaineAttribut::POINT_PRIMITIVE;
            }
            if (taille_des_donnees == nombre_de_primitives) {
                return eAbcDomaineAttribut::PRIMITIVE;
            }

            break;
        }
        case AbcGeom::kVertexScope:
        {
            if (taille_des_donnees == nombre_de_points) {
                return eAbcDomaineAttribut::POINT;
            }

            break;
        }
        case AbcGeom::kFacevaryingScope:
        {
            if (taille_des_donnees == nombre_de_points_primitives) {
                return eAbcDomaineAttribut::POINT_PRIMITIVE;
            }

            break;
        }
        case AbcGeom::kUnknownScope:
        {
            break;
        }
    }

    return eAbcDomaineAttribut::AUCUNE;
}

template <typename Trait>
static void gere_attribut(ConvertisseuseImportAttributs *convertisseuse,
                          AbcGeom::ITypedGeomParam<Trait> param,
                          double temps)
{
    using TypeEchantillon = typename AbcGeom::ITypedGeomParam<Trait>::Sample;
    using TypeDeDonnes = typename Trait::value_type;
    using ConvertisseuseValeur = convertisseuse_valeur<TypeDeDonnes>;

    if (!param.valid()) {
        return;
    }

    const auto iss = AbcGeom::ISampleSelector(temps);

    TypeEchantillon sample;
    param.getIndexed(sample, iss);

    if (!sample.valid()) {
        return;
    }

    const auto valeurs = sample.getVals();
    const auto indices = sample.getIndices();

    if (!valeurs || !indices) {
        return;
    }

    const auto portee = determine_portee(convertisseuse, param.getScope(), indices);
    if (portee == eAbcDomaineAttribut::AUCUNE) {
        // À FAIRE : nous pourrions avoir une manière de quand même importer ces données
        return;
    }

    //  À FAIRE : passe les informations sur le type de donnée (bool, float, etc. | 1d, 2d, etc).
    const auto &nom = param.getName();
    auto ptr_attribut = convertisseuse->ajoute_attribut(
        convertisseuse, nom.c_str(), nom.size(), portee);
    if (!ptr_attribut) {
        return;
    }

    for (auto i = 0ul; i < indices->size(); ++i) {
        auto idx = (*indices)[i];

        if (idx >= valeurs->size()) {
            continue;
        }

        ConvertisseuseValeur::convertis(convertisseuse, ptr_attribut, i, (*valeurs)[idx]);
    }
}

static void convertis_attribut(ConvertisseuseImportAttributs *convertisseuse,
                               const AbcGeom::ICompoundProperty &parent,
                               const Abc::PropertyHeader &entete,
                               double temps)
{
    if (AbcGeom::IV2fGeomParam::matches(entete) && Alembic::AbcGeom::isUV(entete)) {
        // À FAIRE : const auto &param = AbcGeom::IV2fGeomParam(parent, entete.getName());
        // Cas spécial pour les UVs.
        return;
    }

#define GERE_ATTRIBUT(TYPE_GEOM_PARAM)                                                            \
    if (TYPE_GEOM_PARAM::matches(entete)) {                                                       \
        const auto &param = TYPE_GEOM_PARAM(parent, entete.getName());                            \
        gere_attribut(convertisseuse, param, temps);                                              \
        return;                                                                                   \
    }

    GERE_ATTRIBUT(AbcGeom::IBoolGeomParam)
    GERE_ATTRIBUT(AbcGeom::IUcharGeomParam)
    GERE_ATTRIBUT(AbcGeom::ICharGeomParam)
    GERE_ATTRIBUT(AbcGeom::IUInt16GeomParam)
    GERE_ATTRIBUT(AbcGeom::IInt16GeomParam)
    GERE_ATTRIBUT(AbcGeom::IUInt32GeomParam)
    GERE_ATTRIBUT(AbcGeom::IInt32GeomParam)
    GERE_ATTRIBUT(AbcGeom::IUInt64GeomParam)
    GERE_ATTRIBUT(AbcGeom::IInt64GeomParam)
    // GERE_ATTRIBUT(AbcGeom::IHalfGeomParam)
    GERE_ATTRIBUT(AbcGeom::IFloatGeomParam)
    GERE_ATTRIBUT(AbcGeom::IDoubleGeomParam)
    GERE_ATTRIBUT(AbcGeom::IStringGeomParam)
    GERE_ATTRIBUT(AbcGeom::IWstringGeomParam)

    GERE_ATTRIBUT(AbcGeom::IV2sGeomParam)
    GERE_ATTRIBUT(AbcGeom::IV2iGeomParam)
    GERE_ATTRIBUT(AbcGeom::IV2fGeomParam)
    GERE_ATTRIBUT(AbcGeom::IV2dGeomParam)

    GERE_ATTRIBUT(AbcGeom::IV3sGeomParam)
    GERE_ATTRIBUT(AbcGeom::IV3iGeomParam)
    GERE_ATTRIBUT(AbcGeom::IV3fGeomParam)
    GERE_ATTRIBUT(AbcGeom::IV3dGeomParam)

    GERE_ATTRIBUT(AbcGeom::IP2sGeomParam)
    GERE_ATTRIBUT(AbcGeom::IP2iGeomParam)
    GERE_ATTRIBUT(AbcGeom::IP2fGeomParam)
    GERE_ATTRIBUT(AbcGeom::IP2dGeomParam)

    GERE_ATTRIBUT(AbcGeom::IP3sGeomParam)
    GERE_ATTRIBUT(AbcGeom::IP3iGeomParam)
    GERE_ATTRIBUT(AbcGeom::IP3fGeomParam)
    GERE_ATTRIBUT(AbcGeom::IP3dGeomParam)

    GERE_ATTRIBUT(AbcGeom::IBox2sGeomParam)
    GERE_ATTRIBUT(AbcGeom::IBox2iGeomParam)
    GERE_ATTRIBUT(AbcGeom::IBox2fGeomParam)
    GERE_ATTRIBUT(AbcGeom::IBox2dGeomParam)

    GERE_ATTRIBUT(AbcGeom::IBox3sGeomParam)
    GERE_ATTRIBUT(AbcGeom::IBox3iGeomParam)
    GERE_ATTRIBUT(AbcGeom::IBox3fGeomParam)
    GERE_ATTRIBUT(AbcGeom::IBox3dGeomParam)

    GERE_ATTRIBUT(AbcGeom::IM33fGeomParam)
    GERE_ATTRIBUT(AbcGeom::IM33dGeomParam)
    GERE_ATTRIBUT(AbcGeom::IM44fGeomParam)
    GERE_ATTRIBUT(AbcGeom::IM44dGeomParam)

    GERE_ATTRIBUT(AbcGeom::IQuatfGeomParam)
    GERE_ATTRIBUT(AbcGeom::IQuatdGeomParam)

    // GERE_ATTRIBUT(AbcGeom::IC3hGeomParam)
    GERE_ATTRIBUT(AbcGeom::IC3fGeomParam)
    GERE_ATTRIBUT(AbcGeom::IC3cGeomParam)

    // GERE_ATTRIBUT(AbcGeom::IC4hGeomParam)
    GERE_ATTRIBUT(AbcGeom::IC4fGeomParam)
    GERE_ATTRIBUT(AbcGeom::IC4cGeomParam)

    GERE_ATTRIBUT(AbcGeom::IN2fGeomParam)
    GERE_ATTRIBUT(AbcGeom::IN2dGeomParam)

    GERE_ATTRIBUT(AbcGeom::IN3fGeomParam)
    GERE_ATTRIBUT(AbcGeom::IN3dGeomParam)

#undef GERE_ATTRIBUT
}

// À FAIRE : ctx.annule
static void lis_attributs(ContexteKuri * /*ctx_kuri*/,
                          ConvertisseuseImportAttributs *convertisseuse,
                          const AbcGeom::ICompoundProperty &attributs,
                          double temps)
{
    if (!attributs.valid()) {
        return;
    }

    // construit la liste de tous les attributs requis
    auto requetes = liste_attributs_requis(convertisseuse, attributs);

    // convertis les attributs
    // -- information_portées

    traverse_propriete_composee(attributs, [&](const auto &parent, const auto &sous_prop) {
        if (requetes.find(sous_prop.getName()) == requetes.end()) {
            return;
        }

        convertis_attribut(convertisseuse, parent, sous_prop, temps);
    });
}

void lis_attributs(ContexteKuri *ctx_kuri,
                   LectriceCache *lectrice,
                   ConvertisseuseImportAttributs *convertisseuse,
                   double temps)
{
    if (!lectrice || !lectrice->iobject.valid()) {
        // contexte->rapporte_erreur(contexte, )
        return;
    }

    if (lectrice->est_un<AbcGeom::IPolyMesh>()) {
        auto poly_mesh = lectrice->comme<AbcGeom::IPolyMesh>();
        lis_attributs(ctx_kuri, convertisseuse, poly_mesh.getSchema(), temps);
    }
    else if (lectrice->est_un<AbcGeom::ISubD>()) {
        auto subd = lectrice->comme<AbcGeom::ISubD>();
        lis_attributs(ctx_kuri, convertisseuse, subd.getSchema(), temps);
    }
    else if (lectrice->est_un<AbcGeom::IPoints>()) {
        auto points = lectrice->comme<AbcGeom::IPoints>();
        lis_attributs(ctx_kuri, convertisseuse, points.getSchema(), temps);
    }
    else if (lectrice->est_un<AbcGeom::INuPatch>()) {
        auto nurbs = lectrice->comme<AbcGeom::INuPatch>();
        lis_attributs(ctx_kuri, convertisseuse, nurbs.getSchema(), temps);
    }
    else if (lectrice->est_un<AbcGeom::ICurves>()) {
        auto courbes = lectrice->comme<AbcGeom::ICurves>();
        lis_attributs(ctx_kuri, convertisseuse, courbes.getSchema(), temps);
    }
    else if (lectrice->est_un<AbcGeom::IXform>()) {
        auto xform = lectrice->comme<AbcGeom::IXform>();
        lis_attributs(ctx_kuri, convertisseuse, xform.getSchema(), temps);
    }
    else if (lectrice->est_un<AbcGeom::ICamera>()) {
        auto camera = lectrice->comme<AbcGeom::ICamera>();
        lis_attributs(ctx_kuri, convertisseuse, camera.getSchema(), temps);
    }
    else if (lectrice->est_un<AbcGeom::ILight>()) {
        auto lumiere = lectrice->comme<AbcGeom::ILight>();
        lis_attributs(ctx_kuri, convertisseuse, lumiere.getSchema(), temps);
    }
    else if (lectrice->est_un<AbcGeom::IFaceSet>()) {
        auto face_set = lectrice->comme<AbcGeom::IFaceSet>();
        lis_attributs(ctx_kuri, convertisseuse, face_set.getSchema(), temps);
    }
    else if (lectrice->est_un<AbcMaterial::IMaterial>()) {
        auto materiau = lectrice->comme<AbcMaterial::IMaterial>();
        lis_attributs(ctx_kuri, convertisseuse, materiau.getSchema(), temps);
    }
    else {
        /* Instances ou invalide. */
    }
}

}  // namespace AbcKuri
