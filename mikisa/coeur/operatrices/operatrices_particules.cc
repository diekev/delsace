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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "operatrices_particules.h"

#include <random>

#include "../corps/corps.h"

#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceCreationPoints final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Points";
	static constexpr auto AIDE = "Crée des points.";

	explicit OperatriceCreationPoints(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_IMAGE;
	}

	int type_sortie(int) const override
	{
		return OPERATRICE_CORPS;
	}

	const char *chemin_entreface() const override
	{
		return "";
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

		auto liste_points = m_corps.points();
		liste_points->reserve(2000);

		std::uniform_real_distribution<float> dist(-10.0f, 10.0f);
		std::mt19937 rng(19937);

		for (size_t i = 0; i < 2000; ++i) {
			auto point = new Point3D();
			point->x = dist(rng);
			point->y = 0.0f;
			point->z = dist(rng);

			liste_points->pousse(point);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

struct Triangle {
	dls::math::vec3f v0{}, v1{}, v2{};
};

std::vector<Triangle> convertis_maillage_triangles(Corps const *corps_entree)
{
	std::vector<Triangle> triangles;
	auto const points = corps_entree->points();
	auto const prims  = corps_entree->prims();

	/* Convertis le maillage en triangles. */
	auto nombre_triangles = 0ul;

	for (auto prim : prims->prims()) {
		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto poly = dynamic_cast<Polygone *>(prim);

		if (poly->type != type_polygone::FERME) {
			continue;
		}

		/* Petit tableau pour comprendre le calcul du nombre de triangles.
		 * +----------------+------------------+
		 * | nombre sommets | nombre triangles |
		 * +----------------+------------------+
		 * | 3              | 1                |
		 * | 4              | 2                |
		 * | 5              | 3                |
		 * | 6              | 4                |
		 * | 7              | 5                |
		 * +----------------+------------------+
		 */
		nombre_triangles += poly->nombre_sommets() - 2;
	}

	triangles.reserve(nombre_triangles);

	for (auto prim : prims->prims()) {
		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto poly = dynamic_cast<Polygone *>(prim);

		if (poly->type != type_polygone::FERME) {
			continue;
		}

		for (size_t i = 2; i < poly->nombre_sommets(); ++i) {
			Triangle triangle;

			triangle.v0 = points->point(poly->index_point(0));
			triangle.v1 = points->point(poly->index_point(i - 1));
			triangle.v2 = points->point(poly->index_point(i));

			triangles.push_back(triangle);
		}
	}

	return triangles;
}

/* À FAIRE : transfère attribut. */
class OperatriceDispersionPoints : public OperatriceCorps {
public:
	static constexpr auto NOM = "Dispersion Points";
	static constexpr auto AIDE = "Disperse des points sur une surface.";

	OperatriceDispersionPoints(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);

		ajoute_propriete("graine", danjo::TypePropriete::ENTIER, 1);
		ajoute_propriete("nombre_points_polys", danjo::TypePropriete::ENTIER, 100);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_dispersion_points.jo";
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

		auto corps_maillage = entree(0)->requiers_corps(rectangle, temps);

		if (corps_maillage == nullptr) {
			this->ajoute_avertissement("Il n'y a pas de corps connecté !");
			return EXECUTION_ECHOUEE;
		}

		auto triangles = convertis_maillage_triangles(corps_maillage);

		if (triangles.empty()) {
			this->ajoute_avertissement("Il n'y a pas de primitives dans le corps d'entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto points_sorties = m_corps.points();

		auto const nombre_points_polys = static_cast<size_t>(evalue_entier("nombre_points_polys"));
		auto const nombre_points = triangles.size() * nombre_points_polys;

		points_sorties->reserve(nombre_points);

		auto const graine = evalue_entier("graine");

		std::mt19937 rng(static_cast<size_t>(19937 + graine));
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);

		for (const Triangle &triangle : triangles) {
			auto const v0 = triangle.v0;
			auto const v1 = triangle.v1;
			auto const v2 = triangle.v2;

			auto const e0 = v1 - v0;
			auto const e1 = v2 - v0;

			for (size_t j = 0; j < nombre_points_polys; ++j) {
				/* Génère des coordonnées barycentriques aléatoires. */
				auto r = dist(rng);
				auto s = dist(rng);

				if (r + s >= 1.0f) {
					r = 1.0f - r;
					s = 1.0f - s;
				}

				auto pos = v0 + r * e0 + s * e1;

				m_corps.ajoute_point(pos.x, pos.y, pos.z);
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_particules(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCreationPoints>());
	usine.enregistre_type(cree_desc<OperatriceDispersionPoints>());
}

#pragma clang diagnostic pop
