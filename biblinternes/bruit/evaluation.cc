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

#include "outils.hh"

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
		case type::PERLIN:
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
		case type::VALEUR:
		{
			bruit::valeur::construit(params, graine);
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
	transforme_point(params, pos);

	auto v = 0.0f;

	switch (params.type_bruit) {
		case type::CELLULE:
		{
			v = bruit::cellule::evalue(params, pos);
			break;
		}
		case type::FOURIER:
		{
			v = bruit::fourrier::evalue(params, pos);
			break;
		}
		case type::PERLIN:
		{
			v = bruit::perlin::evalue(params, pos);
			break;
		}
		case type::SIMPLEX:
		{
			v = bruit::simplex::evalue(params, pos);
			break;
		}
		case type::ONDELETTE:
		{
			v = bruit::ondelette::evalue(params, pos);
			break;
		}
		case type::VALEUR:
		{
			v = bruit::valeur::evalue(params, pos);
			break;
		}
		case type::VORONOI_F1:
		{
			v = bruit::voronoi_f1::evalue(params, pos);
			break;
		}
		case type::VORONOI_F2:
		{
			v = bruit::voronoi_f2::evalue(params, pos);
			break;
		}
		case type::VORONOI_F3:
		{
			v = bruit::voronoi_f3::evalue(params, pos);
			break;
		}
		case type::VORONOI_F4:
		{
			v = bruit::voronoi_f4::evalue(params, pos);
			break;
		}
		case type::VORONOI_F1F2:
		{
			v = bruit::voronoi_f1f2::evalue(params, pos);
			break;
		}
		case type::VORONOI_CR:
		{
			v = bruit::voronoi_cr::evalue(params, pos);
			break;
		}
	}

	transforme_valeur(params, v);
	return v;
}

float evalue_derivee(const parametres &params, dls::math::vec3f pos, dls::math::vec3f &derivee)
{
	transforme_point(params, pos);

	auto v = 0.0f;

	switch (params.type_bruit) {
		case type::CELLULE:
		{
			v = bruit::cellule::evalue_derivee(params, pos, derivee);
			break;
		}
		case type::FOURIER:
		{
			v = bruit::fourrier::evalue_derivee(params, pos, derivee);
			break;
		}
		case type::PERLIN:
		{
			v = bruit::perlin::evalue_derivee(params, pos, derivee);
			break;
		}
		case type::SIMPLEX:
		{
			v = bruit::simplex::evalue_derivee(params, pos, derivee);
			break;
		}
		case type::ONDELETTE:
		{
			v = bruit::ondelette::evalue_derivee(params, pos, derivee);
			break;
		}
		case type::VALEUR:
		{
			v = bruit::valeur::evalue_derivee(params, pos, derivee);
			break;
		}
		case type::VORONOI_F1:
		{
			v = bruit::voronoi_f1::evalue_derivee(params, pos, derivee);
			break;
		}
		case type::VORONOI_F2:
		{
			v = bruit::voronoi_f2::evalue_derivee(params, pos, derivee);
			break;
		}
		case type::VORONOI_F3:
		{
			v = bruit::voronoi_f3::evalue_derivee(params, pos, derivee);
			break;
		}
		case type::VORONOI_F4:
		{
			v = bruit::voronoi_f4::evalue_derivee(params, pos, derivee);
			break;
		}
		case type::VORONOI_F1F2:
		{
			v = bruit::voronoi_f1f2::evalue_derivee(params, pos, derivee);
			break;
		}
		case type::VORONOI_CR:
		{
			v = bruit::voronoi_cr::evalue_derivee(params, pos, derivee);
			break;
		}
	}

	transforme_valeur(params, v);

	return v;
}

float evalue_turb(parametres const &params, const param_turbulence &params_turb, dls::math::vec3f pos)
{
	transforme_point(params, pos);

	auto v = 0.0f;

	switch (params.type_bruit) {
		case type::CELLULE:
		{
			v = bruit::turbulent<bruit::cellule>::evalue(params, params_turb, pos);
			break;
		}
		case type::FOURIER:
		{
			v = bruit::turbulent<bruit::fourrier>::evalue(params, params_turb, pos);
			break;
		}
		case type::PERLIN:
		{
			v = bruit::turbulent<bruit::perlin>::evalue(params, params_turb, pos);
			break;
		}
		case type::SIMPLEX:
		{
			v = bruit::turbulent<bruit::simplex>::evalue(params, params_turb, pos);
			break;
		}
		case type::ONDELETTE:
		{
			v = bruit::turbulent<bruit::ondelette>::evalue(params, params_turb, pos);
			break;
		}
		case type::VALEUR:
		{
			v = bruit::turbulent<bruit::valeur>::evalue(params, params_turb, pos);
			break;
		}
		case type::VORONOI_F1:
		{
			v = bruit::turbulent<bruit::voronoi_f1>::evalue(params, params_turb, pos);
			break;
		}
		case type::VORONOI_F2:
		{
			v = bruit::turbulent<bruit::voronoi_f2>::evalue(params, params_turb, pos);
			break;
		}
		case type::VORONOI_F3:
		{
			v = bruit::turbulent<bruit::voronoi_f3>::evalue(params, params_turb, pos);
			break;
		}
		case type::VORONOI_F4:
		{
			v = bruit::turbulent<bruit::voronoi_f4>::evalue(params, params_turb, pos);
			break;
		}
		case type::VORONOI_F1F2:
		{
			v = bruit::turbulent<bruit::voronoi_f1f2>::evalue(params, params_turb, pos);
			break;
		}
		case type::VORONOI_CR:
		{
			v = bruit::turbulent<bruit::voronoi_cr>::evalue(params, params_turb, pos);
			break;
		}
	}

	transforme_valeur(params, v);

	return v;
}

float evalue_turb_derivee(parametres const &params, const param_turbulence &params_turb, dls::math::vec3f pos, dls::math::vec3f &derivee)
{
	transforme_point(params, pos);

	auto v = 0.0f;

	switch (params.type_bruit) {
		case type::CELLULE:
		{
			v = bruit::turbulent<bruit::cellule>::evalue_derivee(params, params_turb, pos, derivee);
			break;
		}
		case type::FOURIER:
		{
			v = bruit::turbulent<bruit::fourrier>::evalue_derivee(params, params_turb, pos, derivee);
			break;
		}
		case type::PERLIN:
		{
			v = bruit::turbulent<bruit::perlin>::evalue_derivee(params, params_turb, pos, derivee);
			break;
		}
		case type::SIMPLEX:
		{
			v = bruit::turbulent<bruit::simplex>::evalue_derivee(params, params_turb, pos, derivee);
			break;
		}
		case type::ONDELETTE:
		{
			v = bruit::turbulent<bruit::ondelette>::evalue_derivee(params, params_turb, pos, derivee);
			break;
		}
		case type::VALEUR:
		{
			v = bruit::turbulent<bruit::valeur>::evalue_derivee(params, params_turb, pos, derivee);
			break;
		}
		case type::VORONOI_F1:
		{
			v = bruit::turbulent<bruit::voronoi_f1>::evalue_derivee(params, params_turb, pos, derivee);
			break;
		}
		case type::VORONOI_F2:
		{
			v = bruit::turbulent<bruit::voronoi_f2>::evalue_derivee(params, params_turb, pos, derivee);
			break;
		}
		case type::VORONOI_F3:
		{
			v = bruit::turbulent<bruit::voronoi_f3>::evalue_derivee(params, params_turb, pos, derivee);
			break;
		}
		case type::VORONOI_F4:
		{
			v = bruit::turbulent<bruit::voronoi_f4>::evalue_derivee(params, params_turb, pos, derivee);
			break;
		}
		case type::VORONOI_F1F2:
		{
			v = bruit::turbulent<bruit::voronoi_f1f2>::evalue_derivee(params, params_turb, pos, derivee);
			break;
		}
		case type::VORONOI_CR:
		{
			v = bruit::turbulent<bruit::voronoi_cr>::evalue_derivee(params, params_turb, pos, derivee);
			break;
		}
	}

	transforme_valeur(params, v);

	return v;
}

}  /* namespace bruit */
