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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <cmath>

static constexpr auto INFINITE  = std::numeric_limits<double>::max();
static constexpr auto INFINITEF = std::numeric_limits<float>::max();

static constexpr auto PI     = 3.14159265358979323846;
static constexpr auto PI_INV = 1 / PI;

static constexpr auto POIDS_DEG_RAD = PI / 180.0;
static constexpr auto POIDS_RAD_DEG = 180.0 / PI;

static constexpr auto TAU     = 2 * PI;
static constexpr auto TAU_INV = 1 / TAU;

static constexpr auto PHI     = 1.6180339887498948482;
static constexpr auto PHI_INV = 0.6180339887498948482;
