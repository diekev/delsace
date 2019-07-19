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

#include "biblinternes/ego/programme.h"
#include "biblinternes/ego/tampon_objet.h"
#include "biblinternes/ego/texture.h"

/* Needs to included after glew.h, which is included in gpu/program.h and
 * gpu/bufferobject.h */
#include <QGLWidget>

class Fluid;

class GLCanvas : public QGLWidget {
	Q_OBJECT

	dls::ego::Programme m_program{};
	dls::ego::TamponObjet::Ptr m_buffer{};

	const float m_vertices[8] = {
	    -1.0f, -1.0f,
	     1.0f, -1.0f,
	     1.0f,  1.0f,
	    -1.0f,  1.0f
	};

	const GLushort m_indices[6] = { 0, 1, 2, 0, 2, 3 };

	Fluid *m_fluid = nullptr;

	int m_width = 0;
	int m_height = 0;

	QTimer *m_timer = nullptr;

public:
	explicit GLCanvas(QWidget *parent = nullptr);
	~GLCanvas();

	GLCanvas(GLCanvas const &) = default;
	GLCanvas &operator=(GLCanvas const &) = default;

	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);

private Q_SLOTS:
	void updateFluid();
};
