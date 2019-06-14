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

#include "detecteur_visage.h"

detecteur_visage::detecteur_visage(
		const std::experimental::filesystem::path &chemin_cascade,
		double facteur_echelle,
		int voisins_minimum,
		double ratio_taille_min,
		double ratio_taille_max)
	: m_classificateur(cv::CascadeClassifier(chemin_cascade.string()))
	, m_facteur_echelle(facteur_echelle)
	, m_voisins_minimum(voisins_minimum)
	, m_ratio_taille_min(ratio_taille_min)
	, m_ratio_taille_max(ratio_taille_max)
{
}

void detecteur_visage::trouve_visages(
		const cv::Mat &image,
		std::vector<cv::Rect> &resultats)
{
	const auto largeur = image.size().width;
	const auto hauteur = image.size().height;

	const auto taille_echelle_min = cv::Size(m_ratio_taille_min * largeur,
											 m_ratio_taille_min * hauteur);

	const auto taille_echelle_max = cv::Size(m_ratio_taille_max * largeur,
											 m_ratio_taille_max * hauteur);

	/* Converti l'image en niveau de gris et normalise l'histogramme. */
	cv::Mat temp;
	cv::cvtColor(image, temp, CV_BGR2GRAY);
	cv::equalizeHist(temp, temp);

	/* Vide le vecteur. */
	resultats.clear();

	/* Détecte les visages. */
	m_classificateur.detectMultiScale(
				temp,
				resultats,
				m_facteur_echelle,
				m_voisins_minimum,
				0,
				taille_echelle_min,
				taille_echelle_max);
}
