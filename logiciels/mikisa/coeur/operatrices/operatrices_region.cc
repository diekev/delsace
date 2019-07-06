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

#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/outils/parallelisme.h"

#include "bibloc/tableau.hh"

#include "../contexte_evaluation.hh"
#include "../operatrice_image.h"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

template <typename TypeOperation>
void applique_fonction(type_image &image, TypeOperation &&op)
{
	auto const res_x = image.nombre_colonnes();
	auto const res_y = image.nombre_lignes();

	boucle_parallele(tbb::blocked_range<int>(0, res_y),
					 [&](tbb::blocked_range<int> const &plage)
	{
		for (int l = plage.begin(); l < plage.end(); ++l) {
			for (int c = 0; c < res_x; ++c) {
				image[l][c] = op(image[l][c]);
			}
		}
	});
}

template <typename TypeOperation>
void applique_fonction_position(type_image &image, TypeOperation &&op)
{
	auto const res_x = image.nombre_colonnes();
	auto const res_y = image.nombre_lignes();

	boucle_parallele(tbb::blocked_range<int>(0, res_y),
					 [&](tbb::blocked_range<int> const &plage)
	{
		for (int l = plage.begin(); l < plage.end(); ++l) {
			for (int c = 0; c < res_x; ++c) {
				image[l][c] = op(image[l][c], l, c);
			}
		}
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
		/* Call node upstream; */
		entree(0)->requiers_image(m_image, contexte, donnees_aval);

		auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
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

		auto image_tampon = type_image(tampon->tampon.dimensions());

		applique_fonction_position(image_tampon,
								   [&](dls::image::PixelFloat const &/*pixel*/, int l, int c)
		{
			auto resultat = dls::image::PixelFloat();
			resultat.a = 1.0f;

			if (filtre == ANALYSE_GRADIENT) {
				if (dir == DIRECTION_X) {
					auto px0 = tampon->valeur(static_cast<size_t>(c - dx0), static_cast<size_t>(l));
					auto px1 = tampon->valeur(static_cast<size_t>(c + dx1), static_cast<size_t>(l));

					resultat.r = (px1.r - px0.r);
					resultat.g = (px1.g - px0.g);
					resultat.b = (px1.b - px0.b);
				}
				else if (dir == DIRECTION_Y) {
					auto py0 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l - dy0));
					auto py1 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l + dy1));

					resultat.r = (py1.r - py0.r);
					resultat.g = (py1.g - py0.g);
					resultat.b = (py1.b - py0.b);
				}
				else if (dir == DIRECTION_XY) {
					auto px0 = tampon->valeur(static_cast<size_t>(c - dx0), static_cast<size_t>(l));
					auto px1 = tampon->valeur(static_cast<size_t>(c + dx1), static_cast<size_t>(l));
					auto py0 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l - dy0));
					auto py1 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l + dy1));

					resultat.r = ((px1.r - px0.r) + (py1.r - py0.r)) * 0.5f;
					resultat.g = ((px1.g - px0.g) + (py1.g - py0.g)) * 0.5f;
					resultat.b = ((px1.b - px0.b) + (py1.b - py0.b)) * 0.5f;
				}
			}
			else if (filtre == ANALYSE_DIVERGENCE) {
				auto valeur = 0.0f;

				if (dir == DIRECTION_X) {
					auto px0 = tampon->valeur(static_cast<size_t>(c - dx0), static_cast<size_t>(l));
					auto px1 = tampon->valeur(static_cast<size_t>(c + dx1), static_cast<size_t>(l));

					valeur = (px1.r - px0.r) + (px1.g - px0.g) + (px1.b - px0.b);

				}
				else if (dir == DIRECTION_Y) {
					auto py0 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l - dy0));
					auto py1 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l + dy1));

					valeur = (py1.r - py0.r) + (py1.g - py0.g) + (py1.b - py0.b);
				}
				else if (dir == DIRECTION_XY) {
					auto px0 = tampon->valeur(static_cast<size_t>(c - dx0), static_cast<size_t>(l));
					auto px1 = tampon->valeur(static_cast<size_t>(c + dx1), static_cast<size_t>(l));
					auto py0 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l - dy0));
					auto py1 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l + dy1));

					valeur = ((px1.r - px0.r) + (py1.r - py0.r)) * 0.5f
							 + ((px1.g - px0.g) + (py1.g - py0.g)) * 0.5f
							 + ((px1.b - px0.b) + (py1.b - py0.b)) * 0.5f;
				}

				resultat.r = valeur;
				resultat.g = valeur;
				resultat.b = valeur;
			}
			else if (filtre == ANALYSE_LAPLACIEN) {
				auto px  = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l));
				if (dir == DIRECTION_X) {
					auto px0 = tampon->valeur(static_cast<size_t>(c - dx0), static_cast<size_t>(l));
					auto px1 = tampon->valeur(static_cast<size_t>(c + dx1), static_cast<size_t>(l));

					resultat.r = px1.r + px0.r - px.r * 2.0f;
					resultat.g = px1.g + px0.g - px.g * 2.0f;
					resultat.b = px1.b + px0.b - px.b * 2.0f;
				}
				else if (dir == DIRECTION_Y) {
					auto py0 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l - dy0));
					auto py1 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l + dy1));

					resultat.r = py1.r + py0.r - px.r * 2.0f;
					resultat.g = py1.g + py0.g - px.g * 2.0f;
					resultat.b = py1.b + py0.b - px.b * 2.0f;
				}
				else if (dir == DIRECTION_XY) {
					auto px0 = tampon->valeur(static_cast<size_t>(c - dx0), static_cast<size_t>(l));
					auto px1 = tampon->valeur(static_cast<size_t>(c + dx1), static_cast<size_t>(l));
					auto py0 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l - dy0));
					auto py1 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l + dy1));

					resultat.r = px1.r + px0.r + py1.r + py0.r - px.r * 4.0f;
					resultat.g = px1.g + px0.g + py1.g + py0.g - px.g * 4.0f;
					resultat.b = px1.b + px0.b + py1.b + py0.b - px.b * 4.0f;
				}
			}
			else if (filtre == ANALYSE_COURBE) {
				auto px  = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l));
				auto gradient = dls::image::PixelFloat();

				if (dir == DIRECTION_X) {
					auto px0 = tampon->valeur(static_cast<size_t>(c - dx0), static_cast<size_t>(l));
					auto px1 = tampon->valeur(static_cast<size_t>(c + dx1), static_cast<size_t>(l));

					gradient.r = (px1.r - px0.r);
					gradient.g = (px1.g - px0.g);
					gradient.b = (px1.b - px0.b);
				}
				else if (dir == DIRECTION_Y) {
					auto py0 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l - dy0));
					auto py1 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l + dy1));

					gradient.r = (py1.r - py0.r);
					gradient.g = (py1.g - py0.g);
					gradient.b = (py1.b - py0.b);
				}
				else if (dir == DIRECTION_XY) {
					auto px0 = tampon->valeur(static_cast<size_t>(c - dx0), static_cast<size_t>(l));
					auto px1 = tampon->valeur(static_cast<size_t>(c + dx1), static_cast<size_t>(l));
					auto py0 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l - dy0));
					auto py1 = tampon->valeur(static_cast<size_t>(c), static_cast<size_t>(l + dy1));

					gradient.r = ((px1.r - px0.r) + (py1.r - py0.r)) * 0.5f;
					gradient.g = ((px1.g - px0.g) + (py1.g - py0.g)) * 0.5f;
					gradient.b = ((px1.b - px0.b) + (py1.b - py0.b)) * 0.5f;
				}

				resultat.r = gradient.g * px.b - gradient.b * px.g;
				resultat.g = gradient.b * px.r - gradient.r * px.b;
				resultat.b = gradient.r * px.g - gradient.g * px.r;
			}

			return resultat;
		});

		tampon->tampon = image_tampon;

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
		/* Call node upstream. */
		entree(0)->requiers_image(m_image, contexte, donnees_aval);

		auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

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

		tampon->tampon = dls::image::operation::applique_convolution(tampon->tampon, filtre);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

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
		entree(0)->requiers_image(m_image, contexte, donnees_aval);

		auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		auto maximum = dls::image::operation::valeur_maximale(tampon->tampon);
		maximum.r = (maximum.r > 0.0f) ? (1.0f / maximum.r) : 0.0f;
		maximum.g = (maximum.g > 0.0f) ? (1.0f / maximum.g) : 0.0f;
		maximum.b = (maximum.b > 0.0f) ? (1.0f / maximum.b) : 0.0f;
		maximum.a = (maximum.a > 0.0f) ? (1.0f / maximum.a) : 0.0f;

		applique_fonction(tampon->tampon,
						  [&](dls::image::Pixel<float> const &pixel)
		{
			return maximum * pixel;
		});

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

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
		entree(0)->requiers_image(m_image, contexte, donnees_aval);

		auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		auto const largeur = static_cast<size_t>(tampon->tampon.nombre_colonnes());
		auto const hauteur = static_cast<size_t>(tampon->tampon.nombre_lignes());

		dls::math::matrice_dyn<dls::image::Pixel<float>> image_tmp(tampon->tampon.dimensions());

		auto const rayon_flou = evalue_decimal("rayon", contexte.temps_courant);
		auto const type_flou = evalue_enum("type");

		/* construit le kernel du flou */
		auto rayon = rayon_flou;

		if (type_flou == "gaussien") {
			rayon = rayon * 2.57f;
		}

		rayon = std::ceil(rayon);

		auto poids = 0.0f;
		dls::tableau<float> kernel(static_cast<long>(2.0f * rayon + 1.0f));

		if (type_flou == "boîte") {
			for (auto i = static_cast<long>(-rayon), k = 0l; i < static_cast<long>(rayon) + 1; ++i, ++k) {
				kernel[k] = 1.0f;
				poids += kernel[k];
			}
		}
		else if (type_flou == "gaussien") {
			for (auto i = static_cast<long>(-rayon), k = 0l; i < static_cast<long>(rayon) + 1; ++i, ++k) {
				kernel[k] = std::exp(-static_cast<float>(i * i) / (2.0f * rayon_flou * rayon_flou)) / (constantes<float>::TAU * rayon_flou * rayon_flou);
				poids += kernel[k];
			}
		}

		poids = (poids != 0.0f) ? 1.0f / poids : 1.0f;

		/* flou horizontal */
		applique_fonction_position(image_tmp,
								   [&](dls::image::Pixel<float> const &/*pixel*/, int ye, int xe)
		{
			auto x = static_cast<long>(xe);
			auto y = static_cast<long>(ye);
			dls::image::Pixel<float> valeur;
			valeur.r = 0.0f;
			valeur.g = 0.0f;
			valeur.b = 0.0f;
			valeur.a = tampon->valeur(static_cast<size_t>(x), static_cast<size_t>(y)).a;

			for (auto ix = x - static_cast<long>(rayon), k = 0l; ix < x + static_cast<long>(rayon) + 1; ix++, ++k) {
				auto const xx = std::min(static_cast<long>(largeur - 1), std::max(0l, ix));
				auto const &p = tampon->valeur(static_cast<size_t>(xx), static_cast<size_t>(y));
				valeur.r += p.r * kernel[k];
				valeur.g += p.g * kernel[k];
				valeur.b += p.b * kernel[k];
			}

			valeur.r *= poids;
			valeur.g *= poids;
			valeur.b *= poids;

			return valeur;
		});

		tampon->tampon.swap(image_tmp);

		/* flou vertical */
		applique_fonction_position(image_tmp,
								   [&](dls::image::Pixel<float> const &/*pixel*/, int ye, int xe)
		{
			auto x = static_cast<long>(xe);
			auto y = static_cast<long>(ye);
			dls::image::Pixel<float> valeur;
			valeur.r = 0.0f;
			valeur.g = 0.0f;
			valeur.b = 0.0f;
			valeur.a = tampon->valeur(static_cast<size_t>(x), static_cast<size_t>(y)).a;

			for (auto iy = y - static_cast<long>(rayon), k = 0l; iy < y + static_cast<long>(rayon) + 1; iy++, ++k) {
				auto const yy = std::min(static_cast<long>(hauteur - 1), std::max(0l, iy));
				auto const &p = tampon->valeur(static_cast<size_t>(x), static_cast<size_t>(yy));
				valeur.r += p.r * kernel[k];
				valeur.g += p.g * kernel[k];
				valeur.b += p.b * kernel[k];
			}

			valeur.r *= poids;
			valeur.g *= poids;
			valeur.b *= poids;

			return valeur;
		});

		tampon->tampon.swap(image_tmp);

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
		entree(0)->requiers_image(m_image, contexte, donnees_aval);

		auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		auto const &hauteur = tampon->tampon.nombre_lignes();
		auto const &largeur = tampon->tampon.nombre_colonnes();
		auto const &hauteur_inverse = 1.0f / static_cast<float>(hauteur);
		auto const &largeur_inverse = 1.0f / static_cast<float>(largeur);

		auto const decalage_x = evalue_decimal("décalage_x", contexte.temps_courant);
		auto const decalage_y = evalue_decimal("décalage_y", contexte.temps_courant);
		auto const taille = evalue_decimal("taille", contexte.temps_courant);
		auto const periodes = evalue_decimal("périodes", contexte.temps_courant);

		auto image_tampon = type_image(tampon->tampon.dimensions());

		applique_fonction_position(image_tampon,
								   [&](dls::image::PixelFloat const &/*pixel*/, int l, int c)
		{
			auto const fc = static_cast<float>(c) * largeur_inverse + decalage_x;
			auto const fl = static_cast<float>(l) * hauteur_inverse + decalage_y;

			auto const rayon = std::hypot(fc, fl) * taille;
			auto const angle = std::atan2(fl, fc) * periodes + rayon;

			auto nc = rayon * std::cos(angle) * static_cast<float>(largeur) + 0.5f;
			auto nl = rayon * std::sin(angle) * static_cast<float>(hauteur) + 0.5f;

			return tampon->echantillone(nc, nl);
		});

		tampon->tampon = image_tampon;

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static void calcule_distance(
		dls::math::matrice_dyn<float> &phi,
		int x, int y, float h)
{
	auto a = std::min(phi[y][x - 1], phi[y][x + 1]);
	auto b = std::min(phi[y - 1][x], phi[y + 1][x]);
	auto xi = 0.0f;

	if (std::abs(a - b) >= h) {
		xi = std::min(a, b) + h;
	}
	else {
		xi =  0.5f * (a + b + std::sqrt(2.0f * h * h - (a - b) * (a - b)));
	}

	phi[y][x] = std::min(phi[y][x], xi);
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
		entree(0)->requiers_image(m_image, contexte, donnees_aval);

		auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		auto const methode = evalue_enum("méthode");
		auto image_grise = dls::image::operation::luminance(tampon->tampon);
		auto image_tampon = dls::math::matrice_dyn<float>(tampon->tampon.dimensions());
		auto valeur_iso = evalue_decimal("valeur_iso", contexte.temps_courant);

		/* À FAIRE :  deux versions de chaque algorithme : signée et non-signée.
		 * cela demandrait de garder trace de deux grilles pour la version
		 * non-signée, comme pour l'algorithme 8SSEDT.
		 */

		if (methode == "balayage_rapide") {
			int res_x = tampon->tampon.nombre_colonnes();
			int res_y = tampon->tampon.nombre_lignes();

			auto h = std::min(1.0f / static_cast<float>(res_x), 1.0f / static_cast<float>(res_y));

			for (int y = 0; y < res_y; ++y) {
				for (int x = 0; x < res_x; ++x) {
					if (image_grise[y][x] > valeur_iso) {
						image_tampon[y][x] = 1.0f;
					}
					else {
						image_tampon[y][x] = 0.0f;
					}
				}
			}

			/* bas-droite */
			for (int x = 1; x < res_x - 1; ++x) {
				for (int y = 1; y < res_y - 1; ++y) {
					if (image_tampon[y][x] != 0.0f) {
						calcule_distance(image_tampon, x, y, h);
					}
				}
			}

			/* bas-gauche */
			for (int x = res_x - 2; x >= 1; --x) {
				for (int y = 1; y < res_y - 1; ++y) {
					if (image_tampon[y][x] != 0.0f) {
						calcule_distance(image_tampon, x, y, h);
					}
				}
			}

			/* haut-gauche */
			for (int x = res_x - 2; x >= 1; --x) {
				for (int y = res_y - 2; y >= 1; --y) {
					if (image_tampon[y][x] != 0.0f) {
						calcule_distance(image_tampon, x, y, h);
					}
				}
			}

			/* haut-droite */
			for (int x = 1; x < res_x - 1; ++x) {
				for (int y = res_y - 2; y >= 1; --y) {
					if (image_tampon[y][x] != 0.0f) {
						calcule_distance(image_tampon, x, y, h);
					}
				}
			}
		}
		else if (methode == "estime") {
			dls::image::operation::navigation_estime::genere_champs_distance(image_grise, image_tampon, valeur_iso);
		}
		else if (methode == "8SSEDT") {
			dls::image::operation::ssedt::genere_champs_distance(image_grise, image_tampon, valeur_iso);
		}

		/* À FAIRE : alpha ou tampon à canaux variables */
		tampon->tampon = dls::image::operation::converti_float_pixel(image_tampon);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static auto moyenne(dls::image::Pixel<float> const &pixel)
{
	return (pixel.r + pixel.g + pixel.b) * 0.3333f;
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
		entree(0)->requiers_image(m_image, contexte, donnees_aval);

		auto nom_calque_a = evalue_chaine("nom_calque_a");
		auto tampon = m_image.calque(nom_calque_a);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque A introuvable !");
			return EXECUTION_ECHOUEE;
		}

		Image image2;
		entree(1)->requiers_image(image2, contexte, donnees_aval);

		auto nom_calque_b = evalue_chaine("nom_calque_b");
		auto tampon2 = image2.calque(nom_calque_b);

		if (tampon2 == nullptr) {
			ajoute_avertissement("Calque B introuvable !");
			return EXECUTION_ECHOUEE;
		}

		int res_x = tampon->tampon.nombre_colonnes();
		int res_y = tampon->tampon.nombre_lignes();

		auto image_tampon = type_image(tampon->tampon.dimensions());

		applique_fonction_position(image_tampon,
								   [&](dls::image::PixelFloat const &/*pixel*/, int l, int c)
		{
			auto const c0 = std::min(res_x - 1, std::max(0, c - 1));
			auto const c1 = std::min(res_x - 1, std::max(0, c + 1));
			auto const l0 = std::min(res_y - 1, std::max(0, l - 1));
			auto const l1 = std::min(res_y - 1, std::max(0, l + 1));

			auto const x0 = moyenne(tampon2->tampon[l][c0]);
			auto const x1 = moyenne(tampon2->tampon[l][c1]);
			auto const y0 = moyenne(tampon2->tampon[l0][c]);
			auto const y1 = moyenne(tampon2->tampon[l1][c]);

			auto const pos_x = x1 - x0;
			auto const pos_y = y1 - y0;

			auto const x = static_cast<float>(c) + pos_x * static_cast<float>(res_x);
			auto const y = static_cast<float>(l) + pos_y * static_cast<float>(res_y);

			return tampon->echantillone(x, y);
		});

		tampon->tampon = image_tampon;

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

using type_image_grise = dls::math::matrice_dyn<float>;

static type_image_grise extrait_canal(type_image const &image, const int chaine)
{
	type_image_grise resultat(image.dimensions());

	boucle_parallele(tbb::blocked_range<int>(0, image.nombre_lignes()),
					 [&](tbb::blocked_range<int> const &plage)
	{
		for (auto l = plage.begin(); l < plage.end(); ++l) {
			for (auto c = 0; c < image.nombre_colonnes(); ++c) {
				resultat[l][c] = image[l][c][chaine];
			}
		}
	});

	return resultat;
}

static void assemble_image(
		type_image &image,
		type_image_grise const &canal_rouge,
		type_image_grise const &canal_vert,
		type_image_grise const &canal_bleu)
{

	boucle_parallele(tbb::blocked_range<int>(0, image.nombre_lignes()),
					 [&](tbb::blocked_range<int> const &plage)
	{
		for (auto l = plage.begin(); l < plage.end(); ++l) {
			for (auto c = 0; c < image.nombre_colonnes(); ++c) {
				image[l][c].r = canal_rouge[l][c];
				image[l][c].g = canal_vert[l][c];
				image[l][c].b = canal_bleu[l][c];
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
	auto const res_x = image.nombre_colonnes();
	auto const res_y = image.nombre_lignes();

	type_image_grise resultat(image.dimensions());

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
				resultat[j][i] = 0.0f;

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
							auto const u = std::max(0.0f, std::min(1.0f, image[int(coin_y)][int(coin_x)]));
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
									resultat[j][i] += 1.0f;
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

				resultat[j][i] /= iter;
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
		entree(0)->requiers_image(m_image, contexte, donnees_aval);

		auto const &nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

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

		auto const &canal_rouge = extrait_canal(tampon->tampon, 0);
		auto const &canal_vert = extrait_canal(tampon->tampon, 1);
		auto const &canal_bleu = extrait_canal(tampon->tampon, 2);

		auto const &bruit_rouge = simule_grain_image(canal_rouge, graine, rayon_grain_r, sigma_rayon_r, sigma_filtre_r);
		auto const &bruit_vert = simule_grain_image(canal_vert, graine, rayon_grain_v, sigma_rayon_v, sigma_filtre_v);
		auto const &bruit_bleu = simule_grain_image(canal_bleu, graine, rayon_grain_b, sigma_rayon_b, sigma_filtre_b);

		assemble_image(tampon->tampon, bruit_rouge, bruit_vert, bruit_bleu);

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
		entree(0)->requiers_image(m_image, contexte, donnees_aval);

		auto const nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		int res_x = tampon->tampon.nombre_colonnes();
		auto inv_res_x = 1.0f / static_cast<float>(res_x);

		auto image_tampon = type_image(tampon->tampon.dimensions());

		applique_fonction_position(image_tampon,
								   [&](dls::image::PixelFloat const &/*pixel*/, int l, int c)
		{
			/* À FAIRE : image carrée ? */
			auto const r = static_cast<float>(l);
			auto const theta = constantes<float>::TAU * static_cast<float>(c) * inv_res_x;
			auto const x = r * std::cos(theta);
			auto const y = r * std::sin(theta);
			return tampon->echantillone(x, y);
		});

		tampon->tampon = image_tampon;

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
		entree(0)->requiers_image(m_image, contexte, donnees_aval);

		auto const nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		auto const iterations = evalue_entier("itérations");
		auto res_x = tampon->tampon.nombre_colonnes();
		auto res_y = tampon->tampon.nombre_lignes();

		auto image_tampon = type_image(tampon->tampon.dimensions());

		auto const coeff = 1.0f / std::sqrt(2.0f);

		for (int i = 0; i < iterations; ++i) {
			/* transformation verticale */
			auto const taille_y = res_y / 2;
			for (int y = 0; y < taille_y; ++y) {
				for (int x = 0; x < res_x; ++x) {
					auto const p0 = tampon->valeur(static_cast<size_t>(x), static_cast<size_t>(y) * 2);
					auto const p1 = tampon->valeur(static_cast<size_t>(x), static_cast<size_t>(y) * 2 + 1);
					auto somme = (p0 + p1) * coeff;
					auto diff = (p0 - p1) * coeff;

					somme.a = 1.0f;
					diff.a = 1.0f;

					image_tampon[y][x] = somme;
					image_tampon[taille_y + y][x] = diff;
				}
			}

			tampon->tampon = image_tampon;

			/* transformation horizontale */
			auto const taille_x = res_x / 2;
			for (int y = 0; y < res_y; ++y) {
				for (int x = 0; x < taille_x; ++x) {
					auto const p0 = tampon->valeur(static_cast<size_t>(x) * 2, static_cast<size_t>(y));
					auto const p1 = tampon->valeur(static_cast<size_t>(x) * 2 + 1, static_cast<size_t>(y));
					auto somme = (p0 + p1) * coeff;
					auto diff = (p0 - p1) * coeff;

					somme.a = 1.0f;
					diff.a = 1.0f;

					image_tampon[y][x] = somme;
					image_tampon[y][taille_x + x] = diff;
				}
			}

			tampon->tampon = image_tampon;

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
		entree(0)->requiers_image(m_image, contexte, donnees_aval);

		auto const nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		auto const res_x = tampon->tampon.nombre_colonnes();
		auto const res_y = tampon->tampon.nombre_lignes();

		auto image_tampon = type_image(tampon->tampon.dimensions());
		auto const rayon = evalue_entier("rayon");

		auto performe_dilation = [&](
								 dls::image::Pixel<float> const &/*pixel*/,
								 int y,
								 int x)
		{
			dls::image::Pixel<float> p0(0.0f);
			p0.a = 1.0f;

			auto const debut_x = std::max(0, x - rayon);
			auto const debut_y = std::max(0, y - rayon);
			auto const fin_x = std::min(res_x, x + rayon);
			auto const fin_y = std::min(res_y, y + rayon);

			for (int sy = debut_y; sy < fin_y; ++sy) {
				for (int sx = debut_x; sx < fin_x; ++sx) {
					auto const p1 = tampon->valeur(static_cast<size_t>(sx), static_cast<size_t>(sy));

					p0.r = std::max(p0.r, p1.r);
					p0.g = std::max(p0.g, p1.g);
					p0.b = std::max(p0.b, p1.b);
				}
			}

			return p0;
		};

		applique_fonction_position(image_tampon, performe_dilation);

		tampon->tampon = image_tampon;

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
		entree(0)->requiers_image(m_image, contexte, donnees_aval);

		auto const nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		auto const res_x = tampon->tampon.nombre_colonnes();
		auto const res_y = tampon->tampon.nombre_lignes();

		auto image_tampon = type_image(tampon->tampon.dimensions());
		auto const rayon = evalue_entier("rayon");

		auto performe_erosion = [&](
								dls::image::Pixel<float> const &/*pixel*/,
								int y,
								int x)
		{
			dls::image::Pixel<float> p0(1.0f);

			auto const debut_x = std::max(0, x - rayon);
			auto const debut_y = std::max(0, y - rayon);
			auto const fin_x = std::min(res_x, x + rayon);
			auto const fin_y = std::min(res_y, y + rayon);

			for (int sy = debut_y; sy < fin_y; ++sy) {
				for (int sx = debut_x; sx < fin_x; ++sx) {
					auto const p1 = tampon->valeur(static_cast<size_t>(sx), static_cast<size_t>(sy));

					p0.r = std::min(p0.r, p1.r);
					p0.g = std::min(p0.g, p1.g);
					p0.b = std::min(p0.b, p1.b);
				}
			}

			return p0;
		};

		applique_fonction_position(image_tampon, performe_erosion);

		tampon->tampon = image_tampon;

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
		entree(0)->requiers_image(m_image, contexte, donnees_aval);

		auto const nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		auto const res_x = tampon->tampon.nombre_colonnes();
		auto const res_y = tampon->tampon.nombre_lignes();

		auto image_tampon = type_image(tampon->tampon.dimensions());

		using pixel_t = dls::image::Pixel<float>;
		using paire_pixel_t = std::pair<pixel_t, int>;

		dls::tableau<paire_pixel_t> histogramme(360ul);

		for (auto &paire : histogramme) {
			paire.first = pixel_t(0.0f);
			paire.second = 0;
		}

		for (int y = 0; y < res_y; ++y) {
			for (int x = 0; x < res_x; ++x) {
				auto const &pixel = tampon->tampon[y][x];
				auto res = pixel_t();
				res.a = 1;
				dls::phys::rvb_vers_hsv(pixel.r, pixel.g, pixel.b, &res.r, &res.g, &res.b);

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
				image_tampon[l][c] = histogramme[index].first;
			}
		}

		tampon->tampon = image_tampon;

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
		dls::image::Pixel<float> *pixel,
		dls::image::Pixel<float> &somme,
		int longueur)
{
	auto const Zp = std::sqrt(3.0f) - 2.0f;
	auto const lambda = (1.0f - Zp) * (1.0f - 1.0f / Zp);
	auto horizon = std::min(12, longueur);
	auto Zn = Zp;

	somme.r = pixel[0].r;
	somme.g = pixel[0].g;
	somme.b = pixel[0].b;
	somme.a = pixel[0].a;

	for (auto compte = 0; compte < horizon; ++compte) {
		somme.r += Zn * pixel[compte].r;
		somme.g += Zn * pixel[compte].g;
		somme.b += Zn * pixel[compte].b;
		somme.a += Zn * pixel[compte].a;

		Zn *= Zp;
	}

	pixel[0].r = lambda * somme.r;
	pixel[0].g = lambda * somme.g;
	pixel[0].b = lambda * somme.b;
	pixel[0].a = lambda * somme.a;
}

static void initialise_anticausal_prefiltre(
		dls::image::Pixel<float> *pixel)
{
	auto const Zp  = std::sqrt(3.0f) - 2.0f;
	auto const iZp = (Zp / (Zp - 1.0f));

	pixel[0].r = iZp * pixel[0].r;
	pixel[0].g = iZp * pixel[0].g;
	pixel[0].b = iZp * pixel[0].b;
	pixel[0].a = iZp * pixel[0].a;
}

static void recursion_prefiltre(dls::image::Pixel<float> *pixel, int longueur, int stride)
{
	auto const Zp = std::sqrt(3.0f) - 2.0f;
	auto const lambda = (1.0f - Zp) * (1.0f - 1.0f / Zp);
	auto somme = dls::image::Pixel<float>{};
	auto compte = 0;

	initialise_causal_prefiltre(pixel, somme, longueur);

	auto prev_coeff = pixel[0];

	for (compte = stride; compte < longueur; ++compte) {
		pixel[compte].r = prev_coeff.r = (lambda * pixel[compte].r) + (Zp * prev_coeff.r);
		pixel[compte].g = prev_coeff.g = (lambda * pixel[compte].g) + (Zp * prev_coeff.g);
		pixel[compte].b = prev_coeff.b = (lambda * pixel[compte].b) + (Zp * prev_coeff.b);
		pixel[compte].a = prev_coeff.a = (lambda * pixel[compte].a) + (Zp * prev_coeff.a);
	}

	compte -= stride;

	initialise_anticausal_prefiltre(&pixel[compte]);

	prev_coeff = pixel[compte];

	for (compte -= stride; compte >= 0; compte -= stride) {
		pixel[compte].r = prev_coeff.r = Zp * (prev_coeff.r - pixel[compte].r);
		pixel[compte].g = prev_coeff.g = Zp * (prev_coeff.g - pixel[compte].g);
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
		entree(0)->requiers_image(m_image, contexte, donnees_aval);

		auto const nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		auto const res_x = tampon->tampon.nombre_colonnes();
		auto const res_y = tampon->tampon.nombre_lignes();

		if (res_x <= 2 || res_y <= 2) {
			this->ajoute_avertissement("L'image d'entrée est trop petite");
			return EXECUTION_ECHOUEE;
		}

		for (auto y = 0; y < res_y; ++y) {
			recursion_prefiltre(tampon->tampon[y], res_x, 1);
		}

		for (auto x = 0; x < res_x; ++x) {
			recursion_prefiltre(&tampon->tampon[0][x], res_y, res_x);
		}

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
}

#pragma clang diagnostic pop
