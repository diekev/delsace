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

#include "operatrices_visualisation.hh"

#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#include "arbre_bvh.hh"
#include "arbre_octernaire.hh"

/* ************************************************************************** */

class OperatriceVisualiationArbreOcternaire : public OperatriceCorps {
public:
	static constexpr auto NOM = "Visualiation Arbre Octernaire";
	static constexpr auto AIDE = "";

	OperatriceVisualiationArbreOcternaire(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte) override
	{
		m_corps.reinitialise();
		auto corps_entree = entree(0)->requiers_corps(contexte);

		if (corps_entree == nullptr) {
			this->ajoute_avertissement("Aucun corps n'est connecté !");
			return EXECUTION_ECHOUEE;
		}

		auto points_entree = corps_entree->points();

		if (points_entree->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto prims_entree = corps_entree->prims();

		if (prims_entree->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto min = dls::math::point3d( constantes<double>::INFINITE);
		auto max = dls::math::point3d(-constantes<double>::INFINITE);

		for (auto i = 0; i < points_entree->taille(); ++i) {
			auto point = corps_entree->transformation(dls::math::point3d(points_entree->point(i)));
			extrait_min_max(point, min, max);
		}

		auto boite = BoiteEnglobante(min, max);

		auto arbre = ArbreOcternaire(boite);

		auto triangles = convertis_maillage_triangles(corps_entree, nullptr);

		for (auto const &triangle : triangles) {
			arbre.ajoute_triangle(triangle);
		}

		rassemble_topologie(arbre.racine(), m_corps);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static auto cree_cube(
		Corps &corps,
		dls::math::vec3f const &min,
		dls::math::vec3f const &max,
		dls::math::vec3f const &couleur)
{
	dls::math::vec3f sommets[8] = {
		dls::math::vec3f(min.x, min.y, min.z),
		dls::math::vec3f(min.x, min.y, max.z),
		dls::math::vec3f(max.x, min.y, max.z),
		dls::math::vec3f(max.x, min.y, min.z),
		dls::math::vec3f(min.x, max.y, min.z),
		dls::math::vec3f(min.x, max.y, max.z),
		dls::math::vec3f(max.x, max.y, max.z),
		dls::math::vec3f(max.x, max.y, min.z),
	};

	long cotes[12][2] = {
		{ 0, 1 },
		{ 1, 2 },
		{ 2, 3 },
		{ 3, 0 },
		{ 0, 4 },
		{ 1, 5 },
		{ 2, 6 },
		{ 3, 7 },
		{ 4, 5 },
		{ 5, 6 },
		{ 6, 7 },
		{ 7, 4 },
	};

	auto attr_C = corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);

	auto decalage = corps.points()->taille();

	for (int i = 0; i < 8; ++i) {
		corps.ajoute_point(sommets[i].x, sommets[i].y, sommets[i].z);
		attr_C->pousse_vec3(couleur);
	}

	for (int i = 0; i < 12; ++i) {
		auto poly = Polygone::construit(&corps, type_polygone::OUVERT, 2);
		poly->ajoute_sommet(decalage + cotes[i][0]);
		poly->ajoute_sommet(decalage + cotes[i][1]);
	}
}

static auto rassemble_topologie(ArbreBVH *arbre, Corps &corps)
{
	dls::math::vec3f couleurs[2] = {
		dls::math::vec3f(0.0f, 1.0f, 0.0f),
		dls::math::vec3f(0.0f, 0.0f, 1.0f),
	};

	for (auto i = 0; i < arbre->totleaf + arbre->totbranch; ++i) {
		auto noeud = arbre->nodes[static_cast<size_t>(i)];
		auto const &min = dls::math::vec3f(noeud->bv[0], noeud->bv[2], noeud->bv[4]);
		auto const &max = dls::math::vec3f(noeud->bv[1], noeud->bv[3], noeud->bv[5]);

		auto couleur = (i < arbre->totleaf) ? couleurs[0] : couleurs[1];

		cree_cube(corps, min, max, couleur);
	}
}

class OperatriceVisualiationArbreBVH : public OperatriceCorps {
public:
	static constexpr auto NOM = "Visualiation Arbre BVH";
	static constexpr auto AIDE = "";

	OperatriceVisualiationArbreBVH(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	int type_entree(int) const override
	{
		return OPERATRICE_CORPS;
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

	int execute(ContexteEvaluation const &contexte) override
	{
		m_corps.reinitialise();
		auto corps_entree = entree(0)->requiers_corps(contexte);

		if (corps_entree == nullptr) {
			this->ajoute_avertissement("Aucun corps n'est connecté !");
			return EXECUTION_ECHOUEE;
		}

		auto points_entree = corps_entree->points();

		if (points_entree->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto prims_entree = corps_entree->prims();

		if (prims_entree->taille() == 0) {
			this->ajoute_avertissement("Le Corps d'entrée est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto min = dls::math::point3d( constantes<double>::INFINITE);
		auto max = dls::math::point3d(-constantes<double>::INFINITE);

		for (auto i = 0; i < points_entree->taille(); ++i) {
			auto point = corps_entree->transformation(dls::math::point3d(points_entree->point(i)));
			extrait_min_max(point, min, max);
		}

		auto nombre_triangles = 0;

		for (auto i = 0; i < prims_entree->taille(); ++i) {
			auto prim = prims_entree->prim(i);

			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto poly = dynamic_cast<Polygone *>(prim);

			if (poly->type != type_polygone::FERME) {
				continue;
			}

			nombre_triangles += static_cast<int>(poly->nombre_sommets()) - 2;
		}

		auto const epsilon = 1e-6f * 2.0f * 10.0f;
		auto arbre = nouvelle_arbre_bvh(nombre_triangles, epsilon, 8, 8);

		for (auto i = 0; i < prims_entree->taille(); ++i) {
			auto prim = prims_entree->prim(i);

			if (prim->type_prim() != type_primitive::POLYGONE) {
				continue;
			}

			auto poly = dynamic_cast<Polygone *>(prim);

			if (poly->type != type_polygone::FERME) {
				continue;
			}

			for (auto j = 2; j < poly->nombre_sommets(); ++j) {
				auto triangle = Triangle{};
				triangle.v0 = points_entree->point(poly->index_point(0));
				triangle.v1 = points_entree->point(poly->index_point(j - 1));
				triangle.v2 = points_entree->point(poly->index_point(j));

				arbre->insert_triangle(i, triangle);
			}
		}

		arbre->balance();

		rassemble_topologie(arbre, m_corps);

		delete arbre;

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_visualisation(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceVisualiationArbreOcternaire>());
	usine.enregistre_type(cree_desc<OperatriceVisualiationArbreBVH>());
}
