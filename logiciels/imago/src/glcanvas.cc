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

#include "glcanvas.h"

#include <cassert>
#include <iostream>

#include "biblinternes/ego/outils.h"
#include "biblinternes/outils/fichier.hh"

GLCanvas::GLCanvas(QWidget *parent)
    : QGLWidget(parent)
    , m_buffer(nullptr)
    , m_texture(nullptr)
{}

void GLCanvas::initializeGL()
{
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();

	if (err != GLEW_OK) {
		std::cerr << "Error: " << glewGetErrorString(err) << "\n";
	}

	m_texture = dls::ego::Texture2D::cree_unique(0);

	m_program.charge(dls::ego::Nuanceur::VERTEX, dls::contenu_fichier("gpu_shaders/vert.glsl"));
	m_program.charge(dls::ego::Nuanceur::FRAGMENT, dls::contenu_fichier("gpu_shaders/frag.glsl"));

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
}

void GLCanvas::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (m_program.est_valide()) {
		m_program.active();
		m_buffer->attache();
		m_texture->attache();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		m_texture->detache();
		m_buffer->detache();
		m_program.desactive();
	}
}

void GLCanvas::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
}

void GLCanvas::loadImage(const QImage &image) const
{
	QImage donnees_image = QGLWidget::convertToGLFormat(image);

	assert((donnees_image.width() > 0) && (donnees_image.height() > 0));

	GLint size[] = { donnees_image.width(), donnees_image.height() };

	m_texture->deloge(true);
	m_texture->attache();
	m_texture->type(GL_UNSIGNED_BYTE, GL_RGBA, GL_RGBA);
	m_texture->filtre_min_mag(GL_LINEAR, GL_LINEAR);
	m_texture->enveloppe(GL_CLAMP);
	m_texture->remplie(donnees_image.bits(), size);
	m_texture->detache();

	dls::ego::util::GPU_check_errors("Unable to create image texture");
}
