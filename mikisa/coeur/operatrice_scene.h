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
#include "bibliotheques/opengl/contexte_rendu.h"
#include "bibliotheques/opengl/pile_matrice.h"

#include "operatrice_image.h"
#include "scene.h"

static constexpr auto NOM_SCENE = "Scène";
static constexpr auto AIDE_SCENE = "Crée une scène.";

class OperatriceScene final : public OperatriceImage {
	Scene m_scene;
	ContexteRendu m_contexte;
	PileMatrice m_pile;
	Graphe m_graphe;

public:
	explicit OperatriceScene(Noeud *node);

	int type() const override;

	int type_entree(int n) const override;

	int type_sortie(int) const override;

	const char *chemin_entreface() const override;

	const char *class_name() const override;

	const char *help_text() const override;

	Scene *scene() override;

	Graphe *graphe();

	int execute(const Rectangle &rectangle, const int temps) override;
};
