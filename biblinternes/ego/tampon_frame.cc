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

#include "tampon_frame.h"

#include <GL/glew.h>

namespace dls {
namespace ego {
EGO_VERSION_NAMESPACE_BEGIN

TamponFrame::TamponFrame()
    : m_id(0)
	, m_cible(GL_FRAMEBUFFER)
{
	glGenFramebuffers(1, &m_id);
}

TamponFrame::~TamponFrame()
{
	if (glIsFramebuffer(m_id)) {
		glDeleteFramebuffers(1, &m_id);
	}
}

TamponFrame::Ptr TamponFrame::cree_unique()
{
	return Ptr(new TamponFrame());
}

TamponFrame::SPtr TamponFrame::cree_partage()
{
	return SPtr(new TamponFrame());
}

void TamponFrame::attache() const noexcept
{
	glBindFramebuffer(m_cible, m_id);
}

void TamponFrame::detache()  const noexcept
{
	glBindFramebuffer(m_cible, 0);
}

void TamponFrame::attache(const Texture1D &tex, unsigned int attachment, int level)
{
	glFramebufferTexture1D(m_cible, attachment, GL_TEXTURE_1D, tex.code_attache(), level);
}

void TamponFrame::attache(const Texture2D &tex, unsigned int attachment, int level)
{
	glFramebufferTexture2D(m_cible, attachment, GL_TEXTURE_2D, tex.code_attache(), level);
}

void TamponFrame::attache(const Texture3D &tex, unsigned int attachment, int layer, int level)
{
	glFramebufferTexture3D(m_cible, attachment, GL_TEXTURE_3D, tex.code_attache(), level, layer);
}

void TamponFrame::attache(const TamponRendu &buffer, unsigned int attachment)
{
	glFramebufferRenderbuffer(m_cible, attachment, GL_RENDERBUFFER, buffer.code_liaison());
}

int TamponFrame::maximum_attaches_couleurs()
{
	return GL_MAX_COLOR_ATTACHMENTS;
}

EGO_VERSION_NAMESPACE_END
}  /* namespace ego */
}  /* namespace dls */
