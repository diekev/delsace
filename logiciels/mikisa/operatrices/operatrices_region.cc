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

#include "operatrices_region.h"

#include "biblinternes/image/operations/champs_distance.h"
#include "biblinternes/image/operations/conversion.h"
#include "biblinternes/image/operations/convolution.h"
#include "biblinternes/image/operations/operations.h"
#include "biblinternes/math/matrice/operations.hh"
#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/dico_fixe.hh"
#include "biblinternes/structures/flux_chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_image.h"
#include "coeur/usine_operatrice.h"

#include "wolika/echantillonnage.hh"
#include "wolika/filtre_2d.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

template <typename T, typename TypeOperation>
void applique_fonction(ChefExecution *chef, wlk::grille_dense_2d<T> &image, TypeOperation &&op)
{
	auto const res_x = image.desc().resolution.x;
	auto const res_y = image.desc().resolution.y;

	boucle_parallele(tbb::blocked_range<int>(0, res_y),
					 [&](tbb::blocked_range<int> const &plage)
	{
		if (chef->interrompu()) {
			return;
		}

		for (int l = plage.begin(); l < plage.end(); ++l) {
			if (chef->interrompu()) {
				return;
			}

			for (int c = 0; c < res_x; ++c) {
				auto index = image.calcul_index(dls::math::vec2i(c, l));
				image.valeur(index) = op(image.valeur(index));
			}
		}

		auto delta = static_cast<float>(plage.end() - plage.begin());
		delta /= static_cast<float>(res_y);
		chef->indique_progression_parallele(delta * 100.0f);
	});
}

template <typename T, typename TypeOperation>
void applique_fonction_position(ChefExecution *chef, wlk::grille_dense_2d<T> &image, TypeOperation &&op)
{
	auto const res_x = image.desc().resolution.x;
	auto const res_y = image.desc().resolution.y;

	boucle_parallele(tbb::blocked_range<int>(0, res_y),
					 [&](tbb::blocked_range<int> const &plage)
	{
		if (chef->interrompu()) {
			return;
		}

		for (int l = plage.begin(); l < plage.end(); ++l) {
			if (chef->interrompu()) {
				return;
			}

			for (int c = 0; c < res_x; ++c) {
				auto index = image.calcul_index(dls::math::vec2i(c, l));
				image.valeur(index) = op(image.valeur(index), l, c);
			}
		}

		auto delta = static_cast<float>(plage.end() - plage.begin());
		delta /= static_cast<float>(res_y);
		chef->indique_progression_parallele(delta * 100.0f);
	});
}

/* ************************************************************************** */

class OperatriceAnalyse : public OperatriceImage {
	enum {
		ANALYSE_GRADIENT   = 0,
		ANALYSE_DIVERGENCE = 1,
		ANALYSE_LAPLACIEN  = 2,
		ANALYSE_COURBE     = 3,
	};

	enum {
		DIRECTION_X  = 0,
		DIRECTION_Y  = 1,
		DIRECTION_XY = 2,
	};

	enum {
		DIFF_AVANT   = 0,
		DIFF_ARRIERE = 1,
	};

public:
	static constexpr auto NOM = "Analyse";
	static constexpr auto AIDE = "Analyse l'image.";

	explicit OperatriceAnalyse(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_analyse.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image = entree(0)->requiers_image(contexte, donnees_aval);
		auto nom_calque = evalue_chaine("nom_calque");
		auto calque_entree = cherche_calque(*this, image, nom_calque);

		if (calque_entree == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto const operation = evalue_enum("operation");
		auto filtre = -1;

		if (operation == "gradient") {
			filtre = ANALYSE_GRADIENT;
		}
		else if (operation == "divergence") {
			filtre = ANALYSE_DIVERGENCE;
		}
		else if (operation == "laplacien") {
			filtre = ANALYSE_LAPLACIEN;
		}
		else if (operation == "courbe") {
			filtre = ANALYSE_COURBE;
		}

		auto const direction = evalue_enum("direction");
		auto dir = -1;

		if (direction == "dir_x") {
			dir = DIRECTION_X;
		}
		else if (direction == "dir_y") {
			dir = DIRECTION_Y;
		}
		else if (direction == "dir_xy") {
			dir = DIRECTION_XY;
		}

		auto dx0 = evalue_entier("dx0");
		auto dx1 = evalue_entier("dx1");
		auto dy0 = evalue_entier("dy0");
		auto dy1 = evalue_entier("dy1");

		auto const differentiation = evalue_enum("différentiation");

		if (differentiation == "arrière") {
			dx0 = -dx0;
			dx1 = -dx1;
			dy0 = -dy0;
			dy1 = -dy1;
		}

		auto tampon = extrait_grille_couleur(calque_entree);
		auto res_x = tampon->desc().resolution.x;

		dy0 *= res_x;
		dy1 *= res_x;

		auto image_tampon = grille_couleur(tampon->desc());

		auto chef = contexte.chef;
		chef->demarre_evaluation("analyse image");

		applique_fonction_position(chef, image_tampon,
								   [&](dls::phys::couleur32 const &/*pixel*/, int l, int c)
		{
			auto index = tampon->calcul_index(dls::math::vec2i(c, l));

			auto resultat = dls::phys::couleur32();
			resultat.a = 1.0f;

			if (filtre == ANALYSE_GRADIENT) {
				if (dir == DIRECTION_X) {
					auto px0 = tampon->valeur(index - dx0);
					auto px1 = tampon->valeur(index + dx1);

					resultat.r = (px1.r - px0.r);
					resultat.v = (px1.v - px0.v);
					resultat.b = (px1.b - px0.b);
				}
				else if (dir == DIRECTION_Y) {
					auto py0 = tampon->valeur(index - dy0);
					auto py1 = tampon->valeur(index + dy1);

					resultat.r = (py1.r - py0.r);
					resultat.v = (py1.v - py0.v);
					resultat.b = (py1.b - py0.b);
				}
				else if (dir == DIRECTION_XY) {
					auto px0 = tampon->valeur(index - dx0);
					auto px1 = tampon->valeur(index + dx1);
					auto py0 = tampon->valeur(index - dy0);
					auto py1 = tampon->valeur(index + dy1);

					resultat.r = ((px1.r - px0.r) + (py1.r - py0.r)) * 0.5f;
					resultat.v = ((px1.v - px0.v) + (py1.v - py0.v)) * 0.5f;
					resultat.b = ((px1.b - px0.b) + (py1.b - py0.b)) * 0.5f;
				}
			}
			else if (filtre == ANALYSE_DIVERGENCE) {
				auto valeur = 0.0f;

				if (dir == DIRECTION_X) {
					auto px0 = tampon->valeur(index - dx0);
					auto px1 = tampon->valeur(index + dx1);

					valeur = (px1.r - px0.r) + (px1.v - px0.v) + (px1.b - px0.b);

				}
				else if (dir == DIRECTION_Y) {
					auto py0 = tampon->valeur(index - dy0);
					auto py1 = tampon->valeur(index + dy1);

					valeur = (py1.r - py0.r) + (py1.v - py0.v) + (py1.b - py0.b);
				}
				else if (dir == DIRECTION_XY) {
					auto px0 = tampon->valeur(index - dx0);
					auto px1 = tampon->valeur(index + dx1);
					auto py0 = tampon->valeur(index - dy0);
					auto py1 = tampon->valeur(index + dy1);

					valeur = ((px1.r - px0.r) + (py1.r - py0.r)) * 0.5f
							 + ((px1.v - px0.v) + (py1.v - py0.v)) * 0.5f
							 + ((px1.b - px0.b) + (py1.b - py0.b)) * 0.5f;
				}

				resultat.r = valeur;
				resultat.v = valeur;
				resultat.b = valeur;
			}
			else if (filtre == ANALYSE_LAPLACIEN) {
				auto px  = tampon->valeur(index);
				if (dir == DIRECTION_X) {
					auto px0 = tampon->valeur(index - dx0);
					auto px1 = tampon->valeur(index + dx1);

					resultat.r = px1.r + px0.r - px.r * 2.0f;
					resultat.v = px1.v + px0.v - px.v * 2.0f;
					resultat.b = px1.b + px0.b - px.b * 2.0f;
				}
				else if (dir == DIRECTION_Y) {
					auto py0 = tampon->valeur(index - dy0);
					auto py1 = tampon->valeur(index + dy1);

					resultat.r = py1.r + py0.r - px.r * 2.0f;
					resultat.v = py1.v + py0.v - px.v * 2.0f;
					resultat.b = py1.b + py0.b - px.b * 2.0f;
				}
				else if (dir == DIRECTION_XY) {
					auto px0 = tampon->valeur(index - dx0);
					auto px1 = tampon->valeur(index + dx1);
					auto py0 = tampon->valeur(index - dy0);
					auto py1 = tampon->valeur(index + dy1);

					resultat.r = px1.r + px0.r + py1.r + py0.r - px.r * 4.0f;
					resultat.v = px1.v + px0.v + py1.v + py0.v - px.v * 4.0f;
					resultat.b = px1.b + px0.b + py1.b + py0.b - px.b * 4.0f;
				}
			}
			else if (filtre == ANALYSE_COURBE) {
				auto px  = tampon->valeur(index);
				auto gradient = dls::phys::couleur32();

				if (dir == DIRECTION_X) {
					auto px0 = tampon->valeur(index - dx0);
					auto px1 = tampon->valeur(index + dx1);

					gradient.r = (px1.r - px0.r);
					gradient.v = (px1.v - px0.v);
					gradient.b = (px1.b - px0.b);
				}
				else if (dir == DIRECTION_Y) {
					auto py0 = tampon->valeur(index - dy0);
					auto py1 = tampon->valeur(index + dy1);

					gradient.r = (py1.r - py0.r);
					gradient.v = (py1.v - py0.v);
					gradient.b = (py1.b - py0.b);
				}
				else if (dir == DIRECTION_XY) {
					auto px0 = tampon->valeur(index - dx0);
					auto px1 = tampon->valeur(index + dx1);
					auto py0 = tampon->valeur(index - dy0);
					auto py1 = tampon->valeur(index + dy1);

					gradient.r = ((px1.r - px0.r) + (py1.r - py0.r)) * 0.5f;
					gradient.v = ((px1.v - px0.v) + (py1.v - py0.v)) * 0.5f;
					gradient.b = ((px1.b - px0.b) + (py1.b - py0.b)) * 0.5f;
				}

				resultat.r = gradient.v * px.b - gradient.b * px.v;
				resultat.v = gradient.b * px.r - gradient.r * px.b;
				resultat.b = gradient.r * px.v - gradient.v * px.r;
			}

			return resultat;
		});

		chef->indique_progression(100.0f);

		auto calque = m_image.ajoute_calque(nom_calque, image_tampon.desc(), wlk::type_grille::COULEUR);
		*calque->tampon() = image_tampon;

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceFiltrage : public OperatriceImage {
public:
	static constexpr auto NOM = "Filtre";
	static constexpr auto AIDE = "Appliquer un filtre à une image.";

	explicit OperatriceFiltrage(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_filtre.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

#if 1
		this->ajoute_avertissement("à réimplémenter");
#else
		auto image = entree(0)->requiers_image(contexte, donnees_aval);
		auto nom_calque = evalue_chaine("nom_calque");
		auto calque_entree = cherche_calque(*this, image, nom_calque);

		if (calque_entree == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto tampon_entree = extrait_grille_couleur(calque_entree);

		auto operation = evalue_enum("operation");
		auto filtre = 0;

		if (operation == "netteté") {
			filtre = dls::image::operation::NOYAU_NETTETE;
		}
		else if (operation == "netteté_autre") {
			filtre = dls::image::operation::NOYAU_NETTETE_ALT;
		}
		else if (operation == "flou_gaussien") {
			filtre = dls::image::operation::NOYAU_FLOU_GAUSSIEN;
		}
		else if (operation == "flou") {
			filtre = dls::image::operation::NOYAU_FLOU_ALT;
		}
		else if (operation == "flou_sans_poids") {
			filtre = dls::image::operation::NOYAU_FLOU_NON_PONDERE;
		}
		else if (operation == "relief_no") {
			filtre = dls::image::operation::NOYAU_RELIEF_NO;
		}
		else if (operation == "relief_ne") {
			filtre = dls::image::operation::NOYAU_RELIEF_NE;
		}
		else if (operation == "relief_so") {
			filtre = dls::image::operation::NOYAU_RELIEF_SO;
		}
		else if (operation == "relief_se") {
			filtre = dls::image::operation::NOYAU_RELIEF_SE;
		}
		else if (operation == "sobel_x") {
			filtre = dls::image::operation::NOYAU_SOBEL_X;
		}
		else if (operation == "sobel_y") {
			filtre = dls::image::operation::NOYAU_SOBEL_Y;
		}

		auto calque = m_image.ajoute_calque(nom_calque, tampon_entree->desc(), wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);
		//calque->tampon = dls::image::operation::applique_convolution(tampon->tampon, filtre);
#endif

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

auto valeur_maximale(grille_couleur const &grille)
{
	auto max = dls::phys::couleur32(-constantes<float>::INFINITE);

	for (auto i = 0; i < grille.nombre_elements(); ++i) {
		auto v = grille.valeur(i);

		for (auto j = 0; j < 4; ++j) {
			if (v[j] > max[j]) {
				max[j] = v[j];
			}
		}
	}

	return max;
}

class OperatriceNormalisationRegion : public OperatriceImage {
public:
	static constexpr auto NOM = "Normalisation Région";
	static constexpr auto AIDE = "Normalise the image.";

	explicit OperatriceNormalisationRegion(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_normalisation.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		entree(0)->requiers_copie_image(m_image, contexte, donnees_aval);
		auto nom_calque = evalue_chaine("nom_calque");
		auto calque = m_image.calque_pour_ecriture(nom_calque);

		if (calque == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto tampon = extrait_grille_couleur(calque);

		auto maximum = valeur_maximale(*tampon);
		maximum.r = (maximum.r > 0.0f) ? (1.0f / maximum.r) : 0.0f;
		maximum.v = (maximum.v > 0.0f) ? (1.0f / maximum.v) : 0.0f;
		maximum.b = (maximum.b > 0.0f) ? (1.0f / maximum.b) : 0.0f;
		maximum.a = (maximum.a > 0.0f) ? (1.0f / maximum.a) : 0.0f;

		auto chef = contexte.chef;
		chef->demarre_evaluation("normalisation image");

		applique_fonction(chef, *tampon,
						  [&](dls::phys::couleur32 const &pixel)
		{
			return maximum * pixel;
		});

		chef->indique_progression(1000.f);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

struct ChefWolika : public wlk::interruptrice {
	ChefExecution *chef;

	ChefWolika(ChefExecution *chef_ex, const char *message)
		: chef(chef_ex)
	{
		chef->demarre_evaluation(message);
	}

	bool interrompue() const override
	{
		return chef->interrompu();
	}

	void indique_progression(float progression) override
	{
		chef->indique_progression(progression);
	}

	void indique_progression_parallele(float delta) override
	{
		chef->indique_progression(delta);
	}
};

class OperatriceFloutage : public OperatriceImage {
public:
	static constexpr auto NOM = "Flou";
	static constexpr auto AIDE = "Applique un flou à l'image.";

	explicit OperatriceFloutage(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_flou.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image = entree(0)->requiers_image(contexte, donnees_aval);
		auto nom_calque = evalue_chaine("nom_calque");
		auto calque_entree = cherche_calque(*this, image, nom_calque);

		if (calque_entree == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto tampon_entree = extrait_grille_couleur(calque_entree);

		auto calque = m_image.ajoute_calque(nom_calque, tampon_entree->desc(), wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);
		copie_donnees_calque(*tampon_entree, *tampon);

		auto const rayon_flou = evalue_decimal("rayon", contexte.temps_courant);
		auto const chaine_flou = evalue_enum("type");

		static auto dico_type = dls::cree_dico(
					dls::paire{ dls::chaine("boîte"), wlk::type_filtre::BOITE },
					dls::paire{ dls::chaine("triangulaire"), wlk::type_filtre::TRIANGULAIRE },
					dls::paire{ dls::chaine("quadratic"), wlk::type_filtre::QUADRATIC },
					dls::paire{ dls::chaine("cubic"), wlk::type_filtre::CUBIC },
					dls::paire{ dls::chaine("gaussien"), wlk::type_filtre::GAUSSIEN },
					dls::paire{ dls::chaine("mitchell"), wlk::type_filtre::MITCHELL },
					dls::paire{ dls::chaine("catrom"), wlk::type_filtre::CATROM });

		auto plg_type = dico_type.trouve(chaine_flou);

		if (plg_type.est_finie()) {
			auto flux = dls::flux_chaine();
			flux << "Type de filter '" << chaine_flou << "' inconnu";
			this->ajoute_avertissement(flux.chn());
			return EXECUTION_ECHOUEE;
		}

		/* construit le kernel du flou */
		auto rayon = rayon_flou;
		auto type = plg_type.front().second;

		if (type == wlk::type_filtre::GAUSSIEN) {
			rayon = rayon * 2.57f;
		}

		auto chef_wolika = ChefWolika(contexte.chef, "filtre");

		wlk::filtre_grille(*tampon, type, rayon, &chef_wolika);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceTournoiement : public OperatriceImage {
public:
	static constexpr auto NOM = "Tournoiement";
	static constexpr auto AIDE = "Applique un tournoiement aux pixels de l'image.";

	explicit OperatriceTournoiement(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_tournoiement.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image = entree(0)->requiers_image(contexte, donnees_aval);
		auto nom_calque = evalue_chaine("nom_calque");
		auto calque_entree = cherche_calque(*this, image, nom_calque);

		if (calque_entree == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto tampon_entree = extrait_grille_couleur(calque_entree);
		auto const &hauteur = tampon_entree->desc().resolution.y;
		auto const &largeur = tampon_entree->desc().resolution.x;
		auto const &hauteur_inverse = 1.0f / static_cast<float>(hauteur);
		auto const &largeur_inverse = 1.0f / static_cast<float>(largeur);

		auto const decalage_x = evalue_decimal("décalage_x", contexte.temps_courant);
		auto const decalage_y = evalue_decimal("décalage_y", contexte.temps_courant);
		auto const taille = evalue_decimal("taille", contexte.temps_courant);
		auto const periodes = evalue_decimal("périodes", contexte.temps_courant);

		auto calque = m_image.ajoute_calque(nom_calque, tampon_entree->desc(), wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);

		auto chef = contexte.chef;
		chef->demarre_evaluation("tournoiement");

		applique_fonction_position(chef, *tampon,
								   [&](dls::phys::couleur32 const &/*pixel*/, int l, int c)
		{
			auto const fc = static_cast<float>(c) * largeur_inverse + decalage_x;
			auto const fl = static_cast<float>(l) * hauteur_inverse + decalage_y;

			auto const rayon = dls::math::hypotenuse(fc, fl) * taille;
			auto const angle = std::atan2(fl, fc) * periodes + rayon;

			auto nc = rayon * std::cos(angle) * static_cast<float>(largeur) + 0.5f;
			auto nl = rayon * std::sin(angle) * static_cast<float>(hauteur) + 0.5f;

			return wlk::echantillonne_lineaire(*tampon, nc, nl);
		});

		chef->indique_progression(100.0f);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static void calcule_distance(
		wlk::grille_dense_2d<float> &phi,
		int x, int y, float h)
{
	auto a = std::min(phi.valeur(dls::math::vec2i(x - 1, y)), phi.valeur(dls::math::vec2i(x + 1, y)));
	auto b = std::min(phi.valeur(dls::math::vec2i(x, y - 1)), phi.valeur(dls::math::vec2i(x, y + 1)));
	auto xi = 0.0f;

	if (std::abs(a - b) >= h) {
		xi = std::min(a, b) + h;
	}
	else {
		xi =  0.5f * (a + b + std::sqrt(2.0f * h * h - (a - b) * (a - b)));
	}

	phi.valeur(dls::math::vec2i(x, y)) = std::min(phi.valeur(dls::math::vec2i(x, y)), xi);
}

static auto luminance(grille_couleur const &grille)
{
	auto res = wlk::grille_dense_2d<float>(grille.desc());

	for (auto i = 0; i < res.nombre_elements(); ++i) {
		auto clr = grille.valeur(i);
		res.valeur(i) = dls::image::outils::luminance_709(clr.r, clr.v, clr.b);
	}

	return res;
}

static auto converti_float_pixel(wlk::grille_dense_2d<float> const &grille)
{
	auto res = grille_couleur(grille.desc());

	for (auto i = 0; i < res.nombre_elements(); ++i) {
		auto pixel = dls::phys::couleur32(grille.valeur(i));
		pixel.a = 1.0f;

		res.valeur(i) = pixel;
	}

	return res;
}

class OperatriceChampsDistance : public OperatriceImage {
public:
	static constexpr auto NOM = "Champs distance";
	static constexpr auto AIDE = "Calcule le champs de distance de l'image d'entrée.";

	explicit OperatriceChampsDistance(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_champs_distance.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image = entree(0)->requiers_image(contexte, donnees_aval);
		auto nom_calque = evalue_chaine("nom_calque");
		auto calque_entree = cherche_calque(*this, image, nom_calque);

		if (calque_entree == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto tampon_entree = extrait_grille_couleur(calque_entree);

		auto const methode = evalue_enum("méthode");
		auto image_grise = luminance(*tampon_entree);
		auto image_tampon = wlk::grille_dense_2d<float>(tampon_entree->desc());
		auto valeur_iso = evalue_decimal("valeur_iso", contexte.temps_courant);

		/* À FAIRE :  deux versions de chaque algorithme : signée et non-signée.
		 * cela demandrait de garder trace de deux grilles pour la version
		 * non-signée, comme pour l'algorithme 8SSEDT.
		 */

		if (methode == "balayage_rapide") {
			int res_x = tampon_entree->desc().resolution.x;
			int res_y = tampon_entree->desc().resolution.y;

			auto h = std::min(1.0f / static_cast<float>(res_x), 1.0f / static_cast<float>(res_y));

			for (int y = 0; y < res_y; ++y) {
				for (int x = 0; x < res_x; ++x) {
					if (image_grise.valeur(dls::math::vec2i(x, y)) > valeur_iso) {
						image_tampon.valeur(dls::math::vec2i(x, y)) = 1.0f;
					}
					else {
						image_tampon.valeur(dls::math::vec2i(x, y)) = 0.0f;
					}
				}
			}

			/* bas-droite */
			for (int x = 1; x < res_x - 1; ++x) {
				for (int y = 1; y < res_y - 1; ++y) {
					if (image_tampon.valeur(dls::math::vec2i(x, y)) != 0.0f) {
						calcule_distance(image_tampon, x, y, h);
					}
				}
			}

			/* bas-gauche */
			for (int x = res_x - 2; x >= 1; --x) {
				for (int y = 1; y < res_y - 1; ++y) {
					if (image_tampon.valeur(dls::math::vec2i(x, y)) != 0.0f) {
						calcule_distance(image_tampon, x, y, h);
					}
				}
			}

			/* haut-gauche */
			for (int x = res_x - 2; x >= 1; --x) {
				for (int y = res_y - 2; y >= 1; --y) {
					if (image_tampon.valeur(dls::math::vec2i(x, y)) != 0.0f) {
						calcule_distance(image_tampon, x, y, h);
					}
				}
			}

			/* haut-droite */
			for (int x = 1; x < res_x - 1; ++x) {
				for (int y = res_y - 2; y >= 1; --y) {
					if (image_tampon.valeur(dls::math::vec2i(x, y)) != 0.0f) {
						calcule_distance(image_tampon, x, y, h);
					}
				}
			}
		}
		else if (methode == "estime") {
			//dls::image::operation::navigation_estime::genere_champs_distance(image_grise, image_tampon, valeur_iso);
		}
		else if (methode == "8SSEDT") {
			//dls::image::operation::ssedt::genere_champs_distance(image_grise, image_tampon, valeur_iso);
		}

		/* À FAIRE : alpha ou tampon à canaux variables */
		auto calque = m_image.ajoute_calque(nom_calque, tampon_entree->desc(), wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);
		*tampon = converti_float_pixel(image_tampon);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static auto moyenne(dls::phys::couleur32 const &pixel)
{
	return (pixel.r + pixel.v + pixel.b) * 0.3333f;
}

class OperatriceDeformation final : public OperatriceImage {
public:
	static constexpr auto NOM = "Déformation";
	static constexpr auto AIDE = "Déforme une image de manière aléatoire.";

	explicit OperatriceDeformation(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(2);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_deformation.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image1 = entree(0)->requiers_image(contexte, donnees_aval);
		auto nom_calque_a = evalue_chaine("nom_calque_a");
		auto calque_a = cherche_calque(*this, image1, nom_calque_a);

		if (calque_a == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto image2 = entree(1)->requiers_image(contexte, donnees_aval);
		auto nom_calque_b = evalue_chaine("nom_calque_b");
		auto calque_b = cherche_calque(*this, image2, nom_calque_b);

		if (calque_b == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto tampon_a = extrait_grille_couleur(calque_a);
		auto tampon_b = extrait_grille_couleur(calque_b);
		auto res_x = tampon_a->desc().resolution.x;
		auto res_y = tampon_a->desc().resolution.y;

		auto calque = m_image.ajoute_calque(nom_calque_a, tampon_a->desc(), wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);

		auto chef = contexte.chef;
		chef->demarre_evaluation("déformation");

		applique_fonction_position(chef, *tampon,
								   [&](dls::phys::couleur32 const &/*pixel*/, int l, int c)
		{
			auto const c0 = std::min(res_x - 1, std::max(0, c - 1));
			auto const c1 = std::min(res_x - 1, std::max(0, c + 1));
			auto const l0 = std::min(res_y - 1, std::max(0, l - 1));
			auto const l1 = std::min(res_y - 1, std::max(0, l + 1));

			auto const x0 = moyenne(tampon_b->valeur(dls::math::vec2i(c0, l)));
			auto const x1 = moyenne(tampon_b->valeur(dls::math::vec2i(c1, l)));
			auto const y0 = moyenne(tampon_b->valeur(dls::math::vec2i(c, l0)));
			auto const y1 = moyenne(tampon_b->valeur(dls::math::vec2i(c, l1)));

			auto const pos_x = x1 - x0;
			auto const pos_y = y1 - y0;

			auto const x = static_cast<float>(c) + pos_x * static_cast<float>(res_x);
			auto const y = static_cast<float>(l) + pos_y * static_cast<float>(res_y);

			return echantillonne_lineaire(*tampon_a, x, y);
		});

		chef->indique_progression(100.0f);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

/**
 * https://hal.archives-ouvertes.fr/hal-01358392/file/Film_grain_rendering_preprint.pdf
 */

static auto const MAX_NIVEAU_GRIS = 255;

unsigned int poisson(const float u, const float lambda)
{
	/* Inverse transform sampling */
	auto prod = std::exp(-lambda);

//    float prod;
//    if (prodIn <= 0)
//        prod = exp(-lambda);
//    /* this should be passed as an argument if used
// * extensively with the same value lambda */
//    else
//        prod = prodIn;

	auto somme = prod;
	auto x = 0u;

	while ((u > somme) && (static_cast<float>(x) < std::floor(10000.0f * lambda))) {
		x = x + 1u;
		prod = prod * lambda / static_cast<float>(x);
		somme = somme + prod;
	}

	return x;
}

using type_image_grise = wlk::grille_dense_2d<float>;

static type_image_grise extrait_canal(grille_couleur const &image, const int canal)
{
	type_image_grise resultat(image.desc());

	boucle_parallele(tbb::blocked_range<int>(0, image.desc().resolution.y),
					 [&](tbb::blocked_range<int> const &plage)
	{
		for (auto l = plage.begin(); l < plage.end(); ++l) {
			for (auto c = 0; c < image.desc().resolution.x; ++c) {
				auto index = image.calcul_index(dls::math::vec2i(c, l));
				resultat.valeur(index) = image.valeur(index)[canal];
			}
		}
	});

	return resultat;
}

static void assemble_image(
		grille_couleur &image,
		type_image_grise const &canal_rouge,
		type_image_grise const &canal_vert,
		type_image_grise const &canal_bleu)
{

	boucle_parallele(tbb::blocked_range<int>(0, image.desc().resolution.y),
					 [&](tbb::blocked_range<int> const &plage)
	{
		for (auto l = plage.begin(); l < plage.end(); ++l) {
			for (auto c = 0; c < image.desc().resolution.x; ++c) {
				auto index = image.calcul_index(dls::math::vec2i(c, l));
				auto pixel = dls::phys::couleur32(1.0f);
				pixel.r = canal_rouge.valeur(index);
				pixel.v = canal_vert.valeur(index);
				pixel.b = canal_bleu.valeur(index);

				image.valeur(index) = pixel;
			}
		}
	});
}

static type_image_grise simule_grain_image(
		type_image_grise const &image,
		const int graine,
		const float rayon_grain,
		const float sigma_rayon,
		const float sigma_filtre)
{
	auto const res_x = image.desc().resolution.x;
	auto const res_y = image.desc().resolution.y;

	type_image_grise resultat(image.desc());

	auto normal_quantile = 3.0902f;	//standard normal quantile for alpha=0.999
	auto rayon_grain2 = rayon_grain * rayon_grain;
	auto rayon_max = rayon_grain;
	auto mu = 0.0f;
	auto sigma = 0.0f;

	if (sigma_rayon > 0.0f) {
		sigma = std::sqrt(std::log1p((sigma_rayon / rayon_grain) * (sigma_rayon / rayon_grain)));
		auto const sigma2 = sigma * sigma;
		mu = std::log(rayon_grain) - sigma2 / 2.0f;
		auto const log_normal_quantile = std::exp(mu + sigma * normal_quantile);
		rayon_max = log_normal_quantile;
	}

	auto const delta = 1.0f / std::ceil(1.0f / rayon_max);
	auto const delta_inv = 1.0f / delta;

	auto gna = GNA(graine);

	/* précalcul des lambdas */
	dls::tableau<float> lambdas(MAX_NIVEAU_GRIS);

	for (auto i = 0; i < MAX_NIVEAU_GRIS; ++i) {
		auto const u = static_cast<float>(i) / static_cast<float>(MAX_NIVEAU_GRIS);
		auto const ag = 1.0f / std::ceil(1.0f / rayon_max);
		auto const lambda_tmp = -((ag * ag) / (constantes<float>::PI * (rayon_max*rayon_max + sigma*sigma))) * std::log(1.0f - u);
		lambdas[i] = lambda_tmp;
	}

	/* précalcul des gaussiens */
	auto const iter = 1; // nombre d'itération de la simulation de Monte-Carlo

	dls::tableau<float> liste_gaussien_x(iter);
	dls::tableau<float> liste_gaussien_y(iter);

	for (auto i = 0; i < iter; ++i) {
		liste_gaussien_x[i] = gna.normale(0.0f, sigma_filtre);
		liste_gaussien_y[i] = gna.normale(0.0f, sigma_filtre);
	}

	boucle_parallele(tbb::blocked_range<int>(0, res_y),
					 [&](tbb::blocked_range<int> const &plage)
	{
		auto gna_local = GNA(graine + plage.begin());

		auto rayon_courant = 0.0f;
		auto rayon_courant2 = rayon_grain2;

		for (int j = plage.begin(); j < plage.end(); ++j) {
			for (int i = 0; i < res_x; ++i) {
				auto index = resultat.calcul_index(dls::math::vec2i(i, j));
				resultat.valeur(index) = 0.0f;

				for (auto k = 0; k < iter; ++k) {
					bool va_suivant = false;
					// décalage aléatoire d'une distribution gaussienne centrée de variance sigma^2
					auto gaussien_x = static_cast<float>(i) + sigma_filtre * liste_gaussien_x[k];
					auto gaussien_y = static_cast<float>(j) + sigma_filtre * liste_gaussien_y[k];

					// obtiens la liste de cellule couvrant les balles (ig, jg)
					auto const min_x = std::floor((gaussien_x - rayon_max) * delta_inv);
					auto const max_x = std::floor((gaussien_x + rayon_max) * delta_inv);
					auto const min_y = std::floor((gaussien_y - rayon_max) * delta_inv);
					auto const max_y = std::floor((gaussien_y + rayon_max) * delta_inv);

					for (int jd = static_cast<int>(min_y); jd <= static_cast<int>(max_y); ++jd) {
						for (int id = static_cast<int>(min_x); id <= static_cast<int>(max_x); ++id) {
							/* coins de la cellule en coordonnées pixel */
							auto coin_x = delta*static_cast<float>(id);
							auto coin_y = delta*static_cast<float>(jd);

							// échantillone image
							auto const u = std::max(0.0f, std::min(1.0f, image.valeur(dls::math::vec2i(int(coin_x), int(coin_y)))));
							auto const index_u = static_cast<long>(u * MAX_NIVEAU_GRIS);
							auto const lambda = lambdas[index_u];
							auto const Q = poisson(gna_local.uniforme(0.0f, 1.0f), lambda);

							for (unsigned l = 1; l <= Q; ++l) {
								// prend un centre aléatoire d'une distribution uniforme dans un carré ([id, id+1), [jd, jd+1))
								auto grain_x = coin_x + gna_local.uniforme(0.0f, 1.0f) * delta;
								auto grain_y = coin_y + gna_local.uniforme(0.0f, 1.0f) * delta;
								auto dx = grain_x - gaussien_x;
								auto dy = grain_y - gaussien_y;

								if (sigma_rayon > 0.0f) {
									rayon_courant = std::min(std::exp(mu + sigma * gna_local.uniforme(0.0f, 1.0f)), rayon_max);
									rayon_courant2 = rayon_courant * rayon_courant;
								}
								else if (sigma_rayon == 0.0f) {
									rayon_courant2 = rayon_grain2;
								}

								if ((dy*dy + dx*dx) < rayon_courant2) {
									resultat.valeur(index) += 1.0f;
									va_suivant = true;
									break; // va vers la prochaine itération MC
								}
							}

							if (va_suivant) {
								break;
							}
						}

						if (va_suivant) {
							break;
						}
					}
				}

				resultat.valeur(index) /= iter;
			}
		}
	});

	return resultat;
}

class OperatriceSimulationGrain final : public OperatriceImage {
public:
	static constexpr auto NOM = "Simulation de grain";
	static constexpr auto AIDE = "Calcul du grain selon l'image d'entrée.";

	explicit OperatriceSimulationGrain(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_simulation_grain.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image = entree(0)->requiers_image(contexte, donnees_aval);
		auto const &nom_calque = evalue_chaine("nom_calque");
		auto calque_entree = cherche_calque(*this, image, nom_calque);

		if (calque_entree == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto tampon_entree = extrait_grille_couleur(calque_entree);

		/* parametres utilisateurs */
		auto const &graine = evalue_entier("graine", contexte.temps_courant) + contexte.temps_courant;

		auto const &rayon_grain_r = evalue_decimal("rayon_grain_r", contexte.temps_courant);
		auto const &sigma_rayon_r = evalue_decimal("sigma_rayon_r", contexte.temps_courant);
		auto const &sigma_filtre_r = evalue_decimal("sigma_filtre_r", contexte.temps_courant);

		auto const &rayon_grain_v = evalue_decimal("rayon_grain_v", contexte.temps_courant);
		auto const &sigma_rayon_v = evalue_decimal("sigma_rayon_v", contexte.temps_courant);
		auto const &sigma_filtre_v = evalue_decimal("sigma_filtre_v", contexte.temps_courant);

		auto const &rayon_grain_b = evalue_decimal("rayon_grain_b", contexte.temps_courant);
		auto const &sigma_rayon_b = evalue_decimal("sigma_rayon_b", contexte.temps_courant);
		auto const &sigma_filtre_b = evalue_decimal("sigma_filtre_b", contexte.temps_courant);

		auto const &canal_rouge = extrait_canal(*tampon_entree, 0);
		auto const &canal_vert = extrait_canal(*tampon_entree, 1);
		auto const &canal_bleu = extrait_canal(*tampon_entree, 2);

		auto const &bruit_rouge = simule_grain_image(canal_rouge, graine, rayon_grain_r, sigma_rayon_r, sigma_filtre_r);
		auto const &bruit_vert = simule_grain_image(canal_vert, graine, rayon_grain_v, sigma_rayon_v, sigma_filtre_v);
		auto const &bruit_bleu = simule_grain_image(canal_bleu, graine, rayon_grain_b, sigma_rayon_b, sigma_filtre_b);

		auto calque = m_image.ajoute_calque(nom_calque, tampon_entree->desc(), wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);
		assemble_image(*tampon, bruit_rouge, bruit_vert, bruit_bleu);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceCoordonneesPolaires final : public OperatriceImage {
public:
	static constexpr auto NOM = "Coordonnées Polaires";
	static constexpr auto AIDE = "Transforme une image entre les coordonnées cartésiennes et les coordonnées polaires.";

	explicit OperatriceCoordonneesPolaires(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_coordonnees_polaires.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image = entree(0)->requiers_image(contexte, donnees_aval);
		auto const nom_calque = evalue_chaine("nom_calque");
		auto calque_entree = cherche_calque(*this, image, nom_calque);

		if (calque_entree == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto tampon_entree = extrait_grille_couleur(calque_entree);

		int res_x = tampon_entree->desc().resolution.x;
		auto inv_res_x = 1.0f / static_cast<float>(res_x);

		auto calque = m_image.ajoute_calque(nom_calque, tampon_entree->desc(), wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);

		auto chef = contexte.chef;
		chef->demarre_evaluation("coordonnées polaire");

		applique_fonction_position(chef, *tampon,
								   [&](dls::phys::couleur32 const &/*pixel*/, int l, int c)
		{
			/* À FAIRE : image carrée ? */
			auto const r = static_cast<float>(l);
			auto const theta = constantes<float>::TAU * static_cast<float>(c) * inv_res_x;
			auto const x = r * std::cos(theta);
			auto const y = r * std::sin(theta);
			return echantillonne_lineaire(*tampon, x, y);
		});

		chef->indique_progression(100.0f);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceOndeletteHaar final : public OperatriceImage {
public:
	static constexpr auto NOM = "Ondelette de Haar";
	static constexpr auto AIDE = "Calcul l'ondelette de Haar d'une image.";

	explicit OperatriceOndeletteHaar(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_ondelette_haar.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image = entree(0)->requiers_image(contexte, donnees_aval);
		auto const nom_calque = evalue_chaine("nom_calque");
		auto calque_entree = cherche_calque(*this, image, nom_calque);

		if (calque_entree == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto tampon_entree = extrait_grille_couleur(calque_entree);

		auto const iterations = evalue_entier("itérations");
		auto res_x = tampon_entree->desc().resolution.x;
		auto res_y = tampon_entree->desc().resolution.y;

		auto calque = m_image.ajoute_calque(nom_calque, tampon_entree->desc(), wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);

		copie_donnees_calque(*tampon_entree, *tampon);

		auto image_tmp = grille_couleur(tampon->desc());

		auto const coeff = 1.0f / std::sqrt(2.0f);

		for (int i = 0; i < iterations; ++i) {
			/* transformation verticale */
			auto const taille_y = res_y / 2;
			for (int y = 0; y < taille_y; ++y) {
				for (int x = 0; x < res_x; ++x) {
					auto const p0 = tampon->valeur(dls::math::vec2i(x, y * 2));
					auto const p1 = tampon->valeur(dls::math::vec2i(x, y * 2 + 1));
					auto somme = (p0 + p1) * coeff;
					auto diff = (p0 - p1) * coeff;

					somme.a = 1.0f;
					diff.a = 1.0f;

					image_tmp.valeur(dls::math::vec2i(x, y)) = somme;
					image_tmp.valeur(dls::math::vec2i(x, y + taille_y)) = diff;
				}
			}

			*tampon = image_tmp;

			/* transformation horizontale */
			auto const taille_x = res_x / 2;
			for (int y = 0; y < res_y; ++y) {
				for (int x = 0; x < taille_x; ++x) {
					auto const p0 = tampon->valeur(dls::math::vec2i(x * 2, y));
					auto const p1 = tampon->valeur(dls::math::vec2i(x * 2 + 1, y));
					auto somme = (p0 + p1) * coeff;
					auto diff = (p0 - p1) * coeff;

					somme.a = 1.0f;
					diff.a = 1.0f;

					image_tmp.valeur(dls::math::vec2i(x, y)) = somme;
					image_tmp.valeur(dls::math::vec2i(x + taille_x, y)) = diff;
				}
			}

			*tampon = image_tmp;

			res_x /= 2;
			res_y /= 2;
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceDilation final : public OperatriceImage {
public:
	static constexpr auto NOM = "Dilation";
	static constexpr auto AIDE = "Dilate les pixels de l'image. Les parties les plus sombres de l'image rapetissent tandis que les plus claires grossissent.";

	explicit OperatriceDilation(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_dilation_erosion.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image = entree(0)->requiers_image(contexte, donnees_aval);
		auto const nom_calque = evalue_chaine("nom_calque");
		auto calque_entree = cherche_calque(*this, image, nom_calque);

		if (calque_entree == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto tampon_entree = extrait_grille_couleur(calque_entree);

		auto calque = m_image.ajoute_calque(nom_calque, tampon_entree->desc(), wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);

		auto const res_x = tampon_entree->desc().resolution.x;
		auto const res_y = tampon_entree->desc().resolution.y;
		auto const rayon = evalue_entier("rayon");

		auto performe_dilation = [&](
								 dls::phys::couleur32 const &/*pixel*/,
								 int y,
								 int x)
		{
			dls::phys::couleur32 p0(0.0f);
			p0.a = 1.0f;

			auto const debut_x = std::max(0, x - rayon);
			auto const debut_y = std::max(0, y - rayon);
			auto const fin_x = std::min(res_x, x + rayon);
			auto const fin_y = std::min(res_y, y + rayon);

			for (int sy = debut_y; sy < fin_y; ++sy) {
				for (int sx = debut_x; sx < fin_x; ++sx) {
					auto const p1 = tampon_entree->valeur(dls::math::vec2i(sx, sy));

					p0.r = std::max(p0.r, p1.r);
					p0.v = std::max(p0.v, p1.v);
					p0.b = std::max(p0.b, p1.b);
				}
			}

			return p0;
		};

		auto chef = contexte.chef;
		chef->demarre_evaluation("normalisation image");

		applique_fonction_position(chef, *tampon, performe_dilation);

		chef->indique_progression(100.0f);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceErosion final : public OperatriceImage {
public:
	static constexpr auto NOM = "Érosion";
	static constexpr auto AIDE = "Érode les pixels de l'image. Les parties les plus claires de l'image rapetissent tandis que les plus sombres grossissent.";

	explicit OperatriceErosion(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_dilation_erosion.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image = entree(0)->requiers_image(contexte, donnees_aval);
		auto const nom_calque = evalue_chaine("nom_calque");
		auto calque_entree = cherche_calque(*this, image, nom_calque);

		if (calque_entree == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto tampon_entree = extrait_grille_couleur(calque_entree);

		auto calque = m_image.ajoute_calque(nom_calque, tampon_entree->desc(), wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);

		auto const res_x = tampon_entree->desc().resolution.x;
		auto const res_y = tampon_entree->desc().resolution.y;
		auto const rayon = evalue_entier("rayon");

		auto performe_erosion = [&](
								dls::phys::couleur32 const &/*pixel*/,
								int y,
								int x)
		{
			dls::phys::couleur32 p0(1.0f);

			auto const debut_x = std::max(0, x - rayon);
			auto const debut_y = std::max(0, y - rayon);
			auto const fin_x = std::min(res_x, x + rayon);
			auto const fin_y = std::min(res_y, y + rayon);

			for (int sy = debut_y; sy < fin_y; ++sy) {
				for (int sx = debut_x; sx < fin_x; ++sx) {
					auto const p1 = tampon_entree->valeur(dls::math::vec2i(sx, sy));

					p0.r = std::min(p0.r, p1.r);
					p0.v = std::min(p0.v, p1.v);
					p0.b = std::min(p0.b, p1.b);
				}
			}

			return p0;
		};

		auto chef = contexte.chef;
		chef->demarre_evaluation("normalisation image");

		applique_fonction_position(chef, *tampon, performe_erosion);

		chef->indique_progression(100.0f);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceExtractionPalette final : public OperatriceImage {
public:
	static constexpr auto NOM = "Extraction Palette";
	static constexpr auto AIDE = "Extrait la palette d'une image.";

	explicit OperatriceExtractionPalette(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_extraction_palette.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image = entree(0)->requiers_image(contexte, donnees_aval);
		auto const nom_calque = evalue_chaine("nom_calque");
		auto calque_entree = cherche_calque(*this, image, nom_calque);

		if (calque_entree == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto tampon_entree = extrait_grille_couleur(calque_entree);

		auto calque = m_image.ajoute_calque(nom_calque, tampon_entree->desc(), wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);

		auto const res_x = tampon_entree->desc().resolution.x;
		auto const res_y = tampon_entree->desc().resolution.y;

		using pixel_t = dls::phys::couleur32;
		using paire_pixel_t = std::pair<pixel_t, int>;

		dls::tableau<paire_pixel_t> histogramme(360ul);

		for (auto &paire : histogramme) {
			paire.first = pixel_t(0.0f);
			paire.second = 0;
		}

		for (int y = 0; y < res_y; ++y) {
			for (int x = 0; x < res_x; ++x) {
				auto const &pixel = tampon_entree->valeur(dls::math::vec2i(x, y));
				auto res = pixel_t();
				res.a = 1;
				dls::phys::rvb_vers_hsv(pixel.r, pixel.v, pixel.b, &res.r, &res.v, &res.b);

				auto index = static_cast<long>(res.r * 360.0f);

				if (histogramme[index].second == 0) {
					histogramme[index].first = pixel;
				}

				histogramme[index].second += 1;
			}
		}

		std::sort(histogramme.debut(), histogramme.fin(),
				  [](paire_pixel_t const &v1, paire_pixel_t const &v2)
		{
			return v1.second > v2.second;
		});

		for (int l = 0; l < res_y; ++l) {
			for (int c = 0; c < res_x; ++c) {
				auto index = static_cast<long>(l / 64) % 360;
				tampon->valeur(dls::math::vec2i(c, l)) = histogramme[index].first;
			}
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

/* Calcul d'un préfiltre pour améliorer les interpolations cubiques.
 * Voir :
 * https://www.youtube.com/watch?v=nfhTET86kdE
 * https://developer.blender.org/D2984
 */

static void initialise_causal_prefiltre(
		dls::phys::couleur32 *pixel,
		dls::phys::couleur32 &somme,
		int longueur)
{
	auto const Zp = std::sqrt(3.0f) - 2.0f;
	auto const lambda = (1.0f - Zp) * (1.0f - 1.0f / Zp);
	auto horizon = std::min(12, longueur);
	auto Zn = Zp;

	somme.r = pixel[0].r;
	somme.v = pixel[0].v;
	somme.b = pixel[0].b;
	somme.a = pixel[0].a;

	for (auto compte = 0; compte < horizon; ++compte) {
		somme.r += Zn * pixel[compte].r;
		somme.v += Zn * pixel[compte].v;
		somme.b += Zn * pixel[compte].b;
		somme.a += Zn * pixel[compte].a;

		Zn *= Zp;
	}

	pixel[0].r = lambda * somme.r;
	pixel[0].v = lambda * somme.v;
	pixel[0].b = lambda * somme.b;
	pixel[0].a = lambda * somme.a;
}

static void initialise_anticausal_prefiltre(
		dls::phys::couleur32 *pixel)
{
	auto const Zp  = std::sqrt(3.0f) - 2.0f;
	auto const iZp = (Zp / (Zp - 1.0f));

	pixel[0].r = iZp * pixel[0].r;
	pixel[0].v = iZp * pixel[0].v;
	pixel[0].b = iZp * pixel[0].b;
	pixel[0].a = iZp * pixel[0].a;
}

static void recursion_prefiltre(dls::phys::couleur32 *pixel, int longueur, int stride)
{
	auto const Zp = std::sqrt(3.0f) - 2.0f;
	auto const lambda = (1.0f - Zp) * (1.0f - 1.0f / Zp);
	auto somme = dls::phys::couleur32{};
	auto compte = 0;

	initialise_causal_prefiltre(pixel, somme, longueur);

	auto prev_coeff = pixel[0];

	for (compte = stride; compte < longueur; ++compte) {
		pixel[compte].r = prev_coeff.r = (lambda * pixel[compte].r) + (Zp * prev_coeff.r);
		pixel[compte].v = prev_coeff.v = (lambda * pixel[compte].v) + (Zp * prev_coeff.v);
		pixel[compte].b = prev_coeff.b = (lambda * pixel[compte].b) + (Zp * prev_coeff.b);
		pixel[compte].a = prev_coeff.a = (lambda * pixel[compte].a) + (Zp * prev_coeff.a);
	}

	compte -= stride;

	initialise_anticausal_prefiltre(&pixel[compte]);

	prev_coeff = pixel[compte];

	for (compte -= stride; compte >= 0; compte -= stride) {
		pixel[compte].r = prev_coeff.r = Zp * (prev_coeff.r - pixel[compte].r);
		pixel[compte].v = prev_coeff.v = Zp * (prev_coeff.v - pixel[compte].v);
		pixel[compte].b = prev_coeff.b = Zp * (prev_coeff.b - pixel[compte].b);
		pixel[compte].a = prev_coeff.a = Zp * (prev_coeff.a - pixel[compte].a);
	}
}

class OperatricePrefiltreCubic final : public OperatriceImage {
public:
	static constexpr auto NOM = "Préfiltre Cubic B-Spline";
	static constexpr auto AIDE = "Créer un préfiltre cubic B-spline.";

	explicit OperatricePrefiltreCubic(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_extraction_palette.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image = entree(0)->requiers_image(contexte, donnees_aval);
		auto const nom_calque = evalue_chaine("nom_calque");
		auto calque_entree = cherche_calque(*this, image, nom_calque);

		if (calque_entree == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto tampon_entree = extrait_grille_couleur(calque_entree);
		auto const res_x = tampon_entree->desc().resolution.x;
		auto const res_y = tampon_entree->desc().resolution.y;

		if (res_x <= 2 || res_y <= 2) {
			this->ajoute_avertissement("L'image d'entrée est trop petite");
			return EXECUTION_ECHOUEE;
		}

		auto calque = m_image.ajoute_calque(nom_calque, tampon_entree->desc(), wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);

		copie_donnees_calque(*tampon_entree, *tampon);

		for (auto y = 0; y < res_y; ++y) {
			recursion_prefiltre(&tampon->valeur(y * res_x), res_x, 1);
		}

		for (auto x = 0; x < res_x; ++x) {
			recursion_prefiltre(&tampon->valeur(x), res_y, res_x);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

/**
 * Tirée de « Volumetric Light Scattering as a Post-Process »
 * https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch13.html
 */
class OpRayonsSoleil final : public OperatriceImage {
public:
	static constexpr auto NOM = "Rayons Soleil";
	static constexpr auto AIDE = "Créer des rayons de soleil.";

	explicit OpRayonsSoleil(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_rayons_soleil.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image = entree(0)->requiers_image(contexte, donnees_aval);
		auto const nom_calque = evalue_chaine("nom_calque");
		auto calque_entree = cherche_calque(*this, image, nom_calque);

		if (calque_entree == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto chef = contexte.chef;
		chef->demarre_evaluation("rayons soleil");

		auto tampon_entree = extrait_grille_couleur(calque_entree);

		auto calque = m_image.ajoute_calque(nom_calque, tampon_entree->desc(), wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);

		/* Controles à considérer :
		 * - taille max des rayons
		 * - échantillonnage haute qualité, sur tous les pixels de la ligne
		 *   (peut requérir un filtrage)
		 * - pré-floutte
		 * - superpose effet sur l'image d'entrée
		 * - pré-multiplie les pixels par leur alpha
		 * - utilisation d'une version noir et blanc de l'image
		 */
		auto const pos_x = evalue_decimal("pos_x");
		auto const pos_y = evalue_decimal("pos_y");
		auto const centre = dls::math::vec2f(pos_x, pos_y);
		auto const nombre_echantillons = evalue_entier("échantillons");
		auto const densite = evalue_decimal("densité");
		auto const poids = evalue_decimal("poids");
		auto const desintegration = evalue_decimal("désintégration");
		auto const exposition = evalue_decimal("exposition");

		auto largeur = tampon->desc().resolution.x;
		auto hauteur = tampon->desc().resolution.y;

		boucle_parallele(tbb::blocked_range<int>(0, hauteur),
						 [&](tbb::blocked_range<int> const &plage)
		{
			for (auto y = plage.begin(); y < plage.end(); ++y) {
				for (auto x = 0; x < largeur; ++x) {
					auto pos_idx = dls::math::vec2i(x, y);
					auto pos_mnd = tampon->index_vers_monde(pos_idx);

					auto delta_co = (pos_mnd - centre);

					delta_co *= 1.0f / static_cast<float>(nombre_echantillons) * densite;

					auto couleur = tampon_entree->valeur(pos_idx);

					auto poids_total = 1.0f;
					auto illumination = 1.0f;

					for (auto i = 0; i < nombre_echantillons; ++i) {
						pos_mnd -= delta_co;

						pos_idx = tampon_entree->monde_vers_index(pos_mnd);

						auto echant = tampon_entree->valeur(pos_idx);
						poids_total += illumination * poids;
						echant *= illumination * poids;

						couleur += echant;

						illumination *= desintegration;
					}

					auto pixel = couleur / poids_total * exposition;
					pixel.a = 1.0f;
					tampon->valeur(dls::math::vec2i(x, y)) = pixel;
				}
			}

			auto delta = static_cast<float>(plage.end() - plage.begin());
			delta /= static_cast<float>(hauteur);
			chef->indique_progression_parallele(delta * 100.0f);
		});

		chef->indique_progression(100.0f);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_region(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceAnalyse>());
	usine.enregistre_type(cree_desc<OperatriceFiltrage>());
	usine.enregistre_type(cree_desc<OperatriceFloutage>());
	usine.enregistre_type(cree_desc<OperatriceChampsDistance>());
	usine.enregistre_type(cree_desc<OperatriceNormalisationRegion>());
	usine.enregistre_type(cree_desc<OperatriceTournoiement>());
	usine.enregistre_type(cree_desc<OperatriceDeformation>());
	usine.enregistre_type(cree_desc<OperatriceSimulationGrain>());
	usine.enregistre_type(cree_desc<OperatriceCoordonneesPolaires>());
	usine.enregistre_type(cree_desc<OperatriceOndeletteHaar>());
	usine.enregistre_type(cree_desc<OperatriceDilation>());
	usine.enregistre_type(cree_desc<OperatriceErosion>());
	usine.enregistre_type(cree_desc<OperatriceExtractionPalette>());
	usine.enregistre_type(cree_desc<OperatricePrefiltreCubic>());
	usine.enregistre_type(cree_desc<OpRayonsSoleil>());
}

#pragma clang diagnostic pop
