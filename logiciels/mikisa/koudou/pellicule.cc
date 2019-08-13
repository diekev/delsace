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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "pellicule.hh"

namespace kdo {

Pellicule::Pellicule()
{}

int Pellicule::hauteur() const
{
	return m_matrice.desc().resolution.y;
}

int Pellicule::largeur() const
{
	return m_matrice.desc().resolution.x;
}

void Pellicule::ajoute_echantillon(long i, long j, dls::math::vec3d const &couleur, const double poids)
{
	auto ii = std::min(i, static_cast<long>(largeur() - 1));
	auto jj = std::min(j, static_cast<long>(hauteur() - 1));

	auto index = ii + static_cast<long>(largeur()) * jj;
	auto &pixel_pellicule = m_pixels_pellicule[index];
	pixel_pellicule.couleur += couleur * poids;
	pixel_pellicule.poids += poids;
}

dls::math::vec3d const &Pellicule::couleur(int i, int j)
{
	return m_matrice.valeur(dls::math::vec2i(i, j));
}

Pellicule::type_grille const &Pellicule::donnees() const
{
	return m_matrice;
}

void Pellicule::reinitialise()
{
	m_pixels_pellicule.redimensionne(largeur() * hauteur());

	for (auto &pixel_pellicule : m_pixels_pellicule) {
		pixel_pellicule.couleur = dls::math::vec3d(0.0);
		pixel_pellicule.poids = 0.0;
	}
}

void Pellicule::creer_image()
{
	if (m_pixels_pellicule.est_vide()) {
		return;
	}

	auto index = 0l;

	for (int i = 0; i < hauteur(); ++i) {
		for (int j = 0; j < largeur(); ++j, ++index) {
			auto const &pixel_pellicule = m_pixels_pellicule[index];

			if (pixel_pellicule.poids == 0.0) {
				m_matrice.valeur(index) = dls::math::vec3d(0.0);
				return;
			}

			m_matrice.valeur(index) = pixel_pellicule.couleur / pixel_pellicule.poids;
		}
	}
}

void Pellicule::redimensionne(dls::math::Hauteur const &hauteur, dls::math::Largeur const &largeur)
{
	auto moitie_x = static_cast<float>(largeur.valeur) * 0.5f;
	auto moitie_y = static_cast<float>(hauteur.valeur) * 0.5f;

	auto desc = wlk::desc_grille_2d();
	desc.etendue.min.x = -moitie_x;
	desc.etendue.min.y = -moitie_y;
	desc.etendue.max.x =  moitie_x;
	desc.etendue.max.y =  moitie_y;
	desc.fenetre_donnees = desc.etendue;
	desc.taille_pixel = 1.0;

	m_matrice = type_grille(desc);

	reinitialise();
}

}  /* namespace kdo */
