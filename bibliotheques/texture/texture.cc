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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "texture.h"

#include <jpeglib.h>
#include <png.h>

#include "../outils/constantes.h"

void convertie_float(unsigned char uchar[3], float d[3])
{
	d[0] = uchar[0] / 255.0f;
	d[1] = uchar[1] / 255.0f;
	d[2] = uchar[2] / 255.0f;
}

numero7::math::matrice<Spectre> image_nulle()
{
	auto image = numero7::math::matrice<Spectre>(
					 numero7::math::Hauteur(1),
					 numero7::math::Largeur(1));

	float rvb[3] = { 1.0f, 0.0f, 1.0f };
	image[0][0] = Spectre::depuis_rgb(rvb);

	return image;
}

numero7::math::matrice<Spectre> ouvre_png(const std::experimental::filesystem::path &chemin)
{
	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

	if (!png) {
		return image_nulle();
	}

	png_infop info = png_create_info_struct(png);

	if (!info) {
		return image_nulle();
	}

	if (setjmp(png_jmpbuf(png))) {
		return image_nulle();
	}

	FILE *file = std::fopen(chemin.c_str(), "rb");

	if (file == nullptr) {
		return image_nulle();
	}

	png_init_io(png, file);

	png_read_info(png, info);

	auto const width      = png_get_image_width(png, info);
	auto const height     = png_get_image_height(png, info);
	auto const color_type = png_get_color_type(png, info);
	auto const bit_depth  = png_get_bit_depth(png, info);

	if (bit_depth == 16) {
		png_set_strip_16(png);
	}

	if (color_type == PNG_COLOR_TYPE_PALETTE){
		png_set_palette_to_rgb(png);
	}

	// PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8){
		png_set_expand_gray_1_2_4_to_8(png);
	}

	if (png_get_valid(png, info, PNG_INFO_tRNS)){
		png_set_tRNS_to_alpha(png);
	}

	// These color_type don't have an alpha channel then fill it with 0xff.
	if (color_type == PNG_COLOR_TYPE_RGB ||
			color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
	}

	if (color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		png_set_gray_to_rgb(png);
	}

	png_read_update_info(png, info);

	png_bytep *row_pointers = static_cast<png_bytep *>(malloc(sizeof(png_bytep) * height));

	for (size_t y = 0; y < height; y++) {
		row_pointers[y] = static_cast<png_byte *>(malloc(png_get_rowbytes(png,info)));
	}

	png_read_image(png, row_pointers);


	auto image = numero7::math::matrice<Spectre>(
					 numero7::math::Hauteur(static_cast<int>(height)),
					 numero7::math::Largeur(static_cast<int>(width)));


	for (size_t y = 0; y < height; y++) {
		auto row = row_pointers[y];

		for (size_t x = 0; x < width; x++) {
			auto px = &(row[x * 4]);

			float rvb[3] = { px[0] / 255.0f, px[1] / 255.0f, px[2] / 255.0f };

			image[static_cast<int>(y)][x] = Spectre::depuis_rgb(rvb);
		}
	}

	for(size_t y = 0; y < height; y++) {
		free(row_pointers[y]);
	}
	free(row_pointers);

	png_destroy_read_struct(&png, &info, nullptr);

	std::fclose(file);

	return image;
}

numero7::math::matrice<Spectre> ouvre_jpeg(const std::experimental::filesystem::path &chemin)
{
	FILE *file = std::fopen(chemin.c_str(), "rb");

	if (file == nullptr) {
		auto image = numero7::math::matrice<Spectre>(
						 numero7::math::Hauteur(1),
						 numero7::math::Largeur(1));

		float rvb[3] = { 1.0f, 0.0f, 1.0f };
		image[0][0] = Spectre::depuis_rgb(rvb);

		return image;
	}

	std::fseek(file, 0L, SEEK_SET);

	struct jpeg_error_mgr jpeg_error;

	struct jpeg_decompress_struct info;
	info.err = jpeg_std_error(&jpeg_error);

	jpeg_create_decompress(&info);
	jpeg_stdio_src(&info, file);

	jpeg_save_markers(&info, JPEG_APP0 + 1, 0xffff);

	jpeg_read_header(&info, TRUE);
	jpeg_start_decompress(&info);

	auto const largeur = info.output_width;
	auto const hauteur = info.output_height;
	auto const channels = info.output_components;

	auto image = numero7::math::matrice<Spectre>(
					 numero7::math::Hauteur(static_cast<int>(hauteur)),
					 numero7::math::Largeur(static_cast<int>(largeur)));

	auto const &stride = largeur * static_cast<unsigned>(channels);

	unsigned char *buffer = new unsigned char[stride];

	float rgb[3];

	for (size_t l = 0; l < hauteur; ++l) {
		jpeg_read_scanlines(&info, &buffer, 1);

		for (size_t ct = 0, c = 0; c < largeur; ct += 3, ++c) {
			convertie_float(buffer + ct, rgb);
			image[static_cast<int>(l)][c] = Spectre::depuis_rgb(rgb);
		}
	}

	delete [] buffer;

	/* cleanup */
	jpeg_finish_decompress(&info);
	jpeg_destroy_decompress(&info);
	std::fclose(file);

	return image;
}

double restreint(double valeur, double min, double max)
{
	if (valeur < min) {
		return min;
	}

	if (valeur > max) {
		return max;
	}

	return valeur;
}

double theta_spherique(const dls::math::vec3d &vecteur)
{
	return std::acos(restreint(vecteur.y, -1.0, 1.0));
}

double phi_spherique(const dls::math::vec3d &vecteur)
{
	auto p = std::atan2(vecteur.z, vecteur.x);
	return (p < 0.0) ? p + TAU : p;
}

/* ************************************************************************** */

void supprime_texture(Texture *&texture)
{
	/* ASAN rapporte un décalage entre la taille d'allocation et celle de
	 * déallocation : la classe de base Texture est plus petite que ses dérivées
	 * car elles ont des propriétés en plus.
	 *
	 * Pas sûr de la bonne manière pour résoudre ce problème. */
	if (dynamic_cast<TextureImage *>(texture)) {
		delete dynamic_cast<TextureImage *>(texture);
	}
	else if (dynamic_cast<TextureCouleur *>(texture)) {
		delete dynamic_cast<TextureCouleur *>(texture);
	}

	texture = nullptr;
}

/* ************************************************************************** */

void TextureCouleur::etablie_spectre(const Spectre &spectre)
{
	m_spectre = spectre;
}

Spectre TextureCouleur::spectre() const
{
	return m_spectre;
}

Spectre TextureCouleur::echantillone(const dls::math::vec3d &direction) const
{
	return m_spectre;
}

TypeTexture TextureCouleur::type() const
{
	return TypeTexture::COULEUR;
}

/* ************************************************************************** */

Spectre TextureImage::echantillone(const dls::math::vec3d &direction) const
{
	/* Mappage sphérique. */
	auto vec = dls::math::normalise(direction - dls::math::vec3d(0.0, 0.0, 0.0));

	auto theta = theta_spherique(vec);
	auto phi = phi_spherique(vec);

	auto t = theta * PI_INV;
	auto s = phi * TAU_INV;

	auto x = static_cast<int>(s * m_image.nombre_colonnes());
	auto y = static_cast<int>(t * m_image.nombre_lignes());

	x %= m_image.nombre_colonnes();
	y %= m_image.nombre_lignes();

	return m_image[y][x];
}

Spectre TextureImage::echantillone_uv(int x, int y)
{
	return m_image[y][x];
}

void TextureImage::charge_donnees(const numero7::math::matrice<numero7::image::PixelFloat> &donnees)
{
	m_image = numero7::math::matrice<Spectre>(donnees.dimensions());

	for (int l = 0; l < m_image.nombre_lignes(); ++l) {
		for (int c = 0; c < m_image.nombre_colonnes(); ++c) {
			m_image[l][c][0] = donnees[l][c].r;
			m_image[l][c][1] = donnees[l][c].g;
			m_image[l][c][2] = donnees[l][c].b;
		}
	}
}

void TextureImage::etablie_image(const numero7::math::matrice<Spectre> &image)
{
	m_image = image;
}

SpectreRGB *TextureImage::donnees()
{
	return m_image.donnees();
}

int TextureImage::largeur()
{
	return m_image.nombre_colonnes();
}

int TextureImage::hauteur()
{
	return m_image.nombre_lignes();
}

void TextureImage::projection(int p)
{
	m_projection = p;
}

int TextureImage::projection() const
{
	return m_projection;
}

void TextureImage::entrepolation(int i)
{
	m_entrepolation = i;
}

int TextureImage::entrepolation() const
{
	return m_entrepolation;
}

void TextureImage::enveloppage(int e)
{
	m_enveloppage = e;
}

int TextureImage::enveloppage() const
{
	return m_enveloppage;
}

void TextureImage::camera(vision::Camera3D *p)
{
	m_camera = p;
}

dls::math::vec3f TextureImage::taille() const
{
	return m_taille;
}

void TextureImage::taille(const dls::math::vec3f &taille)
{
	m_taille = taille;
}

vision::Camera3D *TextureImage::camera() const
{
	return m_camera;
}

void TextureImage::etablie_chemin(const std::experimental::filesystem::path &chemin)
{
	m_chemin = chemin;
}

const std::experimental::filesystem::path &TextureImage::chemin() const
{
	return m_chemin;
}

TypeTexture TextureImage::type() const
{
	return TypeTexture::IMAGE;
}

TextureImage *charge_texture(const std::experimental::filesystem::path &chemin)
{
	auto texture = new TextureImage();

	numero7::math::matrice<Spectre> image;

	if (chemin.extension() == ".png") {
		image = ouvre_png(chemin);
	}
	else {
		image = ouvre_jpeg(chemin);
	}

	texture->etablie_image(image);
	texture->etablie_chemin(chemin);

	return texture;
}
