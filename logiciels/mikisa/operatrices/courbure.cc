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

#include "courbure.hh"

#include "biblexternes/Patate/grenaille.h"

#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/structures/grille_particules.hh"
#include "biblinternes/structures/tableau.hh"

#include "corps/corps.h"

#include "coeur/chef_execution.hh"

using namespace Grenaille;

using Scalaire = double;
using Vecteur = Eigen::Matrix<Scalaire, 3, 1>;

/* ************************************************************************** */

struct DonneesMaillage {
	ListePoints3D const *points{};
	Attribut *normaux{};
	Attribut *courbure_min{};
	Attribut *courbure_max{};
	Attribut *direction_min{};
	Attribut *direction_max{};
	Attribut *var_geom{};
};

static auto converti_eigen(dls::math::vec3f const &v)
{
	return Vecteur(static_cast<double>(v.x), static_cast<double>(v.y), static_cast<double>(v.z));
}

static auto converti_depuis_eigen(Vecteur const &v)
{
	return dls::math::vec3f(static_cast<float>(v(0)), static_cast<float>(v(1)), static_cast<float>(v(2)));
}

/* ************************************************************************** */

/* Cette classe définie le format des données d'entrées. */
class GLSPoint {
	Vecteur m_pos{};
	Vecteur m_norm{};

public:
	enum {
		Dim = 3
	};

	using Scalar = ::Scalaire;
	using VectorType = Eigen::Matrix<Scalaire, Dim, 1>;
	using MatrixType = Eigen::Matrix<Scalaire, Dim, Dim>;

	MULTIARCH inline GLSPoint() = default;

	MULTIARCH inline GLSPoint(DonneesMaillage const &mesh, long vx)
		: m_pos(converti_eigen(mesh.points->point(vx)))
	{
		auto n = dls::math::vec3f();
		extrait(mesh.normaux->r32(vx), n);
		m_norm = converti_eigen(n);
	}

	MULTIARCH inline const VectorType &pos()    const
	{
		return m_pos;
	}

	MULTIARCH inline const VectorType &normal() const
	{
		return m_norm;
	}
};

/* ************************************************************************** */

using WeightFunc = Grenaille::DistWeightFunc<GLSPoint, Grenaille::SmoothWeightKernel<Scalaire> >;

using Fit = Basket<GLSPoint, WeightFunc, OrientedSphereFit, GLSParam,
			   OrientedSphereScaleSpaceDer, GLSDer, GLSCurvatureHelper, GLSGeomVar>;

/* À FAIRE : arbre KD. */

/* ************************************************************************** */

static auto calcul_courbure(
		ChefExecution *chef,
		DonneesMaillage &donnees_maillage,
		Scalaire radius)
{
	chef->demarre_evaluation("courbure, calcul");

	auto donnees_ret = DonneesCalculCourbure{};

	auto const r = Scalaire(2) * radius;
	auto const r2 = r * r;
	auto const nombre_sommets = donnees_maillage.points->taille();

	auto grille_points = GrillePoint::construit_avec_fonction(
				[&](long idx)
	{
		return donnees_maillage.points->point(idx);
	}, donnees_maillage.points->taille(), static_cast<float>(r));

	boucle_parallele(tbb::blocked_range<long>(0, nombre_sommets),
					 [&](tbb::blocked_range<long> const &plage)
	{
		for (auto i = plage.begin(); i < plage.end(); ++i) {
			if (chef->interrompu()) {
				break;
			}

			auto p0 = converti_eigen(donnees_maillage.points->point(i));

			Fit fit;
			fit.init(p0);
			fit.setWeightFunc(WeightFunc(r));

			auto point = donnees_maillage.points->point(i);
			auto cellules = grille_points.cellules_autour(point, static_cast<float>(r));
			auto c = GrillePoint::coord();

			for (c.z = cellules.min.z; c.z < cellules.max.z; ++c.z) {
				for (c.y = cellules.min.y; c.y < cellules.max.y; ++c.y) {
					for (c.x = cellules.min.x; c.x < cellules.max.x; ++c.x) {
						auto idx_c = grille_points.index_cellule(c);
						auto debut = grille_points.debut_points(idx_c);
						auto fin = grille_points.fin_points(idx_c);

						for (; debut != fin; ++debut) {
							if (longueur_carree(debut->pos - point) < static_cast<float>(r2)) {
								fit.addNeighbor(GLSPoint(donnees_maillage, debut->idx));
							}
						}
					}
				}
			}

			fit.finalize();

			if (fit.isReady()) {
				if (!fit.isStable()) {
					++donnees_ret.nombre_instable;
				}

				assigne(donnees_maillage.direction_max->r32(i), converti_depuis_eigen(fit.GLSk1Direction()));
				assigne(donnees_maillage.direction_min->r32(i), converti_depuis_eigen(fit.GLSk2Direction()));
				assigne(donnees_maillage.courbure_max->r32(i), static_cast<float>(fit.GLSk1()));
				assigne(donnees_maillage.courbure_min->r32(i), static_cast<float>(fit.GLSk2()));
				assigne(donnees_maillage.var_geom->r32(i), static_cast<float>(fit.geomVar()));
			}
			else {
				++donnees_ret.nombre_impossible;

				assigne(donnees_maillage.direction_max->r32(i), dls::math::vec3f(0.0f));
				assigne(donnees_maillage.direction_min->r32(i), dls::math::vec3f(0.0f));
				assigne(donnees_maillage.courbure_max->r32(i), std::numeric_limits<float>::quiet_NaN());
				assigne(donnees_maillage.courbure_min->r32(i), std::numeric_limits<float>::quiet_NaN());
				assigne(donnees_maillage.var_geom->r32(i), std::numeric_limits<float>::quiet_NaN());
			}
		}

		auto delta = static_cast<float>(plage.end() - plage.begin()) * 100.0f;
		chef->indique_progression_parallele(delta / static_cast<float>(nombre_sommets));
	});

	return donnees_ret;
}

DonneesCalculCourbure calcule_courbure(
		ChefExecution *chef,
		Corps &corps,
		double rayon)
{
	auto points_entree = corps.points_pour_lecture();

	chef->demarre_evaluation("courbure, préparation attributs");

	auto donnees_maillage = DonneesMaillage{};
	donnees_maillage.points = points_entree;
	donnees_maillage.normaux = corps.attribut("N");
	donnees_maillage.courbure_min = corps.ajoute_attribut("courbure_min", type_attribut::R32, 1, portee_attr::POINT);
	donnees_maillage.courbure_max = corps.ajoute_attribut("courbure_max", type_attribut::R32, 1, portee_attr::POINT);
	donnees_maillage.direction_min = corps.ajoute_attribut("direction_min", type_attribut::R32, 3, portee_attr::POINT);
	donnees_maillage.direction_max = corps.ajoute_attribut("direction_max", type_attribut::R32, 3, portee_attr::POINT);
	donnees_maillage.var_geom = corps.ajoute_attribut("var_geom", type_attribut::R32, 1, portee_attr::POINT);

	chef->indique_progression(100.0f);

	return calcul_courbure(chef, donnees_maillage, rayon);
}
