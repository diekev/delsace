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

class GPUBuffer {
	GLuint m_vao;
	GLuint m_vertex_buffer;
	GLuint m_index_buffer;
	GLuint m_color_buffer;

	void generateBuffer(GLuint &id, const GLvoid *data, const size_t size, GLenum target) const;

public:
	GPUBuffer();
	~GPUBuffer();

	void bind() const;
	void unbind() const;

	void attribPointer(GLuint index, GLint size) const;

	void generateVertexBuffer(const GLvoid *vertices, const size_t size);
	void generateIndexBuffer(const GLvoid *indices, const size_t size);
	void generateNormalBuffer(const GLvoid *colors, const size_t size);

	void updateIndexBuffer(const GLvoid *indices, const size_t size) const;
	void updateVertexBuffer(const GLvoid *vertices, const size_t size) const;
};
