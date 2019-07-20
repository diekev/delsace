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

#include "biblinternes/ego/outils.h"
#include "biblinternes/image/pixel.h"

#include "../editeur_canevas.h"

/* ************************************************************************** */

static void extrait_canal(
		dls::math::matrice_dyn<dls::math::vec3d> const &image,
		float *donnees,
		int canal)
{
	for (int l = 0; l < image.nombre_lignes(); ++l) {
		for (int c = 0; c < image.nombre_colonnes(); ++c) {
			switch (canal) {
				case dls::image::CANAL_R:
					*donnees++ = static_cast<float>(image[l][c][0]);
					break;
				case dls::image::CANAL_G:
					*donnees++ = static_cast<float>(image[l][c][1]);
					break;
				case dls::image::CANAL_B:
					*donnees++ = static_cast<float>(image[l][c][2]);
					break;
				case dls::image::CANAL_A:
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

static void generate_texture(dls::ego::Texture2D::Ptr &texture, const float *data, GLint size[2])
{
	texture->deloge(true);
	texture->attache();
	texture->type(GL_FLOAT, GL_RED, GL_RED);
	texture->filtre_min_mag(GL_LINEAR, GL_LINEAR);
	texture->enveloppe(GL_CLAMP);
	texture->remplie(data, size);
	texture->detache();
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
	m_texture_R = dls::ego::Texture2D::cree_unique(0);
	m_texture_G = dls::ego::Texture2D::cree_unique(1);
	m_texture_B = dls::ego::Texture2D::cree_unique(2);
	m_texture_A = dls::ego::Texture2D::cree_unique(3);

	m_program.charge(dls::ego::Nuanceur::VERTEX, vertex_source);
	m_program.charge(dls::ego::Nuanceur::FRAGMENT, fragment_source);

	m_program.cree_et_lie_programme();

	m_program.active();
	{
		m_program.ajoute_attribut("vertex");
		m_program.ajoute_uniforme("red_channel");
		m_program.ajoute_uniforme("green_channel");
		m_program.ajoute_uniforme("blue_channel");
		m_program.ajoute_uniforme("alpha_channel");

		glUniform1i(m_program("red_channel"), static_cast<int>(m_texture_R->code_attache()));
		glUniform1i(m_program("green_channel"), static_cast<int>(m_texture_G->code_attache()));
		glUniform1i(m_program("blue_channel"), static_cast<int>(m_texture_B->code_attache()));
		glUniform1i(m_program("alpha_channel"), static_cast<int>(m_texture_A->code_attache()));
	}
	m_program.desactive();

	m_buffer = dls::ego::TamponObjet::cree_unique();

	m_buffer->attache();
	m_buffer->genere_tampon_sommet(m_vertices, sizeof(float) * 8);
	m_buffer->genere_tampon_index(&m_indices[0], sizeof(GLushort) * 6);
	m_buffer->pointeur_attribut(static_cast<unsigned>(m_program["vertex"]), 2);
	m_buffer->detache();
}

void VisionneurImage::peint_opengl()
{
	if (m_program.est_valide()) {
		m_program.active();
		m_buffer->attache();
		m_texture_R->attache();
		m_texture_G->attache();
		m_texture_B->attache();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		m_texture_B->detache();
		m_texture_G->detache();
		m_texture_R->detache();
		m_buffer->detache();
		m_program.desactive();
	}
}

void VisionneurImage::redimensionne(int largeur, int hauteur)
{
	glViewport(0, 0, largeur, hauteur);
}

void VisionneurImage::charge_image(dls::math::matrice_dyn<dls::math::vec3d> const &image)
{
	if ((image.nombre_colonnes() == 0) || (image.nombre_lignes() == 0)) {
		return;
	}

	GLint size[] = {
		image.nombre_colonnes(),
		image.nombre_lignes()
	};

	auto const resolution = image.nombre_colonnes() * image.nombre_lignes();

	m_donnees_r.redimensionne(resolution);
	m_donnees_g.redimensionne(resolution);
	m_donnees_b.redimensionne(resolution);

	extrait_canal(image, &m_donnees_r[0], dls::image::CANAL_R);
	extrait_canal(image, &m_donnees_g[0], dls::image::CANAL_G);
	extrait_canal(image, &m_donnees_b[0], dls::image::CANAL_B);

	if (m_largeur != size[0] || m_hauteur != size[1]) {
		m_hauteur = size[0];
		m_largeur = size[1];

		m_parent->resize(m_hauteur, m_largeur);
	}

	generate_texture(m_texture_R, m_donnees_r.donnees(), size);
	generate_texture(m_texture_G, m_donnees_g.donnees(), size);
	generate_texture(m_texture_B, m_donnees_b.donnees(), size);

	dls::ego::util::GPU_check_errors("Unable to create image texture");
}
