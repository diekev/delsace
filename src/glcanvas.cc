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

#include <iostream>

#include "glcanvas.h"
#include "GPUBuffer.h"
#include "GPUTexture.h"
#include "util_opengl.h"

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

	m_texture = std::unique_ptr<GPUTexture>(new GPUTexture(GL_TEXTURE_2D, 0));

	m_program.loadFromFile(GL_VERTEX_SHADER, "shaders/vert.glsl");
	m_program.loadFromFile(GL_FRAGMENT_SHADER, "shaders/frag.glsl");

	m_program.createAndLinkProgram();

	m_program.enable();
	{
		m_program.addAttribute("vertex");
		m_program.addUniform("image");

		glUniform1i(m_program("image"), m_texture->unit());
	}
	m_program.disable();

	m_buffer = std::unique_ptr<GPUBuffer>(new GPUBuffer());

	m_buffer->bind();
	m_buffer->generateVertexBuffer(m_vertices, sizeof(float) * 8);
	m_buffer->generateIndexBuffer(&m_indices[0], sizeof(GLushort) * 6);
	m_buffer->attribPointer(m_program["vertex"], 2);
	m_buffer->unbind();
}

void GLCanvas::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (m_program.isValid()) {
		m_program.enable();
		m_buffer->bind();
		m_texture->bind();

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		m_texture->unbind();
		m_buffer->unbind();
		m_program.disable();
	}
}

void GLCanvas::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
}

void GLCanvas::loadImage(QImage *image)
{
	QImage data = QGLWidget::convertToGLFormat(*image);

	assert((data.width() > 0) && (data.height() > 0));

	GLint size[] = { data.width(), data.height() };

	m_texture->free(true);
	m_texture->bind();
	m_texture->setType(GL_UNSIGNED_BYTE, GL_RGBA, GL_RGB);
	m_texture->setMinMagFilter(GL_LINEAR, GL_LINEAR);
	m_texture->setWrapping(GL_CLAMP);
	m_texture->create(data.bits(), size);
	m_texture->unbind();

	gl_check_errors();
}
