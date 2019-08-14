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

#include "operatrices_pixel.h"

#include <random>

#include "biblinternes/image/operations/melange.h"
#include "biblinternes/image/outils/couleurs.h"
#include "biblinternes/math/bruit.hh"
#include "biblinternes/math/entrepolation.hh"
#include "biblinternes/math/matrice.hh"

#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/moultfilage/boucle.hh"

#include "danjo/types/courbe_bezier.h"
#include "danjo/types/rampe_couleur.h"

#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_graphe_pixel.h"
#include "coeur/operatrice_image.h"
#include "coeur/operatrice_pixel.h"
#include "coeur/usine_operatrice.h"

/**
 * OpératriceImage
 * |_ OpératricePixel
 * |_ OpératriceGraphePixel
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceSaturation final : public OperatricePixel {
	enum {
		SATURATION_REC709,
		SATURATION_CCIR601,
		SATURATION_MOYENNE,
		SATURATION_MINIMUM,
		SATURATION_MAXIMUM,
		SATURATION_MAGNITUDE,
	};

	int m_operation = SATURATION_REC709;
	float m_saturation = 1.0f;

public:
	static constexpr auto NOM = "Saturation";
	static constexpr auto AIDE = "Applique une saturation à l'image.";

	explicit OperatriceSaturation(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_saturation.jo";
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
		auto operation = evalue_enum("operation");

		if (operation == "rec_709") {
			m_operation = SATURATION_REC709;
		}
		else if (operation == "ccir_601") {
			m_operation = SATURATION_CCIR601;
		}
		else if (operation == "moyenne") {
			m_operation = SATURATION_MOYENNE;
		}
		else if (operation == "minimum") {
			m_operation = SATURATION_MINIMUM;
		}
		else if (operation == "maximum") {
			m_operation = SATURATION_MAXIMUM;
		}
		else if (operation == "magnitude") {
			m_operation = SATURATION_MAGNITUDE;
		}

		m_saturation = evalue_decimal("saturation", temps);
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);

		if (m_saturation == 1.0f) {
			return pixel;
		}

		auto sat = 0.0f;

		if (m_operation == SATURATION_REC709) {
			sat = dls::image::outils::luminance_709(pixel.r, pixel.v, pixel.b);
		}
		else if (m_operation == SATURATION_CCIR601) {
			sat = dls::image::outils::luminance_601(pixel.r, pixel.v, pixel.b);
		}
		else if (m_operation == SATURATION_MOYENNE) {
			sat = dls::image::outils::moyenne(pixel.r, pixel.v, pixel.b);
		}
		else if (m_operation == SATURATION_MINIMUM) {
			sat = std::min(pixel.r, std::min(pixel.v, pixel.b));
		}
		else if (m_operation == SATURATION_MAXIMUM) {
			sat = std::max(pixel.r, std::max(pixel.v, pixel.b));
		}
		else if (m_operation == SATURATION_MAGNITUDE) {
			sat = std::sqrt(pixel.r * pixel.r + pixel.v * pixel.v + pixel.b * pixel.b);
		}

		dls::phys::couleur32 resultat;
		resultat.a = pixel.a;

		if (m_saturation != 0.0f) {
			resultat.r = dls::math::entrepolation_lineaire(sat, pixel.r, m_saturation);
			resultat.v = dls::math::entrepolation_lineaire(sat, pixel.v, m_saturation);
			resultat.b = dls::math::entrepolation_lineaire(sat, pixel.b, m_saturation);
		}
		else {
			resultat.r = sat;
			resultat.v = sat;
			resultat.b = sat;
		}

		return resultat;
	}
};

/* ************************************************************************** */

class OperatriceMelange final : public OperatriceImage {
public:
	static constexpr auto NOM = "Mélanger";
	static constexpr auto AIDE = "Mélange deux images.";

	explicit OperatriceMelange(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
	}

	virtual int type() const override
	{
		return OPERATRICE_PIXEL;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_melange.jo";
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

		auto operation = evalue_enum("operation");
		auto facteur = evalue_decimal("facteur", contexte.temps_courant);
		auto melange = 0;

		if (operation == "mélanger") {
			melange = dls::image::operation::MELANGE_NORMAL;
		}
		else if (operation == "ajouter") {
			melange = dls::image::operation::MELANGE_ADDITION;
		}
		else if (operation == "soustraire") {
			melange = dls::image::operation::MELANGE_SOUSTRACTION;
		}
		else if (operation == "diviser") {
			melange = dls::image::operation::MELANGE_DIVISION;
		}
		else if (operation == "multiplier") {
			melange = dls::image::operation::MELANGE_MULTIPLICATION;
		}
		else if (operation == "écran") {
			melange = dls::image::operation::MELANGE_ECRAN;
		}
		else if (operation == "superposer") {
			melange = dls::image::operation::MELANGE_SUPERPOSITION;
		}
		else if (operation == "différence") {
			melange = dls::image::operation::MELANGE_DIFFERENCE;
		}

		auto calque = m_image.ajoute_calque(
					nom_calque_a,
					desc_depuis_rectangle(contexte.resolution_rendu),
					wlk::type_grille::COULEUR);

		auto tampon = dynamic_cast<grille_couleur *>(calque->tampon);
		auto tampon_a = dynamic_cast<grille_couleur *>(calque_a->tampon);
		auto tampon_b = dynamic_cast<grille_couleur *>(calque_b->tampon);

		copie_donnees_calque(*tampon_a, *tampon);

		dls::image::operation::melange_images(tampon, tampon_b, melange, facteur);
#endif
		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

/**
 * Implémentation basée sur https://keithp.com/~keithp/porterduff/p253-porter.pdf
 */
class OperatriceFusionnage final : public OperatriceImage {
	enum {
		FUSION_CLARIFICATION,
		FUSION_A,
		FUSION_B,
		FUSION_A_SUR_B,
		FUSION_B_SUR_A,
		FUSION_A_DANS_B,
		FUSION_B_DANS_A,
		FUSION_A_HORS_B,
		FUSION_B_HORS_A,
		FUSION_A_DESSUS_B,
		FUSION_B_DESSUS_A,
		FUSION_PLUS,
		FUSION_XOR,
	};

public:
	static constexpr auto NOM = "Fusionnage";
	static constexpr auto AIDE = "Fusionne deux images selon les algorithmes de Porter & Duff.";

	explicit OperatriceFusionnage(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
	}

	virtual int type() const override
	{
		return OPERATRICE_PIXEL;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_fusion.jo";
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
		auto nom_calque_a = evalue_chaine("nom_calque_a");
		auto calque_a = cherche_calque(*this, image, nom_calque_a);

		if (calque_a == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto image2 = entree(1)->requiers_image(contexte, donnees_aval);
		auto nom_calque_b = evalue_chaine("nom_calque_b");
		auto calque_b = cherche_calque(*this, image2, nom_calque_b);

		if (calque_b == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto operation = evalue_enum("operation");
		auto fusion = -1;

		if (operation == "clarification") {
			fusion = FUSION_CLARIFICATION;
		}
		else if (operation == "A") {
			fusion = FUSION_A;
		}
		else if (operation == "B") {
			fusion = FUSION_B;
		}
		else if (operation == "A_sur_B") {
			fusion = FUSION_A_SUR_B;
		}
		else if (operation == "B_sur_A") {
			fusion = FUSION_B_SUR_A;
		}
		else if (operation == "A_dans_B") {
			fusion = FUSION_A_DANS_B;
		}
		else if (operation == "B_dans_A") {
			fusion = FUSION_B_DANS_A;
		}
		else if (operation == "A_hors_B") {
			fusion = FUSION_A_HORS_B;
		}
		else if (operation == "B_hors_A") {
			fusion = FUSION_B_HORS_A;
		}
		else if (operation == "A_au_dessus_de_B") {
			fusion = FUSION_A_DESSUS_B;
		}
		else if (operation == "B_au_dessus_de_A") {
			fusion = FUSION_B_DESSUS_A;
		}
		else if (operation == "plus") {
			fusion = FUSION_PLUS;
		}
		else if (operation == "xor") {
			fusion = FUSION_XOR;
		}

		auto tampon_a = extrait_grille_couleur(calque_a);
		auto tampon_b = extrait_grille_couleur(calque_b);

		auto calque = m_image.ajoute_calque(
					nom_calque_a,
					tampon_a->desc(),
					wlk::type_grille::COULEUR);

		auto tampon = extrait_grille_couleur(calque);

		auto const &rectangle = contexte.resolution_rendu;

		boucle_parallele(tbb::blocked_range<int>(0l, static_cast<int>(rectangle.hauteur)),
					 [&](tbb::blocked_range<int> const &plage)
		{
			for (auto l = plage.begin(); l < plage.end(); ++l) {
				for (auto c = 0; c < static_cast<int>(rectangle.largeur); ++c) {
					auto index = tampon->calcul_index(dls::math::vec2i(c, l));
					auto resultat = dls::phys::couleur32(0.0f);

					switch (fusion) {
						case FUSION_CLARIFICATION:
						{
							/* aucune opération, le resultat est déjà 0 */
							break;
						}
						case FUSION_A:
						{
							resultat = tampon_a->valeur(index);
							break;
						}
						case FUSION_B:
						{
							resultat = tampon_b->valeur(index);
							break;
						}
						case FUSION_A_SUR_B:
						{
							auto a = tampon_a->valeur(index);
							auto b = tampon_b->valeur(index);
							auto fac_a = 1.0f;
							auto fac_b = 1.0f - a.a;
							resultat = a * fac_a + b * fac_b;
							break;
						}
						case FUSION_B_SUR_A:
						{
							auto a = tampon_a->valeur(index);
							auto b = tampon_b->valeur(index);
							auto fac_a = 1.0f - b.a;
							auto fac_b = 1.0f;
							resultat = a * fac_a + b * fac_b;
							break;
						}
						case FUSION_A_DANS_B:
						{
							auto a = tampon_a->valeur(index);
							auto b = tampon_b->valeur(index);
							auto fac_a = b.a;
							resultat = a * fac_a;
							break;
						}
						case FUSION_B_DANS_A:
						{
							auto a = tampon_a->valeur(index);
							auto b = tampon_b->valeur(index);
							auto fac_b = a.a;
							resultat = b * fac_b;
							break;
						}
						case FUSION_A_HORS_B:
						{
							auto a = tampon_a->valeur(index);
							auto b = tampon_b->valeur(index);
							auto fac_a = 1.0f - b.a;
							resultat = a * fac_a;
							break;
						}
						case FUSION_B_HORS_A:
						{
							auto a = tampon_a->valeur(index);
							auto b = tampon_b->valeur(index);
							auto fac_b = 1.0f - a.a;
							resultat = b * fac_b;
							break;
						}
						case FUSION_A_DESSUS_B:
						{
							auto a = tampon_a->valeur(index);
							auto b = tampon_b->valeur(index);
							auto fac_a = b.a;
							auto fac_b = 1.0f - a.a;
							resultat = a * fac_a + b * fac_b;
							break;
						}
						case FUSION_B_DESSUS_A:
						{
							auto a = tampon_a->valeur(index);
							auto b = tampon_b->valeur(index);
							auto fac_a = 1.0f - b.a;
							auto fac_b = a.a;
							resultat = a * fac_a + b * fac_b;
							break;
						}
						case FUSION_PLUS:
						{
							auto a = tampon_a->valeur(index);
							auto b = tampon_b->valeur(index);
							resultat = a + b;
							break;
						}
						case FUSION_XOR:
						{
							auto a = tampon_a->valeur(index);
							auto b = tampon_b->valeur(index);
							auto fac_a = 1.0f - b.a;
							auto fac_b = 1.0f - a.a;
							resultat = a * fac_a + b * fac_b;
							break;
						}
					}

					tampon->valeur(index, resultat);
				}
			}
		});

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

/* À FAIRE : problème de concurrence critique, RNG. */
class OperatriceBruitage final : public OperatricePixel {
	std::mt19937 m_rng{};
	std::uniform_real_distribution<float> m_dist{};

public:
	static constexpr auto NOM = "Bruitage";
	static constexpr auto AIDE = "Applique un bruit à l'image.";

	explicit OperatriceBruitage(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(0);
		m_dist = std::uniform_real_distribution<float>(0.0f, 1.0f);
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

		m_rng.seed(19937);
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);

		dls::phys::couleur32 resultat;
		resultat.r = m_dist(m_rng);
		resultat.v = m_dist(m_rng);
		resultat.b = m_dist(m_rng);
		resultat.a = pixel.a;

		return resultat;
	}
};

/* ************************************************************************** */

class OperatriceConstante final : public OperatricePixel {
	dls::phys::couleur32 m_couleur{};

public:
	static constexpr auto NOM = "Constante";
	static constexpr auto AIDE = "Applique une couleur constante à toute l'image.";

	explicit OperatriceConstante(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(0);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_constante.jo";
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
		m_couleur = evalue_couleur("couleur_constante");
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(pixel);
		INUTILISE(x);
		INUTILISE(y);
		dls::phys::couleur32 resultat;
		resultat.r = m_couleur[0];
		resultat.v = m_couleur[1];
		resultat.b = m_couleur[2];
		resultat.a = m_couleur[3];

		return resultat;
	}
};

/* ************************************************************************** */

class OperatriceDegrade final : public OperatricePixel {
	enum {
		MASK_R = (1 << 0),
		MASK_G = (1 << 1),
		MASK_B = (1 << 2),
		MASK_A = (1 << 3),
	};

	float m_decalage = 0.0f;
	int m_canaux = 0;

	float m_angle = 0.0f;
	float m_cos_angle = 1.0f;
	float m_sin_angle = 0.0f;

	int pad = 0;

	RampeCouleur const *m_rampe{};

public:
	static constexpr auto NOM = "Dégradé";
	static constexpr auto AIDE = "Génère un dégradé sur l'image.";

	explicit OperatriceDegrade(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(0);
	}

	OperatriceDegrade(OperatriceDegrade const &) = default;
	OperatriceDegrade &operator=(OperatriceDegrade const &) = default;

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_degrade.jo";
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
		m_decalage = evalue_decimal("décalage", temps);
		m_canaux = 0;

		if (evalue_bool("rouge")) {
			m_canaux |= MASK_R;
		}

		if (evalue_bool("vert")) {
			m_canaux |= MASK_G;
		}

		if (evalue_bool("bleu")) {
			m_canaux |= MASK_B;
		}

		if (evalue_bool("alpha")) {
			m_canaux |= MASK_A;
		}

		m_angle = evalue_decimal("angle") * constantes<float>::POIDS_DEG_RAD;
		m_cos_angle = std::cos(m_angle);
		m_sin_angle = std::sin(m_angle);

		m_rampe = evalue_rampe_couleur("degrade");
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		dls::phys::couleur32 resultat = pixel;
		float valeur = m_decalage;

		/* centre le dégradé au milieu de l'image */
		auto xi = 2.0f * x - 1.0f;
		auto yi = 2.0f * y - 1.0f;
		auto pos = m_cos_angle * xi + m_sin_angle * yi;

		/* normalise la position du dégradé dans la plage [0, 1) */
		valeur += 0.5f + 0.5f * pos;

		auto couleur = ::evalue_rampe_couleur(*m_rampe, valeur);

		resultat.r = ((m_canaux & MASK_R) == 0) ? 0.0f : couleur.r;
		resultat.v = ((m_canaux & MASK_G) == 0) ? 0.0f : couleur.v;
		resultat.b = ((m_canaux & MASK_B) == 0) ? 0.0f : couleur.b;
		resultat.a = ((m_canaux & MASK_A) == 0) ? 1.0f : couleur.a;

		return resultat;
	}
};

/* ************************************************************************** */

class OperatriceNuage final : public OperatricePixel {
	dls::math::BruitFlux2D m_bruit_flux{};

	dls::math::vec3f m_frequence = dls::math::vec3f(1.0f);
	dls::math::vec3f m_decalage = dls::math::vec3f(0.0f);
	float m_amplitude = 1.0f;
	int m_octaves = 1.0f;
	float m_lacunarite = 1.0f;
	float m_durete = 1.0f;
	int m_dimensions = 1;
	bool m_dur = false;
	char pad[7];

	std::mt19937 m_rng;
	std::normal_distribution<float> m_dist;

public:
	static constexpr auto NOM = "Nuage";
	static constexpr auto AIDE = "Crée un bruit de nuage";

	explicit OperatriceNuage(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
		, m_rng(1)
		, m_dist(0.0f, 1.0f)
	{
		entrees(0);
		//GenerateNoiseTile(noiseTileSize, 0);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_nuage.jo";
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
		auto const time = evalue_decimal("temps", temps);
		m_bruit_flux.change_temps(time);

		m_frequence = evalue_vecteur("fréquence", temps);
		m_decalage = evalue_vecteur("décalage", temps);
		m_amplitude = evalue_decimal("amplitude", temps);
		m_octaves = evalue_entier("octaves", temps);
		m_lacunarite = evalue_decimal("lacunarité", temps);
		m_durete = evalue_decimal("persistence", temps);
		m_dur = evalue_bool("dur");

		auto const dimensions = evalue_enum("dimension");
		m_dimensions = (dimensions == "3D") ? 3 : 1;
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(pixel);
		auto somme_x = 0.0f;
		auto somme_y = 0.0f;
		auto somme_z = 0.0f;
		float p[3] = { 0.0f, 0.0f, 0.0f };

		if (m_dimensions == 1) {
			auto frequence = m_frequence.x;
			auto amplitude = m_amplitude;

			for (int i = 0; i <= m_octaves; i++, amplitude *= m_durete) {
				p[0] = frequence * x + m_decalage.x;
				p[1] = frequence * y + m_decalage.y;

				//auto t = 0.5f + 0.5f * WNoise(p);
				auto t = 0.5f + 0.5f * m_bruit_flux(p[0], p[1]);

				if (m_dur) {
					t = std::fabs(2.0f * t - 1.0f);
				}

				somme_x += t * amplitude;
				somme_y += t * amplitude;
				somme_z += t * amplitude;
				frequence *= m_lacunarite;
			}
		}
		else {
			auto frequence_x = m_frequence.x;
			auto frequence_y = m_frequence.y;
			auto frequence_z = m_frequence.z;
			auto amplitude = m_amplitude;

			for (int i = 0; i <= m_octaves; i++, amplitude *= m_durete) {
				auto tx = 0.5f + 0.5f * m_bruit_flux(frequence_x * x + m_decalage.x, frequence_x * y + m_decalage.y);
				auto ty = 0.5f + 0.5f * m_bruit_flux(frequence_y * x + m_decalage.x + 1.0f, frequence_y * y + m_decalage.y);
				auto tz = 0.5f + 0.5f * m_bruit_flux(frequence_z * x + m_decalage.x, frequence_z * y + 1.0f + m_decalage.y);

				if (m_dur) {
					tx = std::fabs(2.0f * tx - 1.0f);
					ty = std::fabs(2.0f * ty - 1.0f);
					tz = std::fabs(2.0f * tz - 1.0f);
				}

				somme_x += tx * amplitude;
				somme_y += ty * amplitude;
				somme_z += tz * amplitude;
				frequence_x *= m_lacunarite;
				frequence_y *= m_lacunarite;
				frequence_z *= m_lacunarite;
			}
		}

		auto bruit_x = somme_x * (static_cast<float>(1 << m_octaves) / static_cast<float>((1 << (m_octaves + 1)) - 1));
		auto bruit_y = somme_y * (static_cast<float>(1 << m_octaves) / static_cast<float>((1 << (m_octaves + 1)) - 1));
		auto bruit_z = somme_z * (static_cast<float>(1 << m_octaves) / static_cast<float>((1 << (m_octaves + 1)) - 1));

		dls::phys::couleur32 resultat;
		resultat.r = bruit_x;
		resultat.b = bruit_y;
		resultat.v = bruit_z;
		resultat.a = 1.0f;

		return resultat;
	}
};

/* ************************************************************************** */

dls::phys::couleur32 converti_en_pixel(dls::phys::couleur32 const &v)
{
	dls::phys::couleur32 pixel;
	pixel.r = v[0];
	pixel.v = v[1];
	pixel.b = v[2];
	pixel.a = v[3];

	return pixel;
}

void restreint(dls::phys::couleur32 &pixel, float min, float max)
{
	if (pixel.r < min) {
		pixel.r = min;
	}
	else if (pixel.r > max) {
		pixel.r = max;
	}

	if (pixel.v < min) {
		pixel.v = min;
	}
	else if (pixel.v > max) {
		pixel.v = max;
	}

	if (pixel.b < min) {
		pixel.b = min;
	}
	else if (pixel.b > max) {
		pixel.b = max;
	}
}

class OperatriceEtalonnage final  : public OperatricePixel {
	bool m_inverse = false;
	bool m_restreint_noir = false;
	bool m_restreint_blanc = false;
	bool m_entrepolation_lineaire = false;
	int pad{};

	dls::phys::couleur32 m_point_noir{};
	dls::phys::couleur32 m_point_blanc{};
	dls::phys::couleur32 m_blanc{};
	dls::phys::couleur32 m_noir{};
	dls::phys::couleur32 m_multiple{};
	dls::phys::couleur32 m_ajoute{};

	dls::phys::couleur32 m_delta_BN = m_point_blanc - m_point_noir;
	dls::phys::couleur32 A1 = m_blanc - m_noir;
	dls::phys::couleur32 B{};
	dls::phys::couleur32 m_gamma{};

public:
	static constexpr auto NOM = "Étalonnage";
	static constexpr auto AIDE = "Étalonne l'image au moyen d'une rampe linéaire et d'une fonction gamma.";

	explicit OperatriceEtalonnage(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_grade.jo";
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
		auto ajoute = evalue_couleur("ajout");
		auto multiplie = evalue_couleur("multiple");
		auto gamma = evalue_couleur("gamma");
		auto blanc = evalue_couleur("blanc");
		auto noir = evalue_couleur("noir");
		auto point_blanc = evalue_couleur("point_blanc");
		auto point_noir = evalue_couleur("point_noir");

		m_inverse = evalue_bool("inverse");
		m_restreint_noir = evalue_bool("limite_noir");
		m_restreint_blanc = evalue_bool("limite_blanc");

		/* Prépare les données. */
		m_point_noir = converti_en_pixel(point_noir);
		m_point_blanc = converti_en_pixel(point_blanc);
		m_blanc = converti_en_pixel(blanc);
		m_noir = converti_en_pixel(noir);
		m_multiple = converti_en_pixel(multiplie);
		m_ajoute = converti_en_pixel(ajoute);

		m_delta_BN = m_point_blanc - m_point_noir;
		A1 = m_blanc - m_noir;

		m_delta_BN.r = (m_delta_BN.r > 0.0f) ? (A1.r / m_delta_BN.r) : 10000.0f;
		m_delta_BN.v = (m_delta_BN.v > 0.0f) ? (A1.v / m_delta_BN.v) : 10000.0f;
		m_delta_BN.b = (m_delta_BN.b > 0.0f) ? (A1.b / m_delta_BN.b) : 10000.0f;

		m_delta_BN *= m_multiple;

		B = m_ajoute + m_noir - m_point_noir * m_delta_BN;
		m_gamma = converti_en_pixel(gamma);

		for (int i = 0; i < 3; ++i) {
			if (m_gamma[i] < 0.008f) {
				m_gamma[i] = 0.0f;
			}
			else if (m_gamma[i] > 125.0f) {
				m_gamma[i] = 125.0f;
			}
		}

		m_entrepolation_lineaire = (m_delta_BN != dls::phys::couleur32(1.0f));
		m_entrepolation_lineaire |= (B != dls::phys::couleur32(0.0f));
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);

		dls::phys::couleur32 resultat = pixel;

		if (!m_inverse) {
			/* entrepolation linéaire */
			if (m_entrepolation_lineaire) {
				resultat = pixel * m_delta_BN + B;
			}

			/* restriction */
			if (m_restreint_blanc || m_restreint_noir) {
				for (int i = 0; i < 3; ++i) {
					if (resultat[i] < 0.0f && m_restreint_noir) {
						resultat[i] = 0.0f;
					}
					else if (resultat[i] > 1.0f && m_restreint_blanc) {
						resultat[i] = 1.0f;
					}
				}
			}

			/* gamma */
			for (int i = 0; i < 3; ++i) {
				if (m_gamma[i] <= 0.0f) {
					if (pixel[i] < 1.0f) {
						resultat[i] = 0.0f;
					}
					else if (pixel[i] > 1.0f) {
						resultat[i] = INFINITY;
					}
				}
				else if (m_gamma[i] != 1.0f) {
					auto g = 1.0f / m_gamma[i];

					if (pixel[i] <= 0.0f) {
						;              //V = 0.0f;
					}
					else if (pixel[i] <= 1e-6f && g > 1.0f) {
						resultat[i] = 0.0f;
					}
					else if (pixel[i] < 1) {
						resultat[i] = std::pow(pixel[i], g);
					}
					else {
						resultat[i] = 1.0f + (pixel[i] - 1.0f) * g;
					}
				}
			}
		}
		else {
			/* inverse le gamma */
			for (int i = 0; i < 3; ++i) {
				if (m_gamma[i] <= 0.0f) {
					resultat[i] = (pixel[i] > 0.0f) ? 1.0f : 0.0f;
				}
				else if (m_gamma[i] != 1.0f) {
					if (pixel[i] <= 0.0f) {
						;              //V = 0.0f;
					}
					else if (pixel[i] <= 1e-6f && m_gamma[i] > 1.0f) {
						resultat[i] = 0.0f;
					}
					else if (pixel[i] < 1.0f) {
						resultat[i] = std::pow(pixel[i], m_gamma[i]);
					}
					else {
						resultat[i] = 1.0f + (pixel[i] - 1.0f) * m_gamma[i];
					}
				}
			}

			/* inverse la partie linéaire */
			if (m_entrepolation_lineaire) {
				dls::phys::couleur32 A_local;
				dls::phys::couleur32 B_local;
				A_local.r = (m_delta_BN.r != 0.0f) ? 1.0f / m_delta_BN.r : 1.0f;
				A_local.v = (m_delta_BN.v != 0.0f) ? 1.0f / m_delta_BN.v : 1.0f;
				A_local.b = (m_delta_BN.b != 0.0f) ? 1.0f / m_delta_BN.b : 1.0f;
				A_local.a = 1.0f;

				B_local.r = -B.r * m_delta_BN.r;
				B_local.v = -B.v * m_delta_BN.v;
				B_local.b = -B.b * m_delta_BN.b;
				B_local.a = 0.0f;

				resultat *= A_local;
				resultat += B_local;
			}

			/* restriction */
			if (m_restreint_blanc || m_restreint_noir) {
				for (int i = 0; i < 3; ++i) {
					if (pixel[i] < 0.0f && m_restreint_noir) {
						resultat[i] = 0.0f;
					}
					else if (pixel[i] > 1.0f && m_restreint_blanc) {
						resultat[i] = 1.0f;
					}
				}
			}
		}

		return resultat;
	}
};

/* ************************************************************************** */

class OperatriceCorrectionGamma : public OperatricePixel {
	float m_gamma = 1.0f;
	int pad{};

public:
	static constexpr auto NOM = "Correction Gamma";
	static constexpr auto AIDE = "Applique une correction gamma à l'image.";

	explicit OperatriceCorrectionGamma(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_gamma.jo";
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
		m_gamma = evalue_decimal("gamma", temps);
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
		dls::phys::couleur32 resultat;
		resultat.r = std::pow(pixel.r, m_gamma);
		resultat.v = std::pow(pixel.v, m_gamma);
		resultat.b = std::pow(pixel.b, m_gamma);
		resultat.a = pixel.a;

		return resultat;
	}
};

/* ************************************************************************** */

/**
 * Opérateurs de mappage tonal de
 * http://filmicworlds.com/blog/filmic-tonemapping-operators/.
 */
class OperatriceMappageTonal : public OperatricePixel {
	enum {
		MAPPAGE_TONAL_LINEAR = 0,
		MAPPAGE_TONAL_REINHARD = 1,
		MAPPAGE_TONAL_HPDCURVE = 2,
		MAPPAGE_TONAL_HBD = 3,
		MAPPAGE_TONAL_UNCHARTED = 4,
		MAPPAGE_TONAL_CUSTOM = 5,
	};

	/* For the Uncharted 2 tone map. */
	static constexpr auto A = 0.15f;
	static constexpr auto B = 0.50f;
	static constexpr auto C = 0.10f;
	static constexpr auto D = 0.20f;
	static constexpr auto E = 0.02f;
	static constexpr auto F = 0.30f;
	static constexpr auto W = 11.2f;

	int m_type = MAPPAGE_TONAL_LINEAR;
	float m_exposition = 0.0f;

public:
	static constexpr auto NOM = "Mappage Tonal";
	static constexpr auto AIDE = "Applique un mappage tonal à l'image.";

	explicit OperatriceMappageTonal(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_tonemap.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	float uncharted_tone_map(float x)
	{
		return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
	}

	void evalue_entrees(int temps) override
	{
		auto type = evalue_enum("type");

		if (type == "linéaire") {
			m_type = MAPPAGE_TONAL_LINEAR;
		}
		else if (type == "reinhard") {
			m_type = MAPPAGE_TONAL_REINHARD;
		}
		else if (type == "courbe_hpd") {
			m_type = MAPPAGE_TONAL_HPDCURVE;
		}
		else if (type == "HBD") {
			m_type = MAPPAGE_TONAL_HBD;
		}
		else if (type == "uncharted_2") {
			m_type = MAPPAGE_TONAL_UNCHARTED;
		}
		else if (type == "personnalisé") {
			m_type = MAPPAGE_TONAL_CUSTOM;
		}

		m_exposition = std::pow(2.0f, evalue_decimal("exposition", temps));
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		auto resultat = pixel;
		auto besoin_correction_gamma = true;

		if (m_type == MAPPAGE_TONAL_LINEAR) {
			/* Exposure adjustment. */
			resultat.r *= m_exposition;
			resultat.v *= m_exposition;
			resultat.b *= m_exposition;
		}
		else if (m_type == MAPPAGE_TONAL_REINHARD) {
			/* Exposure adjustment. */
			resultat.r *= m_exposition;
			resultat.v *= m_exposition;
			resultat.b *= m_exposition;

			/* Simple Reinhard: 1/(1+x). */
			resultat.r = resultat.r / (1.0f + resultat.r);
			resultat.v = resultat.v / (1.0f + resultat.v);
			resultat.b = resultat.b / (1.0f + resultat.b);
		}
		else if (m_type == MAPPAGE_TONAL_HPDCURVE) {
			besoin_correction_gamma = false;
			/* Exposure adjustment. */
			resultat.r *= m_exposition;
			resultat.v *= m_exposition;
			resultat.b *= m_exposition;

			auto const ld = 0.002f;
			auto const lin_reference = 0.18f;
			auto const log_reference = 444.0f;
			auto const logGamma = 0.455f;

			resultat.r = (std::log(0.4f * resultat.r / lin_reference) / ld * logGamma + log_reference) / 1023.0f;
			resultat.v = (std::log(0.4f * resultat.v / lin_reference) / ld * logGamma + log_reference) / 1023.0f;
			resultat.b = (std::log(0.4f * resultat.b / lin_reference) / ld * logGamma + log_reference) / 1023.0f;

			restreint(resultat, 0.0f, 1.0f);
		}
		else if (m_type == MAPPAGE_TONAL_HBD) {
			besoin_correction_gamma = false;
			/* Exposure adjustment. */
			resultat.r *= m_exposition;
			resultat.v *= m_exposition;
			resultat.b *= m_exposition;

			auto const r = std::max(0.0f, resultat.r - 0.004f);
			resultat.r = (r * (6.2f * r + 0.5f)) / (r * (6.2f * r + 1.7f) + 0.06f);

			auto const g = std::max(0.0f, resultat.v - 0.004f);
			resultat.v = (g * (6.2f * g + 0.5f)) / (g * (6.2f * g + 1.7f) + 0.06f);

			auto const b = std::max(0.0f, resultat.b - 0.004f);
			resultat.b = (b * (6.2f * b + 0.5f)) / (b * (6.2f * b + 1.7f) + 0.06f);
		}
		else if (m_type == MAPPAGE_TONAL_UNCHARTED) {
			auto const exposure_bias = 2.0f;
			auto const white_scale = 1.0f / uncharted_tone_map(W);

			/* Exposure adjustment. */
			resultat.r *= m_exposition;
			resultat.v *= m_exposition;
			resultat.b *= m_exposition;

			auto const r = uncharted_tone_map(exposure_bias * resultat.r);
			resultat.r = r * white_scale;

			auto const g = uncharted_tone_map(exposure_bias * resultat.v);
			resultat.v = g * white_scale;

			auto const b = uncharted_tone_map(exposure_bias * resultat.b);
			resultat.b = b * white_scale;
		}
		else if (m_type == MAPPAGE_TONAL_CUSTOM) {
			/* À FAIRE */

			/* Exposure adjustment. */
			resultat.r *= m_exposition;
			resultat.v *= m_exposition;
			resultat.b *= m_exposition;
		}

		/* Adjust for the monitor's gamma. */
		if (besoin_correction_gamma) {
			resultat.r = std::pow(resultat.r, 1.0f / 2.2f);
			resultat.v = std::pow(resultat.v, 1.0f / 2.2f);
			resultat.b = std::pow(resultat.b, 1.0f / 2.2f);
		}

		return resultat;
	}
};

/* ************************************************************************** */

class OperatriceCorrectionCouleur final : public OperatricePixel {
	dls::phys::couleur32 m_decalage{};
	dls::phys::couleur32 m_pente{};
	dls::phys::couleur32 m_puissance{};

public:
	static constexpr auto NOM = "Correction Couleur";
	static constexpr auto AIDE = "Corrige les couleur de l'image selon la formule de l'ASC CDL.";

	explicit OperatriceCorrectionCouleur(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_correction_couleur.jo";
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
		m_decalage = evalue_couleur("décalage");
		m_pente = evalue_couleur("pente");
		m_puissance = evalue_couleur("puissance");
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
		auto resultat = pixel;

		resultat.r = resultat.r * m_pente[0] + m_decalage[0];
		resultat.v = resultat.v * m_pente[1] + m_decalage[1];
		resultat.b = resultat.b * m_pente[2] + m_decalage[2];

		resultat.r = std::pow(resultat.r, m_puissance[0]);
		resultat.v = std::pow(resultat.v, m_puissance[1]);
		resultat.b = std::pow(resultat.b, m_puissance[2]);

		return resultat;
	}
};

/* ************************************************************************** */

class OperatriceInversement final : public OperatricePixel {
public:
	static constexpr auto NOM = "Inversement";
	static constexpr auto AIDE = "Inverse les couleurs de l'image.";

	explicit OperatriceInversement(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_visionnage.jo";
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

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
		dls::phys::couleur32 resultat;
		resultat.r = 1.0f - pixel.r;
		resultat.v = 1.0f - pixel.v;
		resultat.b = 1.0f - pixel.b;
		resultat.a = pixel.a;

		return resultat;
	}
};

/* ************************************************************************** */

class OperatriceIncrustation final : public OperatricePixel {
	dls::phys::couleur32 m_couleur = dls::phys::couleur32(0.0f);
	float m_angle = 0.0f;
	float m_a = 0.0f;
	float m_b = 0.0f;
	int pad = 0;

public:
	static constexpr auto NOM = "Incrustation";
	static constexpr auto AIDE = "Supprime les couleurs vertes d'une image.";

	explicit OperatriceIncrustation(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_incrustation.jo";
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
		m_couleur = evalue_couleur("masque");
		m_angle = std::cos(evalue_decimal("angle") * constantes<float>::POIDS_DEG_RAD);
		m_a = evalue_decimal("a");
		m_b = evalue_decimal("b");
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
#if 0
		auto const cos_angle = pixel.r * m_couleur.r
							   + pixel.v * m_couleur.v
							   + pixel.b * m_couleur.b;

		if (cos_angle > m_angle) {
			return pixel;
		}

		dls::math::vec4f T;
		T.r = pixel.r - m_couleur.r;
		T.v = pixel.v - m_couleur.v;
		T.b = pixel.b - m_couleur.b;
		T.a = pixel.a;

		auto const facteur = std::sqrt(T.r * T.r + T.v * T.v + T.b * T.b);

		dls::phys::couleur32 resultat;
		resultat.r = (1.0f - facteur) * T.r + facteur * pixel.r;
		resultat.v = (1.0f - facteur) * T.v + facteur * pixel.v;
		resultat.b = (1.0f - facteur) * T.b + facteur * pixel.b;
		resultat.a = pixel.a;

		return resultat;
#else
		auto resultat = pixel;

		/* simple suppression du débordement de vert sur l'avant-plan */
		resultat.b = std::min(pixel.b, pixel.v);

		/* simple suppression de l'écran vert */
		resultat.a = m_a * (resultat.r + resultat.b) - m_b * resultat.v;
		resultat.a = std::max(0.0f, std::min(1.0f, resultat.a));

		return resultat;
#endif
	}
};

/* ************************************************************************** */

class OperatricePremultiplication final : public OperatricePixel {
	bool m_inverse = false;
	bool pad[7];

public:
	static constexpr auto NOM = "Pré-multiplication";
	static constexpr auto AIDE = "Prémultiplie les couleurs des pixels par leurs valeurs alpha respectives.";

	explicit OperatricePremultiplication(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_premultiplication.jo";
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
		m_inverse = evalue_bool("inverse");
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
		auto facteur = pixel.a;

		if (m_inverse && (facteur != 0.0f)) {
			facteur = 1.0f / facteur;
		}

		auto resultat = pixel;
		resultat.r *= facteur;
		resultat.v *= facteur;
		resultat.b *= facteur;

		return resultat;
	}
};

/* ************************************************************************** */

class OperatriceNormalisationPixel final : public OperatricePixel {
public:
	static constexpr auto NOM = "Normalisation Pixel";
	static constexpr auto AIDE = "Normalise les couleurs des pixels.";

	explicit OperatriceNormalisationPixel(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_visionnage.jo";
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

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
		dls::phys::couleur32 resultat;

		auto facteur = pixel.r * 0.2126f + pixel.v * 0.7152f + pixel.b * 0.0722f;

		if (facteur == 0.0f) {
			resultat.r = 0.0f;
			resultat.v = 0.0f;
			resultat.b = 0.0f;
		}
		else {
			resultat.r = pixel.r / facteur;
			resultat.v = pixel.v / facteur;
			resultat.b = pixel.b / facteur;
		}

		resultat.a = pixel.a;

		return resultat;
	}
};

/* ************************************************************************** */

class OperatriceContraste final : public OperatricePixel {
	float m_pivot{};
	float m_contraste{};

public:
	static constexpr auto NOM = "Contraste";
	static constexpr auto AIDE = "Ajuste le contraste de l'image.";

	explicit OperatriceContraste(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_contraste.jo";
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
		m_pivot = evalue_decimal("pivot", temps);
		m_contraste = evalue_decimal("contraste", temps);
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
		dls::phys::couleur32 resultat;

		if (m_pivot > 0.0f) {
			resultat.r = std::pow(pixel.r / m_pivot, m_contraste) * m_pivot;
			resultat.v = std::pow(pixel.v / m_pivot, m_contraste) * m_pivot;
			resultat.b = std::pow(pixel.b / m_pivot, m_contraste) * m_pivot;
		}
		else {
			resultat.r = 0.0f;
			resultat.v = 0.0f;
			resultat.b = 0.0f;
		}

		resultat.a = pixel.a;

		return resultat;
	}
};

/* ************************************************************************** */

class OperatriceCourbeCouleur final : public OperatricePixel {
	CourbeCouleur const *m_courbe{};

public:
	static constexpr auto NOM = "Courbe Couleur";
	static constexpr auto AIDE = "Modifie l'image selon une courbe de couleur.";

	explicit OperatriceCourbeCouleur(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(1);
	}

	OperatriceCourbeCouleur(OperatriceCourbeCouleur const &) = default;
	OperatriceCourbeCouleur &operator=(OperatriceCourbeCouleur const &) = default;

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_courbe_couleur.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	void evalue_entrees(int /*temps*/) override
	{
		m_courbe = evalue_courbe_couleur("courbe");
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
		dls::phys::couleur32 temp;
		temp.r = pixel.r;
		temp.v = pixel.v;
		temp.b = pixel.b;
		temp.a = pixel.a;

		temp = ::evalue_courbe_couleur(*m_courbe, temp);

		dls::phys::couleur32 resultat;
		resultat.r = temp.r;
		resultat.v = temp.v;
		resultat.b = temp.b;
		resultat.a = temp.a;

		return resultat;
	}
};

/* ************************************************************************** */

class OperatriceTraduction final : public OperatricePixel {
	dls::phys::couleur32 m_vieux_min{};
	dls::phys::couleur32 m_vieux_max{};
	dls::phys::couleur32 m_neuf_min{};
	dls::phys::couleur32 m_neuf_max{};

public:
	static constexpr auto NOM = "Traduction";
	static constexpr auto AIDE = "Traduit les composants de l'image d'une plage à une autre.";

	explicit OperatriceTraduction(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_traduction.jo";
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
		m_vieux_min = evalue_couleur("vieux_min");
		m_vieux_max = evalue_couleur("vieux_max");
		m_neuf_min = evalue_couleur("neuf_min");
		m_neuf_max = evalue_couleur("neuf_max");
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
		dls::phys::couleur32 resultat;

		for (int i = 0; i < 3; ++i) {
			resultat[i] = dls::math::traduit(pixel[i], m_vieux_min[i], m_vieux_max[i], m_neuf_min[i], m_neuf_max[i]);
		}

		resultat.a = pixel.a;

		return resultat;
	}
};

/* ************************************************************************** */

class OperatriceMinMax final : public OperatricePixel {
	dls::phys::couleur32 m_neuf_min{};
	dls::phys::couleur32 m_neuf_max{};

public:
	static constexpr auto NOM = "MinMax";
	static constexpr auto AIDE = "Change le point blanc et la point noir de l'image.";

	explicit OperatriceMinMax(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_min_max.jo";
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
		m_neuf_min = evalue_couleur("neuf_min");
		m_neuf_max = evalue_couleur("neuf_max");
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
		dls::phys::couleur32 resultat;
		resultat.r = (pixel.r - m_neuf_min.r) / (m_neuf_max.r - m_neuf_min.r);
		resultat.v = (pixel.v - m_neuf_min.v) / (m_neuf_max.v - m_neuf_min.v);
		resultat.b = (pixel.b - m_neuf_min.b) / (m_neuf_max.b - m_neuf_min.b);
		resultat.a = pixel.a;

		return resultat;
	}
};

/* ************************************************************************** */

enum {
	DALTONISME_AUCUN = 0,
	DALTONISME_PROTANOPIE,
	DALTONISME_PROTANOMALIE,
	DALTONISME_DEUTERANOPIE,
	DALTONISME_DEUTERANOMALIE,
	DALTONISME_TRITANOPIE,
	DALTONISME_TRITANOMALIE,
	DALTONISME_ACHROMATOPSIE,
	DALTONISME_ACHROMATOMALIE,
};

/* source
 * https://web.archive.org/web/20081014161121/http://www.colorjack.com/labs/colormatrix/
 */
static dls::math::mat4x4f matrices_daltonisme[] = {
	dls::math::mat4x4f{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	},
	dls::math::mat4x4f{
		0.56667f, 0.43333f, 0.0f, 0.0f,
		0.55833f, 0.44167f, 0.0f, 0.0f,
		0.0f, 0.24167f, 0.75833f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	},
	dls::math::mat4x4f{
		0.81667f, 0.18333f, 0.0f, 0.0f,
		0.33333f, 0.66667f, 0.0f, 0.0f,
		0.0f, 0.125f, 0.875f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	},
	dls::math::mat4x4f{
		0.625f, 0.375f, 0.0f, 0.0f,
		0.70f, 0.3f, 0.0f, 0.0f,
		0.0f, 0.3f, 0.7f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	},
	dls::math::mat4x4f{
		0.8f, 0.2f, 0.0f, 0.0f,
		0.25833f, 0.74167f, 0.0f, 0.0f,
		0.0f, 0.14167f, 0.85833f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	},
	dls::math::mat4x4f{
		0.95f, 0.05f, 0.0f, 0.0f,
		0.0f, 0.43333f, 0.56667f, 0.0f,
		0.0f, 0.475f, 0.525f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	},
	dls::math::mat4x4f{
		0.96667f, 0.03333f, 0.0f, 0.0f,
		0.0f, 0.73333f, 0.26667f, 0.0f,
		0.0f, 0.18333f, 0.81667f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	},
	/* utilisation des poids ITU-R BT.709 */
	dls::math::mat4x4f{
		0.212671f, 0.715160f, 0.072169f, 0.0f,
		0.212671f, 0.715160f, 0.072169f, 0.0f,
		0.212671f, 0.715160f, 0.072169f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	},
	dls::math::mat4x4f{
		0.618f, 0.320f, 0.062f, 0.0f,
		0.163f, 0.775f, 0.062f, 0.0f,
		0.163f, 0.320f, 0.516f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	},
};

auto operator*(dls::phys::couleur32 const &p, dls::math::mat4x4f const &m)
{
	dls::phys::couleur32 r;
	r.r = p.r * m[0][0] + p.v * m[0][1] + p.b * m[0][2] + p.a * m[0][3];
	r.v = p.r * m[1][0] + p.v * m[1][1] + p.b * m[1][2] + p.a * m[1][3];
	r.b = p.r * m[2][0] + p.v * m[2][1] + p.b * m[2][2] + p.a * m[2][3];
	r.a = p.r * m[3][0] + p.v * m[3][1] + p.b * m[3][2] + p.a * m[3][3];

	return r;
}

class OperatriceDaltonisme final : public OperatricePixel {
	dls::math::mat4x4f m_matrice{};

public:
	static constexpr auto NOM = "Daltonisme";
	static constexpr auto AIDE = "Simule l'effet du daltonisme.";

	explicit OperatriceDaltonisme(Graphe &graphe_parent, Noeud *noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_daltonisme.jo";
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
		auto type = evalue_enum("type");

		if (type == "protanopie") {
			m_matrice = matrices_daltonisme[DALTONISME_PROTANOPIE];
		}
		else if (type == "protanomalie") {
			m_matrice = matrices_daltonisme[DALTONISME_PROTANOMALIE];
		}
		else if (type == "deuteranopie") {
			m_matrice = matrices_daltonisme[DALTONISME_DEUTERANOPIE];
		}
		else if (type == "deuteranomalie") {
			m_matrice = matrices_daltonisme[DALTONISME_DEUTERANOMALIE];
		}
		else if (type == "tritanopie") {
			m_matrice = matrices_daltonisme[DALTONISME_TRITANOPIE];
		}
		else if (type == "tritanomalie") {
			m_matrice = matrices_daltonisme[DALTONISME_TRITANOMALIE];
		}
		else if (type == "achromatopsie") {
			m_matrice = matrices_daltonisme[DALTONISME_ACHROMATOPSIE];
		}
		else if (type == "achromatomalie") {
			m_matrice = matrices_daltonisme[DALTONISME_ACHROMATOMALIE];
		}
		else {
			m_matrice = matrices_daltonisme[DALTONISME_AUCUN];
		}
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);
		return pixel * m_matrice;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_pixel(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceGraphePixel>());

	usine.enregistre_type(cree_desc<OperatriceNuage>());
	usine.enregistre_type(cree_desc<OperatriceConstante>());
	usine.enregistre_type(cree_desc<OperatriceCorrectionGamma>());
	usine.enregistre_type(cree_desc<OperatriceEtalonnage>());
	usine.enregistre_type(cree_desc<OperatriceDegrade>());
	usine.enregistre_type(cree_desc<OperatriceMelange>());
	usine.enregistre_type(cree_desc<OperatriceBruitage>());
	usine.enregistre_type(cree_desc<OperatriceSaturation>());
	usine.enregistre_type(cree_desc<OperatriceMappageTonal>());
	usine.enregistre_type(cree_desc<OperatriceCorrectionCouleur>());
	usine.enregistre_type(cree_desc<OperatriceInversement>());
	usine.enregistre_type(cree_desc<OperatriceIncrustation>());
	usine.enregistre_type(cree_desc<OperatriceFusionnage>());
	usine.enregistre_type(cree_desc<OperatricePremultiplication>());
	usine.enregistre_type(cree_desc<OperatriceNormalisationPixel>());
	usine.enregistre_type(cree_desc<OperatriceContraste>());
	usine.enregistre_type(cree_desc<OperatriceCourbeCouleur>());
	usine.enregistre_type(cree_desc<OperatriceTraduction>());
	usine.enregistre_type(cree_desc<OperatriceMinMax>());
	usine.enregistre_type(cree_desc<OperatriceDaltonisme>());
}

#pragma clang diagnostic pop
