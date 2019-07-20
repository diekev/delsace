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

#include <QTimer>

#include "biblinternes/ego/outils.h"
#include "biblinternes/outils/fichier.hh"

#include "fluid.h"

GLCanvas::GLCanvas(QWidget *parent)
    : QGLWidget(parent)
    , m_buffer(nullptr)
    , m_fluid(new Fluid)
    , m_width(600)
    , m_height(1000)
    , m_timer(new QTimer(this))
{}

GLCanvas::~GLCanvas()
{
	delete m_fluid;
}

void GLCanvas::initializeGL()
{
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();

	if (err != GLEW_OK) {
		std::cerr << "Error: " << glewGetErrorString(err) << "\n";
	}

	m_program.charge(dls::ego::Nuanceur::VERTEX, dls::contenu_fichier("shaders/simple.vert"));
	m_program.charge(dls::ego::Nuanceur::FRAGMENT, dls::contenu_fichier("shaders/render.frag"));

	m_program.cree_et_lie_programme();

	m_program.active();
	{
		m_program.ajoute_attribut("vertex");
		m_program.ajoute_uniforme("sampler");
		m_program.ajoute_uniforme("fill_color");
		m_program.ajoute_uniforme("scale");
	}
	m_program.desactive();

	m_buffer = dls::ego::TamponObjet::cree_unique();

	m_buffer->attache();
	m_buffer->genere_tampon_sommet(m_vertices, sizeof(float) * 8);
	m_buffer->genere_tampon_index(&m_indices[0], sizeof(GLushort) * 6);
	m_buffer->pointeur_attribut(static_cast<unsigned>(m_program["vertex"]), 2);
	m_buffer->detache();

	m_fluid->init(m_width, m_height);

	m_timer->setInterval(1000);

	connect(m_timer, SIGNAL(timeout()), this, SLOT(updateFluid()));
	m_timer->start();
}

void GLCanvas::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);

	if (m_program.est_valide()) {
		m_program.active();
		m_buffer->attache();

		glViewport(0, 0, m_width, m_height);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glUniform1i(m_program("sampler"), static_cast<int>(m_fluid->m_density.ping.texture->code_attache()));

		m_fluid->bindTexture(FLUID_FIELD_DENSITY);
		glUniform3f(m_program("fill_color"), 1.0f, 1.0f, 1.0f);
		glUniform2f(m_program("scale"), 1.0f / static_cast<float>(m_width), 1.0f / static_cast<float>(m_height));

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		m_fluid->unbindTexture(FLUID_FIELD_DENSITY);

#if 0
		glUniform1i(m_program("sampler"), m_fluid->m_obstacles.texture->bindcode());

		m_fluid->bindTexture(FLUID_FIELD_OBSTACLE);
		glUniform3f(m_program("fill_color"), 0.125f, 0.4f, 0.75f);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		m_fluid->unbindTexture(FLUID_FIELD_OBSTACLE);
#endif

		m_buffer->detache();
		m_program.desactive();
	}

	glDisable(GL_BLEND);
}

void GLCanvas::resizeGL(int w, int h)
{
	m_width = w;
	m_height = h;
	glViewport(0, 0, w, h);
}

void GLCanvas::updateFluid()
{
	m_fluid->step(0);
	updateGL();
}
