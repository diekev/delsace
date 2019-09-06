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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/math/matrice.hh"
#include "biblinternes/phys/couleur.hh"

#include "contexte_execution.hh"

namespace lcc {

struct pile {
private:
	dls::tableau<float> m_donnees{};

public:
	int loge_donnees(int taille_loge)
	{
		auto d = m_donnees.taille();
		m_donnees.redimensionne(m_donnees.taille() + taille_loge);
		return static_cast<int>(d);
	}

	float *donnees()
	{
		return m_donnees.donnees();
	}

	template <typename T>
	void pousse(T const &v)
	{
		m_donnees.pousse(static_cast<float>(v));
	}

	long taille() const
	{
		return m_donnees.taille();
	}

	/* chargement données */

	inline int charge_entier(int &idx) const
	{
		return static_cast<int>(m_donnees[idx++]);
	}

	inline float charge_decimal(int &idx) const
	{
		return m_donnees[idx++];
	}

	inline dls::math::vec2f charge_vec2(int &idx) const
	{
		dls::math::vec2f v;
		v.x = charge_decimal(idx);
		v.y = charge_decimal(idx);
		return v;
	}

	inline dls::math::vec3f charge_vec3(int &idx) const
	{
		dls::math::vec3f v;
		v.x = charge_decimal(idx);
		v.y = charge_decimal(idx);
		v.z = charge_decimal(idx);
		return v;
	}

	inline dls::math::vec4f charge_vec4(int &idx) const
	{
		dls::math::vec4f v;
		v.x = charge_decimal(idx);
		v.y = charge_decimal(idx);
		v.z = charge_decimal(idx);
		v.w = charge_decimal(idx);
		return v;
	}

	inline dls::math::mat3x3f charge_mat3(int &idx) const
	{
		dls::math::mat3x3f v;

		for (auto i = 0ul; i < 3; ++i) {
			for (auto j = 0ul; j < 3; ++j) {
				v[i][j] = charge_decimal(idx);
			}
		}

		return v;
	}

	inline dls::math::mat4x4f charge_mat4(int &idx) const
	{
		dls::math::mat4x4f v;

		for (auto i = 0ul; i < 4; ++i) {
			for (auto j = 0ul; j < 4; ++j) {
				v[i][j] = charge_decimal(idx);
			}
		}

		return v;
	}

	inline dls::phys::couleur32 charge_couleur(int &idx) const
	{
		dls::phys::couleur32 c;
		c.r = charge_decimal(idx);
		c.v = charge_decimal(idx);
		c.b = charge_decimal(idx);
		c.a = charge_decimal(idx);
		return c;
	}

	/* charge depuis un pointeur */

	inline int charge_entier(int &idx, pile const &pointeur) const
	{
		auto ptr = pointeur.charge_entier(idx);
		return this->charge_entier(ptr);
	}

	inline float charge_decimal(int &idx, pile const &pointeur) const
	{
		auto ptr = pointeur.charge_entier(idx);
		return this->charge_decimal(ptr);
	}

	inline dls::math::vec2f charge_vec2(int &idx, pile const &pointeur) const
	{
		auto ptr = pointeur.charge_entier(idx);
		return this->charge_vec2(ptr);
	}

	inline dls::math::vec3f charge_vec3(int &idx, pile const &pointeur) const
	{
		auto ptr = pointeur.charge_entier(idx);
		return this->charge_vec3(ptr);
	}

	inline dls::math::vec4f charge_vec4(int &idx, pile const &pointeur) const
	{
		auto ptr = pointeur.charge_entier(idx);
		return this->charge_vec4(ptr);
	}

	inline dls::math::mat3x3f charge_mat3(int &idx, pile const &pointeur) const
	{
		auto ptr = pointeur.charge_entier(idx);
		return this->charge_mat3(ptr);
	}

	inline dls::math::mat4x4f charge_mat4(int &idx, pile const &pointeur) const
	{
		auto ptr = pointeur.charge_entier(idx);
		return this->charge_mat4(ptr);
	}

	inline dls::phys::couleur32 charge_couleur(int &idx, pile const &pointeur) const
	{
		auto ptr = pointeur.charge_entier(idx);
		return this->charge_couleur(ptr);
	}

	/* stockage données */

	inline void stocke(int &idx, int const &v)
	{
		m_donnees[idx++] = static_cast<float>(v);
	}

	inline void stocke(int &idx, float const &v)
	{
		m_donnees[idx++] = v;
	}

	inline void stocke(int &idx, dls::math::vec2f const &v)
	{
		for (auto i = 0ul; i < 2; ++i) {
			m_donnees[idx++] = v[i];
		}
	}

	inline void stocke(int &idx, dls::math::vec3f const &v)
	{
		for (auto i = 0ul; i < 3; ++i) {
			m_donnees[idx++] = v[i];
		}
	}

	inline void stocke(int &idx, dls::math::vec4f const &v)
	{
		for (auto i = 0ul; i < 4; ++i) {
			m_donnees[idx++] = v[i];
		}
	}

	inline void stocke(int &idx, dls::math::mat3x3f const &v)
	{
		for (auto i = 0ul; i < 3; ++i) {
			for (auto j = 0ul; j < 3; ++j) {
				m_donnees[idx++] = v[i][j];
			}
		}
	}

	inline void stocke(int &idx, dls::math::mat4x4f const &v)
	{
		for (auto i = 0ul; i < 4; ++i) {
			for (auto j = 0ul; j < 4; ++j) {
				m_donnees[idx++] = v[i][j];
			}
		}
	}

	inline void stocke(int &idx, dls::phys::couleur32 const &v)
	{
		m_donnees[idx++] = v.r;
		m_donnees[idx++] = v.v;
		m_donnees[idx++] = v.b;
		m_donnees[idx++] = v.a;
	}

	/* stocke pointeur */

	inline void stocke(int &idx, pile const &pointeur, int const &v)
	{
		auto ptr = pointeur.charge_entier(idx);
		stocke(ptr, v);
	}

	inline void stocke(int &idx, pile const &pointeur, float const &v)
	{
		auto ptr = pointeur.charge_entier(idx);
		stocke(ptr, v);
	}

	inline void stocke(int &idx, pile const &pointeur, dls::math::vec2f const &v)
	{
		auto ptr = pointeur.charge_entier(idx);
		stocke(ptr, v);
	}

	inline void stocke(int &idx, pile const &pointeur, dls::math::vec3f const &v)
	{
		auto ptr = pointeur.charge_entier(idx);
		stocke(ptr, v);
	}

	inline void stocke(int &idx, pile const &pointeur, dls::math::vec4f const &v)
	{
		auto ptr = pointeur.charge_entier(idx);
		stocke(ptr, v);
	}

	inline void stocke(int &idx, pile const &pointeur, dls::math::mat3x3f const &v)
	{
		auto ptr = pointeur.charge_entier(idx);
		stocke(ptr, v);
	}

	inline void stocke(int &idx, pile const &pointeur, dls::math::mat4x4f const &v)
	{
		auto ptr = pointeur.charge_entier(idx);
		stocke(ptr, v);
	}

	inline void stocke(int &idx, pile const &pointeur, dls::phys::couleur32 const &v)
	{
		auto ptr = pointeur.charge_entier(idx);
		stocke(ptr, v);
	}
};

void execute_pile(
		ctx_exec &contexte,
		pile &pile_donnees,
		pile const &insts,
		int graine);

}  /* namespace lcc */
