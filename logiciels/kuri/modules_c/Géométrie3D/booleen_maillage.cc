/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "booleen_maillage.hh"

#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include "biblinternes/outils/gna.hh"
#include "booleen/boolean_operations.hpp"
#include "booleen/properties_polyhedron_3.h"

#include "fracture.hh"
#include "outils.hh"

namespace geo {

static std::unique_ptr<EnrichedPolyhedron> convertis_vers_polyhedre(Maillage const &maillage)
{
    std::unique_ptr<EnrichedPolyhedron> résultat = std::make_unique<EnrichedPolyhedron>();
    auto point_map = get(boost::vertex_point, *résultat);

    /* Exporte les points. */
    using vertex_descriptor = boost::graph_traits<EnrichedPolyhedron>::vertex_descriptor;
    kuri::tableau<vertex_descriptor> vertices;
    vertices.reserve(maillage.nombreDePoints());

    for (int64_t i = 0; i < maillage.nombreDePoints(); i++) {
        auto point = maillage.pointPourIndex(i);
        auto vd = CGAL::add_vertex(*résultat);
        put(point_map, vd, Point3d(point.x, point.y, point.z));
        vertices.ajoute(vd);
    }

    /* Exporte les polygones. */
    kuri::tableau<int> temp_access_index_sommet;

    for (int64_t i = 0; i < maillage.nombreDePolygones(); i++) {
        const int64_t nombre_sommets = maillage.nombreDeSommetsPolygone(i);

        temp_access_index_sommet.redimensionne(nombre_sommets);
        maillage.indexPointsSommetsPolygone(i, temp_access_index_sommet.donnees());

        std::vector<vertex_descriptor> face_vertices;
        face_vertices.reserve(nombre_sommets);

        for (int64_t j = 0; j < nombre_sommets; j++) {
            face_vertices.push_back(vertices[temp_access_index_sommet[j]]);
        }

        auto face = CGAL::Euler::add_face(face_vertices, *résultat);

        if (face == boost::graph_traits<EnrichedPolyhedron>::null_face()) {
            std::cerr << "Erreur lors de la construction de la face !\n";
        }
    }

    return résultat;
}

static std::unique_ptr<EnrichedPolyhedron> convertis_vers_polyhedre(CelluleVoronoi const &cellule)
{
    std::unique_ptr<EnrichedPolyhedron> résultat = std::make_unique<EnrichedPolyhedron>();
    auto point_map = get(boost::vertex_point, *résultat);

    /* Exporte les points. */
    using vertex_descriptor = boost::graph_traits<EnrichedPolyhedron>::vertex_descriptor;
    kuri::tableau<vertex_descriptor> vertices;
    vertices.reserve(cellule.totvert);

    for (auto i = 0; i < cellule.totvert; ++i) {
        auto px = static_cast<float>(cellule.verts[i * 3]);
        auto py = static_cast<float>(cellule.verts[i * 3 + 1]);
        auto pz = static_cast<float>(cellule.verts[i * 3 + 2]);
        auto vd = CGAL::add_vertex(*résultat);
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
        for (int64_t j = nombre_sommets - 1; j >= 0; j--) {
            face_vertices.push_back(vertices[cellule.poly_indices[skip + j + 1]]);
        }

        auto face = CGAL::Euler::add_face(face_vertices, *résultat);

        if (face == boost::graph_traits<EnrichedPolyhedron>::null_face()) {
            std::cerr << "Erreur lors de la construction de la face !\n";
        }
        skip += (nombre_sommets + 1);
    }

    return résultat;
}

void convertis_vers_maillage(EnrichedPolyhedron &polyhedre, Maillage &maillage)
{
    const int64_t num_verts = polyhedre.size_of_vertices();
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

    const int64_t num_faces = polyhedre.size_of_facets();
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

void subdivise_polyedre(EnrichedPolyhedron &polyedre)
{
    if (!polyedre.is_pure_triangle()) {
        // À FAIRE
        return;
    }

    auto pMesh = &polyedre;

    Facet_iterator pFacet;
    using Vector3 = CGAL::Vector_3<CGAL::Cartesian<double>>;
    Vector3 Vcenter;

    // Initialization of the tags
    for (pFacet = pMesh->facets_begin(); pFacet != pMesh->facets_end(); pFacet++) {
        auto pHEcirc = pFacet->facet_begin();
        pFacet->Issub = false;
        pHEcirc->Isnew = false;
        pHEcirc->vertex()->Isnew = false;
        pHEcirc++;
        pHEcirc->Isnew = false;
        pHEcirc->vertex()->Isnew = false;
        pHEcirc++;
        pHEcirc->Isnew = false;
        pHEcirc->vertex()->Isnew = false;
    }
    // For each facet of the polyhedron
    for (pFacet = pMesh->facets_begin(); pFacet != pMesh->facets_end(); pFacet++) {
        // We subdivide the facet if it is not already done
        if (!(pFacet->Issub)) {
            Halfedge_handle pHE = pFacet->facet_begin();
            for (uint32_t i = 0; i != 5; i++) {
                if (!pHE->Isnew) {
                    // each edge is splited in its center
                    Vcenter = Vector3(0.0, 0.0, 0.0);
                    Vcenter = ((pHE->vertex()->point() - CGAL::ORIGIN) +
                               (pHE->opposite()->vertex()->point() - CGAL::ORIGIN)) /
                              2;
                    pHE = pMesh->split_edge(pHE);
                    pHE->vertex()->point() = CGAL::ORIGIN + Vcenter;
                    // update of the tags (the new vertex and the four new halfedges
                    pHE->vertex()->Isnew = true;
                    pHE->Isnew = true;
                    pHE->opposite()->Isnew = true;
                    pHE->next()->Isnew = true;
                    pHE->next()->opposite()->Isnew = true;
                }
                pHE = pHE->next();
            }
            // Three new edges are build between the three new vertices, and the tags of the facets
            // are updated
            if (!pHE->vertex()->Isnew)
                pHE = pHE->next();
            pHE = pMesh->split_facet(pHE, pHE->next()->next());
            pHE->opposite()->facet()->Issub = true;
            pHE = pMesh->split_facet(pHE, pHE->next()->next());
            pHE->opposite()->facet()->Issub = true;
            pHE = pMesh->split_facet(pHE, pHE->next()->next());
            pHE->opposite()->facet()->Issub = true;
            pHE->facet()->Issub = true;
        }
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

// Calcul du volume d'un maillage triangulé selon la méthode de
// http://chenlab.ece.cornell.edu/Publication/Cha/icip01_Cha.pdf
static float calcule_volume(std::vector<EnrichedPolyhedron::Point_3> vertices,
                            std::vector<std::vector<uint64_t>> triangles)
{
    double volume = 0.0;

    for (const auto &triangle : triangles) {
        auto p1 = vertices[triangle[0]];
        auto p2 = vertices[triangle[1]];
        auto p3 = vertices[triangle[2]];
        auto v321 = p3.x() * p2.y() * p1.z();
        auto v231 = p2.x() * p3.y() * p1.z();
        auto v312 = p3.x() * p1.y() * p2.z();
        auto v132 = p1.x() * p3.y() * p2.z();
        auto v213 = p2.x() * p1.y() * p3.z();
        auto v123 = p1.x() * p2.y() * p3.z();
        volume += (1.0 / 6.0) * (-v321 + v231 + v312 - v132 - v213 + v123);
    }

    return static_cast<float>(abs(volume));
}

static math::vec3f calcule_centroide(std::vector<EnrichedPolyhedron::Point_3> vertices,
                                     std::vector<std::vector<uint64_t>> triangles)
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    int compte = 0;

    for (const auto &triangle : triangles) {
        for (const auto index : triangle) {
            x += vertices[index].x();
            y += vertices[index].y();
            z += vertices[index].z();
            compte += 1;
        }
    }

    x /= compte;
    y /= compte;
    z /= compte;

    return math::vec3f(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
}

bool construit_maillage_pour_cellules_voronoi(Maillage const &maillage_a,
                                              dls::tableau<CelluleVoronoi> const &cellules,
                                              const ParametresFracture &params,
                                              Maillage &maillage_sortie)
{
    auto mesh_A = convertis_vers_polyhedre(maillage_a);

    struct PointsEtTrianglesCellule {
        std::vector<EnrichedPolyhedron::Point_3> vertices;
        std::vector<std::vector<uint64_t>> triangles;
    };

    try {
        dls::tableau<PointsEtTrianglesCellule> cellules_finales;
        cellules_finales.reserve(cellules.taille());

        int64_t nombre_de_points = 0;
        int64_t nombre_de_triangles = 0;

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

        auto attr_C = maillage_sortie.ajouteAttributPolygone<COULEUR>("C");
        auto gna = GNA();

        if (attr_C) {
            auto index_polygone = 0;
            for (int i = 0; i < cellules.taille(); i++) {
                auto couleur = gna.uniforme_vec3(0.0f, 1.0f);
                for (int j = 0; j < cellules_finales[i].triangles.size(); j++) {
                    attr_C.ecris_couleur(index_polygone++, math::vec4f(couleur, 1.0f));
                }
            }
        }

        if (params.cree_attribut_volume_cellule) {
            AttributReel attr_volume = maillage_sortie.ajouteAttributPolygone<R32>("volume");

            if (attr_volume) {
                auto index_polygone = 0;
                for (int i = 0; i < cellules.taille(); i++) {
                    auto volume = calcule_volume(cellules_finales[i].vertices,
                                                 cellules_finales[i].triangles);

                    for (int j = 0; j < cellules_finales[i].triangles.size(); j++) {
                        attr_volume.ecris_reel(index_polygone++, volume);
                    }
                }
            }
        }

        if (params.cree_attribut_centroide) {
            AttributVec3 attr_centroide = maillage_sortie.ajouteAttributPolygone<VEC3>(
                "centroide");

            if (attr_centroide) {
                auto index_polygone = 0;
                for (int i = 0; i < cellules.taille(); i++) {
                    auto centroide = calcule_centroide(cellules_finales[i].vertices,
                                                       cellules_finales[i].triangles);

                    for (int j = 0; j < cellules_finales[i].triangles.size(); j++) {
                        attr_centroide.ecris_vec3(index_polygone++, centroide);
                    }
                }
            }
        }

#if 0
        if (params.cree_attribut_cellule_voisine) {
            AttributEntier attr_voisine = maillage_sortie.ajouteAttributPolygone<Z32>("voisine");
            if (attr_voisine) {
                // À FAIRE : préserve l'origine des triangles pour savoir de quel polygone ils viennent
            }
        }
#endif

        std::string nom_base_groupe;
        if (params.ptr_nom_base_groupe) {
            nom_base_groupe = vers_std_string(params.ptr_nom_base_groupe,
                                              params.taille_nom_base_groupe);
        }
        else {
            nom_base_groupe = "fracture";
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
