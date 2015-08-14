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

#pragma once

#include <QGLShaderProgram>
#include <QGLWidget>

#include "mainwindow.h"

class GLWindow : public QGLWidget {
	QGLShaderProgram m_program;
	GLuint m_textures[1];
	GLuint m_vaoID;
	GLuint m_vbo_vertsID;
	GLuint m_vbo_indexID;
	QVector2D m_vertices[4];
	GLushort m_indices[6];

protected:
	class MainWindow &m_main_win;
	QImage *m_data, m_gl_data;

public:
	GLWindow(QWidget *parent, MainWindow &window);

	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);
	void loadImage();
};
