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

#include "reconnaissant_facial.h"

reconnaissant_facial::reconnaissant_facial()
{
	m_taille_visage = cv::Size(256, 256);

	/* Construit le modèle de reconnaissance. */
	m_model = cv::face::createEigenFaceRecognizer(/* composants = */80);
}

void reconnaissant_facial::entraine(
		const std::vector<cv::Mat> &images,
		const std::vector<int> &etiquettes)
{
	m_model->train(images, etiquettes);
}

bool reconnaissant_facial::reconnaissance(const cv::Mat &visage, double &confiance) const
{
	cv::Mat histogramme;
	cv::cvtColor(visage, histogramme, cv::COLOR_RGB2GRAY);
	cv::resize(histogramme, histogramme, m_taille_visage);

	int etiquette;
	m_model->predict(histogramme, etiquette, confiance);

	return (etiquette == 0);
}

std::vector<std::pair<int, double>> reconnaissant_facial::reconnaissance(const cv::Mat &visage)
{
	auto collecteur = cv::face::StandardCollector::create();

	cv::Mat histogramme;
	cv::cvtColor(visage, histogramme, cv::COLOR_RGB2GRAY);
	cv::resize(histogramme, histogramme, m_taille_visage);

	cv::imwrite("modèles/visage.png", visage);
	cv::imwrite("modèles/visage_gris.png", histogramme);

	m_model->predict(histogramme, collecteur);

	return collecteur->getResults(true);
}

void reconnaissant_facial::sauvegarde(const std::string &chemin)
{
	m_model->save(chemin);
}

static cv::Mat norm_0_255(cv::InputArray _src)
{
	cv::Mat src = _src.getMat();
	// Create and return normalized image:
	cv::Mat dst;
	switch(src.channels()) {
		case 1:
			cv::normalize(_src, dst, 0, 255, cv::NORM_MINMAX, CV_8UC1);
			break;
		case 3:
			cv::normalize(_src, dst, 0, 255, cv::NORM_MINMAX, CV_8UC3);
			break;
		default:
			src.copyTo(dst);
			break;
	}

	return dst;
}

void reconnaissant_facial::sauvegarde_eigenvectors()
{
	auto modele = static_cast<cv::face::BasicFaceRecognizer *>(m_model.get());
	auto W = modele->getEigenVectors();

	for (int i = 0; i < std::min(80, W.cols); i++) {
		auto ev = W.col(i).clone();

		/* Redimensionne à la taille originale & normalisation [0...255] */
		auto grayscale = norm_0_255(ev.reshape(1, 256));

		/* Application d'un dégradé de couleur. */
		cv::Mat cgrayscale;
		cv::applyColorMap(grayscale, cgrayscale, cv::COLORMAP_JET);

		cv::imwrite(cv::format("modèles/eigenface_%d.png", i),
					norm_0_255(cgrayscale));
	}
}

void reconnaissant_facial::charge(const std::string &chemin)
{
	m_model->load(chemin);
}
