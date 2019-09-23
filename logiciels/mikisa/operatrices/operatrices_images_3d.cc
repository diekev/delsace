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

#include "operatrices_images_3d.hh"

#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/outils/gna.hh"

#include "coeur/base_de_donnees.hh"
#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

/**
 * Création d'un nuage de points basé sur une propriété d'une image, en somme
 * visualisation 3-d d'un histogramme.
 *
 * À CONSIDÉRER :
 * - utilisation d'un calque séparé pour placer les points (passe rendu position)
 * - utilisation d'un calque séparé pour fournir les couleurs
 * - utilisation d'une caméra pour placer les points (pour une passe rendu)
 * - fusion avec l'opératrice points depuis profonde
 * - commandes pour créer et choisir le composite ou son noeud
 * - utilisation d'une expression pour définir ce qu'il faut observer (histogramme personnalisé)
 */
class OpPointsDepuisImage final : public OperatriceCorps {
	enum {
		VIS_SATURATION,
		VIS_LUMINANCE,
		VIS_TEINTE,
	};

public:
	static constexpr auto NOM = "Points depuis Image";
	static constexpr auto AIDE = "Crée un histogramme 3d depuis les pixels d'une image pour mieux l'analyser.";

	OpPointsDepuisImage(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(0);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_points_depuis_image.jo";
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

		auto noeud_image = cherche_noeud_pour_chemin(*contexte.bdd, chemin_noeud);

		if (noeud_image == nullptr) {
			this->ajoute_avertissement("Impossible de trouver le noeud !");
			return EXECUTION_ECHOUEE;
		}

		if (noeud_image->type != type_noeud::OPERATRICE) {
			this->ajoute_avertissement("Le noeud n'est pas un noeud composite !");
			return EXECUTION_ECHOUEE;
		}

		auto op = extrait_opimage(noeud_image->donnees);
		auto image = op->image();

		auto calque = image->calque_pour_lecture("image");

		if (calque == nullptr) {
			this->ajoute_avertissement("Calque « image » introuvable !");
			return EXECUTION_ECHOUEE;
		}

		auto gna = GNA();
		auto probabilite = evalue_decimal("probabilité");

		if (probabilite == 0.0f) {
			return EXECUTION_REUSSIE;
		}

		auto chn_visualisation = evalue_enum("visualisation");
		auto visualisation = VIS_SATURATION;

		if (chn_visualisation == "teinte") {
			visualisation = VIS_TEINTE;
		}
		else if (chn_visualisation == "luminance") {
			visualisation = VIS_LUMINANCE;
		}

		auto tampon = extrait_grille_couleur(calque);
		auto desc = tampon->desc();

		auto nombre_de_pixels = tampon->nombre_elements();

		auto chef = contexte.chef;
		chef->demarre_evaluation("points depuis image");

		auto points = m_corps.points_pour_ecriture();
		points->redimensionne(nombre_de_pixels);

		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::R32, 3);

		boucle_parallele(tbb::blocked_range<int>(0, desc.resolution.y),
						 [&](tbb::blocked_range<int> const &plage)
		{
			if (chef->interrompu()) {
				return;
			}

			for (auto y = plage.begin(); y < plage.end(); ++y) {
				if (chef->interrompu()) {
					return;
				}

				for (auto x = 0; x < desc.resolution.x; ++x) {
					if (probabilite != 1.0f) {
						if (gna.uniforme(0.0f, 1.0f) >= probabilite) {
							continue;
						}
					}

					auto index = tampon->calcul_index(dls::math::vec2i(x, y));
					auto res = dls::phys::couleur32();
					auto rvb = tampon->valeur(index);

					/* préservation de l'aspect -> division par la même dimension */
					auto px = static_cast<float>(x) / static_cast<float>(desc.resolution.x);
					auto pz = static_cast<float>(y) / static_cast<float>(desc.resolution.x);
					auto py = 0.0f;

					if (visualisation == VIS_TEINTE) {
						dls::phys::rvb_vers_hsv(rvb, res);
						py = res.r;
					}
					else if (visualisation == VIS_SATURATION) {
						dls::phys::rvb_vers_hsv(rvb, res);
						py = res.v;
					}
					else if (visualisation == VIS_LUMINANCE) {
						py = dls::phys::luminance(rvb);
					}

					points->point(index, dls::math::vec3f(px, py, pz));

					assigne(attr_C->r32(index), dls::math::vec3f(rvb.r, rvb.v, rvb.b));
				}
			}

			auto delta = static_cast<float>(plage.end() - plage.begin());
			delta /= static_cast<float>(desc.resolution.y);
			chef->indique_progression_parallele(delta * 100.0f);
		});

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_images_3d(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OpPointsDepuisImage>());
}

#pragma clang diagnostic pop
