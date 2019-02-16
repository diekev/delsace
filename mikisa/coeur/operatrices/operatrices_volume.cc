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

#include "operatrices_volume.hh"

#include <random>

#include "bibliotheques/outils/definitions.hh"
#include "bibliotheques/outils/parallelisme.h"

#include "../corps/volume.hh"

#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceAdvectionVolume;
class OperatriceConditionLimite;
class OperatriceAjoutFlottabilite;
class OperatriceResolutionPression;

/* ************************************************************************** */

class OperatriceCreationVolume : public OperatriceCorps {
public:
	static constexpr auto NOM = "Créer volume";
	static constexpr auto AIDE = "";

	explicit OperatriceCreationVolume(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(0);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		INUTILISE(rectangle);
		INUTILISE(temps);

		m_corps.reinitialise();

		std::mt19937 rng(19937);
		std::uniform_real_distribution dist(0.0f, 1.0f);

		auto volume = new Volume();
		auto grille_scalaire = new Grille<float>;
		grille_scalaire->initialise(32, 32, 32);

		for (size_t x = 0; x < 32; ++x) {
			for (size_t y = 0; y < 32; ++y) {
				for (size_t z = 0; z < 32; ++z) {
					grille_scalaire->valeur(x, y, z, dist(rng));
				}
			}
		}

		volume->grille = grille_scalaire;
		m_corps.prims()->pousse(volume);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

struct Triangle {
	using type_vec = dls::math::vec3f;
	type_vec v0;
	type_vec v1;
	type_vec v2;
};

struct Rayon {
	Triangle::type_vec direction;
	Triangle::type_vec origine;
};

/**
 * Algorithme de Möller-Trumbore.
 * https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_entresection_algorithm
 */
static bool entresecte_triangle(Triangle const &triangle, Rayon const &rayon, float &distance)
{
	constexpr auto epsilon = 0.000001f;

	auto const &vertex0 = triangle.v0;
	auto const &vertex1 = triangle.v1;
	auto const &vertex2 = triangle.v2;

	auto const &cote1 = vertex1 - vertex0;
	auto const &cote2 = vertex2 - vertex0;
	auto const &h = dls::math::produit_croix(rayon.direction, cote2);
	auto const angle = dls::math::produit_scalaire(cote1, h);

	if (angle > -epsilon && angle < epsilon) {
		return false;
	}

	auto const f = 1.0f / angle;
	auto const &s = Triangle::type_vec(rayon.origine) - vertex0;
	auto const angle_u = f * dls::math::produit_scalaire(s, h);

	if (angle_u < 0.0f || angle_u > 1.0f) {
		return false;
	}

	auto const q = dls::math::produit_croix(s, cote1);
	auto const angle_v = f * dls::math::produit_scalaire(rayon.direction, q);

	if (angle_v < 0.0f || angle_u + angle_v > 1.0f) {
		return false;
	}

	/* À cette étape on peut calculer t pour trouver le point d'entresection sur
	 * la ligne. */
	auto const t = f * dls::math::produit_scalaire(cote2, q);

	/* Entresection avec le rayon. */
	if (t > epsilon) {
		distance = t;
		return true;
	}

	/* Cela veut dire qu'il y a une entresection avec une ligne, mais pas avec
	 * le rayon. */
	return false;
}

static long cherche_collision(Corps const *corps_collision,
							  Rayon const &rayon_part,
							  float &dist)
{
	auto const prims_collision = corps_collision->prims();
	auto const points_collision = corps_collision->points();

	/* À FAIRE : collision particules
	 * - structure accélération
	 */
	for (auto ip = 0; ip < prims_collision->taille(); ++ip) {
		auto prim = prims_collision->prim(ip);

		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto poly = dynamic_cast<Polygone *>(prim);

		if (poly->type != type_polygone::FERME) {
			continue;
		}

		for (auto j = 2; j < poly->nombre_sommets(); ++j) {
			auto const &v0 = points_collision->point(poly->index_point(0));
			auto const &v1 = points_collision->point(poly->index_point(j - 1));
			auto const &v2 = points_collision->point(poly->index_point(j));

			auto const &v0_d = corps_collision->transformation(dls::math::point3d(v0));
			auto const &v1_d = corps_collision->transformation(dls::math::point3d(v1));
			auto const &v2_d = corps_collision->transformation(dls::math::point3d(v2));

			auto triangle = Triangle{};
			triangle.v0 = dls::math::vec3f(
						static_cast<float>(v0_d.x),
						static_cast<float>(v0_d.y),
						static_cast<float>(v0_d.z));
			triangle.v1 = dls::math::vec3f(
						static_cast<float>(v1_d.x),
						static_cast<float>(v1_d.y),
						static_cast<float>(v1_d.z));
			triangle.v2 = dls::math::vec3f(
						static_cast<float>(v2_d.x),
						static_cast<float>(v2_d.y),
						static_cast<float>(v2_d.z));

			if (entresecte_triangle(triangle, rayon_part, dist)) {
				return static_cast<long>(prim->index);
			}
		}
	}
	return -1;
}

static auto axis_dominant_v3_single(dls::math::vec3f const &vec)
{
	const float x = std::abs(vec[0]);
	const float y = std::abs(vec[1]);
	const float z = std::abs(vec[2]);

	return ((x > y) ? ((x > z) ? 0ul : 2ul) : ((y > z) ? 1ul : 2ul));
}

class OperatriceMaillageVersVolume : public OperatriceCorps {
public:
	static constexpr auto NOM = "Maillage vers Volume";
	static constexpr auto AIDE = "";

	explicit OperatriceMaillageVersVolume(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_maillage_vers_volume.jo";
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		m_corps.reinitialise();

		auto corps = entree(0)->requiers_corps(rectangle, temps);

		if (corps == nullptr) {
			ajoute_avertissement("Aucun corps connecté !");
			return EXECUTION_ECHOUEE;
		}

		auto prims = corps->prims();

		if (prims->taille() == 0) {
			ajoute_avertissement("Aucune primitive dans le corps !");
			return EXECUTION_ECHOUEE;
		}

		/* calcul boite englobante */
		auto min = dls::math::vec3f(std::numeric_limits<float>::max());
		auto max = dls::math::vec3f(std::numeric_limits<float>::min());

		auto liste_points = corps->points();

		for (auto i = 0; i < liste_points->taille(); ++i) {
			auto point = liste_points->point(i);

			for (size_t j = 0; j < 3; ++j) {
				if (point[j] < min[j]) {
					min[j] = point[j];
				}
				else if (point[j] > max[j]) {
					max[j] = point[j];
				}
			}
		}

		auto dim = max - min;

		auto const taille_voxel = evalue_decimal("taille_voxel");
		auto const densite = evalue_decimal("densité");

		auto dim_d = corps->transformation(dls::math::point3d(dim));
		auto res_x = static_cast<size_t>(dim_d.x / static_cast<double>(taille_voxel));
		auto res_y = static_cast<size_t>(dim_d.y / static_cast<double>(taille_voxel));
		auto res_z = static_cast<size_t>(dim_d.z / static_cast<double>(taille_voxel));

		auto volume = new Volume();
		auto grille_scalaire = new Grille<float>;
		grille_scalaire->initialise(res_x, res_y, res_z);
		grille_scalaire->min = min;
		grille_scalaire->max = max;
		grille_scalaire->dim = dim;

		boucle_parallele(tbb::blocked_range<size_t>(0, res_x),
						 [&](tbb::blocked_range<size_t> const &plage)
		{
			for (size_t x = plage.begin(); x < plage.end(); ++x) {
				for (size_t y = 0; y < res_y; ++y) {
					for (size_t z = 0; z < res_z; ++z) {
						/* coordonnées objet */
						auto const x_obj = static_cast<float>(x) / static_cast<float>(res_x);
						auto const y_obj = static_cast<float>(y) / static_cast<float>(res_y);
						auto const z_obj = static_cast<float>(z) / static_cast<float>(res_z);

						/* coordonnées mondiales */
						auto const x_mond = x_obj * dim.x + min.x;
						auto const y_mond = y_obj * dim.y + min.y;
						auto const z_mond = z_obj * dim.z + min.z;

						auto rayon = Rayon{};
						rayon.origine = dls::math::vec3f(x_mond, y_mond, z_mond);

						auto axis = axis_dominant_v3_single(rayon.origine);

						rayon.direction = dls::math::vec3f(0.0f, 0.0f, 0.0f);
						rayon.direction[axis] = 1.0f;

						auto distance = dim.x * 0.5f;

						auto index_prim = cherche_collision(corps, rayon, distance);

						if (index_prim < 0) {
							continue;
						}

						auto prim_coll = corps->prims()->prim(index_prim);

						/* calcul normal au niveau de la prim */
						auto poly = dynamic_cast<Polygone *>(prim_coll);
						auto const &v0 = liste_points->point(poly->index_point(0));
						auto const &v1 = liste_points->point(poly->index_point(1));
						auto const &v2 = liste_points->point(poly->index_point(2));

						auto const e1 = v1 - v0;
						auto const e2 = v2 - v0;
						auto nor_poly = normalise(produit_croix(e1, e2));

						//grille_scalaire->valeur(x, y, z, dist(rng));
						if (produit_scalaire(nor_poly, rayon.direction) < 0.0f) {
							continue;
						}

						rayon.direction = -rayon.direction;
						distance = dim.x * 0.5f;

						if (cherche_collision(corps, rayon, distance) != -1) {
							grille_scalaire->valeur(x, y, z, densite);
						}
					}
				}
			}
		});

		volume->grille = grille_scalaire;
		m_corps.prims()->pousse(volume);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_volume(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCreationVolume>());
	usine.enregistre_type(cree_desc<OperatriceMaillageVersVolume>());
}

#pragma clang diagnostic pop
