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

#include "alembic_export.hh"

#include <variant>

#include "../../InterfaceCKuri/contexte_kuri.hh"

#include "alembic_import.hh"
#include "utilitaires.hh"

#include "../alembic.h"
#include "../alembic_types.h"

using namespace Alembic;

struct EcrivainCache {
    std::variant<AbcGeom::OCamera,
                 AbcGeom::OCurves,
                 AbcGeom::OFaceSet,
                 AbcGeom::OLight,
                 AbcGeom::ONuPatch,
                 AbcGeom::OPoints,
                 AbcGeom::OPolyMesh,
                 AbcGeom::OSubD,
                 AbcGeom::OXform,
                 AbcMaterial::OMaterial>
        o_schema_object{};

    template <typename T>
    static EcrivainCache *cree(ContexteKuri *ctx, EcrivainCache *parent, const std::string &nom)
    {
        auto ecrivain = kuri_loge<EcrivainCache>(ctx);

        if (parent) {
            ecrivain->o_schema_object = T(parent->oobject(), nom);
        }
        else {
            ecrivain->o_schema_object = T({}, nom);
        }

        return ecrivain;
    }

    static EcrivainCache *cree_instance(ContexteKuri *ctx,
                                        EcrivainCache *instance,
                                        const std::string &nom)
    {
        auto ecrivain = kuri_loge<EcrivainCache>(ctx);
        // À FAIRE
        return ecrivain;
    }

    template <typename T>
    bool est_un() const
    {
        return std::holds_alternative<T>(o_schema_object);
    }

    template <typename T>
    T &comme()
    {
        return std::get<T>(o_schema_object);
    }

    AbcGeom::OObject oobject()
    {
        if (est_un<AbcGeom::OPolyMesh>()) {
            return std::get<AbcGeom::OPolyMesh>(o_schema_object);
        }

        if (est_un<AbcGeom::OSubD>()) {
            return std::get<AbcGeom::OSubD>(o_schema_object);
        }

        if (est_un<AbcGeom::OPoints>()) {
            return std::get<AbcGeom::OPoints>(o_schema_object);
        }

        if (est_un<AbcGeom::ONuPatch>()) {
            return std::get<AbcGeom::ONuPatch>(o_schema_object);
        }

        if (est_un<AbcGeom::OCurves>()) {
            return std::get<AbcGeom::OCurves>(o_schema_object);
        }

        if (est_un<AbcGeom::OXform>()) {
            return std::get<AbcGeom::OXform>(o_schema_object);
        }

        if (est_un<AbcGeom::OCamera>()) {
            return std::get<AbcGeom::OCamera>(o_schema_object);
        }

        if (est_un<AbcGeom::OFaceSet>()) {
            return std::get<AbcGeom::OFaceSet>(o_schema_object);
        }

        if (est_un<AbcGeom::OLight>()) {
            return std::get<AbcGeom::OLight>(o_schema_object);
        }

        if (est_un<AbcMaterial::OMaterial>()) {
            return std::get<AbcMaterial::OMaterial>(o_schema_object);
        }

        return {};
    }
};

namespace AbcKuri {

static void abc_export_materiau(ConvertisseuseExportMateriau *convertisseuse,
                                EcrivainCache *ecrivain)
{
    auto &omateriau = ecrivain->comme<AbcMaterial::OMaterial>();
    auto schema = omateriau.getSchema();

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

static void abc_export_poly_mesh(ConvertisseuseExportPolyMesh *convertisseuse,
                                 EcrivainCache *ecrivain)
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
    auto &o_poly_mesh = ecrivain->comme<AbcGeom::OPolyMesh>();
    auto &schema = o_poly_mesh.getSchema();

    AbcGeom::OPolyMeshSchema::Sample sample;
    sample.setPositions(positions);
    sample.setFaceCounts(face_counts);
    sample.setFaceIndices(face_indices);

    schema.set(sample);
}

EcrivainCache *cree_ecrivain_cache_depuis_ref(ContexteKuri *ctx,
                                              LectriceCache *lectrice,
                                              EcrivainCache *parent)
{
    if (AbcGeom::IPolyMesh::matches(lectrice->iobject.getHeader())) {
        return EcrivainCache::cree<AbcGeom::OPolyMesh>(ctx, parent, lectrice->iobject.getName());
    }

    if (AbcGeom::ISubD::matches(lectrice->iobject.getHeader())) {
        return EcrivainCache::cree<AbcGeom::OSubD>(ctx, parent, lectrice->iobject.getName());
    }

    if (AbcGeom::IPoints::matches(lectrice->iobject.getHeader())) {
        return EcrivainCache::cree<AbcGeom::OPoints>(ctx, parent, lectrice->iobject.getName());
    }

    if (AbcGeom::INuPatch::matches(lectrice->iobject.getHeader())) {
        return EcrivainCache::cree<AbcGeom::ONuPatch>(ctx, parent, lectrice->iobject.getName());
    }

    if (AbcGeom::ICurves::matches(lectrice->iobject.getHeader())) {
        return EcrivainCache::cree<AbcGeom::OCurves>(ctx, parent, lectrice->iobject.getName());
    }

    if (AbcGeom::IXform::matches(lectrice->iobject.getHeader())) {
        return EcrivainCache::cree<AbcGeom::OXform>(ctx, parent, lectrice->iobject.getName());
    }

    if (AbcGeom::ICamera::matches(lectrice->iobject.getHeader())) {
        return EcrivainCache::cree<AbcGeom::OCamera>(ctx, parent, lectrice->iobject.getName());
    }

    if (AbcGeom::IFaceSet::matches(lectrice->iobject.getHeader())) {
        return EcrivainCache::cree<AbcGeom::OFaceSet>(ctx, parent, lectrice->iobject.getName());
    }

    if (AbcGeom::ILight::matches(lectrice->iobject.getHeader())) {
        return EcrivainCache::cree<AbcGeom::OLight>(ctx, parent, lectrice->iobject.getName());
    }

    if (AbcMaterial::IMaterial::matches(lectrice->iobject.getHeader())) {
        return EcrivainCache::cree<AbcMaterial::OMaterial>(
            ctx, parent, lectrice->iobject.getName());
    }

    {
        /* Instance. */
    }

    return nullptr;
}

EcrivainCache *cree_ecrivain_cache(ContexteKuri *ctx,
                                   EcrivainCache *parent,
                                   const char *nom,
                                   size_t taille_nom,
                                   eTypeObjetAbc type_objet)
{
    switch (type_objet) {
        case eTypeObjetAbc::CAMERA:
        {
            return EcrivainCache::cree<AbcGeom::OCamera>(ctx, parent, {nom, taille_nom});
        }
        case eTypeObjetAbc::COURBES:
        {
            return EcrivainCache::cree<AbcGeom::OCurves>(ctx, parent, {nom, taille_nom});
        }
        case eTypeObjetAbc::FACE_SET:
        {
            return EcrivainCache::cree<AbcGeom::OFaceSet>(ctx, parent, {nom, taille_nom});
        }
        case eTypeObjetAbc::LUMIERE:
        {
            return EcrivainCache::cree<AbcGeom::OLight>(ctx, parent, {nom, taille_nom});
        }
        case eTypeObjetAbc::MATERIAU:
        {
            return EcrivainCache::cree<AbcMaterial::OMaterial>(ctx, parent, {nom, taille_nom});
        }
        case eTypeObjetAbc::NURBS:
        {
            return EcrivainCache::cree<AbcGeom::ONuPatch>(ctx, parent, {nom, taille_nom});
        }
        case eTypeObjetAbc::POINTS:
        {
            return EcrivainCache::cree<AbcGeom::OPoints>(ctx, parent, {nom, taille_nom});
        }
        case eTypeObjetAbc::POLY_MESH:
        {
            return EcrivainCache::cree<AbcGeom::OPolyMesh>(ctx, parent, {nom, taille_nom});
        }
        case eTypeObjetAbc::SUBD:
        {
            return EcrivainCache::cree<AbcGeom::OSubD>(ctx, parent, {nom, taille_nom});
        }
        case eTypeObjetAbc::XFORM:
        {
            return EcrivainCache::cree<AbcGeom::OXform>(ctx, parent, {nom, taille_nom});
        }
    }

    return nullptr;
}

EcrivainCache *cree_instance(ContexteKuri *ctx,
                             EcrivainCache *instance,
                             const char *nom,
                             size_t taille_nom)
{
    return EcrivainCache::cree_instance(ctx, instance, {nom, taille_nom});
}

void ecris_objet(ContexteKuri *ctx, EcrivainCache *ecrivain)
{
    if (ecrivain->est_un<AbcGeom::OPolyMesh>()) {
        abc_export_poly_mesh(nullptr, ecrivain);
        return;
    }

    if (ecrivain->est_un<AbcGeom::OSubD>()) {
        return;
    }

    if (ecrivain->est_un<AbcGeom::OPoints>()) {
        return;
    }

    if (ecrivain->est_un<AbcGeom::ONuPatch>()) {
        return;
    }

    if (ecrivain->est_un<AbcGeom::OCurves>()) {
        return;
    }

    if (ecrivain->est_un<AbcGeom::OXform>()) {
        return;
    }

    if (ecrivain->est_un<AbcGeom::OCamera>()) {
        return;
    }

    if (ecrivain->est_un<AbcGeom::OFaceSet>()) {
        return;
    }

    if (ecrivain->est_un<AbcGeom::OLight>()) {
        return;
    }

    if (ecrivain->est_un<AbcMaterial::OMaterial>()) {
        return;
    }
}

}  // namespace AbcKuri
