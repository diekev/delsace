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

#include <GL/glew.h>
#include <memory>

#include "version.h"

namespace dls {
namespace ego {
EGO_VERSION_NAMESPACE_BEGIN

/* À FAIRE:
 * - Make regular type:
 *   - Default constructible
 *   - Copyable
 *   - Assignable
 *   - Movable
 *   - Destructible
 *   - Equality-comparable
 */

template <unsigned int CIBLE>
class Texture {
	unsigned int m_code_attache = 0;
	int m_format_interne = 0;
	int m_bordure = 0;
	unsigned int m_unite = 0;
	unsigned int m_format = 0;
	unsigned int m_type = 0;

public:
	Texture() = default;
	explicit Texture(unsigned int texture);
	Texture(Texture &&texture);

	~Texture();

	Texture &operator=(Texture &&texture);

	using Ptr = std::unique_ptr<Texture>;
	using SPtr = std::shared_ptr<Texture>;

	/**
	 * @brief Create and return a unique pointer to this object.
	 */
	static Ptr cree_unique(unsigned int texture);

	/**
	 * @brief Create and return a shared pointer to this object.
	 */
	static SPtr cree_partage(unsigned int texture);

	void deloge(bool renew);
	void attache() const;
	void detache() const;

	void type(unsigned int type, unsigned int format, int internal_format);
	void filtre_min_mag(int min, int mag) const;
	void enveloppe(int wrap) const;
	void genere_mip_map(int base, int max) const;

	void remplie(const void *data, int *size) const;
	void sous_image(const void *data, int *size, int *offset) const;

	unsigned int code_attache() const;
	unsigned int unite() const;
	void unite(unsigned int texture_unit);

private:
	void swap(Texture &&texture)
	{
		using std::swap;

		swap(m_code_attache, texture.m_code_attache);
		swap(m_format_interne, texture.m_format_interne);
		swap(m_bordure, texture.m_bordure);
		swap(m_unite, texture.m_unite);
		swap(m_format, texture.m_format);
		swap(m_type, texture.m_type);
	}
};

#include "texture.tcc"

using Texture1D = Texture<GL_TEXTURE_1D>;
using Texture2D = Texture<GL_TEXTURE_2D>;
using Texture3D = Texture<GL_TEXTURE_3D>;

EGO_VERSION_NAMESPACE_END
}  /* namespace ego */
}  /* namespace dls */
