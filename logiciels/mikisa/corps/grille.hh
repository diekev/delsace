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

#include "biblinternes/math/limites.hh"
#include "biblinternes/structures/tableau.hh"

namespace dls::math {

template <typename TypeRet, typename T, int O, int... Ns>
[[nodiscard]] inline auto converti_type(vecteur<O, T, Ns...> const &vec)
{
	auto tmp = vecteur<O, TypeRet, Ns...>();
	((tmp[Ns] = static_cast<TypeRet>(vec[Ns])), ...);
	return tmp;
}

template <int O, typename T, int... Ns>
[[nodiscard]] inline auto produit_interne(vecteur<O, T, Ns...> const &vec)
{
	return (vec[Ns] * ...);
}

[[nodiscard]] inline auto calcul_index(vec2i const &co, vec2i const &res)
{
	return static_cast<long>(co.x + co.y * res.x);
}

[[nodiscard]] inline auto calcul_index(vec3i const &co, vec3i const &res)
{
	return static_cast<long>(co.x + (co.y + co.z * res.y) * res.x);
}

}  /* namespace dls::math */

enum class type_grille {
	Z8,
	Z32,
	R32,
	R64,
	VEC2,
	VEC3,
};

template <template<typename> typename type_vec>
struct description_grille {
	limites<type_vec<float>> etendue{};
	limites<type_vec<float>> fenetre_donnees{};
	type_vec<int> resolution{};
	double taille_pixel = 0.0;
	type_grille type_donnees{};
	char pad[4];
};

template <template<typename> typename type_vec>
struct base_grille {
	using type_desc = description_grille<type_vec>;

protected:
	description_grille<type_vec> m_desc{};
	long m_nombre_elements = 0;

	bool hors_des_limites(type_vec<int> const &co) const
	{
		return dls::math::hors_limites(co, type_vec<int>(0), m_desc.resolution - type_vec<int>(1));
	}

public:
	base_grille(type_desc const &descr)
		: m_desc(descr)
	{
		auto taille = dls::math::converti_type<double>(m_desc.etendue.taille());
		taille /= m_desc.taille_pixel;

		m_desc.resolution = dls::math::converti_type<int>(taille);

		auto res_long = dls::math::converti_type<long>(m_desc.resolution);
		m_nombre_elements = dls::math::produit_interne(res_long);
	}

	virtual ~base_grille() = default;

	/* entreface */

	virtual base_grille *copie() const = 0;

	/* accès propriétés */

	type_desc const &desc() const
	{
		return m_desc;
	}

	long nombre_elements() const
	{
		return m_nombre_elements;
	}

	long calcul_index(type_vec<int> const &co) const
	{
		return dls::math::calcul_index(co, m_desc.resolution);
	}

	/* conversion coordonnées */

	type_vec<float> index_vers_unit(const type_vec<int> &vsp) const
	{
		auto p = dls::math::discret_vers_continu<float>(vsp);
		return index_vers_unit(p);
	}

	type_vec<float> index_vers_unit(const type_vec<float> &vsp) const
	{
		auto resf = dls::math::converti_type<float>(this->m_desc.resolution);
		return vsp / resf;
	}

	type_vec<float> index_vers_monde(const type_vec<int> &isp) const
	{
		auto const dim = m_desc.etendue.taille();
		auto const min = m_desc.etendue.min;
		auto const cont = dls::math::discret_vers_continu<float>(isp);
		auto resf = dls::math::converti_type<float>(this->m_desc.resolution);
		return cont / resf * dim + min;
	}

	type_vec<float> unit_vers_monde(const type_vec<float> &vsp) const
	{
		return vsp * m_desc.etendue.taille() + m_desc.etendue.min;
	}

	type_vec<float> monde_vers_unit(const type_vec<float> &wsp) const
	{
		return (wsp - m_desc.etendue.min) / m_desc.etendue.taille();
	}

	type_vec<float> monde_vers_continu(const type_vec<float> &wsp) const
	{
		auto resf = dls::math::converti_type<float>(this->m_desc.resolution);
		return monde_vers_unit(wsp) * resf;
	}

	type_vec<int> monde_vers_index(const type_vec<float> &wsp) const
	{
		auto idx = monde_vers_continu(wsp);
		return dls::math::converti_type<int>(idx);
	}

	type_vec<float> continu_vers_monde(const type_vec<float> &csp) const
	{
		return (csp / m_desc.etendue.taille()) + m_desc.etendue.min;
	}
};

template <typename T, template<typename> typename type_vec>
struct grille_dense : public base_grille<type_vec> {
	using type_desc = typename base_grille<type_vec>::type_desc;

private:
	dls::tableau<T> m_donnees = {};

	T m_arriere_plan = T(0);

public:
	grille_dense() = default;

	grille_dense(type_desc const &descr, T arriere_plan = T(0))
		: base_grille<type_vec>(descr)
		, m_arriere_plan(arriere_plan)
	{
		m_donnees.redimensionne(this->m_nombre_elements, m_arriere_plan);
	}

	T &valeur(long index)
	{
		if (index >= this->m_nombre_elements) {
			return m_arriere_plan;
		}

		return m_donnees[index];
	}

	T const &valeur(long index) const
	{
		if (index >= this->m_nombre_elements) {
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

	void const *donnees() const
	{
		return m_donnees.donnees();
	}

	long taille_octet() const
	{
		return this->m_nombre_elements * sizeof(T);
	}

	base_grille<type_vec> *copie() const override
	{
		auto grille = memoire::loge<grille_dense<T, type_vec>>("grille_dense", this->desc());
		grille->m_arriere_plan = this->m_arriere_plan;
		grille->m_donnees = this->m_donnees;

		return grille;
	}

	void echange(grille_dense<T, type_vec> &autre)
	{
		std::swap(this->m_desc.etendue, autre.m_desc.etendue);
		std::swap(this->m_desc.resolution, autre.m_desc.resolution);
		std::swap(this->m_desc.fenetre_donnees, autre.m_desc.fenetre_donnees);
		std::swap(this->m_desc.taille_pixel, autre.m_desc.taille_pixel);

		m_donnees.echange(autre.m_donnees);

		std::swap(m_arriere_plan, autre.m_arriere_plan);
		std::swap(this->m_nombre_elements, autre.m_nombre_elements);
	}
};

using desc_grille_2d = description_grille<dls::math::vec2>;
using base_grille_2d = base_grille<dls::math::vec2>;

template <typename T>
using grille_dense_2d = grille_dense<T, dls::math::vec2>;

void deloge_grille(base_grille_2d *&tampon);
