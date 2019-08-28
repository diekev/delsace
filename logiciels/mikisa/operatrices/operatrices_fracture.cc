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

#include "biblexternes/voro/voro++.hh"

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/tableau.hh"

#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "corps/limites_corps.hh"

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
	dls::tableau<double> verts{};
	dls::tableau<int> poly_totvert{};
	dls::tableau<int> poly_indices{};
	dls::tableau<int> voisines{};

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

		if (!valide_corps_entree(*this, corps_maillage, true, false)) {
			return EXECUTION_ECHOUEE;
		}

		auto corps_points = entree(1)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, corps_points, true, false, 1)) {
			return EXECUTION_ECHOUEE;
		}

		/* création du conteneur */
		auto limites = calcule_limites_mondiales_corps(*corps_maillage);
		auto min = dls::math::converti_type<double>(limites.min);
		auto max = dls::math::converti_type<double>(limites.max);

		/* Divise le domaine de calcul. */
		auto nombre_block = dls::math::vec3i(8);

		auto const periodic_x = evalue_bool("périodic_x");
		auto const periodic_y = evalue_bool("périodic_y");
		auto const periodic_z = evalue_bool("périodic_z");

		auto points_entree = corps_points->points_pour_lecture();

		/* À FAIRE : rayon de particules : container_poly. */
		auto cont_voro = memoire::loge<voro::container>(
					"voro::container",
					min.x, max.x, min.y, max.y, min.z, max.z,
					nombre_block.x, nombre_block.y, nombre_block.z,
					periodic_x, periodic_y, periodic_z,
					static_cast<int>(points_entree->taille()));

		auto ordre_parts = memoire::loge<voro::particle_order>("voro::particle_order");

		/* ajout des particules */

		for (auto i = 0; i < points_entree->taille(); ++i) {
			auto point = points_entree->point(i);
			auto point_monde = corps_points->transformation(dls::math::point3d(point.x, point.y, point.z));
			cont_voro->put(*ordre_parts, i, point_monde.x, point_monde.y, point_monde.z);
		}

		/* création des cellules */
		auto cellules = dls::tableau<cell>();
		cellules.reserve(points_entree->taille());

		/* calcul des cellules */

		container_compute_cells(cont_voro, ordre_parts, cellules);

		/* conversion des données */
		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::PRIMITIVE);
		auto gna = GNA();

		auto poly_index_offset = 0;

		for (auto &c : cellules) {
			for (auto i = 0; i < c.totvert; ++i) {
				auto px = static_cast<float>(c.verts[i * 3]);
				auto py = static_cast<float>(c.verts[i * 3 + 1]);
				auto pz = static_cast<float>(c.verts[i * 3 + 2]);
				m_corps.ajoute_point(px, py, pz);
			}

			auto couleur = gna.uniforme_vec3(0.0f, 1.0f);
			auto skip = 0;

			for (auto i = 0; i < c.totpoly; ++i) {
				auto nombre_verts = c.poly_totvert[i];

				auto poly = Polygone::construit(&m_corps, type_polygone::FERME, nombre_verts);

				attr_C->pousse(couleur);
				for (auto j = 0; j < nombre_verts; ++j) {
					poly->ajoute_sommet(poly_index_offset + c.poly_indices[skip + 1 + j]);
				}

				skip += (nombre_verts + 1);
			}

			poly_index_offset += c.totvert;
		}

		memoire::deloge("voro::container", cont_voro);
		memoire::deloge("voro::particle_order", ordre_parts);

		return EXECUTION_REUSSIE;
	}

	void container_compute_cells(voro::container* cn, voro::particle_order* po, dls::tableau<cell> &cells)
	{
		voro::voronoicell_neighbor vc;
		voro::c_loop_order vl(*cn, *po);

		if (!vl.start()) {
			return;
		}
		cell c;

		do {
			if (!cn->compute_cell(vc, vl)) {
				/* cellule invalide */
				continue;
			}

			/* adapté de voro++ */
			double *pp = vl.p[vl.ijk] + vl.ps * vl.q;

			/* index de la particule de la cellule */
			c.index = cn->id[vl.ijk][vl.q];

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

			cells.pousse(c);
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
