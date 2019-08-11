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

#include "lecture.h"

#include "biblinternes/structures/tableau.hh"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string.h>

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

#ifdef AVEC_TIFF
#	include <tiffio.h>
#endif

#include "../outils/flux.h"
#include "../../outils/definitions.h"

#include "../operations/conversion.h"

namespace dls {
namespace image {
namespace flux {

/* ************************************************************************** */

math::matrice_dyn<PixelChar> lecture_uchar(const filesystem::path &chemin)
{
	const auto &extension = chemin.extension();

	if (outils::est_extension_jpeg(extension.c_str())) {
		return LecteurJPEG::ouvre(chemin);
	}

	if (outils::est_extension_pnm(extension.c_str())) {
		return LecteurPNM::ouvre(chemin);
	}

	if (outils::est_extension_exr(extension.c_str())) {
		auto float_image = LecteurEXR::ouvre(chemin);
		return operation::converti_en_char(float_image);
	}

	assert("Cannot open file for reading, file type not supported!\n" != nullptr);
	return {};
}

math::matrice_dyn<PixelFloat> lecture_float(const filesystem::path &chemin)
{
	const auto &extension = chemin.extension();

	if (outils::est_extension_jpeg(extension.c_str())) {
		auto byte_image = LecteurJPEG::ouvre(chemin);
		return operation::converti_en_float(byte_image);
	}

	if (outils::est_extension_pnm(extension.c_str())) {
		auto byte_image = LecteurPNM::ouvre(chemin);
		return operation::converti_en_float(byte_image);
	}

	if (outils::est_extension_exr(extension.c_str())) {
		return LecteurEXR::ouvre(chemin);
	}

	assert("Cannot open file for reading, file type not supported!\n" != nullptr);
	return {};
}

/* ************************************************************************** */

math::matrice_dyn<PixelChar> LecteurJPEG::ouvre(const filesystem::path &chemin)
{
#ifdef AVEC_JPEG
	FILE *file = std::fopen(chemin.c_str(), "rb");

	if (file == nullptr) {
		std::cerr << "Cannot open file: " << chemin << "\n";
		return { math::Hauteur(0), math::Largeur(0) };
	}

	fseek(file, 0L, SEEK_SET);

	struct jpeg_error_mgr jpeg_error;

	struct jpeg_decompress_struct info;
	info.err = jpeg_std_error(&jpeg_error);

	jpeg_create_decompress(&info);
	jpeg_stdio_src(&info, file);

	jpeg_save_markers(&info, JPEG_APP0 + 1, 0xffff);

	jpeg_read_header(&info, TRUE);
	jpeg_start_decompress(&info);

	const auto width = static_cast<int>(info.output_width);
	const auto height = static_cast<int>(info.output_height);
	const auto channels = info.output_components;

	auto image = math::matrice_dyn<PixelChar>(
				math::Hauteur(height),
				math::Largeur(width));

	const auto &stride = width * channels;

	auto buffer = dls::tableau<unsigned char>(stride);
	auto ptr = buffer.donnees();

	for (auto xi = 0; xi < height; ++xi) {
		jpeg_read_scanlines(&info, &ptr, 1);

		for (auto yb = 0, yi = 0; yi < width; yb += 3, ++yi) {
			image[xi][yi].r = buffer[yb];
			image[xi][yi].g = buffer[yb + 1];
			image[xi][yi].b = buffer[yb + 2];
		}
	}

	/* cleanup */
	jpeg_finish_decompress(&info);
	jpeg_destroy_decompress(&info);
	std::fclose(file);

	return image;
#else
	INUTILISE(chemin);
	return {};
#endif
}

/* ************************************************************************** */

math::matrice_dyn<PixelChar> LecteurPNM::ouvre(const filesystem::path &chemin)
{
	FILE *fichier = std::fopen(chemin.c_str(), "r");

	if (!fichier) {
		return { math::Hauteur(0), math::Largeur(0) };
	}

	std::fseek(fichier, 0L, SEEK_SET);

	char id[2];
	int nombre_colonnes, nombre_lignes;
	auto lu = std::fscanf(fichier, "%2c\n%d %d\n%*3d\n", id, &nombre_colonnes, &nombre_lignes);

	if (lu == 0) {
		std::fclose(fichier);
		return { math::Hauteur(0), math::Largeur(0) };
	}

	const int channels = ((strcmp(id, "P5") == 0) ? 1 : 3);

	if (channels == 1) {
		std::fclose(fichier);
		return { math::Hauteur(0), math::Largeur(0) };
	}

	auto img = math::matrice_dyn<PixelChar>(
				math::Hauteur(nombre_lignes),
				math::Largeur(nombre_colonnes));

	for (int x = 0; x < img.nombre_lignes(); ++x) {
		for (int y = 0; y < img.nombre_colonnes(); ++y) {
			lu = std::fscanf(fichier, "%c%c%c", &img[x][y].r, &img[x][y].g, &img[x][y].b);

			if (lu == 0) {
				break;
			}
		}
	}

	std::fclose(fichier);

	return img;
}

/* ************************************************************************** */

math::matrice_dyn<PixelChar> LecteurTIF::ouvre(const filesystem::path &chemin)
{
#ifdef AVEC_TIFF
	TIFF *file = TIFFOpen(chemin.c_str(), "r");

	if (file == nullptr) {
		std::cerr << "Ne peut ouvrir le fichier : " << chemin << "\n";
		return { 0, 0 };
	}

	int imagelength, imageWidth;
	TIFFGetField(file, TIFFTAG_IMAGEWIDTH, &imageWidth);
	TIFFGetField(file, TIFFTAG_IMAGELENGTH, &imagelength);

	tdata_t buf = _TIFFmalloc(TIFFScanlineSize(file));

	for (int row = 0; row < imagelength; ++row) {
		TIFFReadScanline(file, buf, row);
	}

	_TIFFfree(buf);
	TIFFClose(file);
#else
	INUTILISE(chemin);
	return { math::Hauteur(0), math::Largeur(0) };
#endif
}

/* ************************************************************************** */

math::matrice_dyn<PixelFloat> LecteurEXR::ouvre(const filesystem::path &chemin)
{
#ifdef AVEC_OPENEXR
	namespace openexr = OPENEXR_IMF_NAMESPACE;

	openexr::RgbaInputFile file(chemin.c_str());

	Imath::Box2i dw = file.dataWindow();

	auto width = dw.max.x - dw.min.x + 1;
	auto height = dw.max.y - dw.min.y + 1;

	dls::tableau<openexr::Rgba> pixels(width * height);

	file.setFrameBuffer(&pixels[0]- dw.min.x - dw.min.y * width, 1, static_cast<size_t>(width));
	file.readPixels(dw.min.y, dw.max.y);

	auto img = math::matrice_dyn<PixelFloat>(
				math::Hauteur(height),
				math::Largeur(width));

	auto idx(0);
	for (auto y(0); y < height; ++y) {
		for (auto x(0), xe(width * 4); x < xe; x += 4, ++idx) {
			img[y][x].r = pixels[idx].r;
			img[y][x].g = pixels[idx].g;
			img[y][x].b = pixels[idx].b;
			img[y][x].a = pixels[idx].a;
		}
	}

	std::cout << "End of EXRReader::read" << "\n";

	return img;
#else
	INUTILISE(chemin);
	return { math::Hauteur(0), math::Largeur(0) };
#endif
}

}  /* namespace flux */
}  /* namespace image */
}  /* namespace dls */
