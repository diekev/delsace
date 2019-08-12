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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "operatrices_image_profonde.hh"

#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/outils/chaine.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/operatrice_image.h"
#include "coeur/usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static Image const *cherche_image_profonde(
		OperatriceImage &op,
		int index,
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval)
{
	auto image = op.entree(index)->requiers_image(contexte, donnees_aval);

	if (image == nullptr) {
		auto flux = dls::flux_chaine();
		flux << "Aucune image trouvée dans l'entrée à l'index " << index << " !";
		op.ajoute_avertissement(flux.chn());
		return nullptr;
	}

	if (!image->est_profonde) {
		auto flux = dls::flux_chaine();
		flux << "L'image à l'index " << index << "n'est pas profonde !";
		op.ajoute_avertissement(flux.chn());
		return nullptr;
	}

	return image;
}

/* ************************************************************************** */

class OpAplanisProfonde : public OperatriceImage {
public:
	static constexpr auto NOM = "Aplanis Image Profonde";
	static constexpr auto AIDE = "";

	OpAplanisProfonde(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image = cherche_image_profonde(*this, 0, contexte, donnees_aval);

		if (image == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto S = image->calque_profond("S");
		auto R = image->calque_profond("R");
		auto G = image->calque_profond("G");
		auto B = image->calque_profond("B");
		auto A = image->calque_profond("A");
		auto Z = image->calque_profond("Z");

		auto tampon_S = dynamic_cast<wlk::grille_dense_2d<unsigned> *>(S->tampon);
		auto tampon_R = dynamic_cast<wlk::grille_dense_2d<float *> *>(R->tampon);
		auto tampon_G = dynamic_cast<wlk::grille_dense_2d<float *> *>(G->tampon);
		auto tampon_B = dynamic_cast<wlk::grille_dense_2d<float *> *>(B->tampon);
		auto tampon_A = dynamic_cast<wlk::grille_dense_2d<float *> *>(A->tampon);
		auto tampon_Z = dynamic_cast<wlk::grille_dense_2d<float *> *>(Z->tampon);

		auto largeur = tampon_S->desc().resolution.x;
		auto hauteur = tampon_S->desc().resolution.y;

		auto rectangle = contexte.resolution_rendu;
		auto tampon = m_image.ajoute_calque("image", contexte.resolution_rendu);

		auto debut_x = std::max(0l, static_cast<long>(rectangle.x));
		auto fin_x = static_cast<long>(std::min(largeur, largeur));
		auto debut_y = std::max(0l, static_cast<long>(rectangle.y));
		auto fin_y = static_cast<long>(std::min(hauteur, hauteur));

		auto chef = contexte.chef;
		chef->demarre_evaluation("aplanis profonde");

		boucle_parallele(tbb::blocked_range<long>(debut_x, fin_x),
						 [&](tbb::blocked_range<long> const &plage)
		{
			if (chef->interrompu()) {
				return;
			}

			for (auto j = plage.begin(); j < plage.end(); ++j) {
				for (auto i = debut_y; i < fin_y; ++i) {
					if (chef->interrompu()) {
						return;
					}

					auto index = j + i * largeur;
					auto n = tampon_S->valeur(index);

					if (n == 0) {
						continue;
					}

					/* tidy image */

					/* Étape 1 : tri les échantillons */
					auto sZ = tampon_Z->valeur(index);
					auto sR = tampon_R->valeur(index);
					auto sG = tampon_G->valeur(index);
					auto sB = tampon_B->valeur(index);
					auto sA = tampon_A->valeur(index);

					for (auto i = 1u; i < n; ++i) {
						while (sZ[i] > sZ[i - 1]) {
							std::swap(sR[i], sR[i - 1]);
							std::swap(sG[i], sG[i - 1]);
							std::swap(sB[i], sB[i - 1]);
							std::swap(sA[i], sA[i - 1]);
							std::swap(sZ[i], sZ[i - 1]);
						}
					}

					/* À FAIRE : Étape 2 : split */

					/* À FAIRE : Étape 3 : fusionne */

					/* compose les échantillons */

					auto pixel = dls::image::PixelFloat();
					pixel.r = sR[0];
					pixel.g = sG[0];
					pixel.b = sB[0];
					pixel.a = sA[0];

					for (auto s = 1u; s < n; ++s) {
//						pixel.r = pixel.r * sA[s - 1] + sR[s] * (1.0f - sA[s]);
//						pixel.g = pixel.g * sA[s - 1] + sG[s] * (1.0f - sA[s]);
//						pixel.b = pixel.b * sA[s - 1] + sB[s] * (1.0f - sA[s]);
//						pixel.a = pixel.a * sA[s - 1] + sA[s] * (1.0f - sA[s]);
						pixel.r = std::max(pixel.r, sR[s]);
						pixel.g = std::max(pixel.g, sG[s]);
						pixel.b = std::max(pixel.b, sB[s]);
						pixel.a = std::max(pixel.a, sA[s]);
					}

					tampon->valeur(j, i, pixel);
				}
			}

			auto taille_totale = static_cast<float>(fin_x - debut_x);
			auto taille_plage = static_cast<float>(plage.end() - plage.begin());
			chef->indique_progression_parallele(taille_plage / taille_totale * 100.0f);
		});

		chef->indique_progression(100.0f);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OpFusionProfonde : public OperatriceImage {
public:
	static constexpr auto NOM = "Fusion Profondes";
	static constexpr auto AIDE = "";

	OpFusionProfonde(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceImage(graphe_parent, noeud)
	{
		entrees(2);
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

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image1 = cherche_image_profonde(*this, 0, contexte, donnees_aval);

		if (image1 == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		auto image2 = cherche_image_profonde(*this, 1, contexte, donnees_aval);

		if (image2 == nullptr) {
			return EXECUTION_ECHOUEE;
		}

		m_image.est_profonde = true;

		auto S1 = image1->calque_profond("S");
		auto R1 = image1->calque_profond("R");
		auto G1 = image1->calque_profond("G");
		auto B1 = image1->calque_profond("B");
		auto A1 = image1->calque_profond("A");
		auto Z1 = image1->calque_profond("Z");

		auto tampon_S1 = dynamic_cast<wlk::grille_dense_2d<unsigned> *>(S1->tampon);
		auto tampon_R1 = dynamic_cast<wlk::grille_dense_2d<float *> *>(R1->tampon);
		auto tampon_G1 = dynamic_cast<wlk::grille_dense_2d<float *> *>(G1->tampon);
		auto tampon_B1 = dynamic_cast<wlk::grille_dense_2d<float *> *>(B1->tampon);
		auto tampon_A1 = dynamic_cast<wlk::grille_dense_2d<float *> *>(A1->tampon);
		auto tampon_Z1 = dynamic_cast<wlk::grille_dense_2d<float *> *>(Z1->tampon);

		auto S2 = image2->calque_profond("S");
		auto R2 = image2->calque_profond("R");
		auto G2 = image2->calque_profond("G");
		auto B2 = image2->calque_profond("B");
		auto A2 = image2->calque_profond("A");
		auto Z2 = image2->calque_profond("Z");

		auto tampon_S2 = dynamic_cast<wlk::grille_dense_2d<unsigned> *>(S2->tampon);
		auto tampon_R2 = dynamic_cast<wlk::grille_dense_2d<float *> *>(R2->tampon);
		auto tampon_G2 = dynamic_cast<wlk::grille_dense_2d<float *> *>(G2->tampon);
		auto tampon_B2 = dynamic_cast<wlk::grille_dense_2d<float *> *>(B2->tampon);
		auto tampon_A2 = dynamic_cast<wlk::grille_dense_2d<float *> *>(A2->tampon);
		auto tampon_Z2 = dynamic_cast<wlk::grille_dense_2d<float *> *>(Z2->tampon);

		/* alloue calques profonds */
		auto largeur = tampon_S1->desc().resolution.x;
		auto hauteur = tampon_S1->desc().resolution.y;

		auto S = m_image.ajoute_calque_profond("S", largeur, hauteur, wlk::type_grille::N32);
		auto R = m_image.ajoute_calque_profond("R", largeur, hauteur, wlk::type_grille::R32_PTR);
		auto G = m_image.ajoute_calque_profond("G", largeur, hauteur, wlk::type_grille::R32_PTR);
		auto B = m_image.ajoute_calque_profond("B", largeur, hauteur, wlk::type_grille::R32_PTR);
		auto A = m_image.ajoute_calque_profond("A", largeur, hauteur, wlk::type_grille::R32_PTR);
		auto Z = m_image.ajoute_calque_profond("Z", largeur, hauteur, wlk::type_grille::R32_PTR);

		auto tampon_S = dynamic_cast<wlk::grille_dense_2d<unsigned> *>(S->tampon);
		auto tampon_R = dynamic_cast<wlk::grille_dense_2d<float *> *>(R->tampon);
		auto tampon_G = dynamic_cast<wlk::grille_dense_2d<float *> *>(G->tampon);
		auto tampon_B = dynamic_cast<wlk::grille_dense_2d<float *> *>(B->tampon);
		auto tampon_A = dynamic_cast<wlk::grille_dense_2d<float *> *>(A->tampon);
		auto tampon_Z = dynamic_cast<wlk::grille_dense_2d<float *> *>(Z->tampon);

		auto chef = contexte.chef;
		chef->demarre_evaluation("fusion profondes");

		auto courant = 0;

		for (auto j = 0; j < largeur; ++j) {
			if (chef->interrompu()) {
				break;
			}

			for (auto i = 0; i < hauteur; ++i, ++courant) {
				if (chef->interrompu()) {
					break;
				}

				auto const index = j + i * largeur;

				auto const n1 = tampon_S1->valeur(index);
				auto const n2 = tampon_S2->valeur(index);
				auto const n  = n1 + n2;

				if (n == 0) {
					continue;
				}

				auto eR = memoire::loge_tableau<float>("deep_r", n);
				auto eG = memoire::loge_tableau<float>("deep_g", n);
				auto eB = memoire::loge_tableau<float>("deep_b", n);
				auto eA = memoire::loge_tableau<float>("deep_a", n);
				auto eZ = memoire::loge_tableau<float>("deep_z", n);

				if (n1 != 0) {
					auto eR1 = tampon_R1->valeur(index);
					auto eG1 = tampon_G1->valeur(index);
					auto eB1 = tampon_B1->valeur(index);
					auto eA1 = tampon_A1->valeur(index);
					auto eZ1 = tampon_Z1->valeur(index);

					for (auto e = 0u; e < n1; ++e) {
						eR[e] = eR1[e];
						eG[e] = eG1[e];
						eB[e] = eB1[e];
						eA[e] = eA1[e];
						eZ[e] = eZ1[e];
					}
				}

				if (n2 != 0) {
					auto eR2 = tampon_R2->valeur(index);
					auto eG2 = tampon_G2->valeur(index);
					auto eB2 = tampon_B2->valeur(index);
					auto eA2 = tampon_A2->valeur(index);
					auto eZ2 = tampon_Z2->valeur(index);

					for (auto e = 0u; e < n2; ++e) {
						eR[e + n1] = eR2[e];
						eG[e + n1] = eG2[e];
						eB[e + n1] = eB2[e];
						eA[e + n1] = eA2[e];
						eZ[e + n1] = eZ2[e];
					}
				}

				tampon_S->valeur(index) = n;
				tampon_R->valeur(index) = eR;
				tampon_G->valeur(index) = eG;
				tampon_B->valeur(index) = eB;
				tampon_A->valeur(index) = eA;
				tampon_Z->valeur(index) = eZ;
			}

			auto total = static_cast<float>(largeur * hauteur);
			chef->indique_progression(static_cast<float>(courant) / total * 100.0f);
		}

		chef->indique_progression(100.0f);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

#include "biblinternes/vision/camera.h"

#include "coeur/base_de_donnees.hh"
#include "coeur/composite.h"
#include "coeur/noeud_image.h"

static Noeud *cherche_entite(BaseDeDonnees const &bdd, dls::chaine const &chemin)
{
	auto morceaux = dls::morcelle(chemin, '/');

	if (morceaux[0] == "composites") {
		if (morceaux.taille() < 2) {
			return nullptr;
		}

		auto composite = bdd.composite(morceaux[1]);

		if (composite == nullptr) {
			return nullptr;
		}

		if (morceaux.taille() < 3) {
			return nullptr;
		}

		for (auto noeud : composite->graph().noeuds()) {
			if (noeud->nom() == morceaux[2]) {
				return noeud;
			}
		}
	}

	return nullptr;
}

class OpPointsDepuisProfonde : public OperatriceCorps {
public:
	static constexpr auto NOM = "Point depuis Profonde";
	static constexpr auto AIDE = "";

	OpPointsDepuisProfonde(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(0);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_points_depuis_profonde.jo";
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
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		auto chemin_noeud = evalue_chaine("chemin_noeud");

		if (chemin_noeud.est_vide()) {
			this->ajoute_avertissement("Le chemin est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto noeud = cherche_entite(*contexte.bdd, chemin_noeud);

		if (noeud == nullptr) {
			this->ajoute_avertissement("Impossible de trouver le noeud !");
			return EXECUTION_ECHOUEE;
		}

		if (noeud->type() != NOEUD_IMAGE_DEFAUT) {
			this->ajoute_avertissement("Le noeud n'est pas un noeud composite !");
			return EXECUTION_ECHOUEE;
		}

		auto op = extrait_opimage(noeud->donnees());
		auto image = op->image();

		if (!image->est_profonde) {
			this->ajoute_avertissement("L'image n'est pas profonde !");
			return EXECUTION_ECHOUEE;
		}

		auto S1 = image->calque_profond("S");
		auto R1 = image->calque_profond("R");
		auto G1 = image->calque_profond("G");
		auto B1 = image->calque_profond("B");
		//auto A1 = image->calque_profond("A");
		auto Z1 = image->calque_profond("Z");

		auto tampon_S1 = dynamic_cast<wlk::grille_dense_2d<unsigned> *>(S1->tampon);
		auto tampon_R1 = dynamic_cast<wlk::grille_dense_2d<float *> *>(R1->tampon);
		auto tampon_G1 = dynamic_cast<wlk::grille_dense_2d<float *> *>(G1->tampon);
		auto tampon_B1 = dynamic_cast<wlk::grille_dense_2d<float *> *>(B1->tampon);
		//auto tampon_A1 = dynamic_cast<wlk::grille_dense_2d<float *> *>(A1->tampon);
		auto tampon_Z1 = dynamic_cast<wlk::grille_dense_2d<float *> *>(Z1->tampon);

		auto largeur = tampon_S1->desc().resolution.x;
		auto hauteur = tampon_S1->desc().resolution.y;

		auto camera = vision::Camera3D(largeur, hauteur);
		camera.ajourne_pour_operatrice();

		auto chef = contexte.chef;
		chef->demarre_evaluation("points depuis profonde");

		auto const total = static_cast<float>(largeur * hauteur);
		auto index = 0;

		auto min_z = std::numeric_limits<float>::max();

		auto C = m_corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);

		for (auto y = 0; y < hauteur; ++y) {
			for (auto x = 0; x < largeur; ++x, ++index) {
				auto n = tampon_S1->valeur(index);

				if (n == 0) {
					continue;
				}

				auto xf = static_cast<float>(x) / static_cast<float>(largeur);
				auto yf = static_cast<float>(y) / static_cast<float>(hauteur);

				auto point = dls::math::point3f(xf, yf, 0.0f);
				auto pmnd = camera.pos_monde(point);

				auto eR = tampon_R1->valeur(index);
				auto eG = tampon_G1->valeur(index);
				auto eB = tampon_B1->valeur(index);
				auto eZ = tampon_Z1->valeur(index);

				for (auto i = 0u; i < n; ++i) {
					pmnd.z = eZ[i];

					min_z = std::min(pmnd.z, min_z);

					m_corps.ajoute_point(pmnd.x, pmnd.y, pmnd.z);

					auto couleur = dls::math::vec3f(eR[i], eG[i], eB[i]);
					C->pousse(couleur);
				}
			}

			chef->indique_progression(static_cast<float>(index) / total * 100.0f);
		}

		chef->indique_progression(100.0f);

		auto points = m_corps.points_pour_ecriture();

		for (auto i = 0; i < points->taille(); ++i) {
			auto p = points->point(i);
			p.z -= min_z;
			points->point(i, p);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_image_profonde(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OpAplanisProfonde>());
	usine.enregistre_type(cree_desc<OpFusionProfonde>());
	usine.enregistre_type(cree_desc<OpPointsDepuisProfonde>());
}

#pragma clang diagnostic pop
