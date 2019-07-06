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

#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/parallelisme.h"

#include "biblinternes/memoire/logeuse_memoire.hh"

#include "contexte_evaluation.hh"
#include "operatrice_pixel.h"
#include "noeud_image.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatricePixelEntree final : public OperatricePixel {
public:
	static constexpr auto NOM = "Entrée";
	static constexpr auto AIDE = "Applique une saturation à l'image.";

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
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	void evalue_entrees(int temps) override
	{
		INUTILISE(temps);
	}

	dls::image::Pixel<float> evalue_pixel(dls::image::Pixel<float> const &pixel, const float x, const float y) override
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

class OperatricePixelSortie final : public OperatricePixel {
public:
	static constexpr auto NOM = "Sortie";
	static constexpr auto AIDE = "Applique une saturation à l'image.";

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
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	void evalue_entrees(int temps) override
	{
		INUTILISE(temps);
	}

	dls::image::Pixel<float> evalue_pixel(dls::image::Pixel<float> const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
		return pixel;
	}

	void compile(CompileuseGraphe &compileuse, int temps) override
	{
		INUTILISE(temps);
		compileuse.ajoute_noeud(NOEUD_SORTIE);
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(0)->pointeur()->liens[0]));
	}
};

class OperatricePixelSaturation final : public OperatricePixel {
public:
	static constexpr auto NOM = "SATURATION";
	static constexpr auto AIDE = "Applique une saturation à l'image.";

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
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	void evalue_entrees(int temps) override
	{
		INUTILISE(temps);
	}

	dls::image::Pixel<float> evalue_pixel(dls::image::Pixel<float> const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
		return pixel;
	}

	void compile(CompileuseGraphe &compileuse, int temps) override
	{
		INUTILISE(temps);
		compileuse.ajoute_noeud(NOEUD_SATURATION);
		compileuse.ajoute_noeud(compileuse.decalage_pile(entree(0)->pointeur()->liens[0]));

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
		dls::phys::couleur32 const &entree,
		dls::phys::couleur32 &sortie)
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
				auto c = pile_charge_couleur(pointeur);

				auto saturation = pile_charge_decimal(courant);
				auto type = pile_charge_entier(courant);

				auto s = luminance(c);
				auto r = dls::phys::couleur32(s, s, s, c.a);

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
	, m_graphe(cree_noeud_image, supprime_noeud_image)
{
	entrees(1);
	sorties(1);

	auto noeud_entree = m_graphe.cree_noeud(OperatricePixelEntree::NOM);
	auto op_entree = memoire::loge<OperatricePixelEntree>(OperatricePixelEntree::NOM, m_graphe, noeud_entree);
	INUTILISE(op_entree);

	synchronise_donnees_operatrice(noeud_entree);

	auto noeud_sortie = m_graphe.cree_noeud(OperatricePixelSortie::NOM);
	auto op_sortie = memoire::loge<OperatricePixelSortie>(OperatricePixelSortie::NOM, m_graphe, noeud_sortie);
	INUTILISE(op_sortie);

	synchronise_donnees_operatrice(noeud_sortie);

	auto noeud_saturation = m_graphe.cree_noeud(OperatricePixelSaturation::NOM);
	auto op_saturation = memoire::loge<OperatricePixelSaturation>(OperatricePixelSaturation::NOM, m_graphe, noeud_saturation);
	INUTILISE(op_saturation);

	synchronise_donnees_operatrice(noeud_saturation);

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

int OperatriceGraphePixel::execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	Calque *tampon = nullptr;

	auto const &rectangle = contexte.resolution_rendu;

	if (!entree(0)->connectee()) {
		tampon = m_image.ajoute_calque("image", rectangle);
	}
	else {
		entree(0)->requiers_image(m_image, contexte, donnees_aval);
		auto nom_calque = evalue_chaine("nom_calque");
		tampon = m_image.calque(nom_calque);
	}

	if (tampon == nullptr) {
		ajoute_avertissement("Calque introuvable !");
		return EXECUTION_ECHOUEE;
	}

	compile_graphe(contexte.temps_courant);

	boucle_parallele(tbb::blocked_range<size_t>(0, static_cast<size_t>(rectangle.hauteur)),
					 [&](tbb::blocked_range<size_t> const &plage)
	{
		/* fais une copie locale pour éviter les problèmes de concurrence
		 * critique */
		auto pile = m_compileuse.pile();

		for (size_t l = plage.begin(); l < plage.end(); ++l) {
			for (size_t c = 0; c < static_cast<size_t>(rectangle.largeur); ++c) {
//				auto const x = c * largeur_inverse;
//				auto const y = l * hauteur_inverse;

				auto courante = tampon->valeur(c, l);

				dls::phys::couleur32 entree;
				entree.r = courante.r;
				entree.v = courante.g;
				entree.b = courante.b;
				entree.a = courante.a;

				dls::phys::couleur32 sortie;
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
		for (auto &sortie : noeud->sorties()) {
			sortie->decalage_pile = 0;
		}

		auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());
		auto operatrice_pixel = dynamic_cast<OperatricePixel *>(operatrice);

		if (operatrice_pixel == nullptr) {
			ajoute_avertissement("Impossible de trouver une opératrice pixel dans le graphe !");
			return;
		}

		operatrice_pixel->compile(m_compileuse, temps);
	}
}

#pragma clang diagnostic pop
