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

// Avant boolean_operations pour calmer les erreurs de compilations...
#include <CGAL/Simple_cartesian.h>

#include "booleen/boolean_operations.hpp"
#include "booleen/properties_polyhedron_3.h"

#include "outils.hh"

namespace geo {

static FEVV::MeshPolyhedron *convertis_vers_polyhedre(Maillage const &maillage)
{
    FEVV::MeshPolyhedron *resultat = new FEVV::MeshPolyhedron;
    auto point_map = get(boost::vertex_point, *resultat);

    /* Exporte les points. */
    using vertex_descriptor = boost::graph_traits<FEVV::MeshPolyhedron>::vertex_descriptor;
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

        if (face == boost::graph_traits<FEVV::MeshPolyhedron>::null_face()) {
            std::cerr << "Erreur lors de la construction de la face !\n";
        }
    }

    return resultat;
}

static void convertis_vers_maillage(FEVV::MeshPolyhedron *polyhedre, Maillage &maillage)
{
    const long num_verts = polyhedre->size_of_vertices();
    if (num_verts == 0) {
        return;
    }
    maillage.reserveNombreDePoints(num_verts);

    auto id_vertex = 0;
    for (auto vert_iter = polyhedre->vertices_begin(); vert_iter != polyhedre->vertices_end();
         ++vert_iter) {

        auto point = vert_iter->point();
        vert_iter->id() = id_vertex++;
        maillage.ajouteUnPoint(point.x(), point.y(), point.z());
    }

    const long num_faces = polyhedre->size_of_facets();
    if (num_faces == 0) {
        return;
    }
    maillage.reserveNombreDePolygones(num_faces);

    for (auto face_iter = polyhedre->facets_begin(); face_iter != polyhedre->facets_end();
         ++face_iter) {

        kuri::tableau<int> sommets;

        auto edge_iter = face_iter->halfedge();
        auto edge_begin = edge_iter;
        do {
            int sommet = edge_iter->vertex()->id();
            sommets.ajoute(sommet);
            edge_iter = edge_iter->next();
        } while (edge_iter != edge_begin);

        maillage.ajouteUnPolygone(sommets.donnees(), sommets.taille());
    }
}

void booleen_maillages(Maillage const &maillage_a,
                       Maillage const &maillage_b,
                       const std::string &operation,
                       Maillage &maillage_sortie)
{
    FEVV::MeshPolyhedron *mesh_A = convertis_vers_polyhedre(maillage_a);
    FEVV::MeshPolyhedron *mesh_B = convertis_vers_polyhedre(maillage_b);
    FEVV::MeshPolyhedron *output_mesh = new FEVV::MeshPolyhedron;

    if (operation == "UNION")
        FEVV::Filters::boolean_union(*mesh_A, *mesh_B, *output_mesh);
    else if (operation == "INTER")
        FEVV::Filters::boolean_inter(*mesh_A, *mesh_B, *output_mesh);
    else
        FEVV::Filters::boolean_minus(*mesh_A, *mesh_B, *output_mesh);

    convertis_vers_maillage(output_mesh, maillage_sortie);
    delete mesh_A;
    delete mesh_B;
    delete output_mesh;
}

void test_conversion_polyedre(Maillage const &maillage_entree, Maillage &maillage_sortie)
{
    FEVV::MeshPolyhedron *polyedre = convertis_vers_polyhedre(maillage_entree);
    convertis_vers_maillage(polyedre, maillage_sortie);
    delete polyedre;
}

}  // namespace geo
