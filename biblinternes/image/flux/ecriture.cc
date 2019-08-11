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

#include "ecriture.h"

#include "biblinternes/structures/tableau.hh"

#include <cassert>
#include <iostream>

#ifdef AVEC_JPEG
#	include <jpeglib.h>
#endif

#ifdef AVEC_OPENEXR
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wregister"
#	pragma GCC diagnostic ignored "-Wold-style-cast"
#	pragma GCC diagnostic ignored "-Wsign-conversion"
#	pragma GCC diagnostic ignored "-Weffc++"
#	pragma GCC diagnostic ignored "-Wconversion"
#	pragma GCC diagnostic ignored "-Wshadow"
#	include <OpenEXR/ImfRgbaFile.h>
#	include <OpenEXR/ImathBox.h>
#	pragma GCC diagnostic pop
#endif

#include "../outils/flux.h"
#include "../../outils/definitions.h"

#include "../operations/conversion.h"

namespace dls {
namespace image {
namespace flux {

void ecris(const filesystem::path &chemin, const math::matrice_dyn<PixelChar> &image)
{
	const auto &extension = chemin.extension();

	if (outils::est_extension_jpeg(extension.c_str())) {
		AuteurJPEG::ecris(chemin, image);
	}
	else if (outils::est_extension_pnm(extension.c_str())) {
		AuteurPNM::ecris(chemin, image);
	}
	else if (outils::est_extension_exr(extension.c_str())) {
		auto float_image = operation::converti_en_float(image);
		AuteurEXR::ecris(chemin, float_image);
	}
	else {
		assert(!"Ne peut ouvrir le fichier pour éciture, type de fichier inconnu !\n");
	}
}

void ecris(const filesystem::path &chemin, const math::matrice_dyn<PixelFloat> &image)
{
	const auto &extension = chemin.extension();

	if (outils::est_extension_jpeg(extension.c_str())) {
		auto byte_image = operation::converti_en_char(image);
		AuteurJPEG::ecris(chemin, byte_image);
	}
	else if (outils::est_extension_pnm(extension.c_str())) {
		auto byte_image = operation::converti_en_char(image);
		AuteurPNM::ecris(chemin, byte_image);
	}
	else if (outils::est_extension_exr(extension.c_str())) {
		AuteurEXR::ecris(chemin, image);
	}
	else {
		assert(!"Ne peut ouvrir le fichier pour éciture, type de fichier inconnu !\n");
	}
}

void AuteurJPEG::ecris(const filesystem::path &chemin, const math::matrice_dyn<PixelChar> &image)
{
#ifdef AVEC_JPEG
	FILE *file = std::fopen(chemin.c_str(), "wb");
	fseek(file, 0L, SEEK_SET);

	struct jpeg_compress_struct info;
	struct jpeg_error_mgr jerr;

	jpeg_create_compress(&info);
	info.err = jpeg_std_error(&jerr);

	jpeg_stdio_dest(&info, file);
	info.image_width = static_cast<unsigned>(image.nombre_colonnes());
	info.image_height = static_cast<unsigned>(image.nombre_lignes());
	info.input_components = 3;

	switch (info.input_components) {
		case 1:
			info.in_color_space = JCS_GRAYSCALE;
			break;
		case 3:
			info.in_color_space = JCS_RGB;
			break;
	}

	jpeg_set_defaults(&info);

	info.comp_info[0].h_samp_factor = 2;
	info.comp_info[0].v_samp_factor = 2;

	jpeg_set_quality(&info, 95, FALSE);
    jpeg_start_compress(&info, TRUE);

	const auto &stride = info.image_width * static_cast<unsigned>(info.input_components);
	unsigned char *buffer = new unsigned char[stride];

	for (auto x = 0; x < image.nombre_lignes(); ++x) {
		auto idx = 0;

		for (auto y = 0; y < image.nombre_colonnes(); ++y) {
			buffer[idx++] = image[x][y].r;
			buffer[idx++] = image[x][y].g;
			buffer[idx++] = image[x][y].b;
		}

		jpeg_write_scanlines(&info, &buffer, 1);
	}

	jpeg_finish_compress(&info);
	jpeg_destroy_compress(&info);
	std::fclose(file);
	delete [] buffer;
#else
	INUTILISE(chemin);
	INUTILISE(image);
#endif
}

void AuteurPNM::ecris(const filesystem::path &chemin, const math::matrice_dyn<PixelChar> &image)
{
	const auto &height = image.nombre_lignes();
	const auto &width = image.nombre_colonnes();
	const auto &channels = 3;

	FILE *fichier = std::fopen(chemin.c_str(), "w");
	std::fseek(fichier, 0L, SEEK_SET);

	switch (channels) {
		case 1:
			std::fprintf(fichier, "P5\n");
			break;
		case 3:
			std::fprintf(fichier, "P6\n");
			break;
	}

	std::fprintf(fichier, "%d %d\n255\n", int(width), int(height));

	for (int x = 0; x < image.nombre_lignes(); ++x) {
		for (int y = 0; y < image.nombre_colonnes(); ++y) {
			std::fprintf(fichier, "%c%c%c", image[x][y].r, image[x][y].g, image[x][y].b);
		}
	}

	std::fclose(fichier);
}


// À FAIRE: catch exceptions (std::exception)?
void AuteurEXR::ecris(const filesystem::path &chemin, const math::matrice_dyn<PixelFloat> &image)
{
#ifdef AVEC_OPENEXR
	namespace openexr = OPENEXR_IMF_NAMESPACE;

	const auto &height = image.nombre_lignes();
	const auto &width = image.nombre_colonnes();

	openexr::Header header(width, height);
	openexr::RgbaOutputFile file(chemin.c_str(), header, openexr::WRITE_RGBA);

	dls::tableau<openexr::Rgba> pixels(image.dimensions().nombre_elements());

	auto idx(0);
	for (auto y = 0; y < height; ++y) {
		for (auto x = 0; x < width; ++x, ++idx) {
			pixels[idx].r = image[y][x].r;
			pixels[idx].g = image[y][x].g;
			pixels[idx].b = image[y][x].b;
			pixels[idx].a = image[y][x].a;
		}
	}

	file.setFrameBuffer(&pixels[0], 1, static_cast<size_t>(width));
	file.writePixels(height);
#else
	INUTILISE(chemin);
	INUTILISE(image);
#endif
}

}  /* namespace flux */
}  /* namespace image */
}  /* namespace dls */
