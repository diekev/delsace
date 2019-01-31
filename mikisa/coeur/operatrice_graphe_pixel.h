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

#include "operatrice_image.h"

#include "bibliotheques/graphe/compileuse_graphe.h"
#include "bibliotheques/graphe/graphe.h"

/* ************************************************************************** */

class OperatriceGraphePixel : public OperatriceImage {
	CompileuseGraphe m_compileuse{};
	Graphe m_graphe{};

public:
    static constexpr auto NOM = "Graphe";
    static constexpr auto AIDE = "Ajoute un graphe travaillant sur les pixels de l'image de manière individuelle";

	explicit OperatriceGraphePixel(Graphe &graphe_parent, Noeud *node);

	virtual const char *class_name() const override;

	virtual const char *help_text() const override;

	const char *chemin_entreface() const override;

	Graphe *graphe();

	virtual int type() const override;

	int execute(const Rectangle &rectangle, const int temps) override;

	void compile_graphe(int temps);
};
