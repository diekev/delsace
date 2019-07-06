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

#include "biblinternes/ego/program.h"
#include "biblinternes/ego/bufferobject.h"
#include "biblinternes/ego/texture.h"

#include <QGLWidget>

class GLCanvas : public QGLWidget {
	ego::Program m_program;
	ego::BufferObject::Ptr m_buffer;
	ego::Texture2D::Ptr m_texture;

	const float m_vertices[8] = {
	    0.0f, 0.0f,
	    1.0f, 0.0f,
	    1.0f, 1.0f,
	    0.0f, 1.0f
	};

	const GLushort m_indices[6] = { 0, 1, 2, 0, 2, 3 };

public:
	explicit GLCanvas(QWidget *parent = nullptr);
	~GLCanvas() = default;

	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);
	void loadImage(const QImage &image) const;
};
