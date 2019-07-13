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

#include "calques.h"

#include <algorithm>

#include "melange.h"

Calque::~Calque()
{
	switch (type_donnees) {
		case TypeDonnees::SCALAIRE:
			delete [] static_cast<float *>(tampon);
			break;
		case TypeDonnees::COULEUR:
			delete [] static_cast<dls::math::vec4f *>(tampon);
			break;
		case TypeDonnees::VECTEUR:
			delete [] static_cast<dls::math::vec3f *>(tampon);
			break;
	}
}

CanauxTexture::~CanauxTexture()
{
	for (auto &canaux : calques) {
		for (auto &calque : canaux) {
			delete calque;
		}
	}

	delete [] tampon_diffusion;
}

Calque *ajoute_calque(CanauxTexture &canaux, TypeCanal type_canal)
{
	auto calque = new Calque;
	calque->nom = "calque";

	auto res = canaux.hauteur * canaux.largeur;

	switch (type_canal) {
		case TypeCanal::DEPLACEMENT_VECTORIEL:
		case TypeCanal::NORMAL:
			calque->type_donnees = VECTEUR;
			calque->tampon = new dls::math::vec3f[res];
			std::fill_n(static_cast<dls::math::vec3f *>(calque->tampon), res, dls::math::vec3f(0.0f));
			break;
		case TypeCanal::DIFFUSION:
			calque->type_donnees = COULEUR;
			calque->tampon = new dls::math::vec4f[res];
			std::fill_n(static_cast<dls::math::vec4f *>(calque->tampon), res, dls::math::vec4f(0.0f));
			break;
		case TypeCanal::SPECULARITE:
		case TypeCanal::GLOSS:
		case TypeCanal::INCANDESCENCE:
		case TypeCanal::OPACITE:
		case TypeCanal::MASQUE_REFLECTION:
		case TypeCanal::RELIEF:
			calque->type_donnees = SCALAIRE;
			calque->tampon = new float[res];
			std::fill_n(static_cast<float *>(calque->tampon), res, 0.0f);
			break;
		default:
			break;
	}

	canaux.calques[type_canal].pousse(calque);

	return calque;
}

void supprime_calque(CanauxTexture &canaux, Calque *calque)
{
	auto &calques = canaux.calques[TypeCanal::DIFFUSION];
	auto iter = std::find(calques.debut(), calques.fin(), calque);

	if (iter == calques.fin()) {
		/* À FAIRE : erreur. */
	}

	delete *iter;
	calques.erase(iter);
}

void fusionne_calques(CanauxTexture &canaux)
{
	auto const res = canaux.hauteur * canaux.largeur;

	if (canaux.tampon_diffusion == nullptr) {
		canaux.tampon_diffusion = new dls::math::vec4f[res];
	}

	std::fill_n(canaux.tampon_diffusion, res, dls::math::vec4f(0.0f));

	for (auto const &calque : canaux.calques[TypeCanal::DIFFUSION]) {
		auto tampon = static_cast<dls::math::vec4f *>(calque->tampon);

		for (size_t i = 0; i < res; ++i) {
			canaux.tampon_diffusion[i] = melange(
											 canaux.tampon_diffusion[i],
											 tampon[i],
											 calque->opacite,
											 calque->mode_fusion);
		}
	}
}
