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

#include <random>

#include <numero7/image/operations/champs_distance.h>
#include <numero7/image/operations/conversion.h>
#include <numero7/image/operations/convolution.h>
#include <numero7/image/operations/operations.h>

#include <numero7/math/matrice/operations.h>
#include <numero7/math/outils.h>

#include "bibliotheques/outils/constantes.h"
#include "bibliotheques/outils/parallelisme.h"

#include "../operatrice_image.h"
#include "../usine_operatrice.h"

/* ************************************************************************** */

template <typename TypeOperation>
void applique_fonction(type_image &image, TypeOperation &&op)
{
	const auto res_x = image.nombre_colonnes();
	const auto res_y = image.nombre_lignes();

	boucle_parallele(tbb::blocked_range<size_t>(0, res_y),
					 [&](const tbb::blocked_range<size_t> &plage)
	{
		for (size_t l = plage.begin(); l < plage.end(); ++l) {
			for (size_t c = 0; c < res_x; ++c) {
				image[l][c] = op(image[l][c]);
			}
		}
	});
}

template <typename TypeOperation>
void applique_fonction_position(type_image &image, TypeOperation &&op)
{
	const auto res_x = image.nombre_colonnes();
	const auto res_y = image.nombre_lignes();

	boucle_parallele(tbb::blocked_range<size_t>(0, res_y),
					 [&](const tbb::blocked_range<size_t> &plage)
	{
		for (size_t l = plage.begin(); l < plage.end(); ++l) {
			for (size_t c = 0; c < res_x; ++c) {
				image[l][c] = op(image[l][c], l, c);
			}
		}
	});
}

/* ************************************************************************** */

static constexpr auto NOM_ANALYSE = "Analyse";
static constexpr auto AIDE_ANALYSE = "Analyse l'image.";

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
	explicit OperatriceAnalyse(Noeud *node)
		: OperatriceImage(node)
	{
		inputs(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_analyse.jo";
	}

	const char *class_name() const override
	{
		return NOM_ANALYSE;
	}

	const char *help_text() const override
	{
		return AIDE_ANALYSE;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		/* Call node upstream; */
		input(0)->requiers_image(m_image, rectangle, temps);

		auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		const auto operation = evalue_enum("operation");
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

		const auto direction = evalue_enum("direction");
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

		const auto differentiation = evalue_enum("différentiation");

		if (differentiation == "arrière") {
			dx0 = -dx0;
			dx1 = -dx1;
			dy0 = -dy0;
			dy1 = -dy1;
		}

		auto image_tampon = type_image(tampon->tampon.dimensions());

		applique_fonction_position(image_tampon,
								   [&](const numero7::image::PixelFloat &/*pixel*/, int l, int c)
		{
			auto resultat = numero7::image::PixelFloat();
			resultat.a = 1.0f;

			if (filtre == ANALYSE_GRADIENT) {
				if (dir == DIRECTION_X) {
					auto px0 = tampon->valeur(c - dx0, l);
					auto px1 = tampon->valeur(c + dx1, l);

					resultat.r = (px1.r - px0.r);
					resultat.g = (px1.g - px0.g);
					resultat.b = (px1.b - px0.b);
				}
				else if (dir == DIRECTION_Y) {
					auto py0 = tampon->valeur(c, l - dy0);
					auto py1 = tampon->valeur(c, l + dy1);

					resultat.r = (py1.r - py0.r);
					resultat.g = (py1.g - py0.g);
					resultat.b = (py1.b - py0.b);
				}
				else if (dir == DIRECTION_XY) {
					auto px0 = tampon->valeur(c - dx0, l);
					auto px1 = tampon->valeur(c + dx1, l);
					auto py0 = tampon->valeur(c, l - dy0);
					auto py1 = tampon->valeur(c, l + dy1);

					resultat.r = ((px1.r - px0.r) + (py1.r - py0.r)) * 0.5f;
					resultat.g = ((px1.g - px0.g) + (py1.g - py0.g)) * 0.5f;
					resultat.b = ((px1.b - px0.b) + (py1.b - py0.b)) * 0.5f;
				}
			}
			else if (filtre == ANALYSE_DIVERGENCE) {
				auto valeur = 0.0f;

				if (dir == DIRECTION_X) {
					auto px0 = tampon->valeur(c - dx0, l);
					auto px1 = tampon->valeur(c + dx1, l);

					valeur = (px1.r - px0.r) + (px1.g - px0.g) + (px1.b - px0.b);

				}
				else if (dir == DIRECTION_Y) {
					auto py0 = tampon->valeur(c, l - dy0);
					auto py1 = tampon->valeur(c, l + dy1);

					valeur = (py1.r - py0.r) + (py1.g - py0.g) + (py1.b - py0.b);
				}
				else if (dir == DIRECTION_XY) {
					auto px0 = tampon->valeur(c - dx0, l);
					auto px1 = tampon->valeur(c + dx1, l);
					auto py0 = tampon->valeur(c, l - dy0);
					auto py1 = tampon->valeur(c, l + dy1);

					valeur = ((px1.r - px0.r) + (py1.r - py0.r)) * 0.5f
							 + ((px1.g - px0.g) + (py1.g - py0.g)) * 0.5f
							 + ((px1.b - px0.b) + (py1.b - py0.b)) * 0.5f;
				}

				resultat.r = valeur;
				resultat.g = valeur;
				resultat.b = valeur;
			}
			else if (filtre == ANALYSE_LAPLACIEN) {
				auto px  = tampon->valeur(c, l);
				if (dir == DIRECTION_X) {
					auto px0 = tampon->valeur(c - dx0, l);
					auto px1 = tampon->valeur(c + dx1, l);

					resultat.r = px1.r + px0.r - px.r * 2.0f;
					resultat.g = px1.g + px0.g - px.g * 2.0f;
					resultat.b = px1.b + px0.b - px.b * 2.0f;
				}
				else if (dir == DIRECTION_Y) {
					auto py0 = tampon->valeur(c, l - dy0);
					auto py1 = tampon->valeur(c, l + dy1);

					resultat.r = py1.r + py0.r - px.r * 2.0f;
					resultat.g = py1.g + py0.g - px.g * 2.0f;
					resultat.b = py1.b + py0.b - px.b * 2.0f;
				}
				else if (dir == DIRECTION_XY) {
					auto px0 = tampon->valeur(c - dx0, l);
					auto px1 = tampon->valeur(c + dx1, l);
					auto py0 = tampon->valeur(c, l - dy0);
					auto py1 = tampon->valeur(c, l + dy1);

					resultat.r = px1.r + px0.r + py1.r + py0.r - px.r * 4.0f;
					resultat.g = px1.g + px0.g + py1.g + py0.g - px.g * 4.0f;
					resultat.b = px1.b + px0.b + py1.b + py0.b - px.b * 4.0f;
				}
			}
			else if (filtre == ANALYSE_COURBE) {
				auto px  = tampon->valeur(c, l);
				auto gradient = numero7::image::PixelFloat();

				if (dir == DIRECTION_X) {
					auto px0 = tampon->valeur(c - dx0, l);
					auto px1 = tampon->valeur(c + dx1, l);

					gradient.r = (px1.r - px0.r);
					gradient.g = (px1.g - px0.g);
					gradient.b = (px1.b - px0.b);
				}
				else if (dir == DIRECTION_Y) {
					auto py0 = tampon->valeur(c, l - dy0);
					auto py1 = tampon->valeur(c, l + dy1);

					gradient.r = (py1.r - py0.r);
					gradient.g = (py1.g - py0.g);
					gradient.b = (py1.b - py0.b);
				}
				else if (dir == DIRECTION_XY) {
					auto px0 = tampon->valeur(c - dx0, l);
					auto px1 = tampon->valeur(c + dx1, l);
					auto py0 = tampon->valeur(c, l - dy0);
					auto py1 = tampon->valeur(c, l + dy1);

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

static constexpr auto NOM_FILTRAGE = "Filtre";
static constexpr auto AIDE_FILTRAGE = "Appliquer un filtre à une image.";

class OperatriceFiltrage : public OperatriceImage {
public:
	explicit OperatriceFiltrage(Noeud *node)
		: OperatriceImage(node)
	{
		inputs(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_filtre.jo";
	}

	const char *class_name() const override
	{
		return NOM_FILTRAGE;
	}

	const char *help_text() const override
	{
		return AIDE_FILTRAGE;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		/* Call node upstream. */
		input(0)->requiers_image(m_image, rectangle, temps);

		auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		auto operation = evalue_enum("operation");
		auto filtre = 0;

		if (operation == "netteté") {
			filtre = numero7::image::operation::NOYAU_NETTETE;
		}
		else if (operation == "netteté_autre") {
			filtre = numero7::image::operation::NOYAU_NETTETE_ALT;
		}
		else if (operation == "flou_gaussien") {
			filtre = numero7::image::operation::NOYAU_FLOU_GAUSSIEN;
		}
		else if (operation == "flou") {
			filtre = numero7::image::operation::NOYAU_FLOU_ALT;
		}
		else if (operation == "flou_sans_poids") {
			filtre = numero7::image::operation::NOYAU_FLOU_NON_PONDERE;
		}
		else if (operation == "relief_no") {
			filtre = numero7::image::operation::NOYAU_RELIEF_NO;
		}
		else if (operation == "relief_ne") {
			filtre = numero7::image::operation::NOYAU_RELIEF_NE;
		}
		else if (operation == "relief_so") {
			filtre = numero7::image::operation::NOYAU_RELIEF_SO;
		}
		else if (operation == "relief_se") {
			filtre = numero7::image::operation::NOYAU_RELIEF_SE;
		}
		else if (operation == "sobel_x") {
			filtre = numero7::image::operation::NOYAU_SOBEL_X;
		}
		else if (operation == "sobel_y") {
			filtre = numero7::image::operation::NOYAU_SOBEL_Y;
		}

		tampon->tampon = numero7::image::operation::applique_convolution(tampon->tampon, filtre);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static constexpr auto NOM_NORMALISE = "Normalise";
static constexpr auto AIDE_NORMALISE = "Normalise the image.";

class OperatriceNormalisation : public OperatriceImage {
public:
	explicit OperatriceNormalisation(Noeud *node)
		: OperatriceImage(node)
	{
		inputs(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_normalisation.jo";
	}

	const char *class_name() const override
	{
		return NOM_NORMALISE;
	}

	const char *help_text() const override
	{
		return AIDE_NORMALISE;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		input(0)->requiers_image(m_image, rectangle, temps);

		auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		auto maximum = numero7::image::operation::valeur_maximale(tampon->tampon);
		maximum.r = (maximum.r > 0.0f) ? (1.0f / maximum.r) : 0.0f;
		maximum.g = (maximum.g > 0.0f) ? (1.0f / maximum.g) : 0.0f;
		maximum.b = (maximum.b > 0.0f) ? (1.0f / maximum.b) : 0.0f;
		maximum.a = (maximum.a > 0.0f) ? (1.0f / maximum.a) : 0.0f;

		applique_fonction(tampon->tampon,
						  [&](const numero7::image::Pixel<float> &pixel)
		{
			return maximum * pixel;
		});

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static constexpr auto NOM_FLOU = "Flou";
static constexpr auto AIDE_FLOU = "Applique un flou à l'image.";

class OperatriceFloutage : public OperatriceImage {
public:
	explicit OperatriceFloutage(Noeud *node)
		: OperatriceImage(node)
	{
		inputs(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_flou.jo";
	}

	const char *class_name() const override
	{
		return NOM_FLOU;
	}

	const char *help_text() const override
	{
		return AIDE_FLOU;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		input(0)->requiers_image(m_image, rectangle, temps);

		auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		const auto largeur = tampon->tampon.nombre_colonnes();
		const auto hauteur = tampon->tampon.nombre_lignes();

		numero7::math::matrice<numero7::image::Pixel<float>> image_tmp(tampon->tampon.dimensions());

		const auto rayon_flou = evalue_decimal("rayon", temps);
		const auto type_flou = evalue_enum("type");

		/* construit le kernel du flou */
		auto rayon = rayon_flou;

		if (type_flou == "gaussien") {
			rayon = rayon * 2.57f;
		}

		rayon = std::ceil(rayon);

		auto poids = 0.0f;
		std::vector<float> kernel(2 * rayon + 1);

		if (type_flou == "boîte") {
			for (int i = -rayon, k = 0; i < rayon + 1; ++i, ++k) {
				kernel[k] = 1.0f;
				poids += kernel[k];
			}
		}
		else if (type_flou == "gaussien") {
			for (int i = -rayon, k = 0; i < rayon + 1; ++i, ++k) {
				kernel[k] = std::exp(-(i * i) / (2.0f * rayon_flou * rayon_flou)) / (TAU * rayon_flou * rayon_flou);
				poids += kernel[k];
			}
		}

		poids = (poids != 0.0f) ? 1.0f / poids : 1.0f;

		/* flou horizontal */
		applique_fonction_position(image_tmp,
								   [&](const numero7::image::Pixel<float> &/*pixel*/, int y, int x)
		{
			numero7::image::Pixel<float> valeur;
			valeur.r = 0.0f;
			valeur.g = 0.0f;
			valeur.b = 0.0f;
			valeur.a = tampon->valeur(x, y).a;

			for (int ix = x - rayon, k = 0; ix < x + rayon + 1; ix++, ++k) {
				const auto xx = std::min(largeur - 1, std::max(0, ix));
				const auto &p = tampon->valeur(xx, y);
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
								   [&](const numero7::image::Pixel<float> &/*pixel*/, int y, int x)
		{
			numero7::image::Pixel<float> valeur;
			valeur.r = 0.0f;
			valeur.g = 0.0f;
			valeur.b = 0.0f;
			valeur.a = tampon->valeur(x, y).a;

			for (int iy = y - rayon, k = 0; iy < y + rayon + 1; iy++, ++k) {
				const auto yy = std::min(hauteur - 1, std::max(0, iy));
				const auto &p = tampon->valeur(x, yy);
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

static constexpr auto NOM_TOURNOIEMENT = "Tournoiement";
static constexpr auto AIDE_TOURNOIEMENT = "Applique un tournoiement aux pixels de l'image.";

class OperatriceTournoiement : public OperatriceImage {
public:
	explicit OperatriceTournoiement(Noeud *node)
		: OperatriceImage(node)
	{
		inputs(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_tournoiement.jo";
	}

	const char *class_name() const override
	{
		return NOM_TOURNOIEMENT;
	}

	const char *help_text() const override
	{
		return AIDE_TOURNOIEMENT;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		input(0)->requiers_image(m_image, rectangle, temps);

		auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		const auto &hauteur = tampon->tampon.nombre_lignes();
		const auto &largeur = tampon->tampon.nombre_colonnes();
		const auto &hauteur_inverse = 1.0f / hauteur;
		const auto &largeur_inverse = 1.0f / largeur;

		const auto decalage_x = evalue_decimal("décalage_x", temps);
		const auto decalage_y = evalue_decimal("décalage_y", temps);
		const auto taille = evalue_decimal("taille", temps);
		const auto periodes = evalue_decimal("périodes", temps);

		auto image_tampon = type_image(tampon->tampon.dimensions());

		applique_fonction_position(image_tampon,
								   [&](const numero7::image::PixelFloat &/*pixel*/, int l, int c)
		{
			const auto fc = c * largeur_inverse + decalage_x;
			const auto fl = l * hauteur_inverse + decalage_y;

			const auto rayon = hypot(fc, fl) * taille;
			const auto angle = std::atan2(fl, fc) * periodes + rayon;

			auto nc = rayon * std::cos(angle) * largeur + 0.5f;
			auto nl = rayon * std::sin(angle) * hauteur + 0.5f;

			return tampon->echantillone(nc, nl);
		});

		tampon->tampon = image_tampon;

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static constexpr auto NOM_CHAMPS_DISTANCE = "Champs distance";
static constexpr auto AIDE_CHAMPS_DISTANCE = "Calcule le champs de distance de l'image d'entrée.";

static void calcule_distance(
		numero7::math::matrice<float> &phi,
		size_t x, size_t y, float h)
{
	auto a = std::min(phi[y][x - 1], phi[y][x + 1]);
	auto b = std::min(phi[y - 1][x], phi[y + 1][x]);
	auto xi = 0.0f;

	if (std::abs(a - b) >= h) {
		xi = std::min(a, b) + h;
	}
	else {
		xi =  0.5f * (a + b + sqrt(2.0f * h * h - (a - b) * (a - b)));
	}

	phi[y][x] = std::min(phi[y][x], xi);
}

class OperatriceChampsDistance : public OperatriceImage {
public:
	explicit OperatriceChampsDistance(Noeud *node)
		: OperatriceImage(node)
	{
		inputs(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_champs_distance.jo";
	}

	const char *class_name() const override
	{
		return NOM_CHAMPS_DISTANCE;
	}

	const char *help_text() const override
	{
		return AIDE_CHAMPS_DISTANCE;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		input(0)->requiers_image(m_image, rectangle, temps);

		auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		const auto methode = evalue_enum("méthode");
		auto image_grise = numero7::image::operation::luminance(tampon->tampon);
		auto image_tampon = numero7::math::matrice<float>(tampon->tampon.dimensions());
		auto valeur_iso = evalue_decimal("valeur_iso", temps);

		/* À FAIRE :  deux versions de chaque algorithme : signée et non-signée.
		 * cela demandrait de garder trace de deux grilles pour la version
		 * non-signée, comme pour l'algorithme 8SSEDT.
		 */

		if (methode == "balayage_rapide") {
			int res_x = tampon->tampon.nombre_colonnes();
			int res_y = tampon->tampon.nombre_lignes();

			auto h = std::min(1.0f / res_x, 1.0f / res_y);

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
			numero7::image::operation::navigation_estime::genere_champs_distance(image_grise, image_tampon, valeur_iso);
		}
		else if (methode == "8SSEDT") {
			numero7::image::operation::ssedt::genere_champs_distance(image_grise, image_tampon, valeur_iso);
		}

		/* À FAIRE : alpha ou tampon à canaux variables */
		tampon->tampon = numero7::image::operation::converti_float_pixel(image_tampon);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static constexpr auto NOM_DEFORMATION = "Déformation";
static constexpr auto AIDE_DEFORMATION = "Déforme une image de manière aléatoire.";

static auto moyenne(const numero7::image::Pixel<float> &pixel)
{
	return (pixel.r + pixel.g + pixel.b) * 0.3333f;
}

class OperatriceDeformation final : public OperatriceImage {
public:
	explicit OperatriceDeformation(Noeud *node)
		: OperatriceImage(node)
	{
		inputs(2);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_deformation.jo";
	}

	const char *class_name() const override
	{
		return NOM_DEFORMATION;
	}

	const char *help_text() const override
	{
		return AIDE_DEFORMATION;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		input(0)->requiers_image(m_image, rectangle, temps);

		auto nom_calque_a = evalue_chaine("nom_calque_a");
		auto tampon = m_image.calque(nom_calque_a);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque A introuvable !");
			return EXECUTION_ECHOUEE;
		}

		Image image2;
		input(1)->requiers_image(image2, rectangle, temps);

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
								   [&](const numero7::image::PixelFloat &/*pixel*/, int l, int c)
		{
			const auto c0 = std::min(res_x - 1, std::max(0, c - 1));
			const auto c1 = std::min(res_x - 1, std::max(0, c + 1));
			const auto l0 = std::min(res_y - 1, std::max(0, l - 1));
			const auto l1 = std::min(res_y - 1, std::max(0, l + 1));

			const auto x0 = moyenne(tampon2->tampon[l][c0]);
			const auto x1 = moyenne(tampon2->tampon[l][c1]);
			const auto y0 = moyenne(tampon2->tampon[l0][c]);
			const auto y1 = moyenne(tampon2->tampon[l1][c]);

			const auto pos_x = x1 - x0;
			const auto pos_y = y1 - y0;

			const auto x = c + pos_x * res_x;
			const auto y = l + pos_y * res_y;

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

static constexpr auto NOM_SIMULATION_GRAIN = "Simulation de grain";
static constexpr auto AIDE_SIMULATION_GRAIN = "Calcul du grain selon l'image d'entrée.";

static const auto MAX_NIVEAU_GRIS = 255;

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

	while ((u > somme) && (x < std::floor(10000.0f * lambda))) {
		x = x + 1u;
		prod = prod * lambda / static_cast<float>(x);
		somme = somme + prod;
	}

	return x;
}

using type_image_grise = numero7::math::matrice<float>;

static type_image_grise extrait_canal(const type_image &image, const int chaine)
{
	type_image_grise resultat(image.dimensions());

	boucle_parallele(tbb::blocked_range<size_t>(0, image.nombre_lignes()),
					 [&](const tbb::blocked_range<size_t> &plage)
	{
		for (size_t l = plage.begin(); l < plage.end(); ++l) {
			for (size_t c = 0; c < image.nombre_colonnes(); ++c) {
				resultat[l][c] = image[l][c][chaine];
			}
		}
	});

	return resultat;
}

static void assemble_image(
		type_image &image,
		const type_image_grise &canal_rouge,
		const type_image_grise &canal_vert,
		const type_image_grise &canal_bleu)
{

	boucle_parallele(tbb::blocked_range<size_t>(0, image.nombre_lignes()),
					 [&](const tbb::blocked_range<size_t> &plage)
	{
		for (size_t l = plage.begin(); l < plage.end(); ++l) {
			for (size_t c = 0; c < image.nombre_colonnes(); ++c) {
				image[l][c].r = canal_rouge[l][c];
				image[l][c].g = canal_vert[l][c];
				image[l][c].b = canal_bleu[l][c];
			}
		}
	});
}

static type_image_grise simule_grain_image(
		const type_image_grise &image,
		const int graine,
		const float rayon_grain,
		const float sigma_rayon,
		const float sigma_filtre)
{
	const auto res_x = image.nombre_colonnes();
	const auto res_y = image.nombre_lignes();

	type_image_grise resultat(image.dimensions());

	auto normal_quantile = 3.0902f;	//standard normal quantile for alpha=0.999
	auto rayon_grain2 = rayon_grain * rayon_grain;
	auto rayon_max = rayon_grain;
	auto mu = 0.0f;
	auto sigma = 0.0f;

	if (sigma_rayon > 0.0f) {
		sigma = std::sqrt(std::log1p((sigma_rayon / rayon_grain) * (sigma_rayon / rayon_grain)));
		const auto sigma2 = sigma * sigma;
		mu = std::log(rayon_grain) - sigma2 / 2.0f;
		const auto log_normal_quantile = std::exp(mu + sigma * normal_quantile);
		rayon_max = log_normal_quantile;
	}

	const auto delta = 1.0f / std::ceil(1.0f / rayon_max);
	const auto delta_inv = 1.0f / delta;

	std::mt19937 rng(graine);

	/* précalcul des lambdas */
	std::vector<float> lambdas(MAX_NIVEAU_GRIS);

	for (int i = 0; i < MAX_NIVEAU_GRIS; ++i) {
		const auto u = static_cast<float>(i) / static_cast<float>(MAX_NIVEAU_GRIS);
		const auto ag = 1.0f / std::ceil(1.0f / rayon_max);
		const auto lambda_tmp = -((ag * ag) / (PI * (rayon_max*rayon_max + sigma*sigma))) * std::log(1.0 - u);
		lambdas[i] = lambda_tmp;
	}

	/* précalcul des gaussiens */
	const auto iter = 1; // nombre d'itération de la simulation de Monte-Carlo

	std::vector<float> liste_gaussien_x(iter);
	std::vector<float> liste_gaussien_y(iter);

	std::normal_distribution<float> dist_normal(0.0, sigma_filtre);

	for (int i = 0; i < iter; ++i) {
		liste_gaussien_x[i] = dist_normal(rng);
		liste_gaussien_y[i] = dist_normal(rng);
	}

	boucle_parallele(tbb::blocked_range<size_t>(0, res_y),
					 [&](const tbb::blocked_range<size_t> &plage)
	{
		std::uniform_real_distribution<float> U(0.0f, 1.0f);
		std::mt19937 rng_local(graine + plage.begin());
		auto rayon_courant = 0.0f;
		auto rayon_courant2 = rayon_grain2;

		for (int j = plage.begin(); j < plage.end(); ++j) {
			for (int i = 0; i < res_x; ++i) {
				resultat[j][i] = 0.0f;

				for (int k = 0; k < iter; ++k) {
					bool va_suivant = false;
					// décalage aléatoire d'une distribution gaussienne centrée de variance sigma^2
					auto gaussien_x = i + sigma_filtre * liste_gaussien_x[k];
					auto gaussien_y = j + sigma_filtre * liste_gaussien_y[k];

					// obtiens la liste de cellule couvrant les balles (ig, jg)
					const auto min_x = std::floor((gaussien_x - rayon_max) * delta_inv);
					const auto max_x = std::floor((gaussien_x + rayon_max) * delta_inv);
					const auto min_y = std::floor((gaussien_y - rayon_max) * delta_inv);
					const auto max_y = std::floor((gaussien_y + rayon_max) * delta_inv);

					for (int jd = min_y; jd <= max_y; ++jd) {
						for (int id = min_x; id <= max_x; ++id) {
							/* coins de la cellule en coordonnées pixel */
							auto coin_x = delta*id;
							auto coin_y = delta*jd;

							// échantillone image
							const auto u = std::max(0.0f, std::min(1.0f, image[int(coin_y)][int(coin_x)]));
							const auto index_u = static_cast<int>(u * MAX_NIVEAU_GRIS);
							const auto lambda = lambdas[index_u];
							const auto Q = poisson(U(rng_local), lambda);

							for (int l = 1; l <= Q; ++l) {
								// prend un centre aléatoire d'une distribution uniforme dans un carré ([id, id+1), [jd, jd+1))
								auto grain_x = coin_x + U(rng_local) * delta;
								auto grain_y = coin_y + U(rng_local) * delta;
								auto dx = grain_x - gaussien_x;
								auto dy = grain_y - gaussien_y;

								if (sigma_rayon > 0.0f) {
									rayon_courant = std::min(std::exp(mu + sigma * U(rng_local)), rayon_max);
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
	explicit OperatriceSimulationGrain(Noeud *node)
		: OperatriceImage(node)
	{
		inputs(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_simulation_grain.jo";
	}

	const char *class_name() const override
	{
		return NOM_SIMULATION_GRAIN;
	}

	const char *help_text() const override
	{
		return AIDE_SIMULATION_GRAIN;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		input(0)->requiers_image(m_image, rectangle, temps);

		const auto &nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		/* parametres utilisateurs */
		const auto &graine = evalue_entier("graine", temps) + temps;

		const auto &rayon_grain_r = evalue_decimal("rayon_grain_r", temps);
		const auto &sigma_rayon_r = evalue_decimal("sigma_rayon_r", temps);
		const auto &sigma_filtre_r = evalue_decimal("sigma_filtre_r", temps);

		const auto &rayon_grain_v = evalue_decimal("rayon_grain_v", temps);
		const auto &sigma_rayon_v = evalue_decimal("sigma_rayon_v", temps);
		const auto &sigma_filtre_v = evalue_decimal("sigma_filtre_v", temps);

		const auto &rayon_grain_b = evalue_decimal("rayon_grain_b", temps);
		const auto &sigma_rayon_b = evalue_decimal("sigma_rayon_b", temps);
		const auto &sigma_filtre_b = evalue_decimal("sigma_filtre_b", temps);

		const auto &canal_rouge = extrait_canal(tampon->tampon, 0);
		const auto &canal_vert = extrait_canal(tampon->tampon, 1);
		const auto &canal_bleu = extrait_canal(tampon->tampon, 2);

		const auto &bruit_rouge = simule_grain_image(canal_rouge, graine, rayon_grain_r, sigma_rayon_r, sigma_filtre_r);
		const auto &bruit_vert = simule_grain_image(canal_vert, graine, rayon_grain_v, sigma_rayon_v, sigma_filtre_v);
		const auto &bruit_bleu = simule_grain_image(canal_bleu, graine, rayon_grain_b, sigma_rayon_b, sigma_filtre_b);

		assemble_image(tampon->tampon, bruit_rouge, bruit_vert, bruit_bleu);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static constexpr auto NOM_COORD_POLAIRE = "Coordonnées Polaires";
static constexpr auto AIDE_COORD_POLAIRE = "Transforme une image entre les coordonnées cartésiennes et les coordonnées polaires.";

class OperatriceCoordonneesPolaires final : public OperatriceImage {
public:
	explicit OperatriceCoordonneesPolaires(Noeud *node)
		: OperatriceImage(node)
	{
		inputs(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_coordonnees_polaires.jo";
	}

	const char *class_name() const override
	{
		return NOM_COORD_POLAIRE;
	}

	const char *help_text() const override
	{
		return AIDE_COORD_POLAIRE;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		input(0)->requiers_image(m_image, rectangle, temps);

		const auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		int res_x = tampon->tampon.nombre_colonnes();

		auto image_tampon = type_image(tampon->tampon.dimensions());

		applique_fonction_position(image_tampon,
								   [&](const numero7::image::PixelFloat &/*pixel*/, int l, int c)
		{
			/* À FAIRE : image carrée ? */
			const auto r = l;
			const auto theta = TAU * c / res_x;
			const auto x = r * std::cos(theta);
			const auto y = r * std::sin(theta);
			return tampon->echantillone(x, y);
		});

		tampon->tampon = image_tampon;

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static constexpr auto NOM_ONDELETTE_HAAR = "Ondelette de Haar";
static constexpr auto AIDE_ONDELETTE_HAAR = "Calcul l'ondelette de Haar d'une image.";

class OperatriceOndeletteHaar final : public OperatriceImage {
public:
	explicit OperatriceOndeletteHaar(Noeud *node)
		: OperatriceImage(node)
	{
		inputs(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_ondelette_haar.jo";
	}

	const char *class_name() const override
	{
		return NOM_ONDELETTE_HAAR;
	}

	const char *help_text() const override
	{
		return AIDE_ONDELETTE_HAAR;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		input(0)->requiers_image(m_image, rectangle, temps);

		const auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		const auto iterations = evalue_entier("itérations");
		auto res_x = tampon->tampon.nombre_colonnes();
		auto res_y = tampon->tampon.nombre_lignes();

		auto image_tampon = type_image(tampon->tampon.dimensions());

		const auto coeff = 1.0f / std::sqrt(2.0f);

		for (int i = 0; i < iterations; ++i) {
			/* transformation verticale */
			const auto taille_y = res_y / 2;
			for (size_t y = 0; y < taille_y; ++y) {
				for (size_t x = 0; x < res_x; ++x) {
					const auto p0 = tampon->valeur(x, y * 2);
					const auto p1 = tampon->valeur(x, y * 2 + 1);
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
			const auto taille_x = res_x / 2;
			for (size_t y = 0; y < res_y; ++y) {
				for (size_t x = 0; x < taille_x; ++x) {
					const auto p0 = tampon->valeur(x * 2, y);
					const auto p1 = tampon->valeur(x * 2 + 1, y);
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

static constexpr auto NOM_DILATION = "Dilation";
static constexpr auto AIDE_DILATION = "Dilate les pixels de l'image. Les parties les plus sombres de l'image rapetissent tandis que les plus claires grossissent.";

class OperatriceDilation final : public OperatriceImage {
public:
	explicit OperatriceDilation(Noeud *node)
		: OperatriceImage(node)
	{
		inputs(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_dilation_erosion.jo";
	}

	const char *class_name() const override
	{
		return NOM_DILATION;
	}

	const char *help_text() const override
	{
		return AIDE_DILATION;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		input(0)->requiers_image(m_image, rectangle, temps);

		const auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		const auto res_x = tampon->tampon.nombre_colonnes();
		const auto res_y = tampon->tampon.nombre_lignes();

		auto image_tampon = type_image(tampon->tampon.dimensions());
		const auto rayon = evalue_entier("rayon");

		auto performe_dilation = [&](
								 const numero7::image::Pixel<float> &/*pixel*/,
								 int y,
								 int x)
		{
			numero7::image::Pixel<float> p0(0.0f);
			p0.a = 1.0f;

			const auto debut_x = std::max(0, x - rayon);
			const auto debut_y = std::max(0, y - rayon);
			const auto fin_x = std::min(res_x, x + rayon);
			const auto fin_y = std::min(res_y, y + rayon);

			for (size_t sy = debut_y; sy < fin_y; ++sy) {
				for (size_t sx = debut_x; sx < fin_x; ++sx) {
					const auto p1 = tampon->valeur(sx, sy);

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

static constexpr auto NOM_EROSION = "Érosion";
static constexpr auto AIDE_EROSION = "Érode les pixels de l'image. Les parties les plus claires de l'image rapetissent tandis que les plus sombres grossissent.";

class OperatriceErosion final : public OperatriceImage {
public:
	explicit OperatriceErosion(Noeud *node)
		: OperatriceImage(node)
	{
		inputs(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_dilation_erosion.jo";
	}

	const char *class_name() const override
	{
		return NOM_EROSION;
	}

	const char *help_text() const override
	{
		return AIDE_EROSION;
	}

	int execute(const Rectangle &rectangle, const int temps) override
	{
		input(0)->requiers_image(m_image, rectangle, temps);

		const auto nom_calque = evalue_chaine("nom_calque");
		auto tampon = m_image.calque(nom_calque);

		if (tampon == nullptr) {
			ajoute_avertissement("Calque introuvable !");
			return EXECUTION_ECHOUEE;
		}

		const auto res_x = tampon->tampon.nombre_colonnes();
		const auto res_y = tampon->tampon.nombre_lignes();

		auto image_tampon = type_image(tampon->tampon.dimensions());
		const auto rayon = evalue_entier("rayon");

		auto performe_erosion = [&](
								const numero7::image::Pixel<float> &/*pixel*/,
								int y,
								int x)
		{
			numero7::image::Pixel<float> p0(1.0f);

			const auto debut_x = std::max(0, x - rayon);
			const auto debut_y = std::max(0, y - rayon);
			const auto fin_x = std::min(res_x, x + rayon);
			const auto fin_y = std::min(res_y, y + rayon);

			for (size_t sy = debut_y; sy < fin_y; ++sy) {
				for (size_t sx = debut_x; sx < fin_x; ++sx) {
					const auto p1 = tampon->valeur(sx, sy);

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

void enregistre_operatrices_region(UsineOperatrice *usine)
{
	usine->register_type(NOM_ANALYSE, cree_desc<OperatriceAnalyse>(NOM_ANALYSE, AIDE_ANALYSE));
	usine->register_type(NOM_FILTRAGE, cree_desc<OperatriceFiltrage>(NOM_FILTRAGE, AIDE_FILTRAGE));
	usine->register_type(NOM_FLOU, cree_desc<OperatriceFloutage>(NOM_FLOU, AIDE_FLOU));
	usine->register_type(NOM_CHAMPS_DISTANCE, cree_desc<OperatriceChampsDistance>(NOM_CHAMPS_DISTANCE, AIDE_CHAMPS_DISTANCE));
	usine->register_type(NOM_NORMALISE, cree_desc<OperatriceNormalisation>(NOM_NORMALISE, AIDE_NORMALISE));
	usine->register_type(NOM_TOURNOIEMENT, cree_desc<OperatriceTournoiement>(NOM_TOURNOIEMENT, AIDE_TOURNOIEMENT));
	usine->register_type(NOM_DEFORMATION, cree_desc<OperatriceDeformation>(NOM_DEFORMATION, AIDE_DEFORMATION));
	usine->register_type(NOM_SIMULATION_GRAIN, cree_desc<OperatriceSimulationGrain>(NOM_SIMULATION_GRAIN, AIDE_SIMULATION_GRAIN));
	usine->register_type(NOM_COORD_POLAIRE, cree_desc<OperatriceCoordonneesPolaires>(NOM_COORD_POLAIRE, AIDE_COORD_POLAIRE));
	usine->register_type(NOM_ONDELETTE_HAAR, cree_desc<OperatriceOndeletteHaar>(NOM_ONDELETTE_HAAR, AIDE_ONDELETTE_HAAR));
	usine->register_type(NOM_DILATION, cree_desc<OperatriceDilation>(NOM_DILATION, AIDE_DILATION));
	usine->register_type(NOM_EROSION, cree_desc<OperatriceErosion>(NOM_EROSION, AIDE_EROSION));
}
