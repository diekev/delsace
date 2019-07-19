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

#include "biblinternes/ego/outils.h"
#include "biblinternes/ego/programme.h"
#include "biblinternes/ego/tampon_objet.h"
#include "biblinternes/outils/fichier.hh"

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

	framebuffer = dls::ego::TamponFrame::cree_unique();
	framebuffer->attache();

	texture = dls::ego::Texture2D::cree_unique(static_cast<unsigned>(unit));
	texture->attache();
	texture->filtre_min_mag(GL_LINEAR, GL_LINEAR);
	texture->enveloppe(GL_CLAMP_TO_EDGE);
	texture->type(GL_FLOAT, format, static_cast<int>(internal_format));
	texture->remplie(nullptr, size);

	dls::ego::util::GPU_check_errors("Unable to create normals texture");

	renderbuffer = dls::ego::TamponRendu::cree();
	renderbuffer->lie();

	framebuffer->attache(*texture, GL_COLOR_ATTACHMENT0);

	dls::ego::util::GPU_check_errors("Unable to attach color buffer");
	dls::ego::util::GPU_check_framebuffer("Unable to create FBO");

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	renderbuffer->delie();
	texture->detache();
	framebuffer->detache();
}

void swap_surfaces(Slab *slab)
{
	std::swap(slab->ping, slab->pong);
}

void clear_surface(const Surface &s, float v)
{
	s.framebuffer->attache();
	glClearColor(v, v, v, v);
	glClear(GL_COLOR_BUFFER_BIT);
}

void create_obstacles(Surface &dest, int width, int height)
{
	dest.framebuffer->attache();
    glViewport(0, 0, width, height);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);

	dls::ego::TamponObjet buffer;

	dls::ego::Programme program;
	program.charge(dls::ego::Nuanceur::VERTEX, dls::contenu_fichier("shaders/simple.vert"));
	program.charge(dls::ego::Nuanceur::FRAGMENT, dls::contenu_fichier("shaders/render.frag"));
	program.cree_et_lie_programme();

	program.active();
	program.ajoute_attribut("vertex");

    const bool draw_border = true;
    if (draw_border) {
        #define T 0.9999f
        float positions[] = { -T, -T, T, -T, T,  T, -T, T };
        #undef T

		const GLushort indices[6] = { 0, 1, 2, 0, 2, 3 };

		buffer.attache();
		buffer.genere_tampon_sommet(positions, sizeof(float) * 8);
		buffer.genere_tampon_index(&indices[0], sizeof(GLushort) * 6);
		buffer.pointeur_attribut(static_cast<unsigned>(program["vertex"]), 2);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		buffer.detache();
    }

	dls::ego::util::GPU_check_framebuffer("");
	dls::ego::util::GPU_check_errors("");

    const bool draw_circle = true;
    if (draw_circle) {
        const int slices = 64;
        float positions[slices * 2 * 3];
        const float twopi = 8 * std::atan(1.0f);
        float theta = 0;
		const float dtheta = twopi / static_cast<float>(slices - 1);
        float *pPositions = &positions[0];

        for (int i = 0; i < slices; i++) {
            *pPositions++ = 0;
            *pPositions++ = 0;

			*pPositions++ = 0.25f * std::cos(theta) * static_cast<float>(height) / static_cast<float>(width);
            *pPositions++ = 0.25f * std::sin(theta);
            theta += dtheta;

			*pPositions++ = 0.25f * std::cos(theta) * static_cast<float>(height) / static_cast<float>(width);
            *pPositions++ = 0.25f * std::sin(theta);
        }

        GLsizeiptr size = sizeof(positions);

		buffer.attache();
		buffer.genere_tampon_sommet(positions, size);
		buffer.pointeur_attribut(static_cast<unsigned>(program["vertex"]), 2);

		glDrawArrays(GL_TRIANGLES, 0, slices * 3);

		buffer.detache();
    }

	dls::ego::util::GPU_check_errors("");

	program.desactive();
}
