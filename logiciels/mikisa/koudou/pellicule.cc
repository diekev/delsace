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
	: m_matrice(dls::math::Hauteur(720), dls::math::Largeur(1280))
{}

int Pellicule::hauteur() const
{
	return m_matrice.dimensions().hauteur;
}

int Pellicule::largeur() const
{
	return m_matrice.dimensions().largeur;
}

void Pellicule::ajoute_echantillon(long i, long j, dls::math::vec3d const &couleur, const double poids)
{
	auto index = i + static_cast<long>(m_matrice.nombre_colonnes()) * j;
	auto &pixel_pellicule = m_pixels_pellicule[index];
	pixel_pellicule.couleur += couleur;
	pixel_pellicule.poids += poids;
}

dls::math::vec3d const &Pellicule::couleur(int i, int j)
{
	return m_matrice[i][j];
}

dls::math::matrice_dyn<dls::math::vec3d> const &Pellicule::donnees()
{
	return m_matrice;
}

void Pellicule::reinitialise()
{
	m_matrice.remplie(dls::math::vec3d(0.0));
	m_pixels_pellicule.redimensionne(m_matrice.nombre_colonnes() * m_matrice.nombre_lignes());

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

	auto const &hauteur = m_matrice.nombre_lignes();
	auto const &largeur = m_matrice.nombre_colonnes();
	auto index = 0l;

	for (int i = 0; i < hauteur; ++i) {
		for (int j = 0; j < largeur; ++j, ++index) {
			auto const &pixel_pellicule = m_pixels_pellicule[index];

			if (pixel_pellicule.poids == 0.0) {
				m_matrice[i][j] = dls::math::vec3d(0.0);
				return;
			}

			m_matrice[i][j] = pixel_pellicule.couleur / pixel_pellicule.poids;
		}
	}
}

void Pellicule::redimensionne(dls::math::Hauteur const &hauteur, dls::math::Largeur const &largeur)
{
	m_matrice = dls::math::matrice_dyn<dls::math::vec3d>(hauteur, largeur);
}

}  /* namespace kdo */
