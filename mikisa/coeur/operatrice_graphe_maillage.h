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

#include "operatrice_corps.h"

#include "bibliotheques/graphe/compileuse_graphe.h"
#include "bibliotheques/graphe/graphe.h"

/* ************************************************************************** */

enum {
	NOEUD_POINT3D_ENTREE,
	NOEUD_POINT3D_SORTIE,
	NOEUD_POINT3D_VALEUR,
	NOEUD_POINT3D_VECTEUR,
	NOEUD_POINT3D_SEPARE_VECTEUR,
	NOEUD_POINT3D_COMBINE_VECTEUR,
	NOEUD_POINT3D_MATH,
	NOEUD_POINT3D_BRUIT_PROC,
	NOEUD_POINT3D_TRAD_VEC,
};

enum {
	OPERATION_MATH_ADDITION,
	OPERATION_MATH_SOUSTRACTION,
	OPERATION_MATH_MULTIPLICATION,
	OPERATION_MATH_DIVISION,
};

/* ************************************************************************** */

namespace dls {
namespace math {
class BruitPerlin3D;
}  /* namespace math */
}  /* namespace numero7 */

class GestionnaireDonneesGraphe {
	std::vector<dls::math::BruitPerlin3D *> m_bruits;

public:
	void reinitialise();

	size_t ajoute_bruit(dls::math::BruitPerlin3D *bruit);

	dls::math::BruitPerlin3D *bruit(size_t index) const;
};

/* ************************************************************************** */

static constexpr auto NOM_GRAPHE_MAILLAGE = "Graphe Maillage";
static constexpr auto AIDE_GRAPHE_MAILLAGE = "Graphe Maillage";

class OperatriceGrapheMaillage : public OperatriceCorps {
	GestionnaireDonneesGraphe m_gestionnaire;
	CompileuseGraphe m_compileuse;
	Graphe m_graphe;

public:
	explicit OperatriceGrapheMaillage(Noeud *noeud);

	virtual const char *class_name() const;

	virtual const char *help_text() const;

	const char *chemin_entreface() const override;

	int type_entree(int) const override;

	int type_sortie(int) const override;

	Graphe *graphe();

	virtual int type() const override;

	int execute(const Rectangle &rectangle, const int temps) override;

	void compile_graphe(int temps);
};

/* ************************************************************************** */

class OperatricePoint3D : public OperatriceImage {
public:
	OperatricePoint3D(Noeud *noeud)
		: OperatriceImage(noeud)
	{}

	int execute(const Rectangle &rectangle, const int temps) override;

	virtual void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, const int temps) = 0;
};
