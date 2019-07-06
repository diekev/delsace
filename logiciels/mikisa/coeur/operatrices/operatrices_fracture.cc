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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "operatrices_fracture.hh"

#include "bibliotheques/outils/gna.hh"
#include "bibliotheques/voro/voro++.hh"

#include "bibloc/logeuse_memoire.hh"
#include "bibloc/tableau.hh"

#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

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

struct cell {
	float (*verts)[3] = nullptr;
	int *poly_totvert = nullptr;
	int **poly_indices = nullptr;
	int *neighbors = nullptr;

	float centroid[3] = { 0.0f, 0.0f, 0.0f };
	float volume = 0.0f;
	int index = 0;
	int totvert = 0;
	int totpoly = 0;
	int pad = 0;

	cell() = default;
};

class OperatriceFractureVoronoi : public OperatriceCorps {
public:
	static constexpr auto NOM = "Fracture Voronoi";
	static constexpr auto AIDE = "";

	OperatriceFractureVoronoi(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(2);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_fracture_voronoi.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		/* À FAIRE : booléens, groupes. */
		this->ajoute_avertissement("Seuls les cubes sont supportés pour le moment !");

		m_corps.reinitialise();

		auto corps_maillage = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps_maillage == nullptr) {
			this->ajoute_avertissement("Aucun maillage connecté !");
			return EXECUTION_ECHOUEE;
		}

		auto corps_points = entree(1)->requiers_corps(contexte, donnees_aval);

		if (corps_points == nullptr) {
			this->ajoute_avertissement("Aucun points connecté !");
			return EXECUTION_ECHOUEE;
		}

		/* création du conteneur */
		auto min = dls::math::vec3d( std::numeric_limits<double>::max());
		auto max = dls::math::vec3d(-std::numeric_limits<double>::max());

		auto points_maillage = corps_maillage->points();

		for (auto i = 0; i < points_maillage->taille(); ++i) {
			auto point = points_maillage->point(i);
			auto point_monde = corps_maillage->transformation(dls::math::point3d(point.x, point.y, point.z));

			for (size_t j = 0; j < 3; ++j) {
				if (point_monde[j] < min[j]) {
					min[j] = point_monde[j];
				}
				else if (point_monde[j] > max[j]) {
					max[j] = point_monde[j];
				}
			}
		}

		/* Divise le domaine de calcul. */
		auto nombre_block = dls::math::vec3i(8);

		/* À FAIRE : 0 ou 1 sur chaque axe. */
		auto periode = dls::math::vec3i(0);

		auto points_entree = corps_points->points();

		/* À FAIRE : rayon de particules : container_poly. */
		auto cont_voro = memoire::loge<voro::container>(
					"voro::container",
					min.x, max.x, min.y, max.y, min.z, max.z,
					nombre_block.x, nombre_block.y, nombre_block.z,
					periode.x, periode.y, periode.z,
					static_cast<int>(points_entree->taille()));

		auto ordre_parts = memoire::loge<voro::particle_order>("voro::particle_order");

		/* ajout des particules */

		for (auto i = 0; i < points_entree->taille(); ++i) {
			auto point = points_entree->point(i);
			auto point_monde = corps_points->transformation(dls::math::point3d(point.x, point.y, point.z));
			cont_voro->put(*ordre_parts, i, point_monde.x, point_monde.y, point_monde.z);
		}

		/* création des cellules */
		auto cellules = dls::tableau<cell>(points_entree->taille());

		/* calcul des cellules */

		container_compute_cells(cont_voro, ordre_parts, cellules.donnees());

		/* conversion des données */
		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::PRIMITIVE);
		auto gna = GNA();

		auto poly_index_offset = 0;

		for (auto &c : cellules) {
			if (c.verts == nullptr) {
				continue;
			}

			for (size_t i = 0; i < static_cast<size_t>(c.totvert); ++i) {
				m_corps.ajoute_point(c.verts[i][0], c.verts[i][1], c.verts[i][2]);
			}

			auto couleur = gna.uniforme_vec3(0.0f, 1.0f);

			for (size_t i = 0; i < static_cast<size_t>(c.totpoly); ++i) {
				auto nombre_verts = c.poly_totvert[i];

				auto poly = Polygone::construit(&m_corps, type_polygone::FERME, nombre_verts);

				attr_C->pousse(couleur);
				for (auto j = 0; j < nombre_verts; ++j) {
					poly->ajoute_sommet(poly_index_offset + c.poly_indices[i][j]);
				}
			}

			poly_index_offset += c.totvert;
		}

		/* libération de la mémoire */

		for (auto &c : cellules) {
			if (c.verts) {
				memoire::deloge_tableau("c.verts", c.verts, c.totvert);
			}

			if (c.neighbors) {
				memoire::deloge_tableau("c.neighbors", c.neighbors, c.totpoly);
			}

			if (c.poly_indices) {
				for (size_t j = 0; j < static_cast<size_t>(c.totpoly); j++) {
					memoire::deloge_tableau("c.poly_indices", c.poly_indices[j], c.poly_totvert[j]);
				}

				memoire::deloge_tableau("c.poly_indices", c.poly_indices, c.totpoly);
			}

			if (c.poly_totvert) {
				memoire::deloge_tableau("c.poly_totvert", c.poly_totvert, c.totpoly);
			}
		}

		memoire::deloge("voro::container", cont_voro);
		memoire::deloge("voro::particle_order", ordre_parts);

		return EXECUTION_REUSSIE;
	}

	void container_compute_cells(voro::container* cn, voro::particle_order* po, cell* cells)
	{
		voro::voronoicell_neighbor vc;
		voro::c_loop_order vl(*cn, *po);

		if (!vl.start()) {
			return;
		}

		int i = 0;
		cell c;

		do {
			if (cn->compute_cell(vc,vl)) {
				// adapted from voro++
				std::vector<double> verts;
				std::vector<int> face_orders;
				std::vector<int> face_verts;
				std::vector<int> neighbors;
				double *pp, centroid[3];
				pp = vl.p[vl.ijk]+vl.ps*vl.q;

				// cell particle index
				c.index = cn->id[vl.ijk][vl.q];

				// verts
				vc.vertices(*pp, pp[1], pp[2], verts);
				c.totvert = vc.p;
				c.verts = memoire::loge_tableau<float[3]>("c.verts", c.totvert);

				for (auto v = 0; v < c.totvert; v++) {
					c.verts[v][0] = static_cast<float>(verts[static_cast<size_t>(v * 3)]);
					c.verts[v][1] = static_cast<float>(verts[static_cast<size_t>(v * 3 + 1)]);
					c.verts[v][2] = static_cast<float>(verts[static_cast<size_t>(v * 3 + 2)]);
				}

				// faces
				c.totpoly = vc.number_of_faces();
				vc.face_orders(face_orders);
				c.poly_totvert = memoire::loge_tableau<int>("c.poly_totvert", c.totpoly);

				for (auto fo = 0; fo < c.totpoly; fo++) {
					c.poly_totvert[fo] = face_orders[static_cast<size_t>(fo)];
				}

				vc.face_vertices(face_verts);
				c.poly_indices = memoire::loge_tableau<int *>("c.poly_indices", c.totpoly);

				int skip = 0;
				for (auto fo = 0; fo < c.totpoly; fo++) {
					int num_verts = c.poly_totvert[fo];
					c.poly_indices[fo] = memoire::loge_tableau<int>("c.poly_indices", num_verts);

					for (auto fv = 0; fv < num_verts; fv++) {
						c.poly_indices[fo][fv] = face_verts[static_cast<size_t>(skip + 1 + fv)];
					}

					skip += (num_verts+1);
				}

				// neighbors
				vc.neighbors(neighbors);
				c.neighbors = memoire::loge_tableau<int>("c.neighbors", c.totpoly);

				for (auto n = 0; n < c.totpoly; n++) {
					c.neighbors[n] = neighbors[static_cast<size_t>(n)];
				}

				// centroid
				vc.centroid(centroid[0], centroid[1], centroid[2]);
				c.centroid[0] = static_cast<float>(centroid[0] + pp[0]);
				c.centroid[1] = static_cast<float>(centroid[1] + pp[1]);
				c.centroid[2] = static_cast<float>(centroid[2] + pp[2]);

				// volume
				c.volume = static_cast<float>(vc.volume());

				// valid cell, store
				cells[i] = c;
			}
			else { // invalid cell, set NULL XXX TODO (Somehow !!!)
				c.centroid[0] = 0.0f;
				c.centroid[1] = 0.0f;
				c.centroid[2] = 0.0f;
				c.index = 0;
				c.neighbors = nullptr;
				c.totpoly = 0;
				c.totvert = 0;
				c.poly_totvert = nullptr;
				c.poly_indices = nullptr;
				c.verts = nullptr;
				cells[i] = c;
			}

			i++;
		}
		while(vl.inc());
	}
};

/* ************************************************************************** */

void enregistre_operatrices_fracture(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceFractureVoronoi>());
}

#pragma clang diagnostic pop
