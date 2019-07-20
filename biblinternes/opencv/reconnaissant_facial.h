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

OUVRE_GARDE_INCLUSION_OPENCV
#	include <opencv/cv.hpp>
#	include <opencv2/face.hpp>
FERME_GARDE_INCLUSION_OPENCV

class reconnaissant_facial {
	cv::Ptr<cv::face::FaceRecognizer> m_model = {};
	cv::Size m_taille_visage = {};

public:
	reconnaissant_facial();

	~reconnaissant_facial() = default;

	/**
	 * Entraine le modèle avec les images et les étiquettes passées en
	 * paramètres.
	 */
	void entraine(
			const dls::tableau<cv::Mat> &images,
			const dls::tableau<int> &etiquettes);

	/**
	 * Essaye de reconnaître le visage passé en paramètre. Retourne la confiance
	 * et vrai si le visage reconnu a pour étiquette '0';
	 */
	bool reconnaissance(const cv::Mat &visage, double &confiance) const;

	/**
	 * Sauvegarde les données du modèle de reconnaissance dans le fichier pointé
	 * par le chemin passé en paramètre.
	 */
	void sauvegarde(const dls::chaine &chemin);

	/**
	 * Charge les données du modèle de reconnaissance depuis le fichier pointé
	 * par le chemin passé en paramètre.
	 */
	void charge(const dls::chaine &chemin);

	/**
	 * Essaye de reconnaître le visage passé en paramètre. Retourne un vecteur
	 * contenant des paires étiquette-confiance faisant correspondre à chaque
	 * visage étiquetté la probabilité qu'il soit le visage à reconnaître. Le
	 * nombre de paires est égale au nombre total d'images utilisées lors de
	 * l'entraînement.
	 */
	dls::tableau<std::pair<int, double>> reconnaissance(const cv::Mat &visage);

	/**
	 * Sauvegarde les eignevectors dans des fichiers images séparés.
	 */
	void sauvegarde_eigenvectors();
};
