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

#include "operatrice_pixel.h"

#include "biblinternes/moultfilage/boucle.hh"

#include "chef_execution.hh"
#include "contexte_evaluation.hh"

OperatricePixel::OperatricePixel(Graphe &graphe_parent, Noeud &noeud_)
	: OperatriceImage(graphe_parent, noeud_)
{}

res_exec OperatricePixel::execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	calque_image *calque = nullptr;

	auto const &rectangle = contexte.resolution_rendu;

	if (entrees() == 0) {
		m_image.reinitialise();
		auto desc = desc_depuis_rectangle(rectangle);
		calque = m_image.ajoute_calque("image", desc, wlk::type_grille::COULEUR);
	}
	else if (entrees() >= 1) {
		entree(0)->requiers_copie_image(m_image, contexte, donnees_aval);
		auto nom_calque = evalue_chaine("nom_calque");
		calque = m_image.calque_pour_ecriture(nom_calque);
	}

	if (calque == nullptr) {
		ajoute_avertissement("Calque introuvable !");
		return res_exec::ECHOUEE;
	}

	auto chef = contexte.chef;
	chef->demarre_evaluation(this->nom_classe());

	auto desc = calque->tampon()->desc();

	auto tampon = extrait_grille_couleur(calque);
	auto largeur_inverse = 1.0f / static_cast<float>(desc.resolution.x);
	auto hauteur_inverse = 1.0f / static_cast<float>(desc.resolution.y);

	this->evalue_entrees(contexte.temps_courant);

	boucle_parallele(tbb::blocked_range<int>(0, desc.resolution.y),
					 [&](tbb::blocked_range<int> const &plage)
	{
		for (auto l = plage.begin(); l < plage.end(); ++l) {
			for (auto c = 0; c < desc.resolution.x; ++c) {
				auto const x = static_cast<float>(c) * largeur_inverse;
				auto const y = static_cast<float>(l) * hauteur_inverse;

				auto index = tampon->calcul_index(dls::math::vec2i(c, l));

				tampon->valeur(index, this->evalue_pixel(tampon->valeur(index), x, y));
			}
		}

		auto delta = static_cast<float>(plage.end() - plage.begin());
		delta /= rectangle.hauteur;

		chef->indique_progression_parallele(delta * 100.0f);
	});

	chef->indique_progression(100.0f);

	return res_exec::REUSSIE;
}
