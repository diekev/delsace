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

#pragma once

#include "tampon_rendu.h"
#include "texture.h"

namespace dls {
namespace ego {
EGO_VERSION_NAMESPACE_BEGIN

class TamponFrame {
	unsigned int m_id;
	unsigned int m_cible;

public:
	TamponFrame();
	~TamponFrame();

	using Ptr = std::unique_ptr<TamponFrame>;
	using SPtr = std::shared_ptr<TamponFrame>;

	/**
	 * @brief Create and return a unique pointer to this object.
	 */
	static Ptr cree_unique();

	/**
	 * @brief Create and return a shared pointer to this object.
	 */
	static SPtr cree_partage();

	/**
	 * @brief bind this frame buffer to the current context
	 */
	void attache() const noexcept;

	/**
	 * @brief unbind this frame buffer from the current context
	 */
	void detache() const noexcept;

	/**
	 * @brief attach a 1D texture to this frame buffer
	 *
	 * @param tex        the texture to be attached
	 * @param attachment the attachment point of the texture
	 * @param level      the mipmap level of the texture to attach
	 */
	void attache(const Texture1D &tex, unsigned int attachment, int level = 0);

	/**
	 * @brief attach a 2D texture to this frame buffer
	 *
	 * @param tex        the texture to be attached
	 * @param attachment the attachment point of the texture
	 * @param level      the mipmap level of the texture to attach
	 */
	void attache(const Texture2D &tex, unsigned int attachment, int level = 0);

	/**
	 * @brief attach a 3D texture to this frame buffer
	 *
	 * @param tex        the texture to be attached
	 * @param attachment the attachment point of the texture
	 * @param layer      the Z-index of the 3D texture to attach
	 * @param level      the mipmap level of the texture to attach
	 */
	void attache(const Texture3D &tex, unsigned int attachment, int layer, int level = 0);

	/**
	 * @brief attach a render buffer to this frame buffer
	 *
	 * @param render_buffer the render buffer to be attached
	 * @param attachment    the attachment point of the frame buffer
	 */
	void attache(const TamponRendu &render_buffer, unsigned int attachment);

	/**
	 * @brief return the maximum number of color attachments available
	 */
	static int maximum_attaches_couleurs();
};

EGO_VERSION_NAMESPACE_END
}  /* namespace ego */
}  /* namespace dls */
