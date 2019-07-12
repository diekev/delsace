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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "tampon_image.hh"

#include <cassert>
#include <GL/glew.h>

namespace dls {
namespace ego {
EGO_VERSION_NAMESPACE_BEGIN

TamponRendu::TamponRendu()
    : m_id(0)
{
	glGenRenderbuffers(1, &m_id);
}

TamponRendu::~TamponRendu()
{
	if (glIsRenderbuffer(m_id)) {
		glDeleteRenderbuffers(1, &m_id);
	}
}

TamponRendu::Ptr TamponRendu::cree()
{
	return Ptr(new TamponRendu());
}

TamponRendu::SPtr TamponRendu::cree_partage()
{
	return SPtr(new TamponRendu());
}

void TamponRendu::lie() const noexcept
{
	glBindRenderbuffer(GL_RENDERBUFFER, m_id);
}

void TamponRendu::delie() const noexcept
{
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void TamponRendu::alloue(const int largeur, const int hauteur, const unsigned int format_interne) const noexcept
{
	glRenderbufferStorage(GL_RENDERBUFFER, format_interne, largeur, hauteur);
}

void TamponRendu::alloue(const int largeur, const int hauteur, const unsigned int format_interne, int echantillons) const noexcept
{
	assert(echantillons <= GL_MAX_SAMPLES);

	glRenderbufferStorageMultisample(GL_RENDERBUFFER, echantillons, format_interne, largeur, hauteur);
}

unsigned int TamponRendu::code_liaison() const noexcept
{
	return m_id;
}

EGO_VERSION_NAMESPACE_END
}  /* namespace ego */
}  /* namespace dls */
