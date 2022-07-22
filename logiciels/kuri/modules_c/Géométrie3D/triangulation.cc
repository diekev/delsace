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

#include "triangulation.hh"

#define CGAL_EIGEN3_ENABLED

#include <CGAL/config.h>

#include <CGAL/Epick_d.h>
#include <CGAL/Triangulation.h>
#include <CGAL/algorithm.h>

#include <vector>

#include "outils.hh"
#include "triangulation_delaunay.hh"

namespace geo {

using K = CGAL::Epick_d<CGAL::Dimension_tag<3>>;
using Triangulation = CGAL::Triangulation<K>;

template <typename T>
inline bool possede(std::set<T> const &s, T v)
{
    return s.find(v) != s.end();
}

void calcule_enveloppe_convexe(const Maillage &maillage_pour, Maillage &resultat)
{
    std::vector<Triangulation::Point> points;
    points.reserve(maillage_pour.nombreDePoints() * 3);

    for (int i = 0; i < maillage_pour.nombreDePoints(); i++) {
        auto point = maillage_pour.pointPourIndex(i);
        Triangulation::Point p(point.x, point.y, point.z);
        points.push_back(p);
    }

    const int D = 3;

    Triangulation t(D);
    t.insert(points.begin(), points.end());

    if (!t.is_valid()) {
        return;
    }

    const int cur_dim = t.current_dimension();

    /* Rassemble les cellules sur l'enveloppe. */
    using Full_cell_iterator = Triangulation::Full_cell_iterator;
    std::vector<Full_cell_iterator> cellules;
    cellules.reserve(t.number_of_full_cells());

    for (auto cit = t.full_cells_begin(); cit != t.full_cells_end(); ++cit) {
        if (!t.is_infinite(cit)) {
            continue;
        }
        cellules.push_back(cit);
    }

    /* Construit la liste des points finaux. */
    std::vector<Triangulation::Vertex_handle> points_finaux;
    points_finaux.reserve(maillage_pour.nombreDePoints());

    std::map<Triangulation::Vertex_handle, int> index_des_sommets;
    std::set<Triangulation::Vertex_handle> vertex_handles;

    for (auto &cell : cellules) {
        for (int j = 0; j <= cur_dim; j++) {
            auto v = cell->vertex(j);

            /* Ignore le point infini servant de base à l'enveloppe convexe. */
            if (t.is_infinite(v)) {
                continue;
            }

            if (possede(vertex_handles, v)) {
                continue;
            }

            vertex_handles.insert(v);
            index_des_sommets[v] = points_finaux.size();
            points_finaux.push_back(v);
        }
    }

    /* Copie les points. */
    resultat.reserveNombreDePoints(points_finaux.size());
    for (auto &v : points_finaux) {
        auto p = v->point();
        resultat.ajouteUnPoint(p[0], p[1], p[2]);
    }

    /* Crée les polygones. */
    std::vector<int> sommets;
    sommets.resize(cur_dim + 1);

    resultat.reserveNombreDePolygones(cellules.size());
    for (auto &cell : cellules) {
        int n = 0;
        for (int j = 0; j <= cur_dim; j++) {
            auto v = cell->vertex(j);

            /* Ignore le point infini servant de base à l'enveloppe convexe. */
            if (t.is_infinite(v)) {
                continue;
            }

            sommets[n++] = index_des_sommets[v];
        }

        resultat.ajouteUnPolygone(sommets.data(), n);
    }
}

/* ************************************************************************** */

void triangulation_delaunay_2d_points_3d(Maillage const &points, Maillage &resultat)
{
    std::vector<double> vertices;
    vertices.reserve(points.nombreDePoints() * 2);
    for (auto i = 0; i < points.nombreDePoints(); i++) {
        auto point = points.pointPourIndex(i);
        vertices.push_back(point.x);
        vertices.push_back(point.y);
    }

    delaunator::Delaunator d(vertices);

    for (auto i = 0; i < points.nombreDePoints(); i++) {
        auto point = points.pointPourIndex(i);
        resultat.ajouteUnPoint(point);
    }

    for (std::size_t i = 0; i < d.triangles.size(); i += 3) {
        int poly[3] = {static_cast<int>(d.triangles[i]),
                       static_cast<int>(d.triangles[i + 1]),
                       static_cast<int>(d.triangles[i + 2])};
        resultat.ajouteUnPolygone(poly, 3);
    }
}

}  // namespace geo
