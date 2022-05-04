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

#include "booleen2/boolean_operations.hpp"
#include "booleen2/properties_polyhedron_3.h"

#include "outils.hh"

namespace geo {

static FEVV::MeshPolyhedron *convertis_vers_polyhedre(Maillage const &maillage)
{
    FEVV::MeshPolyhedron *resultat = new FEVV::MeshPolyhedron;
    auto point_map = get(boost::vertex_point, *resultat);

    /* Exporte les points. */
    using vertex_descriptor = boost::graph_traits<FEVV::MeshPolyhedron>::vertex_descriptor;
    dls::tableau<vertex_descriptor> vertices;
    vertices.reserve(maillage.nombreDePoints());

    for (long i = 0; i < maillage.nombreDePoints(); i++) {
        auto vd = CGAL::add_vertex(*resultat);
        put(point_map, vd, Point3d());  // À FAIRE
        vertices.ajoute(vd);
    }

    /* Exporte les polygones. */
    dls::tableau<int> temp_access_index_sommet;

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
            // erreur ?
        }
    }

    return resultat;
}

static void convertis_vers_maillage(const FEVV::MeshPolyhedron *polyhedre, Maillage &maillage)
{
}

void bool_operation(Maillage const &maillage_a, Maillage const &maillage_b)
{
    std::string m_operation = "UNION";  // "INTER", "MINUS"

    FEVV::MeshPolyhedron *mesh_A = convertis_vers_polyhedre(maillage_a);
    FEVV::MeshPolyhedron *mesh_B = convertis_vers_polyhedre(maillage_b);
    FEVV::MeshPolyhedron *output_mesh = new FEVV::MeshPolyhedron;

    if (m_operation == "UNION")
        FEVV::Filters::boolean_union(*mesh_A, *mesh_B, *output_mesh);
    else if (m_operation == "INTER")
        FEVV::Filters::boolean_inter(*mesh_A, *mesh_B, *output_mesh);
    else
        FEVV::Filters::boolean_minus(*mesh_A, *mesh_B, *output_mesh);
}

}  // namespace geo
