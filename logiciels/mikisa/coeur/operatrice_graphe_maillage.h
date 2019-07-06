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

#include "biblinternes/graphe/compileuse_graphe.h"
#include "biblinternes/graphe/graphe.h"

#include "bibloc/tableau.hh"

/* ************************************************************************** */

enum : size_t {
	NOEUD_POINT3D_ENTREE,
	NOEUD_POINT3D_SORTIE,
	NOEUD_POINT3D_VALEUR,
	NOEUD_POINT3D_VECTEUR,
	NOEUD_POINT3D_SEPARE_VECTEUR,
	NOEUD_POINT3D_COMBINE_VECTEUR,
	NOEUD_POINT3D_MATH,
	NOEUD_POINT3D_BRUIT_PROC,
	NOEUD_POINT3D_TRAD_VEC,
	NOEUD_POINT3D_NORMALISE,
	NOEUD_POINT3D_COMPLEMENT,
	NOEUD_POINT3D_EP_FLUIDE_O1,
	NOEUD_POINT3D_EP_FLUIDE_O2,
	NOEUD_POINT3D_EP_FLUIDE_O3,
	NOEUD_POINT3D_EP_FLUIDE_O4,
	NOEUD_POINT3D_EP_FLUIDE_O5,
	NOEUD_POINT3D_EP_FLUIDE_O6,
	NOEUD_POINT3D_PRODUIT_SCALAIRE,
	NOEUD_POINT3D_PRODUIT_CROIX,
};

enum : size_t {
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
}  /* namespace dls */

class GestionnaireDonneesGraphe {
	dls::tableau<dls::math::BruitPerlin3D *> m_bruits{};

public:
	void reinitialise();

	size_t ajoute_bruit(dls::math::BruitPerlin3D *bruit);

	dls::math::BruitPerlin3D *bruit(size_t index) const;
};

/* ************************************************************************** */

class OperatriceGrapheMaillage : public OperatriceCorps {
	GestionnaireDonneesGraphe m_gestionnaire{};
	CompileuseGraphe m_compileuse{};
	Graphe m_graphe;

public:
	static constexpr auto NOM = "Graphe Maillage";
	static constexpr auto AIDE = "Graphe Maillage";

	explicit OperatriceGrapheMaillage(Graphe &graphe_parent, Noeud *noeud);

	virtual const char *nom_classe() const override;

	virtual const char *texte_aide() const override;

	const char *chemin_entreface() const override;

	int type_entree(int) const override;

	int type_sortie(int) const override;

	Graphe *graphe();

	virtual int type() const override;

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override;

	void compile_graphe(int temps);
};

/* ************************************************************************** */

class OperatricePoint3D : public OperatriceImage {
public:
	OperatricePoint3D(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override;

	virtual void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, const int temps) = 0;
};
