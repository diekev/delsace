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

#include "biblinternes/structures/tableau.hh"

#include "base_grille.hh"

namespace wlk {

template <typename T, template<typename> typename type_vec>
struct grille_dense : public base_grille<type_vec> {
	using type_desc = typename base_grille<type_vec>::type_desc;

protected:
	dls::tableau<T> m_donnees = {};

	T m_arriere_plan = T(0);

public:
	grille_dense() = default;

	grille_dense(grille_dense const &) = default;
	grille_dense &operator=(grille_dense const &) = default;

	grille_dense(type_desc const &descr, T arriere_plan = T(0))
		: base_grille<type_vec>(descr)
		, m_arriere_plan(arriere_plan)
	{
		this->m_desc.type_donnees = selectrice_type_grille<T>::type;
		m_donnees.redimensionne(this->m_nombre_elements, m_arriere_plan);
	}

	T &valeur(long index)
	{
		if (index < 0 || index >= this->m_nombre_elements) {
			return m_arriere_plan;
		}

		return m_donnees[index];
	}

	T const &valeur(long index) const
	{
		if (index < 0 || index >= this->m_nombre_elements) {
			return m_arriere_plan;
		}

		return m_donnees[index];
	}

	T const &valeur(type_vec<int> const &co) const
	{
		if (this->hors_des_limites(co)) {
			return m_arriere_plan;
		}

		return m_donnees[this->calcul_index(co)];
	}

	/* XXX ne fais pas de vérification de limites */
	T &valeur(type_vec<int> const &co)
	{
		if (this->hors_des_limites(co)) {
			return m_arriere_plan;
		}

		return m_donnees[this->calcul_index(co)];
	}

	void valeur(long index, T v)
	{
		if (index >= this->m_nombre_elements) {
			return;
		}

		m_donnees[index] = v;
	}

	void valeur(type_vec<int> const &co, T v)
	{
		if (this->hors_des_limites(co)) {
			return;
		}

		m_donnees[this->calcul_index(co)] = v;
	}

	void copie_donnees(const grille_dense<T, type_vec> &autre)
	{
		for (auto i = 0; i < this->m_nombre_elements; ++i) {
			m_donnees[i] = autre.m_donnees[i];
		}
	}

	void arriere_plan(const T &v)
	{
		m_arriere_plan = v;
	}

	void *donnees()
	{
		return m_donnees.donnees();
	}

	void const *donnees() const
	{
		return m_donnees.donnees();
	}

	long taille_octet() const
	{
		return this->m_nombre_elements * static_cast<long>(sizeof(T));
	}

	base_grille<type_vec> *copie() const override
	{
		auto grille = memoire::loge<grille_dense<T, type_vec>>("grille_dense", this->desc());
		grille->m_arriere_plan = this->m_arriere_plan;
		grille->m_donnees = this->m_donnees;

		return grille;
	}

	void permute(grille_dense<T, type_vec> &autre)
	{
		std::swap(this->m_desc.etendue, autre.m_desc.etendue);
		std::swap(this->m_desc.resolution, autre.m_desc.resolution);
		std::swap(this->m_desc.fenetre_donnees, autre.m_desc.fenetre_donnees);
		std::swap(this->m_desc.taille_pixel, autre.m_desc.taille_pixel);

		m_donnees.permute(autre.m_donnees);

		std::swap(m_arriere_plan, autre.m_arriere_plan);
		std::swap(this->m_nombre_elements, autre.m_nombre_elements);
	}
};

template <typename T>
using grille_dense_2d = grille_dense<T, dls::math::vec2>;

template <typename T>
using grille_dense_3d = grille_dense<T, dls::math::vec3>;

/* ************************************************************************** */

class GrilleMAC : public grille_dense_3d<dls::math::vec3f> {
public:
	GrilleMAC(desc_grille_3d const &descr)
		: grille_dense_3d<dls::math::vec3f>(descr)
	{}

	dls::math::vec3f valeur_centree(dls::math::vec3i const &pos) const
	{
		if (hors_des_limites(pos)) {
			return this->m_arriere_plan;
		}

		auto idx = calcul_index(pos);

		auto vc = this->valeur(idx);
		auto vx = this->valeur(idx + 1);
		auto vy = this->valeur(idx + m_desc.resolution.x);
		auto vz = this->valeur(idx + m_desc.resolution.x * m_desc.resolution.y);

		return dls::math::vec3f(
					0.5f * (vc.x + vx.x),
					0.5f * (vc.y + vy.y),
					0.5f * (vc.z + vz.z));
	}

	dls::math::vec3f valeur_centree(int i, int j, int k) const
	{
		return valeur_centree(dls::math::vec3i(i, j, k));
	}
};

}  /* namespace wlk */
