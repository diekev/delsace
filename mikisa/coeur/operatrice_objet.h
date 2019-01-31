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

#pragma once

#include "bibliotheques/graphe/graphe.h"

#include "manipulatrice.h"
#include "objet.h"
#include "operatrice_image.h"

class OperatriceObjet final : public OperatriceImage {
	vision::Camera3D *m_camera = nullptr;

	ManipulatricePosition3D m_manipulatrice_position{};
	ManipulatriceEchelle3D m_manipulatrice_echelle{};
	ManipulatriceRotation3D m_manipulatrice_rotation{};

	Objet m_objet{};
	Graphe m_graphe{};

public:
	static constexpr auto NOM = "Objet";
	static constexpr auto AIDE = "Crée un objet.";

	explicit OperatriceObjet(Graphe &graphe_parent, Noeud *noeud);

	OperatriceObjet(OperatriceObjet const &) = default;
	OperatriceObjet &operator=(OperatriceObjet const &) = default;

	int type() const override;

	int type_entree(int n) const override;

	const char *nom_entree(int n) override;

	int type_sortie(int) const override;

	const char *chemin_entreface() const override;

	const char *nom_classe() const override;

	const char *texte_aide() const override;

	Objet *objet() override;

	vision::Camera3D *camera() override;

	Graphe *graphe();

	bool possede_manipulatrice_3d(int type) const override;

	Manipulatrice3D *manipulatrice_3d(int type) override;

	void ajourne_selon_manipulatrice_3d(int type, const int temps) override;

	int execute(const Rectangle &rectangle, const int temps) override;
};
