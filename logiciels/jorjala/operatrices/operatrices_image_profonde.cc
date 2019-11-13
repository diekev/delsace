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
#include "biblinternes/outils/constantes.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/vision/camera.h"

#include "coeur/base_de_donnees.hh"
#include "coeur/chef_execution.hh"
#include "coeur/composite.h"
#include "coeur/contexte_evaluation.hh"
#include "coeur/noeud_image.h"
#include "coeur/objet.h"
#include "coeur/operatrice_corps.h"
#include "coeur/operatrice_image.h"
#include "coeur/usine_operatrice.h"

#include "evaluation/reseau.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

static Image const *cherche_image_profonde(
		OperatriceImage &op,
		int index,
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval,
		int index_lien = 0)
{
	auto image = op.entree(index)->requiers_image(contexte, donnees_aval, index_lien);

	if (image == nullptr) {
		op.ajoute_avertissement("Aucune image trouvée dans l'entrée à l'index ", index, ", lien ", index_lien, " !");
		return nullptr;
	}

	if (!image->est_profonde) {
		op.ajoute_avertissement("L'image à l'index ", index, "n'est pas profonde !");
		return nullptr;
	}

	return image;
}

/* ************************************************************************** */

static auto fusionne_echant_surposes(
		float a1, float c1,
		float a2, float c2,
		float &am, float &cm)
{
	a1 = std::max(0.0f, std::min(a1, 1.0f));
	a2 = std::max(0.0f, std::min(a2, 1.0f));
	am = a1 + a2 - a1 * a2;
	if (a1 == 1.0f && a2 == 1.0f) {
		cm = (c1 + c2) / 2;
	}
	else if (a1 == 1.0f) {
		cm = c1;
	}
	else if (a2 == 1.0f) {
		cm = c2;
	}
	else {
		constexpr auto MAX = std::numeric_limits<float>::max();
		auto u1 = -std::log1p(-a1);
		auto v1 = (u1 < a1 * MAX)? u1 / a1 : 1.0f;
		auto u2 = -std::log1p(-a2);
		auto v2 = (u2 < a2 * MAX)? u2 / a2 : 1.0f;
		auto u = u1 + u2;
		auto w = (u > 1 || am < u * MAX)? am / u: 1;
		cm = (c1 * v1 + c2 * v2) * w;
	}
}

class OpAplanisProfonde final : public OperatriceImage {
public:
	static constexpr auto NOM = "Aplanis Image Profonde";
	static constexpr auto AIDE = "";

	OpAplanisProfonde(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
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

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto image = cherche_image_profonde(*this, 0, contexte, donnees_aval);

		if (image == nullptr) {
			return res_exec::ECHOUEE;
		}

		auto S = image->calque_profond_pour_lecture("S");
		auto R = image->calque_profond_pour_lecture("R");
		auto G = image->calque_profond_pour_lecture("G");
		auto B = image->calque_profond_pour_lecture("B");
		auto A = image->calque_profond_pour_lecture("A");
		auto Z = image->calque_profond_pour_lecture("Z");

		auto tampon_S = dynamic_cast<wlk::grille_dense_2d<unsigned> const *>(S->tampon());
		auto tampon_R = dynamic_cast<wlk::grille_dense_2d<float *> const *>(R->tampon());
		auto tampon_G = dynamic_cast<wlk::grille_dense_2d<float *> const *>(G->tampon());
		auto tampon_B = dynamic_cast<wlk::grille_dense_2d<float *> const *>(B->tampon());
		auto tampon_A = dynamic_cast<wlk::grille_dense_2d<float *> const *>(A->tampon());
		auto tampon_Z = dynamic_cast<wlk::grille_dense_2d<float *> const *>(Z->tampon());

		auto largeur = tampon_S->desc().resolution.x;
		auto hauteur = tampon_S->desc().resolution.y;

		auto calque = m_image.ajoute_calque("image", tampon_S->desc(), wlk::type_grille::COULEUR);
		auto tampon = extrait_grille_couleur(calque);

		auto chef = contexte.chef;
		chef->demarre_evaluation("aplanis profonde");

		boucle_parallele(tbb::blocked_range<long>(0, largeur),
						 [&](tbb::blocked_range<long> const &plage)
		{
			if (chef->interrompu()) {
				return;
			}

			for (auto j = plage.begin(); j < plage.end(); ++j) {
				for (auto i = 0; i < hauteur; ++i) {
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

					for (auto e = 1u; e < n; ++e) {
						auto ej = e;

						while (ej > 0 && sZ[ej - 1] > sZ[ej]) {
							std::swap(sR[ej], sR[ej - 1]);
							std::swap(sG[ej], sG[ej - 1]);
							std::swap(sB[ej], sB[ej - 1]);
							std::swap(sA[ej], sA[ej - 1]);
							std::swap(sZ[ej], sZ[ej - 1]);

							ej -= 1;
						}
					}

					/* À FAIRE : Étape 2 : split */

					/* À FAIRE : Étape 3 : fusionne */
//					for (auto i = 1u; i < n; ++i) {
//						if (sZ[i] == sZ[i - 1]) {
//							sR[i] = fusionne_echant_surposes(sR[i], sA[i], sR[i - 1], sA[i - 1], sR[i], sA[i])
//						}
//					}

					/* compose les échantillons */

					auto pixel = dls::phys::couleur32();
					pixel.r = sR[0];
					pixel.v = sG[0];
					pixel.b = sB[0];
					pixel.a = sA[0];

					for (auto e = 1u; e < n; ++e) {
						auto denum = 1.0f / (pixel.a + sA[e] * (1.0f - pixel.a));
						pixel.r = (pixel.r * pixel.a + sR[e] * sA[e] * (1.0f - pixel.a)) * denum;
						pixel.v = (pixel.v * pixel.a + sG[e] * sA[e] * (1.0f - pixel.a)) * denum;
						pixel.b = (pixel.b * pixel.a + sB[e] * sA[e] * (1.0f - pixel.a)) * denum;
						pixel.a = pixel.a + sA[e] * (1.0f - pixel.a);
					}

					tampon->valeur(index) = pixel;
				}
			}

			auto taille_totale = static_cast<float>(largeur);
			auto taille_plage = static_cast<float>(plage.end() - plage.begin());
			chef->indique_progression_parallele(taille_plage / taille_totale * 100.0f);
		});

		chef->indique_progression(100.0f);

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OpFusionProfonde final : public OperatriceImage {
public:
	static constexpr auto NOM = "Fusion Profondes";
	static constexpr auto AIDE = "";

	OpFusionProfonde(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
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

	bool connexions_multiples(int n) const override
	{
		return n == 0;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_image.reinitialise();

		auto images = dls::tableau<Image const *>();

		for (auto i = 0; i < entree(0)->nombre_connexions(); ++i) {
			auto img = cherche_image_profonde(*this, 0, contexte, donnees_aval, i);

			if (img != nullptr) {
				images.pousse(img);
			}
		}

		if (images.est_vide()) {
			return res_exec::ECHOUEE;
		}

		m_image.est_profonde = true;

		auto tampons_S = dls::tableau<wlk::grille_dense_2d<unsigned> const *>();
		auto tampons_R = dls::tableau<wlk::grille_dense_2d<float *> const *>();
		auto tampons_G = dls::tableau<wlk::grille_dense_2d<float *> const *>();
		auto tampons_B = dls::tableau<wlk::grille_dense_2d<float *> const *>();
		auto tampons_A = dls::tableau<wlk::grille_dense_2d<float *> const *>();
		auto tampons_Z = dls::tableau<wlk::grille_dense_2d<float *> const *>();

		auto desc = wlk::desc_grille_2d{};
		desc.taille_pixel = 1.0;
		desc.etendue.min = dls::math::vec2f( constantes<float>::INFINITE);
		desc.etendue.max = dls::math::vec2f(-constantes<float>::INFINITE);

		auto echantillons_total = 0l;

		for (auto img : images) {
			auto S = img->calque_profond_pour_lecture("S");
			auto R = img->calque_profond_pour_lecture("R");
			auto G = img->calque_profond_pour_lecture("G");
			auto B = img->calque_profond_pour_lecture("B");
			auto A = img->calque_profond_pour_lecture("A");
			auto Z = img->calque_profond_pour_lecture("Z");

			auto tampon_S = dynamic_cast<wlk::grille_dense_2d<unsigned> const *>(S->tampon());
			auto tampon_R = dynamic_cast<wlk::grille_dense_2d<float *> const *>(R->tampon());
			auto tampon_G = dynamic_cast<wlk::grille_dense_2d<float *> const *>(G->tampon());
			auto tampon_B = dynamic_cast<wlk::grille_dense_2d<float *> const *>(B->tampon());
			auto tampon_A = dynamic_cast<wlk::grille_dense_2d<float *> const *>(A->tampon());
			auto tampon_Z = dynamic_cast<wlk::grille_dense_2d<float *> const *>(Z->tampon());

			tampons_S.pousse(tampon_S);
			tampons_R.pousse(tampon_R);
			tampons_G.pousse(tampon_G);
			tampons_B.pousse(tampon_B);
			tampons_A.pousse(tampon_A);
			tampons_Z.pousse(tampon_Z);

			auto desc1 = tampon_S->desc();
			desc.etendue.min.x = std::min(desc.etendue.min.x, desc1.etendue.min.x);
			desc.etendue.min.y = std::min(desc.etendue.min.y, desc1.etendue.min.y);
			desc.etendue.max.x = std::max(desc.etendue.max.x, desc1.etendue.max.x);
			desc.etendue.max.y = std::max(desc.etendue.max.y, desc1.etendue.max.y);

			echantillons_total += R->echantillons.taille();
		}

		desc.fenetre_donnees = desc.etendue;

#if 0
		auto imprime_etendue = [](const char *message, dls::math::vec2f const &min, dls::math::vec2f const &max)
		{
			std::cerr << message << " : " << min << " - > " << max << '\n';
		};

		imprime_etendue("étendue grille  ", desc.etendue.min, desc.etendue.max);
#endif

		auto S = m_image.ajoute_calque_profond("S", desc, wlk::type_grille::N32);
		auto R = m_image.ajoute_calque_profond("R", desc, wlk::type_grille::R32_PTR);
		auto G = m_image.ajoute_calque_profond("G", desc, wlk::type_grille::R32_PTR);
		auto B = m_image.ajoute_calque_profond("B", desc, wlk::type_grille::R32_PTR);
		auto A = m_image.ajoute_calque_profond("A", desc, wlk::type_grille::R32_PTR);
		auto Z = m_image.ajoute_calque_profond("Z", desc, wlk::type_grille::R32_PTR);

		auto tampon_S = dynamic_cast<wlk::grille_dense_2d<unsigned> *>(S->tampon());
		auto tampon_R = dynamic_cast<wlk::grille_dense_2d<float *> *>(R->tampon());
		auto tampon_G = dynamic_cast<wlk::grille_dense_2d<float *> *>(G->tampon());
		auto tampon_B = dynamic_cast<wlk::grille_dense_2d<float *> *>(B->tampon());
		auto tampon_A = dynamic_cast<wlk::grille_dense_2d<float *> *>(A->tampon());
		auto tampon_Z = dynamic_cast<wlk::grille_dense_2d<float *> *>(Z->tampon());

		auto chef = contexte.chef;
		chef->demarre_evaluation("fusion profondes");

		R->echantillons.redimensionne(echantillons_total);
		G->echantillons.redimensionne(echantillons_total);
		B->echantillons.redimensionne(echantillons_total);
		A->echantillons.redimensionne(echantillons_total);
		Z->echantillons.redimensionne(echantillons_total);

		auto largeur = tampon_S->desc().resolution.x;
		auto hauteur = tampon_S->desc().resolution.y;
		auto courant = 0;
		auto decalage = 0l;

		for (auto j = 0; j < largeur; ++j) {
			if (chef->interrompu()) {
				break;
			}

			for (auto i = 0; i < hauteur; ++i, ++courant) {
				if (chef->interrompu()) {
					break;
				}

				auto const index = j + i * largeur;

				auto const pos_mnd = tampon_S->index_vers_monde(dls::math::vec2i(j, i));

				auto eR = &R->echantillons[decalage];
				auto eG = &G->echantillons[decalage];
				auto eB = &B->echantillons[decalage];
				auto eA = &A->echantillons[decalage];
				auto eZ = &Z->echantillons[decalage];

				auto decalage_n = 0u;
				for (auto t = 0; t < tampons_S.taille(); ++t) {
					auto const pos1 = tampons_S[t]->monde_vers_index(pos_mnd);
					auto const index1 = tampons_S[t]->calcul_index(pos1);
					auto const n1 = tampons_S[t]->valeur(pos1);

					if (n1 == 0) {
						continue;
					}

					auto eR1 = tampons_R[t]->valeur(index1);
					auto eG1 = tampons_G[t]->valeur(index1);
					auto eB1 = tampons_B[t]->valeur(index1);
					auto eA1 = tampons_A[t]->valeur(index1);
					auto eZ1 = tampons_Z[t]->valeur(index1);

					for (auto e = 0u; e < n1; ++e) {
						eR[e + decalage_n] = eR1[e];
						eG[e + decalage_n] = eG1[e];
						eB[e + decalage_n] = eB1[e];
						eA[e + decalage_n] = eA1[e];
						eZ[e + decalage_n] = eZ1[e];
					}

					decalage_n += n1;
				}

				decalage += decalage_n;

				tampon_S->valeur(index) = decalage_n;
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

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

class OpPointsDepuisProfonde final : public OperatriceCorps {
	dls::chaine m_nom_objet = "";
	Objet *m_objet = nullptr;

public:
	static constexpr auto NOM = "Point depuis Profonde";
	static constexpr auto AIDE = "";

	OpPointsDepuisProfonde(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(0);
		sorties(1);
	}

	OpPointsDepuisProfonde(OpPointsDepuisProfonde const &) = default;
	OpPointsDepuisProfonde &operator=(OpPointsDepuisProfonde const &) = default;

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

	Objet *trouve_objet(ContexteEvaluation const &contexte)
	{
		auto nom_objet = evalue_chaine("nom_caméra");

		if (nom_objet.est_vide()) {
			return nullptr;
		}

		if (nom_objet != m_nom_objet || m_objet == nullptr) {
			m_nom_objet = nom_objet;
			m_objet = contexte.bdd->objet(nom_objet);
		}

		return m_objet;
	}

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		auto noeud_image = cherche_noeud_image(contexte);

		if (noeud_image == nullptr) {
			return res_exec::ECHOUEE;
		}

		auto op = extrait_opimage(noeud_image->donnees);
		auto image = op->image();

		if (!image->est_profonde) {
			this->ajoute_avertissement("L'image n'est pas profonde !");
			return res_exec::ECHOUEE;
		}

		m_objet = trouve_objet(contexte);

		if (m_objet == nullptr) {
			this->ajoute_avertissement("Ne peut pas trouver l'objet caméra !");
			return res_exec::ECHOUEE;
		}

		if (m_objet->type != type_objet::CAMERA) {
			this->ajoute_avertissement("L'objet n'est pas une caméra !");
			return res_exec::ECHOUEE;
		}

		auto S1 = image->calque_profond_pour_lecture("S");
		auto R1 = image->calque_profond_pour_lecture("R");
		auto G1 = image->calque_profond_pour_lecture("G");
		auto B1 = image->calque_profond_pour_lecture("B");
		//auto A1 = image->calque_profond_pour_lecture("A");
		auto Z1 = image->calque_profond_pour_lecture("Z");

		auto tampon_S1 = dynamic_cast<wlk::grille_dense_2d<unsigned> const *>(S1->tampon());
		auto tampon_R1 = dynamic_cast<wlk::grille_dense_2d<float *> const *>(R1->tampon());
		auto tampon_G1 = dynamic_cast<wlk::grille_dense_2d<float *> const *>(G1->tampon());
		auto tampon_B1 = dynamic_cast<wlk::grille_dense_2d<float *> const *>(B1->tampon());
		//auto tampon_A1 = dynamic_cast<wlk::grille_dense_2d<float *> const *>(A1->tampon());
		auto tampon_Z1 = dynamic_cast<wlk::grille_dense_2d<float *> const *>(Z1->tampon());

		auto largeur = tampon_S1->desc().resolution.x;
		auto hauteur = tampon_S1->desc().resolution.y;

		auto camera = static_cast<vision::Camera3D *>(nullptr);

		m_objet->donnees.accede_ecriture([&](DonneesObjet *donnees)
		{
			camera = &extrait_camera(donnees);
		});

		camera->ajourne_pour_operatrice();

		auto chef = contexte.chef;
		chef->demarre_evaluation("points depuis profonde");

		auto const total = static_cast<float>(largeur * hauteur);
		auto index = 0;

		auto min_z = std::numeric_limits<float>::max();
		auto gna = GNA{};
		auto const probabilite = evalue_decimal("probabilité");
		auto echelle = evalue_decimal("échelle");
		echelle = (echelle == 0.0f) ? 0.0f : 1.0f / echelle;

		auto C = m_corps.ajoute_attribut("C", type_attribut::R32, 3, portee_attr::POINT);
		auto points = m_corps.points_pour_ecriture();

		for (auto y = 0; y < hauteur; ++y) {
			for (auto x = 0; x < largeur; ++x, ++index) {
				auto n = tampon_S1->valeur(index);

				if (n == 0) {
					continue;
				}

				auto xf = static_cast<float>(x) / static_cast<float>(largeur);
				auto yf = static_cast<float>(y) / static_cast<float>(hauteur);

				/* 1 - xf car la caméra est inversée sur l'axe Y */
				auto point = dls::math::point3f(xf, 1.0f - yf, 0.0f);
				auto pmnd = camera->pos_monde(point);

				auto eR = tampon_R1->valeur(index);
				auto eG = tampon_G1->valeur(index);
				auto eB = tampon_B1->valeur(index);
				auto eZ = tampon_Z1->valeur(index);

				for (auto i = 0u; i < n; ++i) {
					/* -eZ nous donne la bonne orientation mais ne devrait-ce
					 * pas être eZ ? */
					pmnd.z = -eZ[i] * echelle;

					if (gna.uniforme(0.0f, 1.0f) >= probabilite) {
						continue;
					}

					min_z = std::min(pmnd.z, min_z);

					auto idx_point = points.ajoute_point(pmnd.x, pmnd.y, pmnd.z);

					auto couleur = dls::math::vec3f(eR[i], eG[i], eB[i]);
					assigne(C->r32(idx_point), couleur);
				}
			}

			chef->indique_progression(static_cast<float>(index) / total * 100.0f);
		}

		chef->indique_progression(100.0f);

		return res_exec::REUSSIE;
	}

	Noeud const *cherche_noeud_image(ContexteEvaluation const &contexte)
	{
		auto chemin_noeud = evalue_chaine("chemin_noeud");

		if (chemin_noeud == "") {
			this->ajoute_avertissement("Le chemin du noeud pour l'image est vide");
			return nullptr;
		}

		auto noeud_image = cherche_noeud_pour_chemin(*contexte.bdd, chemin_noeud);

		if (noeud_image == nullptr) {
			this->ajoute_avertissement("Impossible de trouver le noeud spécifié");
			return nullptr;
		}

		if (noeud_image->type != type_noeud::OPERATRICE) {
			this->ajoute_avertissement("Le noeud image n'est pas un composite");
			return nullptr;
		}

		return noeud_image;
	}

	void renseigne_dependance(ContexteEvaluation const &contexte, CompilatriceReseau &compilatrice, NoeudReseau *noeud_reseau) override
	{
		if (m_objet == nullptr) {
			m_objet = trouve_objet(contexte);

			if (m_objet != nullptr) {
				compilatrice.ajoute_dependance(noeud_reseau, m_objet->noeud);
			}
		}

		auto noeud_image = cherche_noeud_image(contexte);

		if (noeud_image == nullptr) {
			return;
		}

		compilatrice.ajoute_dependance(noeud_reseau, noeud_base_hierarchie(const_cast<Noeud *>(noeud_image)));
	}

	void obtiens_liste(
			ContexteEvaluation const &contexte,
			dls::chaine const &raison,
			dls::tableau<dls::chaine> &liste) override
	{
		if (raison == "nom_caméra") {
			for (auto &objet : contexte.bdd->objets()) {
				if (objet->type != type_objet::CAMERA) {
					continue;
				}

				liste.pousse(objet->noeud->nom);
			}
		}
	}

	void performe_versionnage() override
	{
		if (propriete("nom_caméra") == nullptr) {
			ajoute_propriete("nom_caméra", danjo::TypePropriete::CHAINE_CARACTERE, dls::chaine(""));
		}

		if (propriete("probabilité") == nullptr) {
			ajoute_propriete("probabilité", danjo::TypePropriete::DECIMAL, 0.25f);
		}

		if (propriete("échelle") == nullptr) {
			ajoute_propriete("échelle", danjo::TypePropriete::DECIMAL, 100.0f);
		}
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
