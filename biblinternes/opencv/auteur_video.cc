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

#include "auteur_video.h"

auteur_video::auteur_video(
		const std::experimental::filesystem::path &chemin_video,
		double cadence,
		cv::Size taille,
		int fourcc)
{
	m_video.open(chemin_video.string(), fourcc, cadence, taille, true);
}

auteur_video::~auteur_video()
{
	m_video.release();
}

void auteur_video::ecrit(cv::Mat &image)
{
	m_video << image;
}
