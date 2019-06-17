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

#include <ego/utils.h>
#include <QTimer>

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

	m_program.load(numero7::ego::VERTEX_SHADER, numero7::ego::util::str_from_file("shaders/simple.vert"));
	m_program.load(numero7::ego::FRAGMENT_SHADER, numero7::ego::util::str_from_file("shaders/render.frag"));

	m_program.createAndLinkProgram();

	m_program.enable();
	{
		m_program.addAttribute("vertex");
		m_program.addUniform("sampler");
		m_program.addUniform("fill_color");
		m_program.addUniform("scale");
	}
	m_program.disable();

	m_buffer = numero7::ego::BufferObject::create();

	m_buffer->bind();
	m_buffer->generateVertexBuffer(m_vertices, sizeof(float) * 8);
	m_buffer->generateIndexBuffer(&m_indices[0], sizeof(GLushort) * 6);
	m_buffer->attribPointer(m_program["vertex"], 2);
	m_buffer->unbind();

	m_fluid->init(m_width, m_height);

	m_timer->setInterval(1000);

	connect(m_timer, SIGNAL(timeout()), this, SLOT(updateFluid()));
	m_timer->start();
}

void GLCanvas::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);

	if (m_program.isValid()) {
		m_program.enable();
		m_buffer->bind();

		glViewport(0, 0, m_width, m_height);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glUniform1i(m_program("sampler"), m_fluid->m_density.ping.texture->bindcode());

		m_fluid->bindTexture(FLUID_FIELD_DENSITY);
		glUniform3f(m_program("fill_color"), 1.0f, 1.0f, 1.0f);
		glUniform2f(m_program("scale"), 1.0f / m_width, 1.0f / m_height);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		m_fluid->unbindTexture(FLUID_FIELD_DENSITY);

#if 0
		glUniform1i(m_program("sampler"), m_fluid->m_obstacles.texture->bindcode());

		m_fluid->bindTexture(FLUID_FIELD_OBSTACLE);
		glUniform3f(m_program("fill_color"), 0.125f, 0.4f, 0.75f);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		m_fluid->unbindTexture(FLUID_FIELD_OBSTACLE);
#endif

		m_buffer->unbind();
		m_program.disable();
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
