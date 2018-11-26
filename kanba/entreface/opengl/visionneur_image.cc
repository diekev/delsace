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
#include <numero7/image/pixel.h>

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

static void generate_texture(numero7::ego::Texture2D::Ptr &texture, const float *data, GLint size[2])
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

VisionneurImage::VisionneurImage(VueCanevas *parent, Kanba *kanba)
	: m_parent(parent)
	, m_buffer(nullptr)
	, m_texture(nullptr)
	, m_kanba(kanba)
{}

void VisionneurImage::initialise()
{
	m_texture = numero7::ego::Texture2D::create(0);

	m_program.charge(numero7::ego::Nuanceur::VERTEX, vertex_source);
	m_program.charge(numero7::ego::Nuanceur::FRAGMENT, fragment_source);

	m_program.cree_et_lie_programme();

	m_program.active();
	{
		m_program.ajoute_attribut("vertex");
		m_program.ajoute_uniforme("image");

		glUniform1i(m_program("image"), m_texture->number());
	}
	m_program.desactive();

	m_buffer = numero7::ego::TamponObjet::create();

	m_buffer->bind();
	m_buffer->generateVertexBuffer(m_vertices, sizeof(float) * 8);
	m_buffer->generateIndexBuffer(&m_indices[0], sizeof(GLushort) * 6);
	m_buffer->attribPointer(m_program["vertex"], 2);
	m_buffer->unbind();

	charge_image(m_kanba->tampon);
}

void VisionneurImage::peint_opengl()
{
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable (GL_BLEND);
	if (m_program.est_valide()) {
		m_program.active();
		m_buffer->bind();
		m_texture->bind();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		m_texture->unbind();
		m_buffer->unbind();
		m_program.desactive();
	}
	glDisable (GL_BLEND);
}

void VisionneurImage::redimensionne(int largeur, int hauteur)
{
	glViewport(0, 0, largeur, hauteur);
}

void VisionneurImage::charge_image(const numero7::math::matrice<dls::math::vec4f> &image)
{
	if ((image.nombre_colonnes() == 0) || (image.nombre_lignes() == 0)) {
		return;
	}

	GLint size[] = {
		static_cast<GLint>(image.nombre_colonnes()),
		static_cast<GLint>(image.nombre_lignes())
	};

	if (m_largeur != size[0] || m_hauteur != size[1]) {
		m_hauteur = size[0];
		m_largeur = size[1];

		m_parent->resize(m_hauteur, m_largeur);
	}

	generate_texture(m_texture, &image[0][0][0], size);

	numero7::ego::util::GPU_check_errors("Unable to create image texture");
}
