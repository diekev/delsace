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

#include <experimental/filesystem>

#include "../../math/matrice/matrice.hh"

#include "../pixel.h"

namespace filesystem = std::experimental::filesystem;

namespace dls {
namespace image {
namespace flux {

/**
 * Lis le fichier dont le chemin est spécifié et retourne une image au format
 * unsigne char. Le chemin doit contenir un nom de fichier et une extension. Si
 * le chemin n'existe pas ou si l'extension du fichier pointé par le chemin est
 * inconnu, la fonction plante sur une assertion fausse.
 */
math::matrice<PixelChar> lecture_uchar(const filesystem::path &chemin);

/**
 * Lis le fichier dont le chemin est spécifié et retourne une image au format
 * float. Le chemin doit contenir un nom de fichier et une extension. Si le
 * chemin n'existe pas ou si l'extension du fichier pointé par le chemin est
 * inconnu, la fonction plante sur une assertion fausse.
 */
math::matrice<PixelFloat> lecture_float(const filesystem::path &chemin);

/**
 * Struct pour créer un type pour lire une image au format JPEG.
 */
struct LecteurJPEG {
	/**
	 * Ouvre le fichier dont le chemin est spécifié et retourne une image au
	 * format unsigned char.
	 */
	static math::matrice<PixelChar> ouvre(const filesystem::path &chemin);
};

/**
 * Struct pour créer un type pour lire une image au format PNM.
 */
struct LecteurPNM {
	/**
	 * Ouvre le fichier dont le chemin est spécifié et retourne une image au
	 * format unsigned char.
	 */
	static math::matrice<PixelChar> ouvre(const filesystem::path &chemin);
};

/**
 * Struct pour créer un type pour lire une image au format TIF.
 */
struct LecteurTIF {
	/**
	 * Ouvre le fichier dont le chemin est spécifié et retourne une image au
	 * format unsigned char.
	 */
	static math::matrice<PixelChar> ouvre(const filesystem::path &chemin);
};

/**
 * Struct pour créer un type pour lire une image au format EXR.
 */
struct LecteurEXR {
	/**
	 * Ouvre le fichier dont le chemin est spécifié et retourne une image au
	 * format unsigned char.
	 */
	static math::matrice<PixelFloat> ouvre(const filesystem::path &chemin);
};

}  /* namespace flux */
}  /* namespace image */
}  /* namespace dls */
