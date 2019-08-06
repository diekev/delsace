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
private:
	dls::tableau<T> m_file{};

public:
	file() = default;

	bool est_vide() const
	{
		return m_file.est_vide();
	}

	long taille() const
	{
		return m_file.taille();
	}

	T &front()
	{
		return m_file.front();
	}

	T const &front() const
	{
		return m_file.front();
	}

	void enfile(T const &valeur)
	{
		m_file.pousse(valeur);
	}

	void enfile(dls::tableau<T> const &valeurs)
	{
		for (auto const &valeur : valeurs) {
			m_file.pousse(valeur);
		}
	}

	void defile()
	{
		m_file.pop_front();
	}

	dls::tableau<T> defile(long compte)
	{
		auto ret = dls::tableau<T>(compte);

		for (auto i = 0; i < compte; ++i) {
			ret.pousse(m_file.front());
			m_file.pop_front();
		}

		return ret;
	}
};

template <typename T, unsigned long N>
struct file_fixe {
private:
	T m_file[N];
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

	long taille() const noexcept
	{
		return m_taille;
	}

	T &front() noexcept
	{
		return m_file[0];
	}

	T const &front() const noexcept
	{
		return m_file[0];
	}

	void enfile(T const &valeur)
	{
		if (m_taille + 1 < N) {
			m_file[m_taille] = valeur;
			m_taille += 1;
		}
	}

	void defile() noexcept
	{
		for (auto i = 1l; i < m_taille; ++i) {
			m_file[i - 1] = m_file[i];
		}

		m_taille -= 1;
	}
};

template <typename T, typename Compare = std::less<T>>
struct file_priorite {
	using type_valeur = T;
	using type_reference = T&;
	using type_reference_const = T const&;
	using type_taille = long;

private:
	std::priority_queue<T, std::vector<T, memoire::logeuse_guardee<T>>, Compare> m_file{};

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

	void defile()
	{
		m_file.pop();
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
