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

#include "rendu_image.h"

#include <ego/outils.h>

#include "bibliotheques/opengl/tampon_rendu.h"

/* ************************************************************************** */

static const char *source_vertex =
		"#version 330 core\n"
		"layout(location = 0) in vec2 vertex;\n"
		"smooth out vec2 UV;\n"
		"uniform mat4 MVP;\n"
		"uniform mat4 matrice;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = MVP * matrice * vec4(vertex * 2.0 - 1.0, 0.0, 1.0);\n"
		"	UV = vertex;\n"
		"}\n";

static const char *source_fragment =
		"#version 330 core\n"
		"layout (location = 0) out vec4 fragment_color;\n"
		"smooth in vec2 UV;\n"
		"uniform sampler2D image;\n"
		" void main()\n"
		"{\n"
		"	vec2 flipped = vec2(UV.x, 1.0f - UV.y);\n"
		"	fragment_color = texture(image, flipped);\n"
		"}\n";

static TamponRendu *cree_tampon_image()
{
	auto tampon = new TamponRendu();

	tampon->charge_source_programme(numero7::ego::Nuanceur::VERTEX, source_vertex);
	tampon->charge_source_programme(numero7::ego::Nuanceur::FRAGMENT, source_fragment);
	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("vertex");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");
	parametre_programme.ajoute_uniforme("image");

	tampon->parametres_programme(parametre_programme);

	tampon->ajoute_texture();
	auto texture = tampon->texture();

	auto programme = tampon->programme();
	programme->active();
	programme->uniforme("image", texture->number());
	programme->desactive();

	float sommets[8] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};

	unsigned int index[6] = { 0, 1, 2, 0, 2, 3 };

	auto parametres_tampon = ParametresTampon();
	parametres_tampon.attribut = "vertex";
	parametres_tampon.dimension_attribut = 2;
	parametres_tampon.pointeur_sommets = sommets;
	parametres_tampon.taille_octet_sommets = sizeof(float) * 8;
	parametres_tampon.pointeur_index = index;
	parametres_tampon.taille_octet_index = sizeof(unsigned int) * 6;
	parametres_tampon.elements = 6;

	tampon->remplie_tampon(parametres_tampon);

	return tampon;
}

static void genere_texture(numero7::ego::Texture2D  *texture, const float *data, GLint size[2])
{
	texture->free(true);
	texture->bind();
	texture->setType(GL_FLOAT, GL_RGBA, GL_RGBA);
	texture->setMinMagFilter(GL_LINEAR, GL_LINEAR);
	texture->setWrapping(GL_CLAMP);
	texture->fill(data, size);
	texture->unbind();
}

/* ************************************************************************** */

static const char *source_vertex_bordure =
		"#version 330 core\n"
		"layout(location = 0) in vec2 sommets;\n"
		"uniform mat4 MVP;\n"
		"uniform mat4 matrice;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = MVP * matrice * vec4(sommets * 2.0 - 1.0, 0.0, 1.0);\n"
		"}\n";

static const char *source_fragment_bordure =
		"#version 330 core\n"
		"layout (location = 0) out vec4 fragment_color;\n"
		" void main()\n"
		"{\n"
		"	fragment_color = vec4(1.0);\n"
		"}\n";

static TamponRendu *cree_tampon_bordure()
{
	auto tampon = new TamponRendu();

	tampon->charge_source_programme(numero7::ego::Nuanceur::VERTEX, source_vertex_bordure);
	tampon->charge_source_programme(numero7::ego::Nuanceur::FRAGMENT, source_fragment_bordure);
	tampon->finalise_programme();

	ParametresProgramme parametre_programme;
	parametre_programme.ajoute_attribut("sommets");
	parametre_programme.ajoute_uniforme("matrice");
	parametre_programme.ajoute_uniforme("MVP");

	tampon->parametres_programme(parametre_programme);

	float sommets[8] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};

	unsigned int index[8] = { 0, 1, 1, 2, 2, 3, 3, 0 };

	auto parametres_tampon = ParametresTampon();
	parametres_tampon.attribut = "sommets";
	parametres_tampon.dimension_attribut = 2;
	parametres_tampon.pointeur_sommets = sommets;
	parametres_tampon.taille_octet_sommets = sizeof(float) * 8;
	parametres_tampon.pointeur_index = index;
	parametres_tampon.taille_octet_index = sizeof(unsigned int) * 8;
	parametres_tampon.elements = 8;

	tampon->remplie_tampon(parametres_tampon);

	auto parametres_dessin = ParametresDessin();
	parametres_dessin.type_dessin(GL_LINES);
	parametres_dessin.taille_ligne(1.0);

	tampon->parametres_dessin(parametres_dessin);

	return tampon;
}

/* ************************************************************************** */

RenduImage::RenduImage()
{
	m_tampon_image = cree_tampon_image();
	m_tampon_bordure = cree_tampon_bordure();
}

RenduImage::~RenduImage()
{
	delete m_tampon_image;
	delete m_tampon_bordure;
}

void RenduImage::charge_image(const numero7::math::matrice<numero7::image::Pixel<float> > &image)
{
	GLint size[2] = {
		image.nombre_colonnes(),
		image.nombre_lignes()
	};

	genere_texture(m_tampon_image->texture(), &image[0][0].r, size);

	numero7::ego::util::GPU_check_errors("Unable to create image texture");
}

void RenduImage::dessine(ContexteRendu const &contexte)
{
	m_tampon_image->dessine(contexte);
	m_tampon_bordure->dessine(contexte);
}
