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

#include "operatrices_point3d.h"

#include <delsace/math/bruit.hh>

#include "../operatrice_graphe_maillage.h"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static constexpr auto NOM_ENTREE = "Entrée";
static constexpr auto AIDE_ENTREE = "";

class OperatricePoint3DEntree final : public OperatricePoint3D {
public:
	explicit OperatricePoint3DEntree(Noeud *noeud)
		: OperatricePoint3D(noeud)
	{
		inputs(0);
		outputs(1);
	}

	int type_sortie(int) const override
	{
		return type_prise::VECTEUR;
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *class_name() const override
	{
		return NOM_ENTREE;
	}

	const char *help_text() const override
	{
		return AIDE_ENTREE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		compileuse.ajoute_noeud(NOEUD_POINT3D_ENTREE);
		compileuse.decalage_pile(output(0)->pointeur());
	}
};

/* ************************************************************************** */

static constexpr auto NOM_VALEUR = "Valeur";
static constexpr auto AIDE_VALEUR = "";

class OperatricePoint3DValeur final : public OperatricePoint3D {
public:
	explicit OperatricePoint3DValeur(Noeud *noeud)
		: OperatricePoint3D(noeud)
	{
		inputs(0);
		outputs(1);
	}

	int type_sortie(int) const override
	{
		return type_prise::DECIMAL;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_point3d_valeur.jo";
	}

	const char *class_name() const override
	{
		return NOM_VALEUR;
	}

	const char *help_text() const override
	{
		return AIDE_VALEUR;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		compileuse.ajoute_noeud(NOEUD_POINT3D_VALEUR);
		auto decalage = compileuse.decalage_pile(output(0)->pointeur());
		compileuse.stocke_decimal(decalage, evalue_decimal("val", temps));
	}
};

/* ************************************************************************** */

static constexpr auto NOM_VECTEUR = "Vecteur";
static constexpr auto AIDE_VECTEUR = "";

class OperatricePoint3DVecteur final : public OperatricePoint3D {
public:
	explicit OperatricePoint3DVecteur(Noeud *noeud)
		: OperatricePoint3D(noeud)
	{
		inputs(0);
		outputs(1);
	}

	int type_sortie(int) const override
	{
		return type_prise::VECTEUR;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_point3d_vecteur.jo";
	}

	const char *class_name() const override
	{
		return NOM_VECTEUR;
	}

	const char *help_text() const override
	{
		return AIDE_VECTEUR;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		compileuse.ajoute_noeud(NOEUD_POINT3D_VECTEUR);
		auto decalage = compileuse.decalage_pile(output(0)->pointeur());
		auto v = evalue_vecteur("vec", temps);
		compileuse.stocke_vec3f(decalage, dls::math::vec3f(v.x, v.y, v.z));
	}
};

/* ************************************************************************** */

static constexpr auto NOM_SORTIE = "Sortie";
static constexpr auto AIDE_SORTIE = "";

class OperatricePoint3DSortie final : public OperatricePoint3D {
public:
	explicit OperatricePoint3DSortie(Noeud *noeud)
		: OperatricePoint3D(noeud)
	{
		inputs(1);
		outputs(0);
	}

	int type_entree(int) const override
	{
		return type_prise::VECTEUR;
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *class_name() const override
	{
		return NOM_SORTIE;
	}

	const char *help_text() const override
	{
		return AIDE_SORTIE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		compileuse.ajoute_noeud(NOEUD_POINT3D_SORTIE);
		compileuse.ajoute_noeud(compileuse.decalage_pile(input(0)->pointeur()->lien));
	}
};

/* ************************************************************************** */

static constexpr auto NOM_MATH = "Mathématiques";
static constexpr auto AIDE_MATH = "";

class OperatricePoint3DMath final : public OperatricePoint3D {
public:
	explicit OperatricePoint3DMath(Noeud *noeud)
		: OperatricePoint3D(noeud)
	{
		inputs(2);
		outputs(1);
	}

	int type_entree(int) const override
	{
		return type_prise::VECTEUR;
	}

	int type_sortie(int) const override
	{
		return type_prise::VECTEUR;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_point3d_math.jo";
	}

	const char *class_name() const override
	{
		return NOM_MATH;
	}

	const char *help_text() const override
	{
		return AIDE_MATH;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		compileuse.ajoute_noeud(NOEUD_POINT3D_MATH);
		compileuse.ajoute_noeud(compileuse.decalage_pile(input(0)->pointeur()->lien));
		compileuse.ajoute_noeud(compileuse.decalage_pile(input(1)->pointeur()->lien));

		auto operation = evalue_enum("opération");

		if (operation == "addition") {
			compileuse.ajoute_noeud(OPERATION_MATH_ADDITION);
		}
		else if (operation == "soustraction") {
			compileuse.ajoute_noeud(OPERATION_MATH_SOUSTRACTION);
		}
		else if (operation == "multiplication") {
			compileuse.ajoute_noeud(OPERATION_MATH_MULTIPLICATION);
		}
		else if (operation == "division") {
			compileuse.ajoute_noeud(OPERATION_MATH_DIVISION);
		}

		compileuse.decalage_pile(output(0)->pointeur());
	}
};

/* ************************************************************************** */

static constexpr auto NOM_SEPARE_VECTEUR = "Sépare Vecteur";
static constexpr auto AIDE_SEPARE_VECTEUR = "";

class OperatricePoint3DSepareVecteur final : public OperatricePoint3D {
public:
	explicit OperatricePoint3DSepareVecteur(Noeud *noeud)
		: OperatricePoint3D(noeud)
	{
		inputs(1);
		outputs(3);
	}

	int type_entree(int) const override
	{
		return type_prise::VECTEUR;
	}

	int type_sortie(int) const override
	{
		return type_prise::DECIMAL;
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *class_name() const override
	{
		return NOM_SEPARE_VECTEUR;
	}

	const char *help_text() const override
	{
		return AIDE_SEPARE_VECTEUR;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		compileuse.ajoute_noeud(NOEUD_POINT3D_SEPARE_VECTEUR);
		compileuse.ajoute_noeud(compileuse.decalage_pile(input(0)->pointeur()->lien));
		compileuse.decalage_pile(output(0)->pointeur());
		compileuse.decalage_pile(output(1)->pointeur());
		compileuse.decalage_pile(output(2)->pointeur());
	}
};

/* ************************************************************************** */

static constexpr auto NOM_COMBINE_VECTEUR = "Combine Vecteur";
static constexpr auto AIDE_COMBINE_VECTEUR = "";

class OperatricePoint3DCombineVecteur final : public OperatricePoint3D {
public:
	explicit OperatricePoint3DCombineVecteur(Noeud *noeud)
		: OperatricePoint3D(noeud)
	{
		inputs(3);
		outputs(1);
	}

	int type_entree(int) const override
	{
		return type_prise::DECIMAL;
	}

	int type_sortie(int) const override
	{
		return type_prise::VECTEUR;
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *class_name() const override
	{
		return NOM_COMBINE_VECTEUR;
	}

	const char *help_text() const override
	{
		return AIDE_COMBINE_VECTEUR;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		compileuse.ajoute_noeud(NOEUD_POINT3D_COMBINE_VECTEUR);
		compileuse.ajoute_noeud(compileuse.decalage_pile(input(0)->pointeur()->lien));
		compileuse.ajoute_noeud(compileuse.decalage_pile(input(1)->pointeur()->lien));
		compileuse.ajoute_noeud(compileuse.decalage_pile(input(2)->pointeur()->lien));
		compileuse.decalage_pile(output(0)->pointeur());
	}
};

/* ************************************************************************** */

static constexpr auto NOM_BRUIT_PROC = "Bruit Procédurel";
static constexpr auto AIDE_BRUIT_PROC = "";

class OperatricePoint3DBruitProc final : public OperatricePoint3D {
	dls::math::BruitFlux3D m_bruit_x{};
	dls::math::BruitFlux3D m_bruit_y{};
	dls::math::BruitFlux3D m_bruit_z{};

public:
	explicit OperatricePoint3DBruitProc(Noeud *noeud)
		: OperatricePoint3D(noeud)
	{
		inputs(1);
		outputs(1);
	}

	int type_entree(int) const override
	{
		return type_prise::VECTEUR;
	}

	int type_sortie(int) const override
	{
		return type_prise::VECTEUR;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_point3d_nuage.jo";
	}

	const char *class_name() const override
	{
		return NOM_BRUIT_PROC;
	}

	const char *help_text() const override
	{
		return AIDE_BRUIT_PROC;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		compileuse.ajoute_noeud(NOEUD_POINT3D_BRUIT_PROC);
		compileuse.ajoute_noeud(compileuse.decalage_pile(input(0)->pointeur()->lien));

		/* prépare le bruit */
		const auto graine = static_cast<unsigned>(evalue_entier("graine"));
		m_bruit_x = dls::math::BruitFlux3D(graine);
		m_bruit_x.change_temps(evalue_decimal("temps", temps));

		const auto dimension = evalue_enum("dimension");
		const auto dur = evalue_bool("dur");
		const auto frequence = evalue_vecteur("fréquence", temps);
		const auto decalage = evalue_vecteur("décalage", temps);
		const auto octaves = evalue_entier("octaves", temps);
		const auto amplitude = evalue_decimal("amplitude", temps);
		const auto persistence = evalue_decimal("persistence", temps);
		const auto lacunarite = evalue_decimal("lacunarité", temps);

		if (dimension == "1D") {
			compileuse.ajoute_noeud(1ul);
			compileuse.ajoute_noeud(gestionnaire.ajoute_bruit(&m_bruit_x));
		}
		else {
			compileuse.ajoute_noeud(3ul);

			m_bruit_y = dls::math::BruitFlux3D(graine + 1);
			m_bruit_y.change_temps(evalue_decimal("temps", temps));

			m_bruit_z = dls::math::BruitFlux3D(graine + 2);
			m_bruit_z.change_temps(evalue_decimal("temps", temps));

			compileuse.ajoute_noeud(gestionnaire.ajoute_bruit(&m_bruit_x),
									gestionnaire.ajoute_bruit(&m_bruit_y),
									gestionnaire.ajoute_bruit(&m_bruit_z));
		}

		compileuse.ajoute_noeud(static_cast<float>(dur));
		compileuse.ajoute_noeud(frequence.x, frequence.y, frequence.z);
		compileuse.ajoute_noeud(decalage.x, decalage.y, decalage.z);
		compileuse.ajoute_noeud(octaves);
		compileuse.ajoute_noeud(amplitude, persistence, lacunarite);

		compileuse.decalage_pile(output(0)->pointeur());
	}
};

/* ************************************************************************** */

static constexpr auto NOM_TRAD_VEC = "Traduction Vecteur";
static constexpr auto AIDE_TRAD_VEC = "Traduit les composants du vecteur d'une plage à une autre.";

class OperatriceTradVec final : public OperatricePoint3D {
public:
	explicit OperatriceTradVec(Noeud *node)
		: OperatricePoint3D(node)
	{
		inputs(1);
		outputs(1);
	}

	int type_entree(int) const override
	{
		return type_prise::VECTEUR;
	}

	int type_sortie(int) const override
	{
		return type_prise::VECTEUR;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_point3d_traduction.jo";
	}

	const char *class_name() const override
	{
		return NOM_TRAD_VEC;
	}

	const char *help_text() const override
	{
		return AIDE_TRAD_VEC;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		compileuse.ajoute_noeud(NOEUD_POINT3D_TRAD_VEC);
		compileuse.ajoute_noeud(compileuse.decalage_pile(input(0)->pointeur()->lien));

		const auto vieux_min = evalue_decimal("vieux_min");
		const auto vieux_max = evalue_decimal("vieux_max");
		const auto neuf_min = evalue_decimal("neuf_min");
		const auto neuf_max = evalue_decimal("neuf_max");

		compileuse.ajoute_noeud(vieux_min, vieux_max, neuf_min, neuf_max);

		compileuse.decalage_pile(output(0)->pointeur());
	}
};

/* ************************************************************************** */

void enregistre_operatrices_point3d(UsineOperatrice &usine)
{
	usine.register_type(NOM_GRAPHE_MAILLAGE,
						 cree_desc<OperatriceGrapheMaillage>(
							 NOM_GRAPHE_MAILLAGE, AIDE_GRAPHE_MAILLAGE));

	usine.register_type(NOM_ENTREE,
						 cree_desc<OperatricePoint3DEntree>(
							 NOM_ENTREE, AIDE_ENTREE));

	usine.register_type(NOM_SORTIE,
						 cree_desc<OperatricePoint3DSortie>(
							 NOM_SORTIE, AIDE_SORTIE));

	usine.register_type(NOM_MATH,
						 cree_desc<OperatricePoint3DMath>(
							 NOM_MATH, AIDE_MATH));

	usine.register_type(NOM_VALEUR,
						 cree_desc<OperatricePoint3DValeur>(
							 NOM_VALEUR, AIDE_VALEUR));

	usine.register_type(NOM_VECTEUR,
						 cree_desc<OperatricePoint3DVecteur>(
							 NOM_VECTEUR, AIDE_VECTEUR));

	usine.register_type(NOM_SEPARE_VECTEUR,
						 cree_desc<OperatricePoint3DSepareVecteur>(
							 NOM_SEPARE_VECTEUR, AIDE_SEPARE_VECTEUR));

	usine.register_type(NOM_COMBINE_VECTEUR,
						 cree_desc<OperatricePoint3DCombineVecteur>(
							 NOM_COMBINE_VECTEUR, AIDE_COMBINE_VECTEUR));

	usine.register_type(NOM_BRUIT_PROC,
						 cree_desc<OperatricePoint3DBruitProc>(
							 NOM_BRUIT_PROC, AIDE_BRUIT_PROC));

	usine.register_type(NOM_TRAD_VEC,
						 cree_desc<OperatriceTradVec>(
							 NOM_TRAD_VEC, AIDE_TRAD_VEC));
}

#pragma clang diagnostic pop
