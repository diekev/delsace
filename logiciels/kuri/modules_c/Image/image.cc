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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "image.h"

#include <OpenImageIO/imageio.h>

extern "C" {

ResultatOperation IMG_ouvre_image(const char *chemin, Image *image)
{
	auto input = OIIO::ImageInput::open(chemin);

	if (input == nullptr) {
		return ResultatOperation::TYPE_IMAGE_NON_SUPPORTE;
	}

	const auto &spec = input->spec();
	int xres = spec.width;
	int yres = spec.height;
	int channels = spec.nchannels;

	image->donnees = new float[xres * yres * channels];
	image->taille_donnees = xres * yres * channels;
	image->largeur = xres;
	image->hauteur = yres;
	image->nombre_composants = channels;

	if (!input->read_image(image->donnees)) {
		input->close();
		OIIO::ImageInput::destroy(input);
		return ResultatOperation::TYPE_IMAGE_NON_SUPPORTE;
	}

	input->close();
	OIIO::ImageInput::destroy(input);

	return ResultatOperation::OK;
}

ResultatOperation IMG_ecris_image(const char *chemin, Image *image)
{
	auto out = OIIO::ImageOutput::create(chemin);

	if (out == nullptr) {
		return ResultatOperation::IMAGE_INEXISTANTE;
	}

	auto spec = OIIO::ImageSpec(image->largeur, image->hauteur, image->nombre_composants, OIIO::TypeDesc::FLOAT);
	out->open(chemin, spec);

	if (!out->write_image(OIIO::TypeDesc::FLOAT, image->donnees)) {
		out->close();
		OIIO::ImageOutput::destroy(out);
		return ResultatOperation::IMAGE_INEXISTANTE;
	}

	out->close();
	OIIO::ImageOutput::destroy(out);

	return ResultatOperation::OK;
}

void IMG_detruit_image(Image *image)
{
	delete [] image->donnees;
	image->donnees = nullptr;
	image->taille_donnees = 0;
	image->largeur = 0;
	image->hauteur = 0;
	image->nombre_composants = 0;
}

}
