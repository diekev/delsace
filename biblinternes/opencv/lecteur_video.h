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
FERME_GARDE_INCLUSION_OPENCV

class lecteur_video {
	cv::VideoCapture m_video = {};
	int m_image_fin = 0;
	int m_delta = 0;

public:
	lecteur_video(
			const std::experimental::filesystem::path &chemin_video,
			int image_debut = -1,
			int image_fin = -1,
			int delta = -1);

	~lecteur_video();

	/**
	 * @brief image_suivante Charge l'image suivante dans une matrice.
	 * @param image Matrice où l'image sera chargée.
	 * @return Vrai si une image a été chargé, faux sinon ou si la vidéo est
	 *         finie.
	 */
	bool image_suivante(cv::Mat &image);

	/**
	 * @brief taille La hauteur et la largeur de la vidéo.
	 * @return Les informations sur la largeur et la hauteur de la vidéo.
	 */
	cv::Size taille() const;

	/**
	 * @brief cadence La cadence de la vidéo en images par seconde.
	 * @return La cadence de la vidéo en images par seconde.
	 */
	double cadence() const;

	/**
	 * Retourne le nombre d'images de la vidéo.
	 */
	int nombre_image() const;

	/**
	 * Change la valeur de delta de la lecture vidéo par celle passée en
	 * paramètre. Cette valeur définie le nombre d'images sautées à chaque appel
	 * de la méthode 'image_suivante'.
	 */
	void delta(int delta);
};
