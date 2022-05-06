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
 * The Original Code is Copyright (C) 2022 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "booleen_maillage.hh"

#include "booleen/boolean_operations.hpp"
#include "booleen/properties_polyhedron_3.h"

#include "fracture.hh"
#include "outils.hh"

namespace geo {

static std::unique_ptr<EnrichedPolyhedron> convertis_vers_polyhedre(Maillage const &maillage)
{
    std::unique_ptr<EnrichedPolyhedron> resultat = std::make_unique<EnrichedPolyhedron>();
    auto point_map = get(boost::vertex_point, *resultat);

    /* Exporte les points. */
    using vertex_descriptor = boost::graph_traits<EnrichedPolyhedron>::vertex_descriptor;
    kuri::tableau<vertex_descriptor> vertices;
    vertices.reserve(maillage.nombreDePoints());

    for (long i = 0; i < maillage.nombreDePoints(); i++) {
        auto point = maillage.pointPourIndex(i);
        auto vd = CGAL::add_vertex(*resultat);
        put(point_map, vd, Point3d(point.x, point.y, point.z));
        vertices.ajoute(vd);
    }

    /* Exporte les polygones. */
    kuri::tableau<int> temp_access_index_sommet;

    for (long i = 0; i < maillage.nombreDePolygones(); i++) {
        const long nombre_sommets = maillage.nombreDeSommetsPolygone(i);

        temp_access_index_sommet.redimensionne(nombre_sommets);
        maillage.indexPointsSommetsPolygone(i, temp_access_index_sommet.donnees());

        std::vector<vertex_descriptor> face_vertices;
        face_vertices.reserve(nombre_sommets);

        for (long j = 0; j < nombre_sommets; j++) {
            face_vertices.push_back(vertices[temp_access_index_sommet[j]]);
        }

        auto face = CGAL::Euler::add_face(face_vertices, *resultat);

        if (face == boost::graph_traits<EnrichedPolyhedron>::null_face()) {
            std::cerr << "Erreur lors de la construction de la face !\n";
        }
    }

    return resultat;
}

static std::unique_ptr<EnrichedPolyhedron> convertis_vers_polyhedre(CelluleVoronoi const &cellule)
{
    std::unique_ptr<EnrichedPolyhedron> resultat = std::make_unique<EnrichedPolyhedron>();
    auto point_map = get(boost::vertex_point, *resultat);

    /* Exporte les points. */
    using vertex_descriptor = boost::graph_traits<EnrichedPolyhedron>::vertex_descriptor;
    kuri::tableau<vertex_descriptor> vertices;
    vertices.reserve(cellule.totvert);

    for (auto i = 0; i < cellule.totvert; ++i) {
        auto px = static_cast<float>(cellule.verts[i * 3]);
        auto py = static_cast<float>(cellule.verts[i * 3 + 1]);
        auto pz = static_cast<float>(cellule.verts[i * 3 + 2]);
        auto vd = CGAL::add_vertex(*resultat);
        put(point_map, vd, Point3d(px, py, pz));
        vertices.ajoute(vd);
    }

    /* Exporte les polygones. */
    kuri::tableau<int> temp_access_index_sommet;

    auto skip = 0;
    for (auto i = 0; i < cellule.totpoly; ++i) {
        auto nombre_sommets = cellule.poly_totvert[i];

        temp_access_index_sommet.redimensionne(nombre_sommets);

        std::vector<vertex_descriptor> face_vertices;
        face_vertices.reserve(nombre_sommets);

        /* Inverse l'ordre des sommets car il semblerait que l'algorithme de calcul booléen y est
         * sensible et que les cellules ont des ordres différents de ce que nous générons. */
        for (long j = nombre_sommets - 1; j >= 0; j--) {
            face_vertices.push_back(vertices[cellule.poly_indices[skip + j + 1]]);
        }

        auto face = CGAL::Euler::add_face(face_vertices, *resultat);

        if (face == boost::graph_traits<EnrichedPolyhedron>::null_face()) {
            std::cerr << "Erreur lors de la construction de la face !\n";
        }
        skip += (nombre_sommets + 1);
    }

    return resultat;
}

static void convertis_vers_maillage(EnrichedPolyhedron &polyhedre, Maillage &maillage)
{
    const long num_verts = polyhedre.size_of_vertices();
    if (num_verts == 0) {
        return;
    }
    maillage.reserveNombreDePoints(num_verts);

    auto id_vertex = 0;
    for (auto vert_iter = polyhedre.vertices_begin(); vert_iter != polyhedre.vertices_end();
         ++vert_iter) {

        auto point = vert_iter->point();
        vert_iter->Label = id_vertex++;
        maillage.ajouteUnPoint(point.x(), point.y(), point.z());
    }

    const long num_faces = polyhedre.size_of_facets();
    if (num_faces == 0) {
        return;
    }
    maillage.reserveNombreDePolygones(num_faces);

    for (auto face_iter = polyhedre.facets_begin(); face_iter != polyhedre.facets_end();
         ++face_iter) {

        kuri::tableau<int> sommets;

        auto edge_iter = face_iter->halfedge();
        auto edge_begin = edge_iter;
        do {
            int sommet = edge_iter->vertex()->Label;
            sommets.ajoute(sommet);
            edge_iter = edge_iter->next();
        } while (edge_iter != edge_begin);

        maillage.ajouteUnPolygone(sommets.donnees(), sommets.taille());
    }
}

bool booleen_maillages(Maillage const &maillage_a,
                       Maillage const &maillage_b,
                       const std::string &operation,
                       Maillage &maillage_sortie)
{
    auto mesh_A = convertis_vers_polyhedre(maillage_a);
    auto mesh_B = convertis_vers_polyhedre(maillage_b);

    try {
        Bool_Op op;
        if (operation == "UNION") {
            op = Bool_Op::UNION;
        }
        else if (operation == "INTER") {
            op = Bool_Op::INTER;
        }
        else {
            op = Bool_Op::MINUS;
        }

        BoolPolyhedra alg(op);
        alg.run(*mesh_A, *mesh_B);

        /* Construction manuelle car CGAL ne permet de créer des polyèdres avec des arêtes
         * partagées par plus de 2 faces, et c'est plus rapide. */
        auto &builder = alg.get_builder();

        auto &vertices = builder.get_vertices();
        maillage_sortie.reserveNombreDePoints(vertices.size());
        for (auto &point : vertices) {
            maillage_sortie.ajouteUnPoint(point.x(), point.y(), point.z());
        }

        auto &triangles = builder.get_triangles();
        maillage_sortie.reserveNombreDePolygones(triangles.size());
        int sommets[3];
        for (auto &triangle : triangles) {
            sommets[0] = static_cast<int>(triangle[0]);
            sommets[1] = static_cast<int>(triangle[1]);
            sommets[2] = static_cast<int>(triangle[2]);
            maillage_sortie.ajouteUnPolygone(sommets, 3);
        }
    }
    catch (std::exception &e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    catch (...) {
        return false;
    }

    return true;
}

bool construit_maillage_pour_cellules_voronoi(Maillage const &maillage_a,
                                              dls::tableau<CelluleVoronoi> const &cellules,
                                              Maillage &maillage_sortie)
{
    auto mesh_A = convertis_vers_polyhedre(maillage_a);

    struct PointsEtTrianglesCellule {
        std::vector<EnrichedPolyhedron::Point_3> vertices;
        std::vector<std::vector<unsigned long>> triangles;
    };

    try {
        dls::tableau<PointsEtTrianglesCellule> cellules_finales;
        cellules_finales.reserve(cellules.taille());

        long nombre_de_points = 0;
        long nombre_de_triangles = 0;

        for (auto const &cellule : cellules) {
            auto mesh_B = convertis_vers_polyhedre(cellule);
            BoolPolyhedra alg(Bool_Op::INTER);
            alg.run(*mesh_A, *mesh_B);

            auto &builder = alg.get_builder();

            auto donnees = PointsEtTrianglesCellule();
            donnees.vertices = builder.get_vertices();
            donnees.triangles = builder.get_triangles();

            nombre_de_points += donnees.vertices.size();
            nombre_de_triangles += donnees.triangles.size();

            cellules_finales.ajoute(donnees);
        }

        maillage_sortie.reserveNombreDePoints(nombre_de_points);
        maillage_sortie.reserveNombreDePolygones(nombre_de_triangles);

        int decalage_triangle = 0;

        for (int i = 0; i < cellules.taille(); i++) {
            for (auto &point : cellules_finales[i].vertices) {
                maillage_sortie.ajouteUnPoint(point.x(), point.y(), point.z());
            }

            int sommets[3];
            for (auto &triangle : cellules_finales[i].triangles) {
                sommets[0] = static_cast<int>(triangle[0]) + decalage_triangle;
                sommets[1] = static_cast<int>(triangle[1]) + decalage_triangle;
                sommets[2] = static_cast<int>(triangle[2]) + decalage_triangle;
                maillage_sortie.ajouteUnPolygone(sommets, 3);
            }

            decalage_triangle += cellules_finales[i].vertices.size();
        }
    }
    catch (std::exception &e) {
        std::cerr << e.what() << "\n";
        return false;
    }
    catch (...) {
        return false;
    }

    return true;
}

void test_conversion_polyedre(Maillage const &maillage_entree, Maillage &maillage_sortie)
{
    auto polyedre = convertis_vers_polyhedre(maillage_entree);
    convertis_vers_maillage(*polyedre, maillage_sortie);
}

}  // namespace geo
