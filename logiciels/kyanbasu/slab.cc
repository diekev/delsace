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

#include "slab.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include <ego/bufferobject.h>
#include <ego/program.h>
#include <ego/utils.h>

Slab create_slab(GLsizei width, GLsizei height, int numComponents, int &unit)
{
	Slab slab;
	slab.ping = Surface(width, height, numComponents, unit++);
	slab.pong = Surface(width, height, numComponents, unit++);
	return std::move(slab);
}

Surface::Surface(GLsizei width, GLsizei height, int numComponents, int unit)
    : Surface()
{
	NumComponents = numComponents;

	GLenum format, internal_format;
	switch (numComponents) {
		case 1: format = GL_RED;  internal_format = GL_R32F;    break;
		case 2: format = GL_RG;   internal_format = GL_RG32F;   break;
		case 3: format = GL_RGB;  internal_format = GL_RGB32F;  break;
		case 4: format = GL_RGBA; internal_format = GL_RGBA32F; break;
		default: assert(0);
	}

	int size[] = { width, height };

	framebuffer = numero7::ego::FrameBuffer::create();
	framebuffer->bind();

	texture = numero7::ego::Texture2D::create(unit);
	texture->bind();
	texture->setMinMagFilter(GL_LINEAR, GL_LINEAR);
	texture->setWrapping(GL_CLAMP_TO_EDGE);
	texture->setType(GL_FLOAT, format, internal_format);
	texture->fill(nullptr, size);

	numero7::ego::util::GPU_check_errors("Unable to create normals texture");

	renderbuffer = numero7::ego::RenderBuffer::create();
	renderbuffer->bind();

	framebuffer->attach(*texture, GL_COLOR_ATTACHMENT0);

	numero7::ego::util::GPU_check_errors("Unable to attach color buffer");
	numero7::ego::util::GPU_check_framebuffer("Unable to create FBO");

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	renderbuffer->unbind();
	texture->unbind();
	framebuffer->unbind();
}

void swap_surfaces(Slab *slab)
{
	std::swap(slab->ping, slab->pong);
}

void clear_surface(const Surface &s, float v)
{
	s.framebuffer->bind();
	glClearColor(v, v, v, v);
	glClear(GL_COLOR_BUFFER_BIT);
}

void create_obstacles(Surface &dest, int width, int height)
{
	dest.framebuffer->bind();
    glViewport(0, 0, width, height);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

	numero7::ego::BufferObject buffer;

	numero7::ego::Program program;
	program.load(numero7::ego::VERTEX_SHADER, numero7::ego::util::str_from_file("shaders/simple.vert"));
	program.load(numero7::ego::FRAGMENT_SHADER, numero7::ego::util::str_from_file("shaders/render.frag"));
	program.createAndLinkProgram();

	program.enable();
	program.addAttribute("vertex");

    const bool draw_border = true;
    if (draw_border) {
        #define T 0.9999f
        float positions[] = { -T, -T, T, -T, T,  T, -T, T };
        #undef T

		const GLushort indices[6] = { 0, 1, 2, 0, 2, 3 };

		buffer.bind();
		buffer.generateVertexBuffer(positions, sizeof(float) * 8);
		buffer.generateIndexBuffer(&indices[0], sizeof(GLushort) * 6);
		buffer.attribPointer(program["vertex"], 2);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		buffer.unbind();
    }

	numero7::ego::util::GPU_check_framebuffer("");
	numero7::ego::util::GPU_check_errors("");

    const bool draw_circle = true;
    if (draw_circle) {
        const int slices = 64;
        float positions[slices * 2 * 3];
        const float twopi = 8 * std::atan(1.0f);
        float theta = 0;
        const float dtheta = twopi / (float) (slices - 1);
        float *pPositions = &positions[0];

        for (int i = 0; i < slices; i++) {
            *pPositions++ = 0;
            *pPositions++ = 0;

            *pPositions++ = 0.25f * std::cos(theta) * height / width;
            *pPositions++ = 0.25f * std::sin(theta);
            theta += dtheta;

            *pPositions++ = 0.25f * std::cos(theta) * height / width;
            *pPositions++ = 0.25f * std::sin(theta);
        }

        GLsizeiptr size = sizeof(positions);

		buffer.bind();
		buffer.generateVertexBuffer(positions, size);
		buffer.attribPointer(program["vertex"], 2);

		glDrawArrays(GL_TRIANGLES, 0, slices * 3);

		buffer.unbind();
    }

	numero7::ego::util::GPU_check_errors("");

	program.disable();
}
