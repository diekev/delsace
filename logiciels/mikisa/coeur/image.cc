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

calque_image::calque_image(const calque_image &autre)
{
	if (autre.tampon == nullptr) {
		return;
	}

	this->tampon = autre.tampon->copie();
}

calque_image::calque_image(calque_image &&autre)
{
	std::swap(tampon, autre.tampon);
}

calque_image &calque_image::operator=(const calque_image &autre)
{
	deloge_grille(tampon);

	if (autre.tampon == nullptr) {
		return *this;
	}

	tampon = autre.tampon->copie();
	return *this;
}

calque_image &calque_image::operator=(calque_image &&autre)
{
	std::swap(tampon, autre.tampon);
	return *this;
}

calque_image::~calque_image()
{
	deloge_grille(tampon);
}

calque_image calque_image::construit_calque(
		wlk::base_grille_2d::type_desc const &desc,
		wlk::type_grille type_donnees)
{
	auto calque = calque_image();

	switch (type_donnees) {
		case wlk::type_grille::N32:
		{
			calque.tampon = memoire::loge<wlk::grille_dense_2d<unsigned int>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::Z8:
		{
			calque.tampon = memoire::loge<wlk::grille_dense_2d<char>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::Z32:
		{
			calque.tampon = memoire::loge<wlk::grille_dense_2d<int>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::R32:
		{
			calque.tampon = memoire::loge<wlk::grille_dense_2d<float>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::R32_PTR:
		{
			calque.tampon = memoire::loge<wlk::grille_dense_2d<float *>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::R64:
		{
			calque.tampon = memoire::loge<wlk::grille_dense_2d<double>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::VEC2:
		{
			calque.tampon = memoire::loge<wlk::grille_dense_2d<dls::math::vec2f>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::VEC3:
		{
			calque.tampon = memoire::loge<wlk::grille_dense_2d<dls::math::vec3f>>("grille_dense_2d", desc);
			break;
		}
	}

	return calque;
}

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

calque_image *Image::ajoute_calque_profond(const dls::chaine &nom, int largeur, int hauteur, wlk::type_grille type)
{
	auto calque = memoire::loge<calque_image>("calque_image");
	calque->nom = nom;

	auto desc = wlk::desc_grille_2d{};
	desc.etendue.min = dls::math::vec2f(0.0f);
	desc.etendue.max = dls::math::vec2f(1.0f, static_cast<float>(hauteur) / static_cast<float>(largeur));
	desc.fenetre_donnees = desc.etendue;
	desc.taille_pixel = 1.0 / static_cast<double>(std::max(largeur, hauteur));

	*calque = calque_image::construit_calque(desc, type);

	m_calques_profond.pousse(calque);

	return calque;
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

calque_image *Image::calque_profond(dls::chaine const &nom) const
{
	for (auto clq : m_calques_profond) {
		if (clq->nom == nom) {
			return clq;
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

static void deloge_donnees_profondes(Image &image)
{
	auto S = image.calque_profond("S");
	auto R = image.calque_profond("R");
	auto G = image.calque_profond("G");
	auto B = image.calque_profond("B");
	auto A = image.calque_profond("A");
	auto Z = image.calque_profond("Z");

	auto tampon_S = dynamic_cast<wlk::grille_dense_2d<unsigned> *>(S->tampon);
	auto tampon_R = dynamic_cast<wlk::grille_dense_2d<float *> *>(R->tampon);
	auto tampon_G = dynamic_cast<wlk::grille_dense_2d<float *> *>(G->tampon);
	auto tampon_B = dynamic_cast<wlk::grille_dense_2d<float *> *>(B->tampon);
	auto tampon_A = dynamic_cast<wlk::grille_dense_2d<float *> *>(A->tampon);
	auto tampon_Z = dynamic_cast<wlk::grille_dense_2d<float *> *>(Z->tampon);

	auto largeur = tampon_S->desc().resolution.x;
	auto hauteur = tampon_S->desc().resolution.y;

	for (auto i = 0; i < hauteur; ++i) {
		for (auto j = 0; j < largeur; ++j) {
			auto index = j + i * largeur;
			auto const n = tampon_S->valeur(index);
			memoire::deloge_tableau("deep_r", tampon_R->valeur(index), n);
			memoire::deloge_tableau("deep_g", tampon_G->valeur(index), n);
			memoire::deloge_tableau("deep_b", tampon_B->valeur(index), n);
			memoire::deloge_tableau("deep_a", tampon_A->valeur(index), n);
			memoire::deloge_tableau("deep_z", tampon_Z->valeur(index), n);
		}
	}
}

void Image::reinitialise(bool garde_memoires)
{
	if (!garde_memoires) {
		for (Calque *tampon : m_calques) {
			memoire::deloge("Calque", tampon);
		}

		if (est_profonde && !m_calques_profond.est_vide()) {
			deloge_donnees_profondes(*this);
		}

		for (auto calque : m_calques_profond) {
			memoire::deloge("calque_image", calque);
		}
	}

	m_calques.efface();
	m_calques_profond.efface();
}

void Image::nom_calque_actif(dls::chaine const &nom)
{
	m_nom_calque = nom;
}

dls::chaine const &Image::nom_calque_actif() const
{
	return m_nom_calque;
}
