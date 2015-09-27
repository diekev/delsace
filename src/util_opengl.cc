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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include <GL/glew.h>
#include <iostream>

#include "util_opengl.h"

void gl_check_errors()
{
	GLenum error = glGetError();

	if (error == GL_NO_ERROR) {
		return;
	}

	switch (error) {
		case GL_INVALID_ENUM:
			std::cerr << "GL Invalid Enum Error\n";
			break;
		case GL_INVALID_VALUE:
			std::cerr << "GL Invalid Value Error\n";
			break;
		case GL_INVALID_OPERATION:
			std::cerr << "GL Invalid Operation Error\n";
			break;
		case GL_OUT_OF_MEMORY:
			std::cerr << "GL Invalid Out of Memory Error\n";
			break;
	}
}
