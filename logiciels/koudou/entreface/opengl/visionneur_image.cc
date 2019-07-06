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

#include "visionneur_image.h"

#include <ego/outils.h>
#include <image/pixel.h>

#include "../editeur_canevas.h"

/* ************************************************************************** */

static void extrait_canal(
		numero7::math::matrice<dls::math::vec3d> const &image,
		float *donnees,
		int canal)
{
	for (int l = 0; l < image.nombre_lignes(); ++l) {
		for (int c = 0; c < image.nombre_colonnes(); ++c) {
			switch (canal) {
				case numero7::image::CANAL_R:
					*donnees++ = static_cast<float>(image[l][c][0]);
					break;
				case numero7::image::CANAL_G:
					*donnees++ = static_cast<float>(image[l][c][1]);
					break;
				case numero7::image::CANAL_B:
					*donnees++ = static_cast<float>(image[l][c][2]);
					break;
				case numero7::image::CANAL_A:
					*donnees++ = static_cast<float>(image[l][c][3]);
					break;
			}
		}
	}
}

/* ************************************************************************** */

static const char *vertex_source =
		"#version 330 core\n"
		"layout(location = 0) in vec2 vertex;\n"
		"smooth out vec2 UV;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = vec4(vertex * 2.0 - 1.0, 0.0, 1.0);\n"
		"	UV = vertex;\n"
		"}\n";

static const char *fragment_source =
		"#version 330 core\n"
		"layout (location = 0) out vec4 fragment_color;\n"
		"smooth in vec2 UV;\n"
		"uniform sampler2D red_channel;\n"
		"uniform sampler2D blue_channel;\n"
		"uniform sampler2D green_channel;\n"
		"uniform sampler2D alpha_channel;\n"
		" void main()\n"
		"{\n"
		"	vec2 flipped = vec2(UV.x, 1.0f - UV.y);\n"
		"	float red = texture(red_channel, flipped).r;\n"
		"	float green = texture(green_channel, flipped).r;\n"
		"	float blue = texture(blue_channel, flipped).r;\n"
		"	float alpha = texture(alpha_channel, flipped).r;\n"
		"	fragment_color = vec4(red, green, blue, 1.0);\n"
		"}\n";

static void generate_texture(numero7::ego::Texture2D::Ptr &texture, const float *data, GLint size[2])
{
	texture->free(true);
	texture->bind();
	texture->setType(GL_FLOAT, GL_RED, GL_RED);
	texture->setMinMagFilter(GL_LINEAR, GL_LINEAR);
	texture->setWrapping(GL_CLAMP);
	texture->fill(data, size);
	texture->unbind();
}

/* ************************************************************************** */

VisionneurImage::VisionneurImage(VueCanevas *parent)
	: m_parent(parent)
	, m_buffer(nullptr)
	, m_texture_R(nullptr)
	, m_texture_G(nullptr)
	, m_texture_B(nullptr)
	, m_texture_A(nullptr)
{}

void VisionneurImage::initialise()
{
	m_texture_R = numero7::ego::Texture2D::create(0);
	m_texture_G = numero7::ego::Texture2D::create(1);
	m_texture_B = numero7::ego::Texture2D::create(2);
	m_texture_A = numero7::ego::Texture2D::create(3);

	m_program.charge(numero7::ego::Nuanceur::VERTEX, vertex_source);
	m_program.charge(numero7::ego::Nuanceur::FRAGMENT, fragment_source);

	m_program.cree_et_lie_programme();

	m_program.active();
	{
		m_program.ajoute_attribut("vertex");
		m_program.ajoute_uniforme("red_channel");
		m_program.ajoute_uniforme("green_channel");
		m_program.ajoute_uniforme("blue_channel");
		m_program.ajoute_uniforme("alpha_channel");

		glUniform1i(static_cast<int>(m_program("red_channel")), static_cast<int>(m_texture_R->number()));
		glUniform1i(static_cast<int>(m_program("green_channel")), static_cast<int>(m_texture_G->number()));
		glUniform1i(static_cast<int>(m_program("blue_channel")), static_cast<int>(m_texture_B->number()));
		glUniform1i(static_cast<int>(m_program("alpha_channel")), static_cast<int>(m_texture_A->number()));
	}
	m_program.desactive();

	m_buffer = numero7::ego::TamponObjet::create();

	m_buffer->bind();
	m_buffer->generateVertexBuffer(m_vertices, sizeof(float) * 8);
	m_buffer->generateIndexBuffer(&m_indices[0], sizeof(GLushort) * 6);
	m_buffer->attribPointer(m_program["vertex"], 2);
	m_buffer->unbind();
}

void VisionneurImage::peint_opengl()
{
	if (m_program.est_valide()) {
		m_program.active();
		m_buffer->bind();
		m_texture_R->bind();
		m_texture_G->bind();
		m_texture_B->bind();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		m_texture_B->unbind();
		m_texture_G->unbind();
		m_texture_R->unbind();
		m_buffer->unbind();
		m_program.desactive();
	}
}

void VisionneurImage::redimensionne(int largeur, int hauteur)
{
	glViewport(0, 0, largeur, hauteur);
}

void VisionneurImage::charge_image(numero7::math::matrice<dls::math::vec3d> const &image)
{
	if ((image.nombre_colonnes() == 0) || (image.nombre_lignes() == 0)) {
		return;
	}

	GLint size[] = {
		image.nombre_colonnes(),
		image.nombre_lignes()
	};

	auto const resolution = static_cast<size_t>(image.nombre_colonnes() * image.nombre_lignes());

	m_donnees_r.resize(resolution);
	m_donnees_g.resize(resolution);
	m_donnees_b.resize(resolution);

	extrait_canal(image, &m_donnees_r[0], numero7::image::CANAL_R);
	extrait_canal(image, &m_donnees_g[0], numero7::image::CANAL_G);
	extrait_canal(image, &m_donnees_b[0], numero7::image::CANAL_B);

	if (m_largeur != size[0] || m_hauteur != size[1]) {
		m_hauteur = size[0];
		m_largeur = size[1];

		m_parent->resize(m_hauteur, m_largeur);
	}

	generate_texture(m_texture_R, m_donnees_r.data(), size);
	generate_texture(m_texture_G, m_donnees_g.data(), size);
	generate_texture(m_texture_B, m_donnees_b.data(), size);

	numero7::ego::util::GPU_check_errors("Unable to create image texture");
}
