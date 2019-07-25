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

#include "corps/volume.hh"

enum {
	TypeNone     = 0,
	TypeFluid    = 1,
	TypeObstacle = 2,
	TypeVide     = 4,
	TypeInflow   = 8,
	TypeOutflow  = 16,
	TypeOpen     = 32,
	TypeStick    = 64,
	// internal use only, for fast marching
	TypeReserved = 256,
	// 2^10 - 2^14 reserved for moving obstacles
};

inline auto est_fluide(Grille<int> const &flags, long idx)
{
	return (flags.valeur(idx) & TypeFluid) != 0;
}

inline auto est_fluide(Grille<int> const &flags, size_t i, size_t j, size_t k)
{
	return est_fluide(flags, flags.calcul_index(i, j, k));
}

inline auto est_vide(Grille<int> const &flags, long idx)
{
	return (flags.valeur(idx) & TypeVide) != 0;
}

inline auto est_vide(Grille<int> const &flags, size_t i, size_t j, size_t k)
{
	return est_vide(flags, flags.calcul_index(i, j, k));
}

inline auto est_outflow(Grille<int> const &flags, long idx)
{
	return (flags.valeur(idx) & TypeOutflow) != 0;
}

inline auto est_outflow(Grille<int> const &flags, size_t i, size_t j, size_t k)
{
	return est_vide(flags, flags.calcul_index(i, j, k));
}
