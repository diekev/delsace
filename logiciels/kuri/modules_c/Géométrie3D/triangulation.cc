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

template <class T>
T half(T x);

template <>
float half(float x)
{
    return 0.5f * x;
}

template <>
double half(double x)
{
    return 0.5 * x;
}

class Edge {
  public:
    using VertexType = long;
    Edge() = default;

    Edge(const VertexType &p1_, const VertexType &p2_) : p1(p1_), p2(p2_), isBad(false)
    {
    }

    Edge(const Edge &e) : p1(e.p1), p2(e.p2), isBad(false)
    {
    }

    VertexType p1;
    VertexType p2;

    bool isBad = false;
};

inline bool operator==(const Edge &e1, const Edge &e2)
{
    return (e1.p1 == e2.p1 && e1.p2 == e2.p2) || (e1.p1 == e2.p2 && e1.p2 == e2.p1);
}

class Triangle {
  public:
    using EdgeType = Edge;
    using VertexType = long;

    Triangle() = default;

    Triangle(const VertexType &_p1, const VertexType &_p2, const VertexType &_p3)
        : p1(_p1), p2(_p2), p3(_p3), e1(_p1, _p2), e2(_p2, _p3), e3(_p3, _p1), isBad(false)
    {
    }

    bool containsVertex(const VertexType &v) const
    {
        return p1 == v || p2 == v || p3 == v;
    }

    VertexType p1;
    VertexType p2;
    VertexType p3;
    EdgeType e1;
    EdgeType e2;
    EdgeType e3;
    bool isBad = false;
};

template <class T>
inline bool operator==(Triangle const &t1, Triangle const &t2)
{
    return (t1.p1 == t2.p1 || t1.p1 == t2.p2 || t1.p1 == t2.p3) &&
           (t1.p2 == t2.p1 || t1.p2 == t2.p2 || t1.p2 == t2.p3) &&
           (t1.p3 == t2.p1 || t1.p3 == t2.p2 || t1.p3 == t2.p3);
}

template <typename Donnees, typename Predicat>
void supprime_si(kuri::tableau<Donnees> &donnees, Predicat predicat)
{
    auto fin = std::remove_if(donnees.debut(), donnees.fin(), predicat);
    auto nouvelle_taille = std::distance(donnees.debut(), fin);
    donnees.redimensionne(nouvelle_taille);
}

class Delaunay {
  public:
    using TriangleType = Triangle;
    using EdgeType = Edge;
    using VertexType = dls::math::vec2f;

  private:
    bool cercle_circontient(TriangleType const &triangle, VertexType const &v)
    {
        auto const &p1 = _vertices[triangle.p1];
        auto const &p2 = _vertices[triangle.p2];
        auto const &p3 = _vertices[triangle.p3];

        auto const ab = longueur_carree(p1);
        auto const cd = longueur_carree(p2);
        auto const ef = longueur_carree(p3);

        auto const circum_x = (ab * (p3.y - p2.y) + cd * (p1.y - p3.y) + ef * (p2.y - p1.y)) /
                              (p1.x * (p3.y - p2.y) + p2.x * (p1.y - p3.y) + p3.x * (p2.y - p1.y));
        auto const circum_y = (ab * (p3.x - p2.x) + cd * (p1.x - p3.x) + ef * (p2.x - p1.x)) /
                              (p1.y * (p3.x - p2.x) + p2.y * (p1.x - p3.x) + p3.y * (p2.x - p1.x));

        const VertexType circum(half(circum_x), half(circum_y));
        auto const circum_radius = longueur_carree(p1 - circum);
        auto const dist = longueur_carree(v - circum);
        return dist <= circum_radius;
    }

  public:
    const kuri::tableau<TriangleType> &triangulate(kuri::tableau<VertexType> &vertices)
    {
        // Store the vertices locally
        _vertices = vertices;

        // Determinate the super triangle
        auto minX = vertices[0].x;
        auto minY = vertices[0].y;
        auto maxX = minX;
        auto maxY = minY;

        for (auto i = 0; i < vertices.taille(); ++i) {
            if (vertices[i].x < minX)
                minX = vertices[i].x;
            if (vertices[i].y < minY)
                minY = vertices[i].y;
            if (vertices[i].x > maxX)
                maxX = vertices[i].x;
            if (vertices[i].y > maxY)
                maxY = vertices[i].y;
        }

        auto const dx = maxX - minX;
        auto const dy = maxY - minY;
        auto const deltaMax = std::max(dx, dy);
        auto const midx = half(minX + maxX);
        auto const midy = half(minY + maxY);

        const VertexType p1(midx - 20 * deltaMax, midy - deltaMax);
        const VertexType p2(midx, midy + 20 * deltaMax);
        const VertexType p3(midx + 20 * deltaMax, midy - deltaMax);

        // std::cerr << "Super triangle " << std::endl << p1 << ", " << p2 << ", " << p3 <<
        // std::endl;

        // Create a list of triangles, and add the supertriangle in it
        auto offset = _vertices.taille();
        _vertices.ajoute(p1);
        _vertices.ajoute(p2);
        _vertices.ajoute(p3);

        _triangles.ajoute(TriangleType(offset + 0, offset + 1, offset + 2));

        for (auto i = 0l; i < vertices.taille(); ++i) {
            kuri::tableau<EdgeType> polygone;

            for (auto &t : _triangles) {
                if (cercle_circontient(t, vertices[i])) {
                    t.isBad = true;
                    polygone.ajoute(t.e1);
                    polygone.ajoute(t.e2);
                    polygone.ajoute(t.e3);
                }
                else {
                    // message erreur?
                }
            }

            supprime_si(_triangles, [](TriangleType &t) { return t.isBad; });

            for (auto e1 = polygone.debut(); e1 != polygone.fin(); ++e1) {
                for (auto e2 = e1 + 1; e2 != polygone.fin(); ++e2) {
                    if (*e1 == *e2) {
                        e1->isBad = true;
                        e2->isBad = true;
                    }
                }
            }

            supprime_si(polygone, [](EdgeType &e) { return e.isBad; });

            for (const auto &e : polygone) {
                _triangles.ajoute(TriangleType(e.p1, e.p2, i));
            }
        }

        supprime_si(_triangles, [offset](TriangleType &t) {
            return t.containsVertex(offset + 0) || t.containsVertex(offset + 1) ||
                   t.containsVertex(offset + 2);
        });

        for (const auto &t : _triangles) {
            _edges.ajoute(t.e1);
            _edges.ajoute(t.e2);
            _edges.ajoute(t.e3);
        }

        // retire les trois vertices du super-triangle
        _vertices.supprime_dernier();
        _vertices.supprime_dernier();
        _vertices.supprime_dernier();

        return _triangles;
    }

    const kuri::tableau<TriangleType> &getTriangles() const
    {
        return _triangles;
    }
    const kuri::tableau<EdgeType> &getEdges() const
    {
        return _edges;
    }
    const kuri::tableau<VertexType> &getVertices() const
    {
        return _vertices;
    }

  private:
    kuri::tableau<TriangleType> _triangles{};
    kuri::tableau<EdgeType> _edges{};
    kuri::tableau<VertexType> _vertices{};
};

void triangulation_delaunay_2d_points_3d(Maillage const &points, Maillage &resultat)
{
    kuri::tableau<dls::math::vec2f> vertices;
    vertices.reserve(points.nombreDePoints());

    for (auto i = 0; i < points.nombreDePoints(); i++) {
        auto point = points.pointPourIndex(i);
        vertices.ajoute(dls::math::vec2f{point.x, point.y});
    }

    auto delaunay = Delaunay();
    delaunay.triangulate(vertices);

    for (auto i = 0; i < points.nombreDePoints(); i++) {
        auto point = points.pointPourIndex(i);
        resultat.ajouteUnPoint(point);
    }

    auto const &cotes = delaunay.getTriangles();

    for (auto const &cote : cotes) {
        int poly[3] = {
            static_cast<int>(cote.p1), static_cast<int>(cote.p2), static_cast<int>(cote.p3)};
        resultat.ajouteUnPolygone(poly, 3);
    }
}

}  // namespace geo
