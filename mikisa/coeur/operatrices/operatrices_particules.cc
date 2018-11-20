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
#include "../corps/nuage_points.h"

#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

/* ************************************************************************** */

static constexpr auto NOM_CREATION_POINTS = "Création Points";
static constexpr auto AIDE_CREATION_POINTS = "Crée des points.";

class OperatriceCreationPoints final : public OperatriceCorps {
public:
	explicit OperatriceCreationPoints(Noeud *noeud)
		: OperatriceCorps(noeud)
	{
		inputs(1);
		outputs(1);
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

	const char *class_name() const override
	{
		return NOM_CREATION_POINTS;
	}

	const char *help_text() const override
	{
		return AIDE_CREATION_POINTS;
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

void enregistre_operatrices_particules(UsineOperatrice *usine)
{
	usine->register_type(NOM_CREATION_POINTS,
						 cree_desc<OperatriceCreationPoints>(
							 NOM_CREATION_POINTS,
							 AIDE_CREATION_POINTS));
}
