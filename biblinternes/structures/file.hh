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

namespace dls {

/* À FAIRE : priority_queue */

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

	void defile()
	{
		m_file.pop_front();
	}
};

}  /* namespace dls */
