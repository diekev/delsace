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

wlk::base_grille_2d *&calque_image::tampon()
{
	return m_tampon;
}

const wlk::base_grille_2d *calque_image::tampon() const
{
	return m_tampon;
}

calque_image::calque_image(const calque_image &autre)
{
	if (autre.m_tampon == nullptr) {
		return;
	}

	this->m_tampon = autre.m_tampon->copie();
}

calque_image::calque_image(calque_image &&autre)
{
	std::swap(m_tampon, autre.m_tampon);
}

calque_image &calque_image::operator=(const calque_image &autre)
{
	deloge_grille(m_tampon);

	if (autre.m_tampon == nullptr) {
		return *this;
	}

	m_tampon = autre.m_tampon->copie();
	return *this;
}

calque_image &calque_image::operator=(calque_image &&autre)
{
	std::swap(m_tampon, autre.m_tampon);
	return *this;
}

calque_image::~calque_image()
{
	deloge_grille(m_tampon);
}

calque_image calque_image::construit_calque(
		wlk::base_grille_2d::type_desc const &desc,
		wlk::type_grille type_donnees)
{
	auto calque = calque_image();

	switch (type_donnees) {
		case wlk::type_grille::N32:
		{
			calque.tampon() = memoire::loge<wlk::grille_dense_2d<unsigned int>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::Z8:
		{
			calque.tampon() = memoire::loge<wlk::grille_dense_2d<char>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::Z32:
		{
			calque.tampon() = memoire::loge<wlk::grille_dense_2d<int>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::R32:
		{
			calque.tampon() = memoire::loge<wlk::grille_dense_2d<float>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::R32_PTR:
		{
			calque.tampon() = memoire::loge<wlk::grille_dense_2d<float *>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::R64:
		{
			calque.tampon() = memoire::loge<wlk::grille_dense_2d<double>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::VEC2:
		{
			calque.tampon() = memoire::loge<wlk::grille_dense_2d<dls::math::vec2f>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::VEC3:
		{
			calque.tampon() = memoire::loge<wlk::grille_dense_2d<dls::math::vec3f>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::VEC3_R64:
		{
			calque.tampon() = memoire::loge<wlk::grille_dense_2d<dls::math::vec3d>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::COULEUR:
		{
			calque.tampon() = memoire::loge<wlk::grille_dense_2d<dls::phys::couleur32>>("grille_dense_2d", desc);
			break;
		}
		case wlk::type_grille::COURBE_PAIRE_TEMPS:
		{
			calque.tampon() = memoire::loge<wlk::grille_dense_2d<wlk::type_courbe>>("grille_dense_2d", desc);
			break;
		}
	}

	return calque;
}

wlk::desc_grille_2d desc_depuis_rectangle(const Rectangle &rectangle)
{
	auto moitie_x = static_cast<float>(rectangle.largeur) * 0.5f;
	auto moitie_y = static_cast<float>(rectangle.hauteur) * 0.5f;

	auto desc = wlk::desc_grille_2d();
	desc.etendue.min.x = -moitie_x;
	desc.etendue.min.y = -moitie_y;
	desc.etendue.max.x =  moitie_x;
	desc.etendue.max.y =  moitie_y;
	desc.fenetre_donnees = desc.etendue;
	desc.taille_pixel = 1.0;

	return desc;
}

/* ************************************************************************** */

static auto supprime_calque_image(calque_image *calque)
{
	memoire::deloge("calque_image", calque);
}

/* ************************************************************************** */

Image::~Image()
{
	reinitialise();
}

calque_image *Image::ajoute_calque(dls::chaine const &nom, wlk::desc_grille_2d const &desc, wlk::type_grille type)
{
	auto calque = memoire::loge<calque_image>("calque_image");
	calque->nom = nom;

	*calque = calque_image::construit_calque(desc, type);

	m_calques.pousse(ptr_calque_profond(calque, supprime_calque_image));

	return calque;
}

calque_image *Image::ajoute_calque_profond(const dls::chaine &nom, wlk::desc_grille_2d const &desc, wlk::type_grille type)
{
	auto calque = memoire::loge<calque_image>("calque_image");
	calque->nom = nom;

	*calque = calque_image::construit_calque(desc, type);

	m_calques_profond.pousse(ptr_calque_profond(calque, supprime_calque_image));

	return calque;
}

calque_image const *Image::calque_pour_lecture(dls::chaine const &nom) const
{
	for (auto tampon : m_calques) {
		if (tampon->nom == nom) {
			return tampon.get();
		}
	}

	return nullptr;
}

calque_image *Image::calque_pour_ecriture(dls::chaine const &nom)
{
	for (auto &clq : m_calques) {
		if (clq->nom != nom) {
			continue;
		}

		if (!clq.unique()) {
			auto ncalque = memoire::loge<calque_image>("calque_image");
			ncalque->nom = clq->nom;
			ncalque->tampon() = clq->tampon()->copie();

			clq = ptr_calque_profond(ncalque, supprime_calque_image);
		}

		return clq.get();
	}

	return nullptr;
}

calque_image const *Image::calque_profond_pour_lecture(dls::chaine const &nom) const
{
	for (auto clq : m_calques_profond) {
		if (clq->nom == nom) {
			return clq.get();
		}
	}

	return nullptr;
}

calque_image *Image::calque_profond_pour_ecriture(const dls::chaine &nom)
{
	for (auto &clq : m_calques_profond) {
		if (clq->nom != nom) {
			continue;
		}

		if (!clq.unique()) {
			auto ncalque = memoire::loge<calque_image>("calque_image");
			ncalque->nom = clq->nom;
			ncalque->tampon() = clq->tampon()->copie();
			ncalque->echantillons = clq->echantillons;

			/* Ajourne pointeurs. */
			auto tamp = ncalque->tampon();

			if (tamp->desc().type_donnees == wlk::type_grille::R32_PTR) {
				auto S = calque_profond_pour_lecture("S");
				auto tampon_S = dynamic_cast<wlk::grille_dense_2d<unsigned> const *>(S->tampon());

				auto decalage = 0l;
				auto hauteur = tamp->desc().resolution.y;
				auto largeur = tamp->desc().resolution.x;

				auto grille = dynamic_cast<wlk::grille_dense_2d<float *> *>(tamp);

				for (auto i = 0; i < hauteur; ++i) {
					for (auto j = 0; j < largeur; ++j) {
						auto index = grille->calcul_index(dls::math::vec2i(j, i));

						auto n = tampon_S->valeur(index);

						if (n == 0) {
							continue;
						}

						grille->valeur(index) = &ncalque->echantillons[decalage];

						decalage += n;
					}
				}
			}

			clq = ptr_calque_profond(ncalque, supprime_calque_image);
		}

		return clq.get();
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

void Image::reinitialise()
{
	est_profonde = false;
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
