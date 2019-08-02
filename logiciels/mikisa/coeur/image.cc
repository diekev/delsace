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

#include "image.hh"

/* ************************************************************************** */

dls::image::Pixel<float> Calque::valeur(long x, long y) const
{
	x = std::max(0l, std::min(x, static_cast<long>(tampon.nombre_colonnes()) - 1));
	y = std::max(0l, std::min(y, static_cast<long>(tampon.nombre_lignes()) - 1));
	return tampon[static_cast<int>(y)][static_cast<int>(x)];
}

void Calque::valeur(long x, long y, dls::image::Pixel<float> const &pixel)
{
	x = std::max(0l, std::min(x, static_cast<long>(tampon.nombre_colonnes()) - 1));
	y = std::max(0l, std::min(y, static_cast<long>(tampon.nombre_lignes()) - 1));
	tampon[static_cast<int>(y)][static_cast<int>(x)] = pixel;
}

dls::image::Pixel<float> Calque::echantillone(float x, float y) const
{
	auto const res_x = tampon.nombre_colonnes();
	auto const res_y = tampon.nombre_lignes();

	auto const entier_x = static_cast<int>(x);
	auto const entier_y = static_cast<int>(y);

	auto const fract_x = x - static_cast<float>(entier_x);
	auto const fract_y = y - static_cast<float>(entier_y);

	auto const x1 = std::max(0, std::min(entier_x, res_x - 1));
	auto const y1 = std::max(0, std::min(entier_y, res_y - 1));
	auto const x2 = std::max(0, std::min(entier_x + 1, res_x - 1));
	auto const y2 = std::max(0, std::min(entier_y + 1, res_y - 1));

	auto valeur = dls::image::Pixel<float>(0.0f);
	valeur += fract_x * fract_y * tampon[y1][x1];
	valeur += (1.0f - fract_x) * fract_y * tampon[y1][x2];
	valeur += fract_x * (1.0f - fract_y) * tampon[y2][x1];
	valeur += (1.0f - fract_x) * (1.0f - fract_y) * tampon[y2][x2];

	return valeur;
}

/* ************************************************************************** */

Image::~Image()
{
	reinitialise();
}

Calque *Image::ajoute_calque(dls::chaine const &nom, Rectangle const &rectangle)
{
	auto tampon = memoire::loge<Calque>("Calque");
	tampon->nom = nom;
	tampon->tampon = type_image(dls::math::Hauteur(static_cast<int>(rectangle.hauteur)),
								dls::math::Largeur(static_cast<int>(rectangle.largeur)));

	auto pixel = dls::image::Pixel<float>(0.0f);
	pixel.a = 1.0f;

	tampon->tampon.remplie(pixel);

	m_calques.pousse(tampon);

	return tampon;
}

Calque *Image::calque(dls::chaine const &nom) const
{
	for (Calque *tampon : m_calques) {
		if (tampon->nom == nom) {
			return tampon;
		}
	}

	return nullptr;
}

Image::plage_calques Image::calques()
{
	return plage_calques(m_calques.debut(), m_calques.fin());
}

Image::plage_calques_const Image::calques() const
{
	return plage_calques_const(m_calques.debut(), m_calques.fin());
}

void Image::reinitialise(bool garde_memoires)
{
	if (!garde_memoires) {
		for (Calque *tampon : m_calques) {
			memoire::deloge("Calque", tampon);
		}
	}

	m_calques.efface();
}

void Image::nom_calque_actif(dls::chaine const &nom)
{
	m_nom_calque = nom;
}

dls::chaine const &Image::nom_calque_actif() const
{
	return m_nom_calque;
}

