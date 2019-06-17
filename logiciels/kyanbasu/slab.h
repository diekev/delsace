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

#include <ego/framebuffer.h>
#include <ego/texture.h>

class Surface {
public:
    GLuint FboHandle = 0;
	numero7::ego::FrameBuffer::Ptr framebuffer;
	numero7::ego::RenderBuffer::Ptr renderbuffer;
	numero7::ego::Texture2D::Ptr texture = nullptr;
    int NumComponents = 0;
	unsigned int render_buffer = 0;

	Surface() = default;
	Surface(GLsizei width, GLsizei height, int numComponents, int unit);

	/* XXX ego::Texture is non-copyable */
	Surface(Surface&&) = default;
	Surface& operator=(Surface&&) = default;
	~Surface() = default;
};

class Slab {
public:
    Surface ping;
    Surface pong;
};

Slab create_slab(GLsizei width, GLsizei height, int numComponents, int &unit);

Surface CreateSurface(GLsizei width, GLsizei height, int numComponents, int unit);
void create_obstacles(Surface &dest, int width, int height);

void swap_surfaces(Slab *slab);

void clear_surface(const Surface &s, float v);
