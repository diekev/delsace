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

#include "operatrices_srirp.hh"

#include <cmath>

#include "biblinternes/outils/indexeuse.hh"
#include "biblinternes/structures/tableau.hh"

#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_image.h"
#include "coeur/usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/**
 * Implémentation de l'algorithme RAISR, via une référence.
 */
static auto masque_gaussien_2d(int m, int n, float sigma = 0.5f)
{
	auto h = dls::tableau<float>(m * n);
	auto mm = m / 2;
	auto nn = n / 2;

	auto max = 0.0f;

	auto index = 0;
	for (auto i = -mm; i <= mm; ++i) {
		for (auto j = -nn; j <= nn; ++j, ++index) {
			auto i_f = static_cast<float>(i);
			auto j_f = static_cast<float>(j);

			h[index] = std::exp(-(i_f * i_f + j_f * j_f) / (2.0f * sigma * sigma));

			max = std::max(h[index], max);
		}
	}

	auto somme = 0.0f;

	for (auto i = 0; i < h.taille(); ++i) {
		if (h[i] < std::numeric_limits<float>::epsilon() * max) {
			h[i] = 0.0f;
		}

		somme += h[i];
	}

	if (somme != 0.0f) {
		for (auto i = 0; i < h.taille(); ++i) {
			h[i] /= somme;
		}
	}

	return h;
}

static auto diagonale(dls::tableau<float> const &entree)
{
	auto sortie = dls::tableau<float>(entree.taille() * entree.taille());

	for (auto i = 0; i < entree.taille(); ++i) {
		sortie[i + i * entree.taille()] = entree[i];
	}

	return sortie;
}

static auto calcul_gradient(
		dls::tableau<float> const &gradientblock,
		int taille_gradient_block,
		dls::tableau<float> &gx,
		dls::tableau<float> &gy)
{
	gx = dls::tableau<float>(gradientblock.taille());
	gy = dls::tableau<float>(gradientblock.taille());

	for (auto j = 0; j < taille_gradient_block; ++j) {
		for (auto i = 0; i < taille_gradient_block; ++i) {
			auto index = i + j * taille_gradient_block;

			if (i == 0) {
				gx[index] = gradientblock[index + 1] - gradientblock[index];
			}
			else if (i == taille_gradient_block - 1) {
				gx[index] = gradientblock[index] - gradientblock[index - 1];
			}
			else {
				gx[index] = gradientblock[index + 1] - gradientblock[index - 1];
			}


			if (j == 0) {
				gy[index] = gradientblock[index + taille_gradient_block] - gradientblock[index];
			}
			else if (j == taille_gradient_block - 1) {
				gy[index] = gradientblock[index] - gradientblock[index - taille_gradient_block];
			}
			else {
				gy[index] = gradientblock[index + taille_gradient_block] - gradientblock[index - taille_gradient_block];
			}
		}
	}
}

static auto calcule_cle_empreinte(
		dls::tableau<float> const &gradientblock,
		int taille_gradient_block,
		float Qangle,
		dls::tableau<float> const &poids,
		float &angle,
		float &strength,
		float &coherence)
{
	auto gx = dls::tableau<float>();
	auto gy = dls::tableau<float>();
	calcul_gradient(gradientblock, taille_gradient_block, gx, gy);

	// à faire SVD calculation
//	G = np.vstack((gx,gy)).T
//    GTWG = G.T.dot(W).dot(G)
//    w, v = np.linalg.eig(GTWG);

	 // Make sure V and D contain only real numbers
//    nonzerow = np.count_nonzero(np.isreal(w))
//    nonzerov = np.count_nonzero(np.isreal(v))
//    if nonzerow != 0:
//        w = np.real(w)
//    if nonzerov != 0:
//        v = np.real(v)

	// Sort w and v according to the descending order of w
//    idx = w.argsort()[::-1]
//    w = w[idx]
//    v = v[:,idx]

	// Calculate theta
//    auto theta = std::atan2(v[1,0], v[0,0])
//    if (theta < 0) {
//        theta = theta + pi;
//	}

//    // Calculate lamda
//    lamda = w[0]

//    // Calculate u
//    sqrtlamda1 = np.sqrt(w[0])
//    sqrtlamda2 = np.sqrt(w[1])
//    if sqrtlamda1 + sqrtlamda2 == 0:
//        u = 0
//    else:
//        u = (sqrtlamda1 - sqrtlamda2)/(sqrtlamda1 + sqrtlamda2)

//    // Quantize
//    angle = floor(theta / pi*Qangle)
//    if (lamda < 0.0001) {
//        strength = 0;
//	}
//	else if (lamda > 0.001) {
//        strength = 2;
//	}
//	else {
//        strength = 1;
//	}

//	if (u < 0.25) {
//        coherence = 0;
//	}
//	else if (u > 0.5) {
//        coherence = 2;
//	}
//    else {
//        coherence = 1;
//	}

//    // Bound the output to the desired ranges
//    if (angle > 23) {
//        angle = 23;
//	}
//	else if (angle < 0) {
//        angle = 0;
//	}
}

/* ************************************************************************** */

class OpTestSRIRP final : public OperatriceImage {
public:
	static constexpr auto NOM = "SRIRP";
	static constexpr auto AIDE = "";

	OpTestSRIRP(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceImage(graphe_parent, noeud_)
	{
		entrees(1);
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
		auto image_entree = entree(0)->requiers_image(contexte, donnees_aval);

		if (image_entree == nullptr) {
			this->ajoute_avertissement("Aucune image connecté");
			return res_exec::ECHOUEE;
		}

		auto calque = cherche_calque(*this, image_entree, "image");

		if (calque == nullptr) {
			return res_exec::ECHOUEE;
		}

		// Define parameters
		auto const R = 2;
		auto const patchsize = 11;
		auto const gradientsize = 9;
		auto const Qangle = 24;
		auto const Qstrength = 3;
		auto const Qcoherence = 3;
		//auto const trainpath = "train";

		// calcul la marge
		auto maxblockize = std::max(patchsize, gradientsize);
		auto margin = static_cast<int>(std::floor(static_cast<float>(maxblockize) * 0.5f));
		auto patchmargin = static_cast<int>(std::floor(static_cast<float>(patchsize) * 0.5f));
		auto gradientmargin = static_cast<int>(std::floor(static_cast<float>(gradientsize) * 0.5f));

		auto idxQ = otl::cree_indexeuse_statique(Qangle, Qstrength, Qcoherence, R * R, patchsize * patchsize, patchsize * patchsize);
		auto idxV = otl::cree_indexeuse_statique(Qangle, Qstrength, Qcoherence, R * R, patchsize * patchsize);

		auto Q = dls::tableau<float>(idxQ.nombre_elements);
		auto V = dls::tableau<float>(idxV.nombre_elements);
		auto h = dls::tableau<float>(idxV.nombre_elements);

		auto poids = masque_gaussien_2d(gradientsize, gradientsize, 2.0f);

//		for (auto i = 0; i < gradientsize; ++i) {
//			for (auto j = 0; j < gradientsize; ++j) {
//				std::cerr << poids[i + j * gradientsize] << ',';
//			}

//			std::cerr << '\n';
//		}

		poids = diagonale(poids);

//		for (auto i = 0; i < gradientsize * gradientsize; ++i) {
//			for (auto j = 0; j < gradientsize * gradientsize; ++j) {
//				std::cerr << poids[i + j * gradientsize * gradientsize] << ',';
//			}

//			std::cerr << '\n';
//		}

		// calcul Q et V

		// converti en noir et blanc
		auto tampon_couleur = extrait_grille_couleur(calque);
		auto tampon_nb = wlk::grille_dense_2d<float>(tampon_couleur->desc());

		for (auto i = 0; i < tampon_couleur->nombre_elements(); ++i) {
			tampon_nb.valeur(i) = dls::phys::luminance(tampon_couleur->valeur(i));
		}

		// À FAIRE : diminue la résolution, et agrandi la

		auto desc = tampon_nb.desc();

		for (auto y = margin; y < desc.resolution.y - margin; ++y) {
			for (auto x = margin; x < desc.resolution.x - margin; ++x) {
				// get patch
				auto taille_patch = patchmargin * patchmargin;
				auto patch = dls::tableau<float>(taille_patch);

				auto index = 0;
				for (auto yp = y - patchmargin; yp < y + patchmargin; ++yp) {
					for (auto xp = x - patchmargin; xp < x + patchmargin; ++xp) {
						patch[index] = tampon_nb.valeur(dls::math::vec2i(xp, yp)); // tampon upsaled
					}
				}

				// get gradient block
				auto taille_gradientblock = gradientmargin * gradientmargin;
				auto gradientblock = dls::tableau<float>(taille_gradientblock);

				index = 0;
				for (auto yp = y - patchmargin; yp < y + patchmargin; ++yp) {
					for (auto xp = x - patchmargin; xp < x + patchmargin; ++xp) {
						gradientblock[index] = tampon_nb.valeur(dls::math::vec2i(xp, yp)); // tampon upsaled
					}
				}

				float angle;
				float strength;
				float coherence;
				calcule_cle_empreinte(gradientblock, gradientmargin, Qangle, poids, angle, strength, coherence);

//				auto pixeltype = ((y - margin) % R) * R + ((x - margin) % R);

//				auto pixelHR = tampon_nb.valeur(dls::math::vec2i(x, y));

				// compute A'A et A'b
//				auto ATA = transpose(patch) * patch;
//				auto ATb = transpose(patch) * pixelHR;
//				ATb = np.array(ATb).ravel();

//				// compute Q ant V
//				Q[idxQ(angle, strength, coherence, pixeltype)] += ATA;
//				V[idxV(angle, strength, coherence, pixeltype)] += ATb;
			}
		}

		return res_exec::REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_srirp(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OpTestSRIRP>());
}

#pragma clang diagnostic pop
