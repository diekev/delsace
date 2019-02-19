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

	int execute(Rectangle const &rectangle, int temps) override
	{
		m_corps.reinitialise();
		auto corps_entree = entree(0)->requiers_corps(rectangle, temps);

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

void enregistre_operatrices_visualisation(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceVisualiationArbreOcternaire>());
}
