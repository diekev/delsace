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
#include "biblinternes/outils/empreintes.hh"
#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/structures/dico_fixe.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "danjo/types/courbe_bezier.h"
#include "danjo/types/rampe_couleur.h"

#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
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

	OperatriceSaturation(Graphe &graphe_parent, Noeud &noeud)
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

/**
 * Mélange d'images
 *
 * Référence pour les mélanges utilisant le canal alpha :
 * « Compositing Digital Images », Porter & Duff
 * http://graphics.pixar.com/library/Compositing/paper.pdf
 */

static float enhaut(float A, float a, float B, float b)
{
	return A * b + B * (1.0f - a);
}

static float moyenne(float A, float B)
{
	return (A + B) * 0.5f;
}

static float densite_couleur_pos(float A, float B)
{
	return (B == 0.0f) ? 0.0f : std::max(1.0f - ((1.0f - A) / B), 0.0f);
}

static float densite_couleur_neg(float A, float B)
{
	return (B >= 1.0f) ? 1.0f : std::min(A / (1.0f - B), 1.0f);
}

static float dessus_conjoint(float A, float a, float B, float b)
{
	if (a > b || b == 0.0f) {
		return A;
	}

	return A + B * (1.0f - a) / b;
}

//static float copie(float A, float B)
//{
//	return A;
//}

static float difference(float A, float B)
{
	return std::abs(A - B);
}

static float dessus_disjoint(float A, float a, float B, float b)
{
	if (a + b < 1.0f) {
		return A + B;
	}

	return A + B * (1.0f - a) / b;
}

static float divise(float A, float B)
{
	if (A < 0.0f || B <= 0.0f) {
		return 0.0f;
	}

	return A / B;
}

static float exclus(float A, float B)
{
	return A + B - 2.0f * A * B;
}

static float depuis(float A, float B)
{
	return B - A;
}

static float geometrique(float A, float B)
{
	return (2.0f * A * B) / (A + B);
}

static float hypot(float A, float B)
{
	return std::sqrt(A * A + B * B);
}

static float dedans(float A, float b)
{
	return A * b;
}
static float masque(float B, float a)
{
	return B * a;
}
static float matte(float A, float a, float B)
{
	return A * a + B * (1.0f - a);
}
static float max(float A, float B)
{
	return std::max(A, B);
}
static float min(float A, float B)
{
	return std::min(A, B);
}
static float moins(float A, float B)
{
	return A - B;
}
static float multiplie(float A, float B)
{
	if (A < 0 && B < 0) {
		return A;
	}

	return A * B;
}
static float dehors(float A, float b)
{
	return A * (1.0f - b);
}
static float dessus(float A, float a, float B)
{
	return A + B * (1.0f - a);
}
static float plus(float A, float B)
{
	return A + B;
}
static float ecran(float A, float B)
{
	if (0.0f <= A && A <= 1.0f && 0.0f <= B && B <= 1.0f) {
		return A + B - A * B;
	}

	if (A > B) {
		return A;
	}

	return B;
}
static float lumiere_douce(float A, float B)
{
	auto const AB = A * B;

	if (AB < 1.0f) {
		return B * (2.0f * A + (B * (1.0f - AB)));
	}

	return 2.0f * AB;
}
static float pochoir(float a, float B)
{
	return B * (1.0f - a);
}
static float dessous(float A, float B, float b)
{
	return A * (1.0f - b) + B;
}
static float ou_exclusif(float A, float a, float B, float b)
{
	return A * (1.0f - b) + B * (1.0f - a);
}

static float lumiere_dure(float A, float B)
{
	if (A < 0.5f) {
		return multiplie(A, B);
	}

	return ecran(A, B);
}
static float superpose(float A, float B)
{
	if (B < 0.5f) {
		return multiplie(A, B);
	}

	return ecran(A, B);
}

static Image const *cherche_image(
		OperatriceImage &op,
		int index,
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval,
		int index_lien = 0)
{
	auto image = op.entree(index)->requiers_image(contexte, donnees_aval, index_lien);

	if (image == nullptr) {
		auto flux = dls::flux_chaine();
		flux << "Aucune image trouvée dans l'entrée à l'index " << index << ", lien " << index_lien << " !";
		op.ajoute_avertissement(flux.chn());
		return nullptr;
	}

	return image;
}

class OperatriceMelange final : public OperatriceImage {
	enum class type_melange {
		copie,
		dedans,
		densite_couleur_pos,
		densite_couleur_neg,
		dehors,
		dessous,
		dessus,
		dessus_conjoint,
		dessus_disjoint,
		difference,
		divise,
		ecran,
		enhaut,
		exclus,
		depuis,
		geometrique,
		lumiere_douce,
		lumiere_dure,
		hypot,
		masque,
		matte,
		max,
		min,
		moins,
		moyenne,
		multiplie,
		ou_exclusif,
		plus,
		pochoir,
		superpose,
	};

public:
	static constexpr auto NOM = "Mélanger";
	static constexpr auto AIDE = "Mélange deux images.";

	OperatriceMelange(Graphe &graphe_parent, Noeud &noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(1);
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

	bool connexions_multiples(int n) const override
	{
		return n == 0;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto nom_calque = evalue_chaine("nom_calque");
		auto tampons = dls::tableau<grille_couleur const *>();

		auto desc = wlk::desc_grille_2d{};
		desc.taille_pixel = 1.0;
		desc.etendue.min = dls::math::vec2f( constantes<float>::INFINITE);
		desc.etendue.max = dls::math::vec2f(-constantes<float>::INFINITE);

		for (auto i = 0; i < entree(0)->nombre_connexions(); ++i) {
			auto img = cherche_image(*this, 0, contexte, donnees_aval, i);

			if (img == nullptr) {
				continue;
			}

			auto clq = img->calque_pour_lecture(nom_calque);

			if (clq == nullptr) {
				continue;
			}

			auto tpn = extrait_grille_couleur(clq);

			tampons.pousse(tpn);

			auto desc1 = tpn->desc();
			desc.etendue.min.x = std::min(desc.etendue.min.x, desc1.etendue.min.x);
			desc.etendue.min.y = std::min(desc.etendue.min.y, desc1.etendue.min.y);
			desc.etendue.max.x = std::max(desc.etendue.max.x, desc1.etendue.max.x);
			desc.etendue.max.y = std::max(desc.etendue.max.y, desc1.etendue.max.y);
		}

		desc.fenetre_donnees = desc.etendue;

		if (tampons.est_vide()) {
			this->ajoute_avertissement("Aucun tampon trouvé");
			return EXECUTION_ECHOUEE;
		}

		auto dico_types = dls::cree_dico(
					dls::paire(dls::chaine("enhaut"), type_melange::enhaut),
					dls::paire(dls::chaine("moyenne"), type_melange::moyenne),
					dls::paire(dls::chaine("densite_couleur_pos"), type_melange::densite_couleur_pos),
					dls::paire(dls::chaine("densite_couleur_neg"), type_melange::densite_couleur_neg),
					dls::paire(dls::chaine("dessus_disjoint"), type_melange::dessus_disjoint),
					dls::paire(dls::chaine("copie"), type_melange::copie),
					dls::paire(dls::chaine("difference"), type_melange::difference),
					dls::paire(dls::chaine("dessus_conjoint"), type_melange::dessus_conjoint),
					dls::paire(dls::chaine("divise"), type_melange::divise),
					dls::paire(dls::chaine("exclus"), type_melange::exclus),
					dls::paire(dls::chaine("depuis"), type_melange::depuis),
					dls::paire(dls::chaine("geometrique"), type_melange::geometrique),
					dls::paire(dls::chaine("lumiere_dure"), type_melange::lumiere_dure),
					dls::paire(dls::chaine("hypot"), type_melange::hypot),
					dls::paire(dls::chaine("dedans"), type_melange::dedans),
					dls::paire(dls::chaine("masque"), type_melange::masque),
					dls::paire(dls::chaine("matte"), type_melange::matte),
					dls::paire(dls::chaine("max"), type_melange::max),
					dls::paire(dls::chaine("min"), type_melange::min),
					dls::paire(dls::chaine("moins"), type_melange::moins),
					dls::paire(dls::chaine("multiplie"), type_melange::multiplie),
					dls::paire(dls::chaine("dehors"), type_melange::dehors),
					dls::paire(dls::chaine("dessus"), type_melange::dessus),
					dls::paire(dls::chaine("superpose"), type_melange::superpose),
					dls::paire(dls::chaine("plus"), type_melange::plus),
					dls::paire(dls::chaine("ecran"), type_melange::ecran),
					dls::paire(dls::chaine("lumiere_douce"), type_melange::lumiere_douce),
					dls::paire(dls::chaine("pochoir"), type_melange::pochoir),
					dls::paire(dls::chaine("dessous"), type_melange::dessous),
					dls::paire(dls::chaine("ou_exclusif"), type_melange::ou_exclusif));

		auto chn_type = evalue_enum("operation");
		auto plg_type = dico_types.trouve(chn_type);

		if (plg_type.est_finie()) {
			this->ajoute_avertissement("l'opération est inconnue");
			return EXECUTION_ECHOUEE;
		}

		auto type_mel = plg_type.front().second;

		auto calque = m_image.ajoute_calque(nom_calque, desc, wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);

		auto largeur = tampon->desc().resolution.x;
		auto hauteur = tampon->desc().resolution.y;
		auto courant = 0;

		auto chef = contexte.chef;
		chef->demarre_evaluation("fusion images");

		boucle_parallele(tbb::blocked_range<int>(0, hauteur),
						 [&](tbb::blocked_range<int> const &plage)
		{
			for (auto j = plage.begin(); j < plage.end(); ++j) {
				if (chef->interrompu()) {
					break;
				}

				for (auto i = 0; i < largeur; ++i, ++courant) {
					if (chef->interrompu()) {
						break;
					}

					auto const index = i + j * largeur;

					auto const pos_mnd = tampon->index_vers_monde(dls::math::vec2i(i, j));

					auto pos_a = tampons[0]->monde_vers_index(pos_mnd);
					auto A = tampons[0]->valeur(pos_a);

					for (auto t = 1; t < tampons.taille(); ++t) {
						auto pos_b = tampons[t]->monde_vers_index(pos_mnd);
						auto B = tampons[t]->valeur(pos_b);

						switch (type_mel) {
							case type_melange::enhaut:
							{
								for (auto c = 0; c < 4; ++c) {
									A[c] = enhaut(A[c], A.a, B[c], B.a);
								}
								break;
							}
							case type_melange::moyenne:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = moyenne(A[c], B[c]);
								}
								break;
							}
							case type_melange::densite_couleur_pos:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = densite_couleur_pos(A[c], B[c]);
								}
								break;
							}
							case type_melange::densite_couleur_neg:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = densite_couleur_neg(A[c], B[c]);
								}
								break;
							}
							case type_melange::dessus_disjoint:
							{
								for (auto c = 0; c < 4; ++c) {
									A[c] = dessus_disjoint(A[c], A.a, B[c], B.a);
								}
								break;
							}
							case type_melange::copie:
							{
								/* return A */
								break;
							}
							case type_melange::difference:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = difference(A[c], B[c]);
								}
								break;
							}
							case type_melange::dessus_conjoint:
							{
								for (auto c = 0; c < 4; ++c) {
									A[c] = dessus_conjoint(A[c], A.a, B[c], B.a);
								}
								break;
							}
							case type_melange::divise:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = divise(A[c], B[c]);
								}
								break;
							}
							case type_melange::exclus:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = exclus(A[c], B[c]);
								}
								break;
							}
							case type_melange::depuis:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = depuis(A[c], B[c]);
								}
								break;
							}
							case type_melange::geometrique:
							{
								for (auto c = 0; c < 4; ++c) {
									A[c] = geometrique(A[c], B[c]);
								}
								break;
							}
							case type_melange::lumiere_dure:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = lumiere_dure(A[c], B[c]);
								}
								break;
							}
							case type_melange::hypot:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = hypot(A[c], B[c]);
								}
								break;
							}
							case type_melange::dedans:
							{
								for (auto c = 0; c < 4; ++c) {
									A[c] = dedans(A[c], B.a);
								}
								break;
							}
							case type_melange::masque:
							{
								for (auto c = 0; c < 4; ++c) {
									A[c] = masque(B[c], A.a);
								}
								break;
							}
							case type_melange::matte:
							{
								for (auto c = 0; c < 4; ++c) {
									A[c] = matte(A[c], A.a, B[c]);
								}
								break;
							}
							case type_melange::max:
							{
								for (auto c = 0; c < 4; ++c) {
									A[c] = max(A[c], B[c]);
								}
								break;
							}
							case type_melange::min:
							{
								for (auto c = 0; c < 4; ++c) {
									A[c] = min(A[c], B[c]);
								}
								break;
							}
							case type_melange::moins:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = moins(A[c], B[c]);
								}
								break;
							}
							case type_melange::multiplie:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = multiplie(A[c], B[c]);
								}
								break;
							}
							case type_melange::dehors:
							{
								for (auto c = 0; c < 4; ++c) {
									A[c] = dehors(A[c], B.a);
								}
								break;
							}
							case type_melange::dessus:
							{
								for (auto c = 0; c < 4; ++c) {
									A[c] = dessus(A[c], A.a, B[c]);
								}
								break;
							}
							case type_melange::superpose:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = superpose(A[c], B[c]);
								}
								break;
							}
							case type_melange::plus:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = plus(A[c], B[c]);
								}
								break;
							}
							case type_melange::ecran:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = ecran(A[c], B[c]);
								}
								break;
							}
							case type_melange::lumiere_douce:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = lumiere_douce(A[c], B[c]);
								}
								break;
							}
							case type_melange::pochoir:
							{
								for (auto c = 0; c < 4; ++c) {
									A[c] = pochoir(A.a, B[c]);
								}
								break;
							}
							case type_melange::dessous:
							{
								for (auto c = 0; c < 3; ++c) {
									A[c] = dessous(A[c], B[c], B.a);
								}
								break;
							}
							case type_melange::ou_exclusif:
							{
								for (auto c = 0; c < 4; ++c) {
									A[c] = ou_exclusif(A[c], A.a, B[c], B.a);
								}
								break;
							}
						}
					}

					tampon->valeur(index) = A;
				}

				auto delta = static_cast<float>(plage.end() - plage.begin());
				delta /= static_cast<float>(largeur);
				chef->indique_progression_parallele(delta * 100.0f);
			}
		});

		chef->indique_progression(100.0f);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceBruitage final : public OperatricePixel {
	bool m_noir_blanc = false;
	REMBOURRE(3);
	int m_graine = 0;

public:
	static constexpr auto NOM = "Bruitage";
	static constexpr auto AIDE = "Crée un bruit blanc.";

	OperatriceBruitage(Graphe &graphe_parent, Noeud &noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(0);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_bruit_blanc.jo";
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
		m_noir_blanc = evalue_enum("bruit::type") == "nb";
		m_graine = evalue_entier("graine", temps);

		if (evalue_bool("anime_graine")) {
			m_graine += temps;
		}
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(pixel);

		dls::phys::couleur32 resultat;
		resultat.a = 1.0f;

		auto hash0 = empreinte_r32_vers_r32(x, y, static_cast<float>(m_graine));

		if (m_noir_blanc) {
			resultat.r = hash0;
			resultat.v = hash0;
			resultat.b = hash0;
		}
		else {
			auto hash1 = empreinte_r32_vers_r32(x, y, static_cast<float>(m_graine) + 1.0f);
			auto hash2 = empreinte_r32_vers_r32(x, y, static_cast<float>(m_graine) + 2.0f);

			resultat.r = hash0;
			resultat.v = hash1;
			resultat.b = hash2;
		}

		return resultat;
	}
};

/* ************************************************************************** */

class OperatriceConstante final : public OperatricePixel {
	dls::phys::couleur32 m_couleur{};

public:
	static constexpr auto NOM = "Constante";
	static constexpr auto AIDE = "Applique une couleur constante à toute l'image.";

	OperatriceConstante(Graphe &graphe_parent, Noeud &noeud)
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

	REMBOURRE(4);

	RampeCouleur const *m_rampe{};

public:
	static constexpr auto NOM = "Dégradé";
	static constexpr auto AIDE = "Génère un dégradé sur l'image.";

	OperatriceDegrade(Graphe &graphe_parent, Noeud &noeud)
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

	REMBOURRE(3);

	std::mt19937 m_rng;
	std::normal_distribution<float> m_dist;

public:
	static constexpr auto NOM = "Nuage";
	static constexpr auto AIDE = "Crée un bruit de nuage";

	OperatriceNuage(Graphe &graphe_parent, Noeud &noeud)
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

	REMBOURRE(4);

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

	OperatriceEtalonnage(Graphe &graphe_parent, Noeud &noeud)
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

class OperatriceCorrectionGamma final : public OperatricePixel {
	float m_gamma = 1.0f;

	REMBOURRE(4);

public:
	static constexpr auto NOM = "Correction Gamma";
	static constexpr auto AIDE = "Applique une correction gamma à l'image.";

	OperatriceCorrectionGamma(Graphe &graphe_parent, Noeud &noeud)
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
 * http://filmicworlds.com/blog/filmic-tonemapping-operators/
 * https://bruop.github.io/tonemapping/
 */

/* version locale */
static auto mappage_ton_reinhard(float x)
{
	return x / (1.0f + x);
}

/* version globale, il nous faut la luminance moyenne de l'image */
static auto mappage_ton_reinhard(float x, float lum_moyenne)
{
	x = (x / (9.6f * lum_moyenne + 0.00001f));
	return mappage_ton_reinhard(x);
}

/* modification du mappage de ton reinhard pour rendre possible une luminosité
 * de 1.0
 * version locale */
static auto mappage_ton_reinhard2(float x, float point_blanc)
{
	return (x * (1.0f + x / (point_blanc * point_blanc))) / (1.0f + x);
}

/* version globale, il nous faut la luminance moyenne de l'image */
static auto mappage_ton_reinhard2(float x, float lum_moyenne, float point_blanc)
{
	x = (x / (9.6f * lum_moyenne + 0.00001f));
	return mappage_ton_reinhard2(x, point_blanc);
}

static auto mappage_ton_ACES(float x) {
	// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
	auto const a = 2.51f;
	auto const b = 0.03f;
	auto const c = 2.43f;
	auto const d = 0.59f;
	auto const e = 0.14f;
	return (x * (a * x + b)) / (x * (c * x + d) + e);
}

static auto mappage_ton_unreal(float x)
{
	// Unreal 3, Documentation: "Color Grading"
	// Adapted to be close to Tonemap_ACES, with similar range
	// Gamma 2.2 correction is baked in, don't use with sRGB conversion!
	return x / (x + 0.155f) * 1.019f;
}

template <typename T>
auto marche(T y, T x)
{
	return (y >= x) ? T(1) : T(0);
}

static auto mappage_ton_uchimura(float x, float P, float a, float m, float l, float c, float b)
{
	// Uchimura 2017, "HDR theory and practice"
	// Math: https://www.desmos.com/calculator/gslcdxvipg
	// Source: https://www.slideshare.net/nikuque/hdr-theory-and-practicce-jp
	auto const l0 = ((P - m) * l) / a;
//	auto const L0 = m - m / a;
//	auto const L1 = m + (1.0f - m) / a;
	auto const S0 = m + l0;
	auto const S1 = m + a * l0;
	auto const C2 = (a * P) / (P - S1);
	auto const CP = -C2 / P;

	auto const w0 = 1.0f - dls::math::entrepolation_fluide<1>(x, 0.0f, m);
	auto const w2 = marche(m + l0, x);
	auto const w1 = 1.0f - w0 - w2;

	auto const T = m * std::pow(x / m, c) + b;
	auto const S = P - (P - S1) * std::exp(CP * (x - S0));
	auto const L = m + a * (x - m);

	return T * w0 + L * w1 + S * w2;
}

static auto mappage_ton_uchimura(float x)
{
	auto const P = 1.0f;  // max display brightness
	auto const a = 1.0f;  // contrast
	auto const m = 0.22f; // linear section start
	auto const l = 0.4f;  // linear section length
	auto const c = 1.33f; // black
	auto const b = 0.0f;  // pedestal
	return mappage_ton_uchimura(x, P, a, m, l, c, b);
}

static auto mappage_ton_lottes(float x)
{
	// Lottes 2016, "Advanced Techniques and Optimization of HDR Color Pipelines"
	auto const a = 1.6f;
	auto const d = 0.977f;
	auto const hdrMax = 8.0f;
	auto const midIn = 0.18f;
	auto const midOut = 0.267f;

	// Can be precomputed
	const float b =
		(-std::pow(midIn, a) + std::pow(hdrMax, a) * midOut) /
		((std::pow(hdrMax, a * d) - std::pow(midIn, a * d)) * midOut);
	const float c =
		(std::pow(hdrMax, a * d) * std::pow(midIn, a) - std::pow(hdrMax, a) * std::pow(midIn, a * d) * midOut) /
		((std::pow(hdrMax, a * d) - std::pow(midIn, a * d)) * midOut);

	return std::pow(x, a) / (std::pow(x, a * d) * b + c);
}

static auto mappage_ton_hbd(float x)
{
	x = std::max(0.0f, x - 0.004f);
	return (x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f);
}

static auto mappage_ton_courbe_hpd(float x)
{
	auto const ld = 0.002f;
	auto const lin_reference = 0.18f;
	auto const log_reference = 444.0f;
	auto const logGamma = 0.455f;

	return (std::log(0.4f * x / lin_reference) / ld * logGamma + log_reference) / 1023.0f;
}

static float mappage_ton_uncharted_impl(float x)
{
	static constexpr auto A = 0.15f;
	static constexpr auto B = 0.50f;
	static constexpr auto C = 0.10f;
	static constexpr auto D = 0.20f;
	static constexpr auto E = 0.02f;
	static constexpr auto F = 0.30f;

	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

static float mappage_ton_uncharted(float x)
{
	static constexpr auto W = 11.2f;

	auto const exposure_bias = 2.0f;
	auto const white_scale = 1.0f / mappage_ton_uncharted_impl(W);

	return mappage_ton_uncharted_impl(exposure_bias * x) * white_scale;
}

class OperatriceMappageTonal final : public OperatricePixel {
	enum {
		MAPPAGE_TON_LINEAIRE,
		MAPPAGE_TON_ACES,
		MAPPAGE_TON_HBD,
		MAPPAGE_TON_HPDCURVE,
		MAPPAGE_TON_LOTTES,
		MAPPAGE_TON_REINHARD,
		MAPPAGE_TON_REINHARD2,
		MAPPAGE_TON_UNCHARTED,
		MAPPAGE_TON_UCHIMURA,
		MAPPAGE_TON_UNREAL,
	};

	int m_type = MAPPAGE_TON_LINEAIRE;
	float m_exposition = 0.0f;
	float m_gamma = 0.45f;
	dls::phys::couleur32 m_point_blanc{};

	REMBOURRE(4);

public:
	static constexpr auto NOM = "Mappage Tonal";
	static constexpr auto AIDE = "Applique un mappage de ton local à l'image.";

	OperatriceMappageTonal(Graphe &graphe_parent, Noeud &noeud)
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

	void evalue_entrees(int temps) override
	{
		auto type = evalue_enum("type");

		if (type == "linéaire") {
			m_type = MAPPAGE_TON_LINEAIRE;
		}
		else if (type == "aces") {
			m_type = MAPPAGE_TON_ACES;
		}
		else if (type == "lottes") {
			m_type = MAPPAGE_TON_LOTTES;
		}
		else if (type == "reinhard") {
			m_type = MAPPAGE_TON_REINHARD;
		}
		else if (type == "reinhard2") {
			m_type = MAPPAGE_TON_REINHARD2;
		}
		else if (type == "courbe_hpd") {
			m_type = MAPPAGE_TON_HPDCURVE;
		}
		else if (type == "HBD") {
			m_type = MAPPAGE_TON_HBD;
		}
		else if (type == "uncharted_2") {
			m_type = MAPPAGE_TON_UNCHARTED;
		}
		else if (type == "unreal") {
			m_type = MAPPAGE_TON_UNREAL;
		}
		else if (type == "uchimura") {
			m_type = MAPPAGE_TON_UCHIMURA;
		}

		m_exposition = std::pow(2.0f, evalue_decimal("exposition", temps));
		m_gamma = evalue_decimal("gamma", temps);
		m_point_blanc = evalue_couleur("point_blanc");
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(x);
		INUTILISE(y);

		auto resultat = pixel;
		auto besoin_correction_gamma = true;

		resultat.r *= m_exposition;
		resultat.v *= m_exposition;
		resultat.b *= m_exposition;

		switch (m_type) {
			case MAPPAGE_TON_LINEAIRE:
			{
				break;
			}
			case MAPPAGE_TON_ACES:
			{
				resultat.r = mappage_ton_ACES(resultat.r);
				resultat.v = mappage_ton_ACES(resultat.v);
				resultat.b = mappage_ton_ACES(resultat.b);
				break;
			}
			case MAPPAGE_TON_HBD:
			{
				resultat.r = mappage_ton_hbd(resultat.r);
				resultat.v = mappage_ton_hbd(resultat.v);
				resultat.b = mappage_ton_hbd(resultat.b);
				besoin_correction_gamma = false;
				break;
			}
			case MAPPAGE_TON_HPDCURVE:
			{
				resultat.r = mappage_ton_courbe_hpd(resultat.r);
				resultat.v = mappage_ton_courbe_hpd(resultat.v);
				resultat.b = mappage_ton_courbe_hpd(resultat.b);
				besoin_correction_gamma = false;
				break;
			}
			case MAPPAGE_TON_LOTTES:
			{
				resultat.r = mappage_ton_lottes(resultat.r);
				resultat.v = mappage_ton_lottes(resultat.v);
				resultat.b = mappage_ton_lottes(resultat.b);
				break;
			}
			case MAPPAGE_TON_REINHARD:
			{
				resultat.r = mappage_ton_reinhard(resultat.r);
				resultat.v = mappage_ton_reinhard(resultat.v);
				resultat.b = mappage_ton_reinhard(resultat.b);
				break;
			}
			case MAPPAGE_TON_REINHARD2:
			{
				resultat.r = mappage_ton_reinhard2(resultat.r, m_point_blanc.r);
				resultat.v = mappage_ton_reinhard2(resultat.v, m_point_blanc.v);
				resultat.b = mappage_ton_reinhard2(resultat.b, m_point_blanc.b);
				break;
			}
			case MAPPAGE_TON_UNCHARTED:
			{
				resultat.r = mappage_ton_uncharted(resultat.r);
				resultat.v = mappage_ton_uncharted(resultat.v);
				resultat.b = mappage_ton_uncharted(resultat.b);
				break;
			}
			case MAPPAGE_TON_UCHIMURA:
			{
				resultat.r = mappage_ton_uchimura(resultat.r);
				resultat.v = mappage_ton_uchimura(resultat.v);
				resultat.b = mappage_ton_uchimura(resultat.b);
				break;
			}
			case MAPPAGE_TON_UNREAL:
			{
				resultat.r = mappage_ton_unreal(resultat.r);
				resultat.v = mappage_ton_unreal(resultat.v);
				resultat.b = mappage_ton_unreal(resultat.b);
				besoin_correction_gamma = false;
				break;
			}
		}

		/* Adjust for the monitor's gamma. */
		if (besoin_correction_gamma) {
			resultat.r = std::pow(resultat.r, m_gamma);
			resultat.v = std::pow(resultat.v, m_gamma);
			resultat.b = std::pow(resultat.b, m_gamma);
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

	OperatriceCorrectionCouleur(Graphe &graphe_parent, Noeud &noeud)
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
		INUTILISE(temps);
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

	OperatriceInversement(Graphe &graphe_parent, Noeud &noeud)
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

	REMBOURRE(4);

public:
	static constexpr auto NOM = "Incrustation";
	static constexpr auto AIDE = "Supprime les couleurs vertes d'une image.";

	OperatriceIncrustation(Graphe &graphe_parent, Noeud &noeud)
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

	REMBOURRE(7);

public:
	static constexpr auto NOM = "Pré-multiplication";
	static constexpr auto AIDE = "Prémultiplie les couleurs des pixels par leurs valeurs alpha respectives.";

	OperatricePremultiplication(Graphe &graphe_parent, Noeud &noeud)
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

	OperatriceNormalisationPixel(Graphe &graphe_parent, Noeud &noeud)
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

	OperatriceContraste(Graphe &graphe_parent, Noeud &noeud)
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

	OperatriceCourbeCouleur(Graphe &graphe_parent, Noeud &noeud)
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

	OperatriceTraduction(Graphe &graphe_parent, Noeud &noeud)
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

	OperatriceMinMax(Graphe &graphe_parent, Noeud &noeud)
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

	OperatriceDaltonisme(Graphe &graphe_parent, Noeud &noeud)
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

#include "biblinternes/bruit/evaluation.hh"

class OperatriceBruit final : public OperatricePixel {
	dls::math::vec3f m_frequence = dls::math::vec3f(1.0f);
	dls::math::vec3f m_decalage = dls::math::vec3f(0.0f);
	float m_amplitude = 1.0f;
	int m_octaves = 1.0f;
	float m_lacunarite = 1.0f;
	float m_durete = 1.0f;
	int m_dimensions = 1;
	bool m_dur = false;
	bool m_turb = true;

	REMBOURRE(2);

	bruit::parametres m_params_bruit{};
	bruit::param_turbulence m_params_turb{};

public:
	static constexpr auto NOM = "Bruit Image";
	static constexpr auto AIDE = "";

	OperatriceBruit(Graphe &graphe_parent, Noeud &noeud)
		: OperatricePixel(graphe_parent, noeud)
	{
		entrees(0);
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
		m_frequence = evalue_vecteur("fréquence", temps);
		m_decalage = evalue_vecteur("décalage", temps);
		m_amplitude = evalue_decimal("amplitude", temps);
		m_octaves = evalue_entier("octaves", temps);
		m_lacunarite = evalue_decimal("lacunarité", temps);
		m_durete = evalue_decimal("persistence", temps);
		m_dur = evalue_bool("dur");

		auto dico_type = dls::cree_dico(
					dls::paire(dls::chaine("cellule"), bruit::type::CELLULE),
					dls::paire(dls::chaine("fourier"), bruit::type::FOURIER),
					dls::paire(dls::chaine("ondelette"), bruit::type::ONDELETTE),
					dls::paire(dls::chaine("simplex"), bruit::type::SIMPLEX),
					dls::paire(dls::chaine("voronoi_f1"), bruit::type::VORONOI_F1),
					dls::paire(dls::chaine("voronoi_f2"), bruit::type::VORONOI_F2),
					dls::paire(dls::chaine("voronoi_f3"), bruit::type::VORONOI_F3),
					dls::paire(dls::chaine("voronoi_f4"), bruit::type::VORONOI_F4),
					dls::paire(dls::chaine("voronoi_f1f2"), bruit::type::VORONOI_F1F2),
					dls::paire(dls::chaine("voronoi_cr"), bruit::type::VORONOI_CR));

		auto chn_type = evalue_enum("type");
		auto plg_type = dico_type.trouve(chn_type);
		auto type = bruit::type{};

		if (plg_type.est_finie()) {
			ajoute_avertissement("type inconnu");
			type = bruit::type::ONDELETTE;
		}
		else {
			type = plg_type.front().second;
		}

		m_params_turb.octaves = static_cast<float>(m_octaves) * 1.618f;
		m_params_turb.lacunarite = m_lacunarite;
		m_params_turb.gain = m_durete;
		m_params_turb.echelle = 10.0f;
		m_params_turb.amplitude = m_amplitude;
		m_params_turb.dur = m_dur;

		if (m_turb) {
			bruit::construit_turb(type, 0, m_params_bruit, m_params_turb);
		}
		else {
			bruit::construit(type, m_params_bruit, 0);
		}
	}

	dls::phys::couleur32 evalue_pixel(dls::phys::couleur32 const &pixel, const float x, const float y) override
	{
		INUTILISE(pixel);

		auto res = 0.0f;

		if (m_turb) {
			res = bruit::evalue_turb(m_params_bruit, m_params_turb, dls::math::vec3f(x, y, 0.0f));
		}
		else {
			res = bruit::evalue(m_params_bruit, dls::math::vec3f(x, y, 0.0f));
		}

		auto rp = dls::phys::couleur32();
		rp.r = res;
		rp.v = res;
		rp.b = res;
		rp.a = 1.0f;
		return rp;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_pixel(UsineOperatrice &usine)
{
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
	usine.enregistre_type(cree_desc<OperatricePremultiplication>());
	usine.enregistre_type(cree_desc<OperatriceNormalisationPixel>());
	usine.enregistre_type(cree_desc<OperatriceContraste>());
	usine.enregistre_type(cree_desc<OperatriceCourbeCouleur>());
	usine.enregistre_type(cree_desc<OperatriceTraduction>());
	usine.enregistre_type(cree_desc<OperatriceMinMax>());
	usine.enregistre_type(cree_desc<OperatriceDaltonisme>());
	usine.enregistre_type(cree_desc<OperatriceBruit>());
}

#pragma clang diagnostic pop
