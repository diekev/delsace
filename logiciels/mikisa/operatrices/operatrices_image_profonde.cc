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

#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_image.h"
#include "coeur/usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

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

		auto image = entree(0)->requiers_image(contexte, donnees_aval);

		if (image == nullptr) {
			this->ajoute_avertissement("Aucune image trouvée en entrée !");
			return EXECUTION_ECHOUEE;
		}

		if (!image->est_profonde) {
			this->ajoute_avertissement("L'image d'entrée n'est pas profonde !");
			return EXECUTION_ECHOUEE;
		}

		auto S = image->calque_profond("S");
		auto R = image->calque_profond("R");
		auto G = image->calque_profond("G");
		auto B = image->calque_profond("B");
		auto A = image->calque_profond("A");
		//auto Z = image.calque_profond("Z");

		auto tampon_S = dynamic_cast<wlk::grille_dense_2d<unsigned> *>(S->tampon);
		auto tampon_R = dynamic_cast<wlk::grille_dense_2d<float *> *>(R->tampon);
		auto tampon_G = dynamic_cast<wlk::grille_dense_2d<float *> *>(G->tampon);
		auto tampon_B = dynamic_cast<wlk::grille_dense_2d<float *> *>(B->tampon);
		auto tampon_A = dynamic_cast<wlk::grille_dense_2d<float *> *>(A->tampon);
		//auto tampon_Z = dynamic_cast<wlk::grille_dense_2d<float *> *>(Z->tampon);

		auto largeur = tampon_S->desc().resolution.x;
		auto hauteur = tampon_S->desc().resolution.y;

		auto rectangle = contexte.resolution_rendu;
		auto tampon = m_image.ajoute_calque("image", contexte.resolution_rendu);

		auto debut_x = std::max(0l, static_cast<long>(rectangle.x));
		auto fin_x = static_cast<long>(std::min(largeur, largeur));
		auto debut_y = std::max(0l, static_cast<long>(rectangle.y));
		auto fin_y = static_cast<long>(std::min(hauteur, hauteur));

		for (auto j = debut_x; j < fin_x; ++j) {
			for (auto i = debut_y; i < fin_y; ++i) {

				auto index = j + i * largeur;
				auto n = tampon_S->valeur(index);

				if (n == 0) {
					continue;
				}

				/* À FAIRE : tidy image */

				/* compose les échantillons */
				auto sR = tampon_R->valeur(index);
				auto sG = tampon_G->valeur(index);
				auto sB = tampon_B->valeur(index);
				auto sA = tampon_A->valeur(index);

				auto pixel = dls::image::PixelFloat();
				pixel.r = sR[0];
				pixel.g = sG[0];
				pixel.b = sB[0];
				pixel.a = sA[0];

				for (auto s = 1u; s < n; ++s) {
					pixel.r = pixel.r + sR[s] * (1.0f - sA[s]);
					pixel.g = pixel.g + sG[s] * (1.0f - sA[s]);
					pixel.b = pixel.b + sB[s] * (1.0f - sA[s]);
					pixel.a = pixel.a + sA[s] * (1.0f - sA[s]);
				}

				tampon->valeur(j, i, pixel);
			}
		}

		return EXECUTION_REUSSIE;
	}
};


/* ************************************************************************** */

void enregistre_operatrices_image_profonde(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OpAplanisProfonde>());
}

#pragma clang diagnostic pop
