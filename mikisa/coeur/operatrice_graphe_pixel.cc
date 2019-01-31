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

#include "operatrice_graphe_pixel.h"

#include "bibliotheques/outils/definitions.hh"
#include "bibliotheques/outils/parallelisme.h"

#include "operatrice_pixel.h"

#include "noeud_image.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static constexpr auto NOM_ENTREE = "Entrée";
static constexpr auto AIDE_ENTREE = "Applique une saturation à l'image.";

class OperatricePixelEntree final : public OperatricePixel {
public:
	explicit OperatricePixelEntree(Graphe &graphe_parent, Noeud *node)
		: OperatricePixel(graphe_parent, node)
	{
		entrees(0);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM_ENTREE;
	}

	const char *texte_aide() const override
	{
		return AIDE_ENTREE;
	}

	void evalue_entrees(int temps) override
	{
		INUTILISE(temps);
	}

	numero7::image::Pixel<float> evalue_pixel(const numero7::image::Pixel<float> &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
		return pixel;
	}

	void compile(CompileuseGraphe &compileuse, int temps) override
	{
		INUTILISE(temps);
		compileuse.ajoute_noeud(NOEUD_ENTREE);
		compileuse.decalage_pile(sortie(0)->pointeur());
	}
};

static constexpr auto NOM_SORTIE = "Sortie";
static constexpr auto AIDE_SORTIE = "Applique une saturation à l'image.";

class OperatricePixelSortie final : public OperatricePixel {
public:
	explicit OperatricePixelSortie(Graphe &graphe_parent, Noeud *node)
		: OperatricePixel(graphe_parent, node)
	{
		entrees(1);
		sorties(0);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM_SORTIE;
	}

	const char *texte_aide() const override
	{
		return AIDE_SORTIE;
	}

	void evalue_entrees(int temps) override
	{
		INUTILISE(temps);
	}

	numero7::image::Pixel<float> evalue_pixel(const numero7::image::Pixel<float> &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
		return pixel;
	}

	void compile(CompileuseGraphe &compileuse, int temps) override
	{
		INUTILISE(temps);
		compileuse.ajoute_noeud(NOEUD_SORTIE);
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(0)->pointeur()->lien));
	}
};

static constexpr auto NOM_SATURATION = "SATURATION";
static constexpr auto AIDE_SATURATION = "Applique une saturation à l'image.";

class OperatricePixelSaturation final : public OperatricePixel {
public:
	explicit OperatricePixelSaturation(Graphe &graphe_parent, Noeud *node)
		: OperatricePixel(graphe_parent, node)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "";
	}

	const char *nom_classe() const override
	{
		return NOM_SATURATION;
	}

	const char *texte_aide() const override
	{
		return AIDE_SATURATION;
	}

	void evalue_entrees(int temps) override
	{
		INUTILISE(temps);
	}

	numero7::image::Pixel<float> evalue_pixel(const numero7::image::Pixel<float> &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
		return pixel;
	}

	void compile(CompileuseGraphe &compileuse, int temps) override
	{
		INUTILISE(temps);
		compileuse.ajoute_noeud(NOEUD_SATURATION);
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(0)->pointeur()->lien));

		/* saturation */
		compileuse.ajoute_noeud(0.5f);

		/* type */
		compileuse.ajoute_noeud(1);

		compileuse.decalage_pile(sortie(0)->pointeur());
	}
};

/* ************************************************************************** */

void execute_graphe(
		CompileuseGraphe::iterateur debut,
		CompileuseGraphe::iterateur fin,
		const couleur32 &entree,
		couleur32 &sortie)
{
	auto courant = debut;

	while (courant != fin) {
		auto operation = pile_charge_entier(courant);

		if (operation == -1) {
			break;
		}

		switch (operation) {
			case NOEUD_ENTREE:
			{
				pile_stocke_couleur(courant, entree);
				break;
			}
			case NOEUD_CONSTANTE:
			{
				/* la couleur constante est déjà chargée */
				break;
			}
			case NOEUD_SATURATION:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				couleur32 c = pile_charge_couleur(pointeur);

				auto saturation = pile_charge_decimal(courant);
				auto type = pile_charge_entier(courant);

				auto s = luminance(c);
				couleur32 r(s, s, s, c.a);

				if ((saturation != 0.0f) && (type == 1)) {
					r = (1.0f - saturation) * r + c * saturation;
				}

				pile_stocke_couleur(courant, r);
				break;
			}
			case NOEUD_SORTIE:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				sortie = pile_charge_couleur(pointeur);
				break;
			}
		}
	}
}

/* ************************************************************************** */

OperatriceGraphePixel::OperatriceGraphePixel(Graphe &graphe_parent, Noeud *node)
	: OperatriceImage(graphe_parent, node)
{
	entrees(1);
	sorties(1);

	auto noeud_entree = new Noeud(supprime_operatrice_image);
	auto op_entree = new OperatricePixelEntree(m_graphe, noeud_entree);
	noeud_entree->nom(op_entree->nom_classe());

	synchronise_donnees_operatrice(noeud_entree);

	m_graphe.ajoute(noeud_entree);

	auto noeud_sortie = new Noeud(supprime_operatrice_image);
	auto op_sortie = new OperatricePixelSortie(m_graphe, noeud_sortie);
	noeud_sortie->nom(op_sortie->nom_classe());

	synchronise_donnees_operatrice(noeud_sortie);

	m_graphe.ajoute(noeud_sortie);

	auto noeud_saturation = new Noeud(supprime_operatrice_image);
	auto op_saturation = new OperatricePixelSaturation(m_graphe, noeud_saturation);
	noeud_saturation->nom(op_saturation->nom_classe());

	synchronise_donnees_operatrice(noeud_saturation);

	m_graphe.ajoute(noeud_saturation);

	m_graphe.connecte(noeud_entree->sortie(0), noeud_saturation->entree(0));
	m_graphe.connecte(noeud_saturation->sortie(0), noeud_sortie->entree(0));
}

const char *OperatriceGraphePixel::nom_classe() const
{
	return NOM;
}

const char *OperatriceGraphePixel::texte_aide() const
{
	return AIDE;
}

const char *OperatriceGraphePixel::chemin_entreface() const
{
	return "entreface/operatrice_visionnage.jo";
}

Graphe *OperatriceGraphePixel::graphe()
{
	return &m_graphe;
}

int OperatriceGraphePixel::type() const
{
	return OPERATRICE_GRAPHE_PIXEL;
}

int OperatriceGraphePixel::execute(const Rectangle &rectangle, const int temps)
{
	Calque *tampon = nullptr;

	if (!entree(0)->connectee()) {
		tampon = m_image.ajoute_calque("image", rectangle);
	}
	else {
		entree(0)->requiers_image(m_image, rectangle, temps);
		auto nom_calque = evalue_chaine("nom_calque");
		tampon = m_image.calque(nom_calque);
	}

	if (tampon == nullptr) {
		ajoute_avertissement("Calque introuvable !");
		return EXECUTION_ECHOUEE;
	}

	compile_graphe(temps);

	boucle_parallele(tbb::blocked_range<size_t>(0, static_cast<size_t>(rectangle.hauteur)),
					 [&](const tbb::blocked_range<size_t> &plage)
	{
		/* fais une copie locale pour éviter les problèmes de concurrence
		 * critique */
		auto pile = m_compileuse.pile();

		for (size_t l = plage.begin(); l < plage.end(); ++l) {
			for (size_t c = 0; c < static_cast<size_t>(rectangle.largeur); ++c) {
//				auto const x = c * largeur_inverse;
//				auto const y = l * hauteur_inverse;

				auto courante = tampon->valeur(c, l);

				couleur32 entree;
				entree.r = courante.r;
				entree.v = courante.g;
				entree.b = courante.b;
				entree.a = courante.a;

				couleur32 sortie;
				execute_graphe(pile.begin(), pile.end(), entree, sortie);

				courante.r = sortie.r;
				courante.g = sortie.v;
				courante.b = sortie.b;
				courante.a = sortie.a;

				tampon->valeur(c, l, courante);
			}
		}
	});

	return EXECUTION_REUSSIE;
}

void OperatriceGraphePixel::compile_graphe(int temps)
{
	m_compileuse = CompileuseGraphe();

	if (m_graphe.besoin_ajournement) {
		tri_topologique(m_graphe);
		m_graphe.besoin_ajournement = false;
	}

	for (auto &noeud : m_graphe.noeuds()) {
		auto pointeur = noeud.get();

		for (auto &sortie : pointeur->sorties()) {
			sortie->decalage_pile = 0;
		}

		auto operatrice = std::any_cast<OperatriceImage *>(pointeur->donnees());
		auto operatrice_pixel = dynamic_cast<OperatricePixel *>(operatrice);

		if (operatrice_pixel == nullptr) {
			ajoute_avertissement("Impossible de trouver une opératrice pixel dans le graphe !");
			return;
		}

		operatrice_pixel->compile(m_compileuse, temps);
	}
}

#pragma clang diagnostic pop
