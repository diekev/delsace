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

#include "evaluation.hh"

namespace bruit {

void construit(type type, parametres &params, int graine)
{
	params.type_bruit = type;

	switch (type) {
		case type::CELLULE:
		{
			bruit::cellule::construit(params, graine);
			break;
		}
		case type::FOURIER:
		{
			bruit::fourrier::construit(params, graine);
			break;
		}
		case type::PERLIN_LONG:
		{
			bruit::perlin::construit(params, graine);
			break;
		}
		case type::SIMPLEX:
		{
			bruit::simplex::construit(params, graine);
			break;
		}
		case type::ONDELETTE:
		{
			bruit::ondelette::construit(params, graine);
			break;
		}
		case type::VORONOI_F1:
		{
			bruit::voronoi_f1::construit(params, graine);
			break;
		}
		case type::VORONOI_F2:
		{
			bruit::voronoi_f2::construit(params, graine);
			break;
		}
		case type::VORONOI_F3:
		{
			bruit::voronoi_f3::construit(params, graine);
			break;
		}
		case type::VORONOI_F4:
		{
			bruit::voronoi_f4::construit(params, graine);
			break;
		}
		case type::VORONOI_F1F2:
		{
			bruit::voronoi_f1f2::construit(params, graine);
			break;
		}
		case type::VORONOI_CR:
		{
			bruit::voronoi_cr::construit(params, graine);
			break;
		}
	}
}

float evalue(const parametres &params, dls::math::vec3f pos)
{
	switch (params.type_bruit) {
		case type::CELLULE:
		{
			return bruit::cellule::evalue(params, pos);
		}
		case type::FOURIER:
		{
			return bruit::fourrier::evalue(params, pos);
		}
		case type::PERLIN_LONG:
		{
			return bruit::perlin::evalue(params, pos);
		}
		case type::SIMPLEX:
		{
			return bruit::simplex::evalue(params, pos);
		}
		case type::ONDELETTE:
		{
			return bruit::ondelette::evalue(params, pos);
		}
		case type::VORONOI_F1:
		{
			return bruit::voronoi_f1::evalue(params, pos);
		}
		case type::VORONOI_F2:
		{
			return bruit::voronoi_f2::evalue(params, pos);
		}
		case type::VORONOI_F3:
		{
			return bruit::voronoi_f3::evalue(params, pos);
		}
		case type::VORONOI_F4:
		{
			return bruit::voronoi_f4::evalue(params, pos);
		}
		case type::VORONOI_F1F2:
		{
			return bruit::voronoi_f1f2::evalue(params, pos);
		}
		case type::VORONOI_CR:
		{
			return bruit::voronoi_cr::evalue(params, pos);
		}
	}

	return 0.0f;
}

void construit_turb(type type, int graine, parametres &params, const param_turbulence &params_turb)
{
	params.type_bruit = type;

	switch (type) {
		case type::CELLULE:
		{
			bruit::turbulent<bruit::cellule>::construit(graine, params, params_turb);
			break;
		}
		case type::FOURIER:
		{
			bruit::turbulent<bruit::fourrier>::construit(graine, params, params_turb);
			break;
		}
		case type::PERLIN_LONG:
		{
			bruit::turbulent<bruit::perlin>::construit(graine, params, params_turb);
			break;
		}
		case type::SIMPLEX:
		{
			bruit::turbulent<bruit::simplex>::construit(graine, params, params_turb);
			break;
		}
		case type::ONDELETTE:
		{
			bruit::turbulent<bruit::ondelette>::construit(graine, params, params_turb);
			break;
		}
		case type::VORONOI_F1:
		{
			bruit::turbulent<bruit::voronoi_f1>::construit(graine, params, params_turb);
			break;
		}
		case type::VORONOI_F2:
		{
			bruit::turbulent<bruit::voronoi_f2>::construit(graine, params, params_turb);
			break;
		}
		case type::VORONOI_F3:
		{
			bruit::turbulent<bruit::voronoi_f3>::construit(graine, params, params_turb);
			break;
		}
		case type::VORONOI_F4:
		{
			bruit::turbulent<bruit::voronoi_f4>::construit(graine, params, params_turb);
			break;
		}
		case type::VORONOI_F1F2:
		{
			bruit::turbulent<bruit::voronoi_f1f2>::construit(graine, params, params_turb);
			break;
		}
		case type::VORONOI_CR:
		{
			bruit::turbulent<bruit::voronoi_cr>::construit(graine, params, params_turb);
			break;
		}
	}
}

float evalue_turb(parametres &params, const param_turbulence &params_turb, dls::math::vec3f pos)
{
	switch (params.type_bruit) {
		case type::CELLULE:
		{
			return bruit::turbulent<bruit::cellule>::evalue(params, params_turb, pos);
		}
		case type::FOURIER:
		{
			return bruit::turbulent<bruit::fourrier>::evalue(params, params_turb, pos);
		}
		case type::PERLIN_LONG:
		{
			return bruit::turbulent<bruit::perlin>::evalue(params, params_turb, pos);
		}
		case type::SIMPLEX:
		{
			return bruit::turbulent<bruit::simplex>::evalue(params, params_turb, pos);
		}
		case type::ONDELETTE:
		{
			return bruit::turbulent<bruit::ondelette>::evalue(params, params_turb, pos);
		}
		case type::VORONOI_F1:
		{
			return bruit::turbulent<bruit::voronoi_f1>::evalue(params, params_turb, pos);
		}
		case type::VORONOI_F2:
		{
			return bruit::turbulent<bruit::voronoi_f2>::evalue(params, params_turb, pos);
		}
		case type::VORONOI_F3:
		{
			return bruit::turbulent<bruit::voronoi_f3>::evalue(params, params_turb, pos);
		}
		case type::VORONOI_F4:
		{
			return bruit::turbulent<bruit::voronoi_f4>::evalue(params, params_turb, pos);
		}
		case type::VORONOI_F1F2:
		{
			return bruit::turbulent<bruit::voronoi_f1f2>::evalue(params, params_turb, pos);
		}
		case type::VORONOI_CR:
		{
			return bruit::turbulent<bruit::voronoi_cr>::evalue(params, params_turb, pos);
		}
	}

	return 0.0f;
}

}  /* namespace bruit */
