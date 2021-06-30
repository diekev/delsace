/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "alembic_import.hh"

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

static void convertis_subd(ContexteKuri * /*ctx_kuri*/,
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
    convertis_plis_sommets(convertisseuse, sample.getCornerSharpnesses(), sample.getCornerIndices());
    convertis_plis_aretes(convertisseuse, sample.getCreaseSharpnesses(), sample.getCreaseLengths(), sample.getCreaseIndices());

    /* Paramètres. */
    auto schema_subdivision = sample.getSubdivisionScheme();
    convertisseuse->marque_schema_subdivision(convertisseuse->donnees, schema_subdivision.c_str(), schema_subdivision.size());
    convertisseuse->marque_propagation_coins_face_varying(convertisseuse->donnees, sample.getFaceVaryingPropagateCorners());
    convertisseuse->marque_interpolation_frontiere_face_varying(convertisseuse->donnees, sample.getFaceVaryingInterpolateBoundary());
    convertisseuse->marque_interpolation_frontiere(convertisseuse->donnees, sample.getInterpolateBoundary());
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

    for (size_t i = 0; i < indices->size(); ++i) {
        convertisseuse->ajoute_index_point(convertisseuse->donnees, i, indices->get()[i]);
    }
}

static void convertis_points(ContexteKuri * /*ctx_kuri*/,
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

static void convertis_courbes(ContexteKuri * /*ctx_kuri*/,
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

static void convertis_nurbs(ContexteKuri * /*ctx_kuri*/,
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

static void convertis_xform(ContexteKuri * /*ctx_kuri*/,
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

static void convertis_poly_mesh(ContexteKuri * /*ctx_kuri*/,
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

    if (!archive->est_lecture()) {
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

    auto lectrice = loge_objet<LectriceCache>(ctx_kuri);
    lectrice->iobject = courant;
    return lectrice;
}

void detruit_lectrice(ContexteKuri *ctx_kuri, LectriceCache *lectrice)
{
    deloge_objet(ctx_kuri, lectrice);
}

void lectrice_ajourne_donnees(LectriceCache *lectrice, void *donnees)
{
    lectrice->donnees = donnees;
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

    if (AbcGeom::IPolyMesh::matches(lectrice->iobject.getHeader())) {
        if (contexte->initialise_convertisseuse_polymesh) {
            ConvertisseusePolyMesh convertisseuse;
            convertisseuse.donnees = lectrice->donnees;
            contexte->initialise_convertisseuse_polymesh(&convertisseuse);

            AbcGeom::IPolyMesh poly_mesh(lectrice->iobject, Abc::kWrapExisting);
            convertis_poly_mesh(ctx_kuri, &convertisseuse, poly_mesh, temps);
        }
    }
    else if (AbcGeom::ISubD::matches(lectrice->iobject.getHeader())) {
        if (contexte->initialise_convertisseuse_subd) {
            ConvertisseuseSubD convertisseuse;
            convertisseuse.donnees = lectrice->donnees;
            contexte->initialise_convertisseuse_subd(&convertisseuse);

            AbcGeom::ISubD subd(lectrice->iobject, Abc::kWrapExisting);
            convertis_subd(ctx_kuri, &convertisseuse, subd, temps);
        }
    }
    else if (AbcGeom::IPoints::matches(lectrice->iobject.getHeader())) {
        if (contexte->initialise_convertisseuse_points) {
            ConvertisseusePoints convertisseuse;
            convertisseuse.donnees = lectrice->donnees;
            contexte->initialise_convertisseuse_points(&convertisseuse);

            AbcGeom::IPoints points(lectrice->iobject, Abc::kWrapExisting);
            convertis_points(ctx_kuri, &convertisseuse, points, temps);
        }
    }
    else if (AbcGeom::INuPatch::matches(lectrice->iobject.getHeader())) {
        if (contexte->initialise_convertisseuse_nurbs) {
            ConvertisseuseNurbs convertisseuse;
            convertisseuse.donnees = lectrice->donnees;
            contexte->initialise_convertisseuse_nurbs(&convertisseuse);

            AbcGeom::INuPatch nurbs(lectrice->iobject, Abc::kWrapExisting);
            convertis_nurbs(ctx_kuri, &convertisseuse, nurbs, temps);
        }
    }
    else if (AbcGeom::ICurves::matches(lectrice->iobject.getHeader())) {
        if (contexte->initialise_convertisseuse_courbes) {
            ConvertisseuseCourbes convertisseuse;
            convertisseuse.donnees = lectrice->donnees;
            contexte->initialise_convertisseuse_courbes(&convertisseuse);

            AbcGeom::ICurves courbes(lectrice->iobject, Abc::kWrapExisting);
            convertis_courbes(ctx_kuri, &convertisseuse, courbes, temps);
        }
    }
    else if (AbcGeom::IXform::matches(lectrice->iobject.getHeader())) {
        if (contexte->initialise_convertisseuse_xform) {
            ConvertisseuseXform convertisseuse;
            convertisseuse.donnees = lectrice->donnees;
            contexte->initialise_convertisseuse_xform(&convertisseuse);

            AbcGeom::IXform xform(lectrice->iobject, Abc::kWrapExisting);
            convertis_xform(ctx_kuri, &convertisseuse, xform, temps);
        }
    }
    else {
        if (lectrice->iobject.isInstanceRoot()) {
            /* Que faire pour les instances ? */
            return;
        }
    }
}

}
