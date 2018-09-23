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

#include "pellicule.h"

Pellicule::Pellicule()
	: m_matrice(numero7::math::Hauteur(720), numero7::math::Largeur(1280))
{}

int Pellicule::hauteur() const
{
	return m_matrice.dimensions().hauteur;
}

int Pellicule::largeur() const
{
	return m_matrice.dimensions().largeur;
}

void Pellicule::ajoute_echantillon(int i, int j, const numero7::math::vec3d &couleur, const double poids)
{
	auto index = i + m_matrice.nombre_colonnes() * j;
	auto &pixel_pellicule = m_pixels_pellicule[index];
	pixel_pellicule.couleur += couleur;
	pixel_pellicule.poids += poids;
}

const numero7::math::vec3d &Pellicule::couleur(int i, int j)
{
	return m_matrice[i][j];
}

const numero7::math::matrice<numero7::math::vec3d> &Pellicule::donnees()
{
	return m_matrice;
}

void Pellicule::reinitialise()
{
	m_matrice.remplie(numero7::math::vec3d(0.0));
	m_pixels_pellicule.resize(m_matrice.nombre_colonnes() * m_matrice.nombre_lignes());

	for (auto &pixel_pellicule : m_pixels_pellicule) {
		pixel_pellicule.couleur = numero7::math::vec3d(0.0);
		pixel_pellicule.poids = 0.0;
	}
}

void Pellicule::creer_image()
{
	if (m_pixels_pellicule.empty()) {
		return;
	}

	const auto &hauteur = m_matrice.nombre_lignes();
	const auto &largeur = m_matrice.nombre_colonnes();
	auto index = 0;

	for (int i = 0; i < hauteur; ++i) {
		for (int j = 0; j < largeur; ++j, ++index) {
			const auto &pixel_pellicule = m_pixels_pellicule[index];

			if (pixel_pellicule.poids == 0.0) {
				m_matrice[i][j] = numero7::math::vec3d(0.0);
				return;
			}

			m_matrice[i][j] = pixel_pellicule.couleur / pixel_pellicule.poids;
		}
	}
}

void Pellicule::redimensionne(const numero7::math::Hauteur &hauteur, const numero7::math::Largeur &largeur)
{
	m_matrice = numero7::math::matrice<numero7::math::vec3d>(hauteur, largeur);
}
