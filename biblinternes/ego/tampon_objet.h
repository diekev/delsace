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

#include <functional>
#include <memory>
#include <vector>

#include "version.h"

namespace dls {
namespace ego {
EGO_VERSION_NAMESPACE_BEGIN

/* TODO:
 * - Make regular type:
 *   - Copyable
 *   - Assignable
 *   - Movable
 *   - Destructible
 *   - Equality-comparable
 */

class TamponObjet {
	unsigned int m_objet_tableau_sommet = 0;
	unsigned int m_tampon_sommet = 0;
	unsigned int m_tampon_index = 0;
	unsigned int m_tampon_normal = 0;
	std::vector<unsigned int> m_tampons_extra = {};

public:
	TamponObjet() noexcept;
	TamponObjet(TamponObjet &&buffer) noexcept;
	~TamponObjet() noexcept;

	TamponObjet &operator=(TamponObjet &&buffer) noexcept;

	using Ptr = std::unique_ptr<TamponObjet>;
	using SPtr = std::shared_ptr<TamponObjet>;

	/**
	 * @brief Retourne un unique_ptr vers une instance de cet objet.
	 */
	static Ptr cree_unique() noexcept;

	/**
	 * @brief Retourne un shared_ptr vers une instance de cet objet.
	 */
	static SPtr cree_partage() noexcept;

	void attache() const noexcept;
	void detache() const noexcept;

	void pointeur_attribut(unsigned int index, int size) const noexcept;

	void genere_tampon_sommet(void const *vertices, long const size);
	void genere_tampon_index(void const *indices, long const size);
	void genere_tampon_normal(void const *colors, long const size);
	void genere_tampon_extra(void const *data, long const size);

	void ajourne_tampon_index(void const *indices, long const size) const noexcept;
	void ajourne_tampon_sommet(void const *vertices, long const size) const noexcept;

	unsigned int objet_tableau_sommet() const;
};

bool operator==(TamponObjet const &b1, TamponObjet const &b2) noexcept;

EGO_VERSION_NAMESPACE_END
}  /* namespace ego */
}  /* namespace dls */

namespace std {

template <>
struct hash<dls::ego::TamponObjet> {
	size_t operator()(const dls::ego::TamponObjet &buffer) const noexcept
	{
		return static_cast<size_t>(buffer.objet_tableau_sommet());
	}
};

}  /* namespace std */
