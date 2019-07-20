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

const char *vert_shader =
        "#version 330 core\n"
        "layout(location = 0) in vec2 vertex;\n"
        "smooth out vec2 UV;\n"
        "void main()\n"
        "{\n"
        "gl_Position = vec4(vertex * 2.0 - 1.0, 0.0, 1.0);\n"
        "UV = vertex;\n"
        "}\n";

const char *basic_fragment =
        "#version 330 core\n\n"
        "layout (location = 0) out vec4 fragment_color;\n"
		"smooth in vec2 UV;\n"
        "uniform float aspect;\n\n"
        "void main()\n"
		"{\n"
		"\tfragment_color = vec4(0.01, 0.01, 0.98, 1.0);\n"
		"}\n";

GLCanvas::GLCanvas(QWidget *parent)
    : QGLWidget(parent)
    , m_buffer(nullptr)
    , m_width(0)
    , m_height(0)
    , m_aspect(0.0f)
{}

void GLCanvas::initializeGL()
{
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();

	if (err != GLEW_OK) {
		std::cerr << "Error: " << glewGetErrorString(err) << "\n";
	}

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

	if (!m_program.est_valide(m_stringstream)) {
		checkErrors();
		return;
	}

	m_program.active();
	m_buffer->attache();

	glUniform1f(m_program("aspect"), m_aspect);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

	m_buffer->detache();
	m_program.desactive();
}

void GLCanvas::resizeGL(int w, int h)
{
	m_width = w;
	m_height = h;
	m_aspect = static_cast<float>(m_width) / static_cast<float>(m_height);
	glViewport(0, 0, w, h);
}

void GLCanvas::loadProgram(const QString &shader)
{
	m_program.charge(dls::ego::Nuanceur::VERTEX, vert_shader, m_stringstream);

	checkErrors();

	m_program.charge(dls::ego::Nuanceur::FRAGMENT, shader.toStdString(), m_stringstream);

	checkErrors();

	m_program.cree_et_lie_programme(m_stringstream);

	checkErrors();

	m_program.active();
	{
		m_program.ajoute_attribut("vertex");
		m_program.ajoute_uniforme("aspect");
	}
	m_program.desactive();

	updateGL();
}

QString GLCanvas::errorLog()
{
	QString str(m_stringstream.str().c_str());
	m_stringstream.str("");
	m_stringstream.clear();
	return str;
}

QImage GLCanvas::makeThumbnail()
{
	const auto pixmap = this->grabFrameBuffer();
	const auto thumb = pixmap.scaled(64, 64, Qt::KeepAspectRatio);
	return thumb;
}

void GLCanvas::checkErrors()
{
	if (!m_stringstream.str().empty()) {
		Q_EMIT shaderErrorOccurred();
	}
}
