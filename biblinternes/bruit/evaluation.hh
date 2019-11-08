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

#include "blanc.hh"
#include "cellule.hh"
#include "flux.hh"
#include "fourier.hh"
#include "ondelette.hh"
#include "perlin.hh"
#include "simplex.hh"
#include "turbulent.hh"
#include "voronoi.hh"

namespace bruit {

void construit(type type, bruit::parametres &params, int graine);

float evalue(bruit::parametres const &params, dls::math::vec3f pos);

float evalue_turb(bruit::parametres const &params, bruit::param_turbulence const &params_turb, dls::math::vec3f pos);

float evalue_derivee(bruit::parametres const &params, dls::math::vec3f pos, dls::math::vec3f &derivee);

float evalue_turb_derivee(bruit::parametres const &params, bruit::param_turbulence const &params_turb, dls::math::vec3f pos, dls::math::vec3f &derivee);

}  /* namespace bruit */
