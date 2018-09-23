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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "atlas_texture.h"

#include <GL/glew.h>

AtlasTexture::AtlasTexture(int nombre)
	: m_nombre(nombre)
{
	glGenTextures(1, &m_bindcode);
}

AtlasTexture::~AtlasTexture()
{
	detruit(false);
}

void AtlasTexture::detruit(bool renouvel)
{
	if (glIsTexture(m_bindcode)) {
		glDeleteTextures(1, &m_bindcode);
	}

	if (renouvel) {
		glGenTextures(1, &m_bindcode);
	}
}

void AtlasTexture::attache() const
{
	glActiveTexture(GL_TEXTURE0 + m_nombre);
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_bindcode);
}

void AtlasTexture::detache() const
{
	glActiveTexture(GL_TEXTURE0 + m_nombre);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void AtlasTexture::typage(int type, int format, int format_interne)
{
	m_type = type;
	m_format = format;
	m_format_interne = format_interne;
}

void AtlasTexture::filtre_min_mag(int min, int mag) const
{
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, mag);
}

void AtlasTexture::rempli(const void *donnees, int taille[]) const
{
	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, m_format_interne, taille[0], taille[1], taille[2],
				 m_bordure, m_format, m_type, donnees);
}

void AtlasTexture::enveloppage(int envelope) const
{
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, envelope);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, envelope);
}

unsigned int AtlasTexture::nombre() const
{
	return m_nombre;
}
