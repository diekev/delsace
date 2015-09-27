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

#include <cassert>
#include <iostream>

#include "glwindow.h"

#include "util_opengl.h"

#define GL_CHECK_ERROR assert(glGetError() == GL_NO_ERROR);

GLWindow::GLWindow(QWidget *parent)
    : QGLWidget(parent)
    , m_buffer_data(nullptr)
{}

GLWindow::~GLWindow()
{
	glDeleteTextures(1, &m_texture);
	delete m_buffer_data;
}

void GLWindow::initializeGL()
{
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();

	if (err != GLEW_OK) {
		std::cerr << "Error: " << glewGetErrorString(err) << "\n";
	}

	GL_CHECK_ERROR;

	m_shader.loadFromFile(GL_VERTEX_SHADER, "gpu_shaders/vertex_shader.glsl");
	m_shader.loadFromFile(GL_FRAGMENT_SHADER, "gpu_shaders/fragment_shader.glsl");

	m_shader.createAndLinkProgram();

	m_shader.use();
	{
		m_shader.addAttribute("vertex");
		m_shader.addUniform("image");

		glUniform1i(m_shader("image"), 0);
	}
	m_shader.unUse();

	m_buffer_data = new VBOData();

	m_buffer_data->bind();
	m_buffer_data->create_vertex_buffer(m_vertices, sizeof(float) * 8);
	m_buffer_data->create_index_buffer(&m_indices[0], sizeof(GLuint) * 6);
	m_buffer_data->attrib_pointer(m_shader["vertex"]);
	m_buffer_data->unbind();
}

void GLWindow::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_shader.use();
	{
		m_buffer_data->bind();
		texture_bind(GL_TEXTURE_2D, m_texture, 0);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

		texture_unbind(GL_TEXTURE_2D, 0);
		m_buffer_data->unbind();
	}
	m_shader.unUse();
}

void GLWindow::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
}

void GLWindow::loadImage(QImage *image)
{
	QImage data = QGLWidget::convertToGLFormat(*image);

	if (glIsTexture(m_texture)) {
		glDeleteTextures(1, &m_texture);
	}

	assert((data.width() > 0) && (data.height() > 0));

	int size[] = { data.width(), data.height() };
	create_texture_2D(m_texture, size, data.bits());

	gl_check_errors();
}
