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

#include "coeur/kanba.h"
#include "../editeur_canevas.h"

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
		"uniform sampler2D image;\n"
		" void main()\n"
		"{\n"
		"	vec2 flipped = vec2(UV.x, 1.0f - UV.y);\n"
		"	fragment_color = texture(image, flipped);\n"
		"}\n";

static void generate_texture(dls::ego::Texture2D::Ptr &texture, const float *data, GLint size[2])
{
	texture->deloge(true);
	texture->attache();
	texture->type(GL_FLOAT, GL_RGBA, GL_RGBA);
	texture->filtre_min_mag(GL_LINEAR, GL_LINEAR);
	texture->enveloppe(GL_CLAMP);
	texture->remplie(data, size);
	texture->detache();
}

/* ************************************************************************** */

VisionneurImage::VisionneurImage(VueCanevas *parent, Kanba *kanba)
	: m_parent(parent)
	, m_buffer(nullptr)
	, m_texture(nullptr)
	, m_kanba(kanba)
{}

void VisionneurImage::initialise()
{
	m_texture = dls::ego::Texture2D::cree_unique(0);

	m_program.charge(dls::ego::Nuanceur::VERTEX, vertex_source);
	m_program.charge(dls::ego::Nuanceur::FRAGMENT, fragment_source);

	m_program.cree_et_lie_programme();

	m_program.active();
	{
		m_program.ajoute_attribut("vertex");
		m_program.ajoute_uniforme("image");

		glUniform1i(m_program("image"), static_cast<int>(m_texture->code_attache()));
	}
	m_program.desactive();

	m_buffer = dls::ego::TamponObjet::cree_unique();

	m_buffer->attache();
	m_buffer->genere_tampon_sommet(m_vertices, sizeof(float) * 8);
	m_buffer->genere_tampon_index(&m_indices[0], sizeof(GLushort) * 6);
	m_buffer->pointeur_attribut(static_cast<unsigned>(m_program["vertex"]), 2);
	m_buffer->detache();

	charge_image(m_kanba->tampon);
}

void VisionneurImage::peint_opengl()
{
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable (GL_BLEND);
	if (m_program.est_valide()) {
		m_program.active();
		m_buffer->attache();
		m_texture->attache();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		m_texture->detache();
		m_buffer->detache();
		m_program.desactive();
	}
	glDisable (GL_BLEND);
}

void VisionneurImage::redimensionne(int largeur, int hauteur)
{
	glViewport(0, 0, largeur, hauteur);
}

void VisionneurImage::charge_image(dls::math::matrice_dyn<dls::math::vec4f> const &image)
{
	if ((image.nombre_colonnes() == 0) || (image.nombre_lignes() == 0)) {
		return;
	}

	GLint size[] = {
		image.nombre_colonnes(),
		image.nombre_lignes()
	};

	if (m_largeur != size[0] || m_hauteur != size[1]) {
		m_hauteur = size[0];
		m_largeur = size[1];

		m_parent->resize(m_hauteur, m_largeur);
	}

	generate_texture(m_texture, &image[0][0][0], size);

	dls::ego::util::GPU_check_errors("Unable to create image texture");
}
