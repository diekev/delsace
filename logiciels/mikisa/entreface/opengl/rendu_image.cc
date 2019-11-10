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

#include "biblinternes/ego/outils.h"

#include "biblinternes/opengl/tampon_rendu.h"

#include "biblinternes/memoire/logeuse_memoire.hh"

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

TamponRendu *cree_tampon_image()
{
	auto tampon = memoire::loge<TamponRendu>("TamponRendu");

	tampon->charge_source_programme(dls::ego::Nuanceur::VERTEX, source_vertex);
	tampon->charge_source_programme(dls::ego::Nuanceur::FRAGMENT, source_fragment);
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
	programme->uniforme("image", texture->code_attache());
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

void genere_texture_image(TamponRendu *tampon, const float *data, int size[])
{
	auto texture = tampon->texture();
	texture->deloge(true);
	texture->attache();
	texture->type(GL_FLOAT, GL_RGBA, GL_RGBA);
	texture->filtre_min_mag(GL_LINEAR, GL_LINEAR);
	texture->enveloppe(GL_CLAMP);
	texture->remplie(data, size);
	texture->detache();
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
	auto tampon = memoire::loge<TamponRendu>("TamponRendu");

	tampon->charge_source_programme(dls::ego::Nuanceur::VERTEX, source_vertex_bordure);
	tampon->charge_source_programme(dls::ego::Nuanceur::FRAGMENT, source_fragment_bordure);
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
	memoire::deloge("TamponRendu", m_tampon_image);
	memoire::deloge("TamponRendu", m_tampon_bordure);
}

void RenduImage::charge_image(const grille_couleur &image)
{
	GLint size[2] = {
		image.desc().resolution.x,
		image.desc().resolution.y
	};

	genere_texture_image(m_tampon_image, &image.valeur(0).r, size);

	dls::ego::util::GPU_check_errors("Unable to create image texture");
}

void RenduImage::dessine(ContexteRendu const &contexte)
{
	m_tampon_image->dessine(contexte);
}

void RenduImage::dessine_bordure(ContexteRendu const &contexte)
{
	m_tampon_bordure->dessine(contexte);
}
