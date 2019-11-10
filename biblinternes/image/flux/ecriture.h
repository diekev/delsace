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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <filesystem>

#include "../../math/matrice/matrice.hh"

#include "../pixel.h"

namespace filesystem = std::filesystem;

namespace dls {
namespace image {
namespace flux {

/**
 * Écris une image de type uchar au chemin précisé. Le chemin doit contenir un
 * nom de fichier et une extension. Si le chemin est inconnu, où l'extension du
 * fichier pointé par le chemin est inconnu, la fonction plante sur une
 * assertion fausse.
 */
void ecris(const filesystem::path &chemin, const math::matrice_dyn<PixelChar> &image);

/**
 * Écris une image de type float au chemin précisé. Le chemin doit contenir un
 * nom de fichier et une extension. Si le chemin est inconnu, où l'extension du
 * fichier pointé par le chemin est inconnu, la fonction plante sur une
 * assertion fausse.
 */
void ecris(const filesystem::path &chemin, const math::matrice_dyn<PixelFloat> &image);

/**
 * Struct pour créer un type pour écrire une image au format JPEG.
 */
struct AuteurJPEG {
	/**
	 * Écris une image au format unsigned char au chemin spécifié au format JPEG.
	 * La fonction espère que le chemin soit valide et le nom de fichier soit un
	 * nom de fichier JPEG valide. Si le chemin n'est pas valide, la fonction ne
	 * fait rien.
	 */
	static void ecris(const filesystem::path &chemin, const math::matrice_dyn<PixelChar> &image);
};

/**
 * Struct pour créer un type pour écrire une image au format PNM.
 */
struct AuteurPNM {
	/**
	 * Écris une image au format unsigned char au chemin spécifié au format PNM.
	 * La fonction espère que le chemin soit valide et le nom de fichier soit un
	 * nom de fichier PNM valide. Si le chemin n'est pas valide, la fonction ne
	 * fait rien.
	 */
	static void ecris(const filesystem::path &chemin, const math::matrice_dyn<PixelChar> &image);
};

/**
 * Struct pour créer un type pour écrire une image au format EXR.
 */
struct AuteurEXR {
	/**
	 * Écris une image au format unsigned char au chemin spécifié au format EXR.
	 * La fonction espère que le chemin soit valide et le nom de fichier soit un
	 * nom de fichier EXR valide. Si le chemin n'est pas valide, la fonction ne
	 * fait rien.
	 */
	static void ecris(const filesystem::path &chemin, const math::matrice_dyn<PixelFloat> &image);
};

}  /* namespace flux */
}  /* namespace image */
}  /* namespace dls */
