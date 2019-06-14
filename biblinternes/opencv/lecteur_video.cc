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

#include "lecteur_video.h"

lecteur_video::lecteur_video(
		const std::experimental::filesystem::path &chemin_video,
		int image_debut,
		int image_fin,
		int delta)
	: m_image_fin(image_fin)
	, m_delta(delta)
{
	if (!m_video.open(chemin_video.string())) {
		throw "Impossible d'ouvrir la vidéo !\n";
	}

	if (image_debut != -1) {
		m_video.set(CV_CAP_PROP_POS_FRAMES, image_debut);
	}
}

lecteur_video::~lecteur_video()
{
	m_video.release();
}

bool lecteur_video::image_suivante(cv::Mat &image)
{
	if (m_delta != -1) {
		m_video.set(CV_CAP_PROP_POS_FRAMES,
					m_video.get(CV_CAP_PROP_POS_FRAMES) + m_delta);
	}

	if (m_image_fin != -1 && m_video.get(CV_CAP_PROP_POS_FRAMES) > m_image_fin) {
		return false;
	}

	return m_video.read(image);
}

cv::Size lecteur_video::taille() const
{
	return {
		static_cast<int>(m_video.get(CV_CAP_PROP_FRAME_WIDTH)),
		static_cast<int>(m_video.get(CV_CAP_PROP_FRAME_HEIGHT))
	};
}

double lecteur_video::cadence() const
{
	return m_video.get(CV_CAP_PROP_FPS);
}

int lecteur_video::nombre_image() const
{
	return m_video.get(CV_CAP_PROP_FRAME_COUNT);
}

void lecteur_video::delta(int delta)
{
	m_delta = delta;
}
