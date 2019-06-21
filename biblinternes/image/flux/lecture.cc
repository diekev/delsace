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

#include <cassert>
#include <cstdio>
#include <iostream>
#include <string.h>

#ifdef AVEC_JPEG
#	include <jpeglib.h>
#endif

#ifdef AVEC_OPENEXR
#	include <OpenEXR/ImfRgbaFile.h>
#	include <OpenEXR/ImathBox.h>
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

math::matrice<PixelChar> lecture_uchar(const filesystem::path &chemin)
{
	const auto &extension = chemin.extension();

	if (outils::est_extension_jpeg(extension)) {
		return LecteurJPEG::ouvre(chemin);
	}
	else if (outils::est_extension_pnm(extension)) {
		return LecteurPNM::ouvre(chemin);
	}
	else if (outils::est_extension_exr(extension)) {
		auto float_image = LecteurEXR::ouvre(chemin);
		return operation::converti_en_char(float_image);
	}
	else {
		assert(!"Cannot open file for reading, file type not supported!\n");
	}
}

math::matrice<PixelFloat> lecture_float(const filesystem::path &chemin)
{
	const auto &extension = chemin.extension();

	if (outils::est_extension_jpeg(extension)) {
		auto byte_image = LecteurJPEG::ouvre(chemin);
		return operation::converti_en_float(byte_image);
	}
	else if (outils::est_extension_pnm(extension)) {
		auto byte_image = LecteurPNM::ouvre(chemin);
		return operation::converti_en_float(byte_image);
	}
	else if (outils::est_extension_exr(extension)) {
		return LecteurEXR::ouvre(chemin);
	}
	else {
		assert(!"Cannot open file for reading, file type not supported!\n");
	}
}

/* ************************************************************************** */

math::matrice<PixelChar> LecteurJPEG::ouvre(const filesystem::path &chemin)
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

	const auto width = info.output_width;
	const auto height = info.output_height;
	const auto channels = info.output_components;

	auto image = math::matrice<PixelChar>(math::Hauteur(height), math::Largeur(width));

	const auto &stride = width * channels;

	unsigned char *buffer = new unsigned char[stride];

	for (size_t xi = 0; xi < height; ++xi) {
		jpeg_read_scanlines(&info, &buffer, 1);

		for (size_t yb = 0, yi = 0; yi < width; yb += 3, ++yi) {
			image[xi][yi].r = buffer[yb];
			image[xi][yi].g = buffer[yb + 1];
			image[xi][yi].b = buffer[yb + 2];
		}
	}

	delete [] buffer;

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

math::matrice<PixelChar> LecteurPNM::ouvre(const filesystem::path &chemin)
{
	FILE *fichier = std::fopen(chemin.c_str(), "r");

	if (!fichier) {
		return { math::Hauteur(0), math::Largeur(0) };
	}

	std::fseek(fichier, 0L, SEEK_SET);

	char id[2];
	int nombre_colonnes, nombre_lignes;
	std::fscanf(fichier, "%2c\n%d %d\n%*3d\n", id, &nombre_colonnes, &nombre_lignes);

	const int channels = ((strcmp(id, "P5") == 0) ? 1 : 3);

	if (channels == 1) {
		std::fclose(fichier);
		return { math::Hauteur(0), math::Largeur(0) };
	}

	auto img = math::matrice<PixelChar>(
				math::Hauteur(nombre_lignes),
				math::Largeur(nombre_colonnes));

	for (int x = 0; x < img.nombre_lignes(); ++x) {
		for (int y = 0; y < img.nombre_colonnes(); ++y) {
			std::fscanf(fichier, "%c%c%c", &img[x][y].r, &img[x][y].g, &img[x][y].b);
		}
	}

	std::fclose(fichier);

	return img;
}

/* ************************************************************************** */

math::matrice<PixelChar> LecteurTIF::ouvre(const filesystem::path &chemin)
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

math::matrice<PixelFloat> LecteurEXR::ouvre(const filesystem::path &chemin)
{
#ifdef AVEC_OPENEXR
	namespace openexr = OPENEXR_IMF_NAMESPACE;

	openexr::RgbaInputFile file(chemin.c_str());

	Imath::Box2i dw = file.dataWindow();

	auto width = dw.max.x - dw.min.x + 1;
	auto height = dw.max.y - dw.min.y + 1;

	std::vector<openexr::Rgba> pixels(width * height);

	file.setFrameBuffer(&pixels[0]- dw.min.x - dw.min.y * width, 1, width);
	file.readPixels(dw.min.y, dw.max.y);

	FloatImage img(width, height, 4);

	size_t idx(0);
	for (size_t y(0); y < height; ++y) {
		for (size_t x(0), xe(width * 4); x < xe; x += 4, ++idx) {
			img(x + 0, y) = pixels[idx].r;
			img(x + 1, y) = pixels[idx].g;
			img(x + 2, y) = pixels[idx].b;
			img(x + 3, y) = pixels[idx].a;
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
