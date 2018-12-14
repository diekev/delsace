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

class OperatricePoint3DEntree final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Entrée";
	static constexpr auto AIDE = "";

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
		return NOM;
	}

	const char *help_text() const override
	{
		return AIDE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		compileuse.ajoute_noeud(NOEUD_POINT3D_ENTREE);
		compileuse.decalage_pile(output(0)->pointeur());
	}
};

/* ************************************************************************** */

class OperatricePoint3DValeur final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Valeur";
	static constexpr auto AIDE = "";

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
		return NOM;
	}

	const char *help_text() const override
	{
		return AIDE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		compileuse.ajoute_noeud(NOEUD_POINT3D_VALEUR);
		auto decalage = compileuse.decalage_pile(output(0)->pointeur());
		compileuse.stocke_decimal(decalage, evalue_decimal("val", temps));
	}
};

/* ************************************************************************** */

class OperatricePoint3DVecteur final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Vecteur";
	static constexpr auto AIDE = "";

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
		return NOM;
	}

	const char *help_text() const override
	{
		return AIDE;
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

class OperatricePoint3DSortie final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Sortie";
	static constexpr auto AIDE = "";

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
		return NOM;
	}

	const char *help_text() const override
	{
		return AIDE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		compileuse.ajoute_noeud(NOEUD_POINT3D_SORTIE);
		compileuse.ajoute_noeud(compileuse.decalage_pile(input(0)->pointeur()->lien));
	}
};

/* ************************************************************************** */

class OperatricePoint3DMath final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Mathématiques";
	static constexpr auto AIDE = "";

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
		return NOM;
	}

	const char *help_text() const override
	{
		return AIDE;
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

class OperatricePoint3DSepareVecteur final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Sépare Vecteur";
	static constexpr auto AIDE = "";

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
		return NOM;
	}

	const char *help_text() const override
	{
		return AIDE;
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

class OperatricePoint3DCombineVecteur final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Combine Vecteur";
	static constexpr auto AIDE = "";

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
		return NOM;
	}

	const char *help_text() const override
	{
		return AIDE;
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

class OperatricePoint3DBruitProc final : public OperatricePoint3D {
	dls::math::BruitFlux3D m_bruit_x{};
	dls::math::BruitFlux3D m_bruit_y{};
	dls::math::BruitFlux3D m_bruit_z{};

public:
	static constexpr auto NOM = "Bruit Procédurel";
	static constexpr auto AIDE = "";

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
		return NOM;
	}

	const char *help_text() const override
	{
		return AIDE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		compileuse.ajoute_noeud(NOEUD_POINT3D_BRUIT_PROC);
		compileuse.ajoute_noeud(compileuse.decalage_pile(input(0)->pointeur()->lien));

		/* prépare le bruit */
		auto const graine = static_cast<unsigned>(evalue_entier("graine"));
		m_bruit_x = dls::math::BruitFlux3D(graine);
		m_bruit_x.change_temps(evalue_decimal("temps", temps));

		auto const dimension = evalue_enum("dimension");
		auto const dur = evalue_bool("dur");
		auto const frequence = evalue_vecteur("fréquence", temps);
		auto const decalage = evalue_vecteur("décalage", temps);
		auto const octaves = evalue_entier("octaves", temps);
		auto const amplitude = evalue_decimal("amplitude", temps);
		auto const persistence = evalue_decimal("persistence", temps);
		auto const lacunarite = evalue_decimal("lacunarité", temps);

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

class OperatriceTradVec final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Traduction Vecteur";
	static constexpr auto AIDE = "Traduit les composants du vecteur d'une plage à une autre.";

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
		return NOM;
	}

	const char *help_text() const override
	{
		return AIDE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		compileuse.ajoute_noeud(NOEUD_POINT3D_TRAD_VEC);
		compileuse.ajoute_noeud(compileuse.decalage_pile(input(0)->pointeur()->lien));

		auto const vieux_min = evalue_decimal("vieux_min");
		auto const vieux_max = evalue_decimal("vieux_max");
		auto const neuf_min = evalue_decimal("neuf_min");
		auto const neuf_max = evalue_decimal("neuf_max");

		compileuse.ajoute_noeud(vieux_min, vieux_max, neuf_min, neuf_max);

		compileuse.decalage_pile(output(0)->pointeur());
	}
};

/* ************************************************************************** */

void enregistre_operatrices_point3d(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceGrapheMaillage>());
	usine.enregistre_type(cree_desc<OperatricePoint3DEntree>());
	usine.enregistre_type(cree_desc<OperatricePoint3DSortie>());
	usine.enregistre_type(cree_desc<OperatricePoint3DMath>());
	usine.enregistre_type(cree_desc<OperatricePoint3DValeur>());
	usine.enregistre_type(cree_desc<OperatricePoint3DVecteur>());
	usine.enregistre_type(cree_desc<OperatricePoint3DSepareVecteur>());
	usine.enregistre_type(cree_desc<OperatricePoint3DCombineVecteur>());
	usine.enregistre_type(cree_desc<OperatricePoint3DBruitProc>());
	usine.enregistre_type(cree_desc<OperatriceTradVec>());
}

#pragma clang diagnostic pop
