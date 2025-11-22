/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "fracture.hh"

#include <memory>

#include "biblinternes/outils/gna.hh"
#include "voro/voro++.hh"

#include "booleen_maillage.hh"
#include "outils.hh"

namespace geo {

/* Necessary Voro++ data for fracture:
 * %i the particle/cell index
 *
 * %w number of vertices (totvert)
 * %P global vertex coordinates
 * v  vertex section delimiter
 *
 * %s number of faces (totpoly)
 * %a number of vertices in each face (sum is totloop)
 * %t the indices to the cell vertices, describes which vertices build each face
 * %n neighboring cell index for each face
 * f  face section delimiter
 *
 * %C the centroid of the voronoi cell
 * c  centroid section delimiter
 */

class BaseContenantParticules {
  public:
    virtual ~BaseContenantParticules() = default;
    virtual bool compute_cell(voro::voronoicell_neighbor &c, voro::c_loop_order &l) = 0;
    virtual int **id() = 0;
    virtual voro::c_loop_order loop_order(voro::particle_order &po) = 0;
};

class ContenantParticules final : public BaseContenantParticules {
    voro::container *cont = nullptr;

  public:
    EMPECHE_COPIE(ContenantParticules);

    ContenantParticules(voro::container *cont_) : cont(cont_)
    {
    }

    ~ContenantParticules()
    {
        delete cont;
    }

    bool compute_cell(voro::voronoicell_neighbor &c, voro::c_loop_order &l) override
    {
        return cont->compute_cell(c, l);
    }

    int **id() override
    {
        return cont->id;
    }

    voro::c_loop_order loop_order(voro::particle_order &po) override
    {
        return voro::c_loop_order(*cont, po);
    }
};

class ContenantParticulesAvecRayon final : public BaseContenantParticules {
    voro::container_poly *cont = nullptr;

  public:
    EMPECHE_COPIE(ContenantParticulesAvecRayon);

    ContenantParticulesAvecRayon(voro::container_poly *cont_) : cont(cont_)
    {
    }

    ~ContenantParticulesAvecRayon()
    {
        delete cont;
    }

    bool compute_cell(voro::voronoicell_neighbor &c, voro::c_loop_order &l) override
    {
        return cont->compute_cell(c, l);
    }

    int **id() override
    {
        return cont->id;
    }

    voro::c_loop_order loop_order(voro::particle_order &po) override
    {
        return voro::c_loop_order(*cont, po);
    }
};

static void container_compute_cells(std::unique_ptr<BaseContenantParticules> const &cn,
                                    voro::particle_order &po,
                                    dls::tableau<CelluleVoronoi> &cells)
{
    voro::voronoicell_neighbor vc;
    voro::c_loop_order vl = cn->loop_order(po);

    if (!vl.start()) {
        return;
    }
    CelluleVoronoi c;

    int **id_particules = cn->id();

    do {
        if (!cn->compute_cell(vc, vl)) {
            /* cellule invalide */
            continue;
        }

        /* adapté de voro++ */
        double *pp = vl.p[vl.ijk] + vl.ps * vl.q;

        /* index de la particule de la cellule */
        c.index = id_particules[vl.ijk][vl.q];

        /* sommets */
        vc.vertices(pp[0], pp[1], pp[2], c.verts);
        c.totvert = vc.p;

        /* polygones */
        c.totpoly = vc.number_of_faces();
        vc.face_orders(c.poly_totvert);
        vc.face_vertices(c.poly_indices);

        /* voisines */
        vc.neighbors(c.voisines);

        /* centroid */
        double centroid[3];
        vc.centroid(centroid[0], centroid[1], centroid[2]);
        c.centroid[0] = static_cast<float>(centroid[0] + pp[0]);
        c.centroid[1] = static_cast<float>(centroid[1] + pp[1]);
        c.centroid[2] = static_cast<float>(centroid[2] + pp[2]);

        /* volume */
        c.volume = static_cast<float>(vc.volume());

        cells.ajoute(c);
    } while (vl.inc());
}

static std::unique_ptr<BaseContenantParticules> initialise_contenant(
    const ParametresFracture &params,
    Maillage const &maillage_a_fracturer,
    Maillage const &maillage_points,
    voro::particle_order &particle_order)
{
    auto limites = maillage_a_fracturer.boiteEnglobante();
    auto min = dls::math::converti_type<double>(limites.min);
    auto max = dls::math::converti_type<double>(limites.max);

    /* Divise le domaine de calcul. */
    auto nombre_block = dls::math::vec3i(8);

    auto const periodic_x = params.periodique_x;
    auto const periodic_y = params.periodique_y;
    auto const periodic_z = params.periodique_z;

    AttributReel attr_rayon;
    bool utilise_rayon = false;
    if (params.utilise_rayon) {
        auto opt_attr = maillage_points.accedeAttributPoint<R32>(
            vers_std_string(params.ptr_nom_attribut_rayon, params.taille_nom_attribut_rayon));

        if (opt_attr.has_value()) {
            attr_rayon = opt_attr.value();

            if (attr_rayon) {
                utilise_rayon = true;
            }
        }
    }

    if (utilise_rayon) {
        auto cont_voro = new voro::container_poly(
            min.x,
            max.x,
            min.y,
            max.y,
            min.z,
            max.z,
            nombre_block.x,
            nombre_block.y,
            nombre_block.z,
            periodic_x,
            periodic_y,
            periodic_z,
            static_cast<int>(maillage_points.nombreDePoints()));

        /* ajout des particules */

        for (auto i = 0; i < maillage_points.nombreDePoints(); ++i) {
            auto point = maillage_points.pointPourIndex(i);
            auto rayon = attr_rayon.lis_reel(i);
            cont_voro->put(particle_order,
                           i,
                           static_cast<double>(point.x),
                           static_cast<double>(point.y),
                           static_cast<double>(point.z),
                           static_cast<double>(rayon));
        }

        return std::make_unique<ContenantParticulesAvecRayon>(cont_voro);
    }

    auto cont_voro = new voro::container(min.x,
                                         max.x,
                                         min.y,
                                         max.y,
                                         min.z,
                                         max.z,
                                         nombre_block.x,
                                         nombre_block.y,
                                         nombre_block.z,
                                         periodic_x,
                                         periodic_y,
                                         periodic_z,
                                         static_cast<int>(maillage_points.nombreDePoints()));

    for (auto i = 0; i < maillage_points.nombreDePoints(); ++i) {
        auto point = maillage_points.pointPourIndex(i);
        cont_voro->put(particle_order,
                       i,
                       static_cast<double>(point.x),
                       static_cast<double>(point.y),
                       static_cast<double>(point.z));
    }

    return std::make_unique<ContenantParticules>(cont_voro);
}

/* À FAIRE : booléens. */
void fracture_maillage_voronoi(const ParametresFracture &params,
                               Maillage const &maillage_a_fracturer,
                               Maillage const &maillage_points,
                               Maillage &maillage_sortie)
{
    voro::particle_order order_particules;

    auto cont_voro = initialise_contenant(
        params, maillage_a_fracturer, maillage_points, order_particules);

    /* création des cellules */
    auto cellules = dls::tableau<CelluleVoronoi>();
    cellules.reserve(maillage_points.nombreDePoints());

    /* calcul des cellules */
    container_compute_cells(cont_voro, order_particules, cellules);

#if 1
    construis_maillage_pour_cellules_voronoi(
        maillage_a_fracturer, cellules, params, maillage_sortie);
#else
    /* conversion des données */
    int64_t nombre_de_points = 0;
    int64_t nombre_de_polygones = 0;
    auto poly_index_offset = 0;
    for (auto &c : cellules) {
        nombre_de_points += c.totvert;
        nombre_de_polygones += c.totpoly;

        auto skip = 0;
        for (auto i = 0; i < c.totpoly; ++i) {
            auto nombre_verts = c.poly_totvert[i];
            for (auto j = 0; j < nombre_verts; ++j) {
                c.poly_indices[skip + 1 + j] += poly_index_offset;
            }
            skip += (nombre_verts + 1);
        }
        poly_index_offset += c.totvert;
    }

    maillage_sortie.reserveNombreDePoints(nombre_de_points);
    maillage_sortie.reserveNombreDePolygones(nombre_de_polygones);

    auto attr_C = maillage_sortie.ajouteAttributPolygone<COULEUR>("C");

    AttributReel attr_volume;
    if (params.cree_attribut_volume_cellule) {
        attr_volume = maillage_sortie.ajouteAttributPolygone<R32>("volume");
    }

    AttributVec3 attr_centroide;
    if (params.cree_attribut_cellule_voisine) {
        attr_centroide = maillage_sortie.ajouteAttributPolygone<VEC3>("centroide");
    }

    AttributEntier attr_voisine;
    if (params.cree_attribut_cellule_voisine) {
        attr_voisine = maillage_sortie.ajouteAttributPolygone<Z32>("voisine");
    }

    std::string nom_base_groupe;
    if (params.ptr_nom_base_groupe) {
        nom_base_groupe = vers_std_string(params.ptr_nom_base_groupe,
                                          params.taille_nom_base_groupe);
    }
    else {
        nom_base_groupe = "fracture";
    }
    auto gna = GNA();

    auto index_polygone = 0;
    auto index_cellule = 0;
    for (auto &c : cellules) {
        for (auto i = 0; i < c.totvert; ++i) {
            auto px = static_cast<float>(c.verts[i * 3]);
            auto py = static_cast<float>(c.verts[i * 3 + 1]);
            auto pz = static_cast<float>(c.verts[i * 3 + 2]);
            maillage_sortie.ajouteUnPoint(px, py, pz);
        }

        auto couleur = gna.uniforme_vec3(0.0f, 1.0f);
        auto skip = 0;

        void *groupe = nullptr;
        if (params.genere_groupes) {
            auto nom_groupe = nom_base_groupe + std::to_string(index_cellule);
            groupe = maillage_sortie.creeUnGroupeDePolygones(nom_groupe);
        }

        for (auto i = 0; i < c.totpoly; ++i) {
            auto nombre_verts = c.poly_totvert[i];
            maillage_sortie.ajouteUnPolygone(&c.poly_indices[skip + 1], nombre_verts);

            if (attr_C) {
                attr_C.ecris_couleur(index_polygone, math::vec4f(couleur, 1.0f));
            }

            if (groupe) {
                maillage_sortie.ajouteAuGroupe(groupe, index_polygone);
            }

            if (params.cree_attribut_cellule_voisine && attr_voisine) {
                attr_voisine.ecris_entier(index_polygone, c.voisines[i]);
            }

            if (params.cree_attribut_centroide && attr_centroide) {
                attr_centroide.ecris_vec3(
                    index_polygone, math::vec3f(c.centroid[0], c.centroid[1], c.centroid[2]));
            }

            if (params.cree_attribut_volume_cellule && attr_volume) {
                attr_volume.ecris_reel(index_polygone, c.volume);
            }

            skip += (nombre_verts + 1);
            index_polygone += 1;
        }

        index_cellule += 1;
    }
#endif
}

}  // namespace geo
