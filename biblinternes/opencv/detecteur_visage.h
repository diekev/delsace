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

#pragma once

#include "definitions.h"

#include <experimental/filesystem>

OUVRE_GARDE_INCLUSION_OPENCV
#	include <opencv/cv.hpp>
#	include <opencv2/objdetect.hpp>
FERME_GARDE_INCLUSION_OPENCV

class detecteur_visage {
	cv::CascadeClassifier m_classificateur = {};

	double m_facteur_echelle = 0.0;
	int m_voisins_minimum = 0;
	double m_ratio_taille_min = 0.0;
	double m_ratio_taille_max = 0.0;

public:
	detecteur_visage(
			const std::experimental::filesystem::path &chemin_cascade,
			double facteur_echelle,
			int voisins_minimum,
			double ratio_taille_min,
			double ratio_taille_max);

	~detecteur_visage() = default;

	void trouve_visages(const cv::Mat &image, dls::tableau<cv::Rect> &resultats);
};
