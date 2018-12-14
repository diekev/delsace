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

#include "bibliotheques/outils/parallelisme.h"

OperatricePixel::OperatricePixel(Noeud *node)
	: OperatriceImage(node)
{}

int OperatricePixel::type() const
{
	return OPERATRICE_PIXEL;
}

int OperatricePixel::execute(const Rectangle &rectangle, int temps)
{
	Calque *tampon = nullptr;

	if (inputs() == 0) {
		m_image.reinitialise();
		tampon = m_image.ajoute_calque("image", rectangle);
	}
	else if (inputs() >= 1) {
		input(0)->requiers_image(m_image, rectangle, temps);
		auto nom_calque = evalue_chaine("nom_calque");
		tampon = m_image.calque(nom_calque);
	}

	if (tampon == nullptr) {
		ajoute_avertissement("Calque introuvable !");
		return EXECUTION_ECHOUEE;
	}

	auto largeur_inverse = 1.0f / rectangle.largeur;
	auto hauteur_inverse = 1.0f / rectangle.hauteur;

	this->evalue_entrees(temps);

	boucle_parallele(tbb::blocked_range<size_t>(0, static_cast<size_t>(rectangle.hauteur)),
					 [&](const tbb::blocked_range<size_t> &plage)
	{
		for (size_t l = plage.begin(); l < plage.end(); ++l) {
			for (size_t c = 0; c < static_cast<size_t>(rectangle.largeur); ++c) {
				auto const x = static_cast<float>(c) * largeur_inverse;
				auto const y = static_cast<float>(l) * hauteur_inverse;

				tampon->valeur(c, l, this->evalue_pixel(tampon->valeur(c, l), x, y));
			}
		}
	});

	return EXECUTION_REUSSIE;
}

void OperatricePixel::compile(CompileuseGraphe &compileuse, int temps)
{}
