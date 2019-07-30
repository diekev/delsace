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

#include "biblinternes/math/bruit.hh"
#include "biblinternes/outils/definitions.h"

#include "coeur/operatrice_graphe_maillage.h"
#include "coeur/usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatricePoint3DEntree final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Entrée";
	static constexpr auto AIDE = "";

	explicit OperatricePoint3DEntree(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePoint3D(graphe_parent, noeud)
	{
		entrees(0);
		sorties(1);
	}

	int type_sortie(int) const override
	{
		return type_prise::VECTEUR;
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

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		INUTILISE(gestionnaire);
		INUTILISE(temps);
		compileuse.ajoute_noeud(NOEUD_POINT3D_ENTREE);
		compileuse.decalage_pile(sortie(0)->pointeur());
	}
};

/* ************************************************************************** */

class OperatricePoint3DValeur final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Valeur";
	static constexpr auto AIDE = "";

	explicit OperatricePoint3DValeur(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePoint3D(graphe_parent, noeud)
	{
		entrees(0);
		sorties(1);
	}

	int type_sortie(int) const override
	{
		return type_prise::DECIMAL;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_point3d_valeur.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		INUTILISE(gestionnaire);
		compileuse.ajoute_noeud(NOEUD_POINT3D_VALEUR);
		auto decalage = compileuse.decalage_pile(sortie(0)->pointeur());
		compileuse.stocke_decimal(decalage, evalue_decimal("val", temps));
	}
};

/* ************************************************************************** */

class OperatricePoint3DVecteur final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Vecteur";
	static constexpr auto AIDE = "";

	explicit OperatricePoint3DVecteur(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePoint3D(graphe_parent, noeud)
	{
		entrees(0);
		sorties(1);
	}

	int type_sortie(int) const override
	{
		return type_prise::VECTEUR;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_point3d_vecteur.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		INUTILISE(gestionnaire);
		compileuse.ajoute_noeud(NOEUD_POINT3D_VECTEUR);
		auto decalage = compileuse.decalage_pile(sortie(0)->pointeur());
		compileuse.stocke_vec3f(decalage, evalue_vecteur("vec", temps));
	}
};

/* ************************************************************************** */

class OperatricePoint3DSortie final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Sortie";
	static constexpr auto AIDE = "";

	explicit OperatricePoint3DSortie(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePoint3D(graphe_parent, noeud)
	{
		entrees(1);
		sorties(0);
	}

	int type_entree(int) const override
	{
		return type_prise::VECTEUR;
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

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		INUTILISE(gestionnaire);
		INUTILISE(temps);
		compileuse.ajoute_noeud(NOEUD_POINT3D_SORTIE);
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(0)->pointeur()->liens[0]));
	}
};

/* ************************************************************************** */

class OperatricePoint3DMath final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Mathématiques";
	static constexpr auto AIDE = "";

	explicit OperatricePoint3DMath(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePoint3D(graphe_parent, noeud)
	{
		entrees(2);
		sorties(1);
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

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		INUTILISE(gestionnaire);
		INUTILISE(temps);
		compileuse.ajoute_noeud(NOEUD_POINT3D_MATH);
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(0)->pointeur()->liens[0]));
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(1)->pointeur()->liens[0]));

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

		compileuse.decalage_pile(sortie(0)->pointeur());
	}
};

/* ************************************************************************** */

class OperatricePoint3DSepareVecteur final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Sépare Vecteur";
	static constexpr auto AIDE = "";

	explicit OperatricePoint3DSepareVecteur(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePoint3D(graphe_parent, noeud)
	{
		entrees(1);
		sorties(3);
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

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		INUTILISE(gestionnaire);
		INUTILISE(temps);
		compileuse.ajoute_noeud(NOEUD_POINT3D_SEPARE_VECTEUR);
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(0)->pointeur()->liens[0]));
		compileuse.decalage_pile(sortie(0)->pointeur());
		compileuse.decalage_pile(sortie(1)->pointeur());
		compileuse.decalage_pile(sortie(2)->pointeur());
	}
};

/* ************************************************************************** */

class OperatricePoint3DCombineVecteur final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Combine Vecteur";
	static constexpr auto AIDE = "";

	explicit OperatricePoint3DCombineVecteur(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePoint3D(graphe_parent, noeud)
	{
		entrees(3);
		sorties(1);
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

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		INUTILISE(gestionnaire);
		INUTILISE(temps);
		compileuse.ajoute_noeud(NOEUD_POINT3D_COMBINE_VECTEUR);
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(0)->pointeur()->liens[0]));
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(1)->pointeur()->liens[0]));
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(2)->pointeur()->liens[0]));
		compileuse.decalage_pile(sortie(0)->pointeur());
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

	explicit OperatricePoint3DBruitProc(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePoint3D(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
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

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		compileuse.ajoute_noeud(NOEUD_POINT3D_BRUIT_PROC);
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(0)->pointeur()->liens[0]));

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
		compileuse.ajoute_noeud(frequence);
		compileuse.ajoute_noeud(decalage);
		compileuse.ajoute_noeud(octaves);
		compileuse.ajoute_noeud(amplitude, persistence, lacunarite);

		compileuse.decalage_pile(sortie(0)->pointeur());
	}
};

/* ************************************************************************** */

class OperatriceTradVec final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Traduction Vecteur";
	static constexpr auto AIDE = "Traduit les composants du vecteur d'une plage à une autre.";

	explicit OperatriceTradVec(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePoint3D(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
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

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		INUTILISE(gestionnaire);
		INUTILISE(temps);
		compileuse.ajoute_noeud(NOEUD_POINT3D_TRAD_VEC);
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(0)->pointeur()->liens[0]));

		auto const vieux_min = evalue_decimal("vieux_min");
		auto const vieux_max = evalue_decimal("vieux_max");
		auto const neuf_min = evalue_decimal("neuf_min");
		auto const neuf_max = evalue_decimal("neuf_max");

		compileuse.ajoute_noeud(vieux_min, vieux_max, neuf_min, neuf_max);

		compileuse.decalage_pile(sortie(0)->pointeur());
	}
};

/* ************************************************************************** */

class OperatricePoint3DNormalise final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Normalise Vecteur";
	static constexpr auto AIDE = "Normalise le vecteur d'entrée.";

	explicit OperatricePoint3DNormalise(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePoint3D(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
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

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		INUTILISE(gestionnaire);
		INUTILISE(temps);
		compileuse.ajoute_noeud(NOEUD_POINT3D_NORMALISE);
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(0)->pointeur()->liens[0]));
		compileuse.decalage_pile(sortie(0)->pointeur());
	}
};

/* ************************************************************************** */

class OperatricePoint3DComplement final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Complément";
	static constexpr auto AIDE = "Retourne le complément de la valeur d'entrée 'x', à savoir 1 - x.";

	explicit OperatricePoint3DComplement(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePoint3D(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	int type_entree(int) const override
	{
		return type_prise::DECIMAL;
	}

	int type_sortie(int) const override
	{
		return type_prise::DECIMAL;
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

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		INUTILISE(gestionnaire);
		INUTILISE(temps);
		compileuse.ajoute_noeud(NOEUD_POINT3D_COMPLEMENT);
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(0)->pointeur()->liens[0]));
		compileuse.decalage_pile(sortie(0)->pointeur());
	}
};

/* ************************************************************************** */

class OperatricePoint3DEPFluide final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "EP Fluide";
	static constexpr auto AIDE = "Performe une entrepolation fluide de la valeur d'entrée.";

	explicit OperatricePoint3DEPFluide(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePoint3D(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	int type_entree(int) const override
	{
		return type_prise::DECIMAL;
	}

	int type_sortie(int) const override
	{
		return type_prise::DECIMAL;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_p3d_ep_fluide.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		INUTILISE(gestionnaire);
		INUTILISE(temps);

		auto ordre = evalue_entier("ordre");
		switch (ordre) {
			default:
			case 1:
				compileuse.ajoute_noeud(NOEUD_POINT3D_EP_FLUIDE_O1);
				break;
			case 2:
				compileuse.ajoute_noeud(NOEUD_POINT3D_EP_FLUIDE_O2);
				break;
			case 3:
				compileuse.ajoute_noeud(NOEUD_POINT3D_EP_FLUIDE_O3);
				break;
			case 4:
				compileuse.ajoute_noeud(NOEUD_POINT3D_EP_FLUIDE_O4);
				break;
			case 5:
				compileuse.ajoute_noeud(NOEUD_POINT3D_EP_FLUIDE_O5);
				break;
			case 6:
				compileuse.ajoute_noeud(NOEUD_POINT3D_EP_FLUIDE_O6);
				break;
		}

		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(0)->pointeur()->liens[0]));
		compileuse.decalage_pile(sortie(0)->pointeur());
	}
};

/* ************************************************************************** */

class OperatricePoint3DProduitScalaire final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Produit Scalaire";
	static constexpr auto AIDE = "Retourne le produit scalaire de deux vecteurs.";

	explicit OperatricePoint3DProduitScalaire(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePoint3D(graphe_parent, noeud)
	{
		entrees(2);
		sorties(1);
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

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		INUTILISE(gestionnaire);
		INUTILISE(temps);
		compileuse.ajoute_noeud(NOEUD_POINT3D_PRODUIT_SCALAIRE);
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(0)->pointeur()->liens[0]));
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(1)->pointeur()->liens[0]));
		compileuse.decalage_pile(sortie(0)->pointeur());
	}
};

/* ************************************************************************** */

class OperatricePoint3DProduitCroix final : public OperatricePoint3D {
public:
	static constexpr auto NOM = "Produit Croix";
	static constexpr auto AIDE = "Retourne le produit en croix de deux vecteurs.";

	explicit OperatricePoint3DProduitCroix(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePoint3D(graphe_parent, noeud)
	{
		entrees(2);
		sorties(1);
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

	void compile(CompileuseGraphe &compileuse, GestionnaireDonneesGraphe &gestionnaire, int temps) override
	{
		INUTILISE(gestionnaire);
		INUTILISE(temps);
		compileuse.ajoute_noeud(NOEUD_POINT3D_PRODUIT_CROIX);
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(0)->pointeur()->liens[0]));
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(1)->pointeur()->liens[0]));
		compileuse.decalage_pile(sortie(0)->pointeur());
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
	usine.enregistre_type(cree_desc<OperatricePoint3DNormalise>());
	usine.enregistre_type(cree_desc<OperatricePoint3DComplement>());
	usine.enregistre_type(cree_desc<OperatricePoint3DEPFluide>());
	usine.enregistre_type(cree_desc<OperatricePoint3DProduitScalaire>());
	usine.enregistre_type(cree_desc<OperatricePoint3DProduitCroix>());
}

#pragma clang diagnostic pop
