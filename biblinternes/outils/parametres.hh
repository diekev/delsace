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

namespace otl {

/**
 * Foncions auxilliaires pour accumuler les valeurs des paramètres de fonctions
 * prenant des arguments variadiques. « donnees » doit être un pointeur vers un
 * tableau ayant suffisament de place pour tous les paramètres. Les types T0 et
 * T1 sont utilisés pour pouvoir avoir une conversion automatique entre le type
 * du paramètre et celui du tableau (p.e. int vers long).
 */

template <typename T0, typename T1>
void accumule(long idx, T0 *donnees, T1 d0)
{
	donnees[idx] = d0;
}

template <typename T0, typename T1, typename... Ts>
void accumule(long idx, T0 *donnees, T1 d0, Ts... reste)
{
	donnees[idx] = d0;
	accumule(idx + 1, donnees, reste...);
}

}  /* namespace otl */
