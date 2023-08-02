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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

namespace dls {

template <typename T1, typename T2>
struct paire {
	T1 premier{};
	T2 second{};

	paire() = default;

	paire(T1 const &p, T2 const &s)
		: premier(p)
		, second(s)
	{}

	paire(paire const &) = default;
	paire &operator=(paire const &) = default;
};

template <typename T0, typename T1, typename T2>
struct triplet {
	T0 t0;
	T1 t1;
	T2 t2;
};

}
