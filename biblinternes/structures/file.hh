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

#include <queue>

#include "biblinternes/structures/tableau.hh"

namespace dls {

template <typename T>
struct file {
	using type_valeur = T;
	using type_reference = T&;
	using type_reference_const = T const&;
	using type_taille = long;

private:
	tableau<type_valeur> m_file{};

public:
	file() = default;

	bool est_vide() const
	{
		return m_file.est_vide();
	}

	type_taille taille() const
	{
		return m_file.taille();
	}

	type_reference front()
	{
		return m_file.front();
	}

	type_reference_const front() const
	{
		return m_file.front();
	}

	void enfile(type_reference_const valeur)
	{
		m_file.pousse(valeur);
	}

	void enfile(tableau<type_valeur> const &valeurs)
	{
		for (auto const &valeur : valeurs) {
			m_file.pousse(valeur);
		}
	}

	type_valeur defile()
	{
		auto t = front();
		m_file.pop_front();
		return t;
	}

	tableau<type_valeur> defile(long compte)
	{
		auto ret = tableau<type_valeur>(compte);

		for (auto i = 0; i < compte; ++i) {
			ret.pousse(m_file.front());
			m_file.pop_front();
		}

		return ret;
	}
};

template <typename T, unsigned long N>
struct file_fixe {
	using type_valeur = T;
	using type_reference = T&;
	using type_reference_const = T const&;
	using type_taille = long;

private:
	type_valeur m_file[N];
	long m_taille = 0;

public:
	file_fixe() = default;

	bool est_vide() const noexcept
	{
		return m_taille == 0;
	}

	bool est_pleine() const noexcept
	{
		return m_taille == N;
	}

	type_taille taille() const noexcept
	{
		return m_taille;
	}

	type_reference front() noexcept
	{
		return m_file[0];
	}

	type_reference_const front() const noexcept
	{
		return m_file[0];
	}

	void enfile(type_reference_const valeur)
	{
		if (m_taille + 1 < N) {
			m_file[m_taille] = valeur;
			m_taille += 1;
		}
	}

	type_valeur defile() noexcept
	{
		auto t = front();

		for (auto i = 1l; i < m_taille; ++i) {
			m_file[i - 1] = m_file[i];
		}

		m_taille -= 1;

		return t;
	}
};

template <typename T, typename Compare = std::less<T>>
struct file_priorite {
	using type_valeur = T;
	using type_reference = T&;
	using type_reference_const = T const&;
	using type_taille = long;

private:
	using type_logeuse = memoire::logeuse_guardee<type_valeur>;
	using type_conteneur = std::vector<type_valeur, type_logeuse>;
	using type_queue = std::priority_queue<type_valeur, type_conteneur, Compare>;

	type_queue m_file{};

public:
	file_priorite() = default;
	~file_priorite() = default;

	bool est_vide() const
	{
		return m_file.empty();
	}

	type_taille taille() const
	{
		return static_cast<long>(m_file.size());
	}

	type_reference_const haut() const
	{
		return m_file.top();
	}

	type_valeur defile()
	{
		auto t = haut();
		m_file.pop();
		return t;
	}

	void enfile(type_valeur const &valeur)
	{
		m_file.push(valeur);
	}

	void enfile(type_valeur &&valeur)
	{
		m_file.push(valeur);
	}

	template<typename... Args>
	void emplace(Args &&... args)
	{
		m_file.emplace(std::forward<Args...>(args...));
	}

	void echange(file_priorite &autre)
	{
		m_file.swap(autre);
	}
};

}  /* namespace dls */
