/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include "creation.h"

#include "biblinternes/math/vecteur.hh"
#include "biblinternes/outils/constantes.h"

#include "booleen/boolops_cpolyhedron_builder.hpp"  // pour Point3d
#include "booleen_maillage.hh"
#include "outils.hh"
#include <CGAL/boost/graph/generators.h>

namespace geo {

/* ************************************************************************** */

void cree_boite(Maillage &maillage,
                const float taille_x,
                const float taille_y,
                const float taille_z,
                const float centre_x,
                const float centre_y,
                const float centre_z)
{
#if 0
	/* À FAIRE: expose this to the UI */
	auto const &x_div = 2;
	auto const &y_div = 2;
	auto const &z_div = 2;

	auto const size = dimension * uniform_scale;

	auto const &start_x = -(size.x / 2.0f) + center.x;
	auto const &start_y = -(size.y / 2.0f) + center.y;
	auto const &start_z = -(size.z / 2.0f) + center.z;

	auto const &x_increment = size.x / (x_div - 1);
	auto const &y_increment = size.y / (y_div - 1);
	auto const &z_increment = size.z / (z_div - 1);

	for (auto x = 0; x < x_div; ++x) {
		vec[0] = start_x + x * x_increment;

		for (auto y = 0; y < y_div; ++y) {
			vec[1] = start_y + y * y_increment;

			for (auto z = 0; z < z_div; ++z) {
				vec[2] = start_z + z * z_increment;

				points->push_back(vec);
			}
		}
	}
#endif

    dls::math::vec3f min(-taille_x + centre_x, -taille_y + centre_y, -taille_z + centre_z);
    dls::math::vec3f max(taille_x + centre_x, taille_y + centre_y, taille_z + centre_z);

    const dls::math::vec3f sommets[8] = {dls::math::vec3f(min[0], min[1], min[2]),
                                         dls::math::vec3f(max[0], min[1], min[2]),
                                         dls::math::vec3f(max[0], max[1], min[2]),
                                         dls::math::vec3f(min[0], max[1], min[2]),
                                         dls::math::vec3f(min[0], min[1], max[2]),
                                         dls::math::vec3f(max[0], min[1], max[2]),
                                         dls::math::vec3f(max[0], max[1], max[2]),
                                         dls::math::vec3f(min[0], max[1], max[2])};

    maillage.reserveNombreDePoints(8);

    for (auto const &sommet : sommets) {
        maillage.ajouteUnPoint(sommet.x, sommet.y, sommet.z);
    }

    const dls::math::vec3f normaux[6] = {
        dls::math::vec3f(-1.0f, 0.0f, 0.0f),
        dls::math::vec3f(1.0f, 0.0f, 0.0f),
        dls::math::vec3f(0.0f, -1.0f, 0.0f),
        dls::math::vec3f(0.0f, 1.0f, 0.0f),
        dls::math::vec3f(0.0f, 0.0f, -1.0f),
        dls::math::vec3f(0.0f, 0.0f, 1.0f),
    };

    AttributVec3 attr_normaux = maillage.ajouteAttributPolygone<TypeAttributGeo3D::VEC3>("N");
    AttributVec2 attr_uvs = maillage.ajouteAttributSommetsPolygone<TypeAttributGeo3D::VEC2>("uv");

    const dls::math::vec2f uvs[4] = {
        dls::math::vec2f{0.0f, 0.0f},
        dls::math::vec2f{0.0f, 1.0f},
        dls::math::vec2f{1.0f, 1.0f},
        dls::math::vec2f{1.0f, 0.0f},
    };

    const int polygones[6][4] = {
        {0, 4, 7, 3},  // min x
        {5, 1, 2, 6},  // max x
        {1, 5, 4, 0},  // min y
        {3, 7, 6, 2},  // max y
        {1, 0, 3, 2},  // min z
        {4, 5, 6, 7},  // max z
    };

    maillage.reserveNombreDePolygones(6);

    for (int64_t i = 0; i < 6; ++i) {
        maillage.ajouteUnPolygone(polygones[i], 4);

        if (attr_normaux) {
            attr_normaux.ecris_vec3(i, normaux[i]);
        }

        if (attr_uvs) {
            for (int j = 0; j < 4; ++j) {
                attr_uvs.ecris_vec2(i * 4 + j, uvs[j]);
            }
        }
    }
}

/* ************************************************************************** */

/* utilisé pour déboguer les algorithmes de déduplications de doublons */
#undef GENERE_DOUBLONS

template <typename T>
auto sphere(const T u, const T v, const T rayon)
{
    return dls::math::vec3<T>(
        std::cos(u) * std::sin(v) * rayon, std::cos(v) * rayon, std::sin(u) * std::sin(v) * rayon);
}

void cree_sphere_uv(Maillage &maillage,
                    const float rayon,
                    int const resolution_u,
                    int const resolution_v,
                    const float centre_x,
                    const float centre_y,
                    const float centre_z)
{
    if (resolution_u == 0 || resolution_v == 0 || rayon == 0.0f) {
        return;
    }

    auto index_point = 0;
    auto attr_normaux = maillage.ajouteAttributPoint<VEC3>("N");

    /* Création d'une sphère UV à partir de son équation paramétrique.
     * U est la longitude, V la latitude. */
    auto const debut_u = 0.0f;
    auto const debut_v = 0.0f;
    auto const fin_u = constantes<float>::TAU;
    auto const fin_v = constantes<float>::PI;

    /* Taille entre deux points de longitude. */
    auto const taille_pas_u = (fin_u - debut_u) / static_cast<float>(resolution_u);

    /* Taille entre deux points de latitude. */
    auto const taille_pas_v = (fin_v - debut_v) / static_cast<float>(resolution_v);

    auto const &centre = dls::math::vec3f(centre_x, centre_y, centre_z);

#ifdef GENERE_DOUBLONS
    int poly[4] = {0, 1, 2, 3};
    const int decalage = 4;

    for (int i = 0; i < resolution_u; i++) {
        for (int j = 0; j < resolution_v; j++) {
            auto const u = static_cast<float>(i) * taille_pas_u + debut_u;
            auto const v = static_cast<float>(j) * taille_pas_v + debut_v;
            auto const un = (i + 1 == resolution_u) ?
                                fin_u :
                                static_cast<float>(i + 1) * taille_pas_u + debut_u;
            auto const vn = (j + 1 == resolution_v) ?
                                fin_v :
                                static_cast<float>(j + 1) * taille_pas_v + debut_v;

            /* Trouve les quatre points de la grille en évaluant la fonction
             * paramétrique.
             *
             * REMARQUE : pour les sphères, le normal est simplement la version
             * normalisée du sommet. Ce n'est généralement pas le cas pour
             * toutes les fonctions paramétriques.
             */

            auto const p0 = sphere(u, v, rayon) + centre;
            auto const p1 = sphere(un, v, rayon) + centre;
            auto const p2 = sphere(un, vn, rayon) + centre;
            auto const p3 = sphere(u, vn, rayon) + centre;

            maillage.ajouteUnPoint(p0.x, p0.y, p0.z);
            maillage.ajouteUnPoint(p1.x, p1.y, p1.z);
            maillage.ajouteUnPoint(p2.x, p2.y, p2.z);
            maillage.ajouteUnPoint(p3.x, p3.y, p3.z);

            if (attr_normaux) {
                auto const n0 = normalise(p0);
                auto const n1 = normalise(p1);
                auto const n2 = normalise(p2);
                auto const n3 = normalise(p3);

                attr_normaux.ecris_vec3(index_point++, n0);
                attr_normaux.ecris_vec3(index_point++, n1);
                attr_normaux.ecris_vec3(index_point++, n2);
                attr_normaux.ecris_vec3(index_point++, n3);
            }

            maillage.ajouteUnPolygone(poly, 4);

            poly[0] += decalage;
            poly[1] += decalage;
            poly[2] += decalage;
            poly[3] += decalage;
        }
    }
#else
    auto const p0 = sphere(debut_u, debut_v, rayon) + centre;
    maillage.ajouteUnPoint(p0.x, p0.y, p0.z);

    if (attr_normaux) {
        auto const n0 = normalise(p0);
        attr_normaux.ecris_vec3(index_point++, n0);
    }

    for (int i = 0; i < resolution_u; i++) {
        for (int j = 1; j < resolution_v; j++) {
            auto const u = static_cast<float>(i) * taille_pas_u + debut_u;
            auto const v = static_cast<float>(j) * taille_pas_v + debut_v;

            /* Trouve les quatre points de la grille en évaluant la fonction
             * paramétrique.
             *
             * REMARQUE : pour les sphères, le normal est simplement la version
             * normalisée du sommet. Ce n'est généralement pas le cas pour
             * toutes les fonctions paramétriques.
             */

            auto const pn = sphere(u, v, rayon) + centre;
            maillage.ajouteUnPoint(pn.x, pn.y, pn.z);

            if (attr_normaux) {
                auto const nn = normalise(pn);
                attr_normaux.ecris_vec3(index_point++, nn);
            }
        }
    }

    auto const p1 = sphere(fin_u, fin_v, rayon) + centre;
    maillage.ajouteUnPoint(p1.x, p1.y, p1.z);

    if (attr_normaux) {
        auto const n1 = normalise(p1);
        attr_normaux.ecris_vec3(index_point++, n1);
    }

    /* Ajout des polygones. */

    auto nombre_points = resolution_u * (resolution_v - 1);

    int poly[4];

    /* triangles du haut */
    poly[0] = 0;

    for (auto i = 0; i < resolution_u; ++i) {
        poly[1] = (i + 1) * (resolution_v - 1);
        poly[2] = i * (resolution_v - 1);

        poly[1] %= nombre_points;
        poly[2] %= nombre_points;

        /* Décalage de 1 pour prendre en compte le premier point. */
        poly[1] += 1;
        poly[2] += 1;

        maillage.ajouteUnPolygone(poly, 3);
    }

    /* quads du centre */
    for (int i = 0; i <= resolution_u; i++) {
        for (int j = 0; j < resolution_v - 2; j++) {
            poly[0] = i * (resolution_v - 1) + j;
            poly[1] = (i + 1) * (resolution_v - 1) + j;
            poly[2] = (i + 1) * (resolution_v - 1) + j + 1;
            poly[3] = i * (resolution_v - 1) + j + 1;

            poly[0] %= nombre_points;
            poly[1] %= nombre_points;
            poly[2] %= nombre_points;
            poly[3] %= nombre_points;

            /* Décalage de 1 pour prendre en compte le premier point. */
            poly[0] += 1;
            poly[1] += 1;
            poly[2] += 1;
            poly[3] += 1;

            maillage.ajouteUnPolygone(poly, 4);
        }
    }

    /* triangles du bas */
    poly[0] = nombre_points + 1;

    for (auto i = 0; i < resolution_u; ++i) {
        poly[1] = i * (resolution_v - 1) + resolution_v - 2;
        poly[2] = (i + 1) * (resolution_v - 1) + resolution_v - 2;

        poly[1] %= nombre_points;
        poly[2] %= nombre_points;

        /* Décalage de 1 pour prendre en compte le premier point. */
        poly[1] += 1;
        poly[2] += 1;

        maillage.ajouteUnPolygone(poly, 3);
    }
#endif
}

/* ************************************************************************** */

void cree_torus(Maillage &maillage,
                const float rayon_mineur,
                const float rayon_majeur,
                const int64_t segment_mineur,
                const int64_t segment_majeur,
                const float centre_x,
                const float centre_y,
                const float centre_z)
{
    auto const vertical_angle_stride = constantes<float>::TAU / static_cast<float>(segment_majeur);
    auto const horizontal_angle_stride = constantes<float>::TAU /
                                         static_cast<float>(segment_mineur);

    int64_t f1 = 0, f2, f3, f4;
    auto const tot_verts = segment_majeur * segment_mineur;

    maillage.reserveNombreDePoints(tot_verts);
    maillage.reserveNombreDePolygones(tot_verts);

    int poly[4];

    for (int64_t i = 0; i < segment_majeur; ++i) {
        auto theta = vertical_angle_stride * static_cast<float>(i);

        for (int64_t j = 0; j < segment_mineur; ++j) {
            auto phi = horizontal_angle_stride * static_cast<float>(j);

            auto x = std::cos(theta) * (rayon_majeur + rayon_mineur * std::cos(phi));
            auto y = rayon_mineur * std::sin(phi);
            auto z = std::sin(theta) * (rayon_majeur + rayon_mineur * std::cos(phi));

            maillage.ajouteUnPoint(x + centre_x, y + centre_y, z + centre_z);

            if (j + 1 == segment_mineur) {
                f2 = i * segment_mineur;
                f3 = f1 + segment_mineur;
                f4 = f2 + segment_mineur;
            }
            else {
                f2 = f1 + 1;
                f3 = f1 + segment_mineur;
                f4 = f3 + 1;
            }

            if (f2 >= tot_verts) {
                f2 -= tot_verts;
            }
            if (f3 >= tot_verts) {
                f3 -= tot_verts;
            }
            if (f4 >= tot_verts) {
                f4 -= tot_verts;
            }

            if (f2 > 0) {
                poly[0] = static_cast<int>(f1);
                poly[1] = static_cast<int>(f2);
                poly[2] = static_cast<int>(f4);
                poly[3] = static_cast<int>(f3);
            }
            else {
                poly[0] = static_cast<int>(f2);
                poly[1] = static_cast<int>(f4);
                poly[2] = static_cast<int>(f3);
                poly[3] = static_cast<int>(f1);
            }

            maillage.ajouteUnPolygone(poly, 4);

            ++f1;
        }
    }
}

/* ************************************************************************** */

void cree_grille(Maillage &maillage,
                 const float taille_x,
                 const float taille_y,
                 const int64_t lignes,
                 const int64_t colonnes,
                 const float centre_x,
                 const float centre_y,
                 const float centre_z)
{
    maillage.reserveNombreDePoints(lignes * colonnes);
    maillage.reserveNombreDePolygones((lignes - 1) * (colonnes - 1));
    auto attr_normaux = maillage.ajouteAttributPolygone<VEC3>("N");
    auto attr_uvs = maillage.ajouteAttributPoint<VEC2>("uv");

    float point[3] = {0.0f, centre_y, 0.0f};

    auto const &increment_x = taille_x * 2.0f / static_cast<float>(lignes - 1);
    auto const &increment_y = taille_y * 2.0f / static_cast<float>(colonnes - 1);
    auto const &debut_x = -taille_x + centre_x;
    auto const &debut_y = -taille_y + centre_z;

    auto const increment_x_uv = 1.0f / static_cast<float>(lignes - 1);
    auto const increment_y_uv = 1.0f / static_cast<float>(colonnes - 1);

    math::vec2f uv;

    auto index_point = 0;
    for (auto y = 0l; y < colonnes; ++y) {
        point[0] = debut_y + static_cast<float>(y) * increment_y;
        uv[1] = static_cast<float>(y) * increment_y_uv;

        for (auto x = 0l; x < lignes; ++x) {
            point[2] = debut_x + static_cast<float>(x) * increment_x;
            uv[0] = static_cast<float>(x) * increment_x_uv;

            maillage.ajouteUnPoint(point[0], point[1], point[2]);
            if (attr_uvs) {
                attr_uvs.ecris_vec2(index_point, uv);
            }
            index_point += 1;
        }
    }

    auto normal = math::vec3f(0.0f, 1.0f, 0.0f);

    int poly[4] = {0, 0, 0, 0};

    /* crée une copie pour le lambda */
    auto const tot_x = lignes;

    auto index = [&tot_x](int64_t x, int64_t y) { return x + y * tot_x; };

    auto index_poly = 0;

    for (auto y = 1l; y < colonnes; ++y) {
        for (auto x = 1l; x < lignes; ++x) {
            poly[0] = static_cast<int>(index(x - 1, y - 1));
            poly[1] = static_cast<int>(index(x, y - 1));
            poly[2] = static_cast<int>(index(x, y));
            poly[3] = static_cast<int>(index(x - 1, y));

            maillage.ajouteUnPolygone(poly, 4);
            if (attr_normaux) {
                attr_normaux.ecris_vec3(index_poly, normal);
            }
            index_poly += 1;
        }
    }
}

/* ************************************************************************** */

void cree_cercle(Maillage &maillage,
                 const int64_t segments,
                 const float rayon,
                 const float centre_x,
                 const float centre_y,
                 const float centre_z)
{
    auto const phid = constantes<float>::TAU / static_cast<float>(segments);
    auto phi = 0.0f;

    maillage.reserveNombreDePoints(segments + 1);
    maillage.reserveNombreDePolygones(segments);
    auto attr_normaux = maillage.ajouteAttributPolygone<VEC3>("N");

    float point[3] = {centre_x, centre_y, centre_z};

    maillage.ajouteUnPoint(point[0], point[1], point[2]);

    for (int64_t a = 0; a < segments; ++a, phi += phid) {
        /* Going this way ends up with normal(s) upward */
        point[0] = centre_x - rayon * std::sin(phi);
        point[2] = centre_z + rayon * std::cos(phi);

        maillage.ajouteUnPoint(point[0], point[1], point[2]);
    }

    auto index = segments;
    int poly[3] = {0, 0, 0};
    auto normal = math::vec3f(0.0f, 1.0f, 0.0f);

    for (auto i = 1l; i <= segments; ++i) {
        poly[1] = static_cast<int>(index);
        poly[2] = static_cast<int>(i);

        maillage.ajouteUnPolygone(poly, 3);
        if (attr_normaux) {
            attr_normaux.ecris_vec3(i - 1, normal);
        }

        index = i;
    }
}

/* ************************************************************************** */

void cree_cylindre(Maillage &maillage,
                   const int64_t segments,
                   const float rayon_mineur,
                   const float rayon_majeur,
                   const float profondeur,
                   const float centre_x,
                   const float centre_y,
                   const float centre_z)
{
    auto const phid = constantes<float>::TAU / static_cast<float>(segments);
    auto phi = 0.0f;

    maillage.reserveNombreDePoints((rayon_majeur != 0.0f) ? segments * 2 + 2 : segments + 2);

    float vec[3] = {0.0f, 0.0f, 0.0f};

    auto const cent1 = 0;
    vec[1] = -profondeur;
    maillage.ajouteUnPoint(vec[0] + centre_x, vec[1] + centre_y, vec[2] + centre_z);

    auto const cent2 = 1;
    vec[1] = profondeur;
    maillage.ajouteUnPoint(vec[0] + centre_x, vec[1] + centre_y, vec[2] + centre_z);

    auto firstv1 = 0;
    auto firstv2 = 0;
    auto lastv1 = 0;
    auto lastv2 = 0;
    auto v1 = 0;
    auto v2 = 0;

    int nombre_points = 2;
    int poly[4];

    for (auto a = 0l; a < segments; ++a, phi += phid) {
        /* Going this way ends up with normal(s) upward */
        vec[0] = -rayon_majeur * std::sin(phi);
        vec[1] = -profondeur;
        vec[2] = rayon_majeur * std::cos(phi);

        v1 = nombre_points++;
        maillage.ajouteUnPoint(vec[0] + centre_x, vec[1] + centre_y, vec[2] + centre_z);

        vec[0] = -rayon_mineur * std::sin(phi);
        vec[1] = profondeur;
        vec[2] = rayon_mineur * std::cos(phi);

        v2 = nombre_points++;
        maillage.ajouteUnPoint(vec[0] + centre_x, vec[1] + centre_y, vec[2] + centre_z);

        if (a > 0) {
            /* Poly for the bottom cap. */
            poly[0] = cent1;
            poly[1] = lastv1;
            poly[2] = v1;

            maillage.ajouteUnPolygone(poly, 3);

            /* Poly for the top cap. */
            poly[0] = cent2;
            poly[1] = v2;
            poly[2] = lastv2;

            maillage.ajouteUnPolygone(poly, 3);

            /* Poly for the side. */
            poly[0] = lastv1;
            poly[1] = lastv2;
            poly[2] = v2;
            poly[3] = v1;

            maillage.ajouteUnPolygone(poly, 4);
        }
        else {
            firstv1 = v1;
            firstv2 = v2;
        }

        lastv1 = v1;
        lastv2 = v2;
    }

    /* Poly for the bottom cap. */
    poly[0] = cent1;
    poly[1] = v1;
    poly[2] = firstv1;

    maillage.ajouteUnPolygone(poly, 3);

    /* Poly for the top cap. */
    poly[0] = cent2;
    poly[1] = firstv2;
    poly[2] = v2;

    maillage.ajouteUnPolygone(poly, 3);

    /* Poly for the side. */
    poly[0] = v1;
    poly[1] = v2;
    poly[2] = firstv2;
    poly[3] = firstv1;

    maillage.ajouteUnPolygone(poly, 4);
}

/* ************************************************************************** */

static const float icovert[12][3] = {{0.000f, 0.000f, -200.000f},
                                     {144.720f, -105.144f, -89.443f},
                                     {-55.277f, -170.128f, -89.443f},
                                     {-178.885f, 0.000f, -89.443f},
                                     {-55.277f, 170.128f, -89.443f},
                                     {144.720f, 105.144f, -89.443f},
                                     {55.277f, -170.128f, 89.443f},
                                     {-144.720f, -105.144f, 89.443f},
                                     {-144.720f, 105.144f, 89.443f},
                                     {55.277f, 170.128f, 89.443f},
                                     {178.885f, 0.000f, 89.443f},
                                     {0.000f, 0.000f, 200.000f}};

static const int icoface[20][3] = {{0, 2, 1},   {1, 5, 0},  {0, 3, 2},  {0, 4, 3},  {0, 5, 4},
                                   {1, 10, 5},  {2, 6, 1},  {3, 7, 2},  {4, 8, 3},  {5, 9, 4},
                                   {1, 6, 10},  {2, 7, 6},  {3, 8, 7},  {4, 9, 8},  {5, 10, 9},
                                   {6, 11, 10}, {7, 11, 6}, {8, 11, 7}, {9, 11, 8}, {10, 11, 9}};

void cree_icosphere(Maillage &maillage,
                    const float rayon,
                    const int subdivision,
                    const float centre_x,
                    const float centre_y,
                    const float centre_z)
{
    auto polyedre = EnrichedPolyhedron();
    using vertex_descriptor = boost::graph_traits<EnrichedPolyhedron>::vertex_descriptor;
    auto point_map = get(boost::vertex_point, polyedre);

    kuri::tableau<vertex_descriptor> vertices;
    vertices.réserve(12);

    maillage.reserveNombreDePoints(12);
    maillage.reserveNombreDePolygones(20);

    auto attr_normaux = maillage.ajouteAttributPoint<VEC3>("N");

    dls::math::vec3f point, nor;
    dls::math::vec3f centre = dls::math::vec3f(centre_x, centre_y, centre_z);

    for (int a = 0; a < 12; a++) {
        point[0] = icovert[a][0];
        point[1] = icovert[a][2];
        point[2] = icovert[a][1];

        auto vd = CGAL::add_vertex(polyedre);
        put(point_map, vd, Point3d(point.x, point.y, point.z));
        vertices.ajoute(vd);
    }

    for (auto i = 0; i < 20; ++i) {
        std::vector<vertex_descriptor> face_vertices(3);
        face_vertices[0] = vertices[icoface[i][0]];
        face_vertices[1] = vertices[icoface[i][1]];
        face_vertices[2] = vertices[icoface[i][2]];

        auto face = CGAL::Euler::add_face(face_vertices, polyedre);
        if (face == boost::graph_traits<EnrichedPolyhedron>::null_face()) {
            std::cerr << "Erreur lors de la construction de la face !\n";
        }
    }

    /* La sphère est déjà à un niveau de subdivions, donc nous évitons le dernier niveau
     * de subdivision. */
    for (int i = 0; i < subdivision - 1; i++) {
        subdivise_polyedre(polyedre);
    }

    /* Normalise tous les points. */
    for (auto vert_iter = polyedre.vertices_begin(); vert_iter != polyedre.vertices_end();
         ++vert_iter) {

        auto point_polyedre = vert_iter->point();

        auto p = dls::math::vec3f(point_polyedre.x(), point_polyedre.y(), point_polyedre.z());
        p = normalise(p) * rayon + centre;
        vert_iter->point() = Point3d(p.x, p.y, p.z);
    }

    convertis_vers_maillage(polyedre, maillage);

    if (attr_normaux) {
        auto index_vertex = 0;
        for (auto vert_iter = polyedre.vertices_begin(); vert_iter != polyedre.vertices_end();
             ++vert_iter) {

            auto point_polyedre = vert_iter->point();

            auto p = dls::math::vec3f(point_polyedre.x(), point_polyedre.y(), point_polyedre.z());
            p = normalise(p);

            attr_normaux.ecris_vec3(index_vertex++, p);
        }
    }
}

}  // namespace geo
