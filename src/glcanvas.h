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

#include "GPUProgram.h"

#include <QGLWidget>
#include <memory>

class GPUBuffer;
class GPUTexture;

class GLCanvas : public QGLWidget {
	GPUProgram m_program;
	std::unique_ptr<GPUBuffer> m_buffer;
	std::unique_ptr<GPUTexture> m_texture;

	const float m_vertices[8] = {
	    0.0f, 0.0f,
	    1.0f, 0.0f,
	    1.0f, 1.0f,
	    0.0f, 1.0f
	};

	const GLushort m_indices[6] = { 0, 1, 2, 0, 2, 3 };

public:
	GLCanvas(QWidget *parent);
	~GLCanvas() = default;

	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);
	void loadImage(QImage *image);
};
