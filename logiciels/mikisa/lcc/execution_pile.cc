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

#include "execution_pile.hh"

#include "biblinternes/bruit/outils.hh"
#include "biblinternes/bruit/evaluation.hh"
#include "biblinternes/bruit/turbulent.hh"
#include "biblinternes/math/entrepolation.hh"
#include "biblinternes/outils/gna.hh"

#include "corps/corps.h"

#include "donnees_type.h"
#include "code_inst.hh"

namespace lcc {

static auto cosinus(float x)
{
	return std::cos(x);
}

static auto sinus(float x)
{
	return std::sin(x);
}

static auto tangeante(float x)
{
	return std::tan(x);
}

static auto arccosinus(float x)
{
	return std::acos(x);
}

static auto arcsinus(float x)
{
	return std::asin(x);
}

static auto arctangeante(float x)
{
	return std::atan(x);
}

static auto absolu(float x)
{
	return std::abs(x);
}

static auto racine_carre(float x)
{
	return std::sqrt(x);
}

static auto exponentiel(float x)
{
	return std::exp(x);
}

static auto logarithme(float x)
{
	return std::log(x);
}

static auto sol(float x)
{
	return std::floor(x);
}

static auto fraction(float x)
{
	return x - sol(x);
}

static auto plafond(float x)
{
	return std::ceil(x);
}

static auto arrondis(float x)
{
	return std::round(x);
}

static auto nie(float x)
{
	return -x;
}

static auto complement(float x)
{
	return 1.0f - x;
}

static auto ajoute(float x, float y)
{
	return x + y;
}

static auto soustrait(float x, float y)
{
	return x - y;
}

static auto multiplie(float x, float y)
{
	return x * y;
}

static auto divise(float x, float y)
{
	if ((x != 0.0f) && (y != 0.0f)) {
		return x / y;
	}

	return 0.0f;
}

static auto modulo(float x, float y)
{
	return std::fmod(x, y);
}

static auto arctangeante2(float x, float y)
{
	return std::atan2(x, y);
}

static auto maximum(float x, float y)
{
	return std::max(x, y);
}

static auto minimum(float x, float y)
{
	return std::min(x, y);
}

static auto plus_grand_que(float x, float y)
{
	return (x > y) ? 1.0f : 0.0f;
}

static auto plus_petit_que(float x, float y)
{
	return (x < y) ? 1.0f : 0.0f;
}

static auto puissance(float x, float y)
{
	return std::pow(x, y);
}

/* ****************************** comparaisons ****************************** */

static auto sont_egaux(float x, float y)
{
	return (std::abs(x - y) <= std::numeric_limits<float>::epsilon()) ? 1.0f : 0.0f;
}

static auto sont_inegaux(float x, float y)
{
	return (std::abs(x - y) > std::numeric_limits<float>::epsilon()) ? 1.0f : 0.0f;
}

static auto est_inferieure(float x, float y)
{
	return (x < y) ? 1.0f : 0.0f;
}

static auto est_superieure(float x, float y)
{
	return (x > y) ? 1.0f : 0.0f;
}

static auto est_inferieure_egale(float x, float y)
{
	return (x <= y) ? 1.0f : 0.0f;
}

static auto est_superieure_egale(float x, float y)
{
	return (x >= y) ? 1.0f : 0.0f;
}

static auto comp_et(float x, float y)
{
	return ((x != 0.0f) && (y != 0.0f)) ? 1.0f : 0.0f;
}

static auto comp_ou(float x, float y)
{
	return ((x != 0.0f) || (y != 0.0f)) ? 1.0f : 0.0f;
}

static auto comp_oux(float x, float y)
{
	return ((x != 0.0f) ^ (y != 0.0f)) ? 1.0f : 0.0f;
}

/* ************************************************************************** */

/* NOEUDS MATRICE
 * - identité
 * - déterminant
 * - norm
 * - transforme (vec/point)
 * - vecteurs_propres
 * - décomposition QR
 *
 * NOEUDS VECTEURS
 * - normal (widget -> créer un vecteur selon une direction)
 *
 * NOEUDS CONVERSIONS
 * - sépare/combine RGB, HSV, XYZ, YCbCr, YUV
 * - rampe couleur
 * - courbe (valeur/couleur)
 *
 * NOEUDS COULEURS
 * - brightness/contraste
 * - niveaux
 * - gamma
 * - hue/saturation/valeur
 * - hue correct
 * - light falloff
 * - mixRGB
 * - balance couleur
 * - correction couleur
 * - alpha over
 * - tone map
 * - z combine (+ deep compositing)
 */

static auto appel_fonction_math_double(
		pile &pile_donnees,
		pile const &pile_insts,
		int &inst_courante,
		std::function<float(float, float)> fonc)
{
	auto donnees_type = static_cast<type_var>(pile_insts.charge_entier(inst_courante));

	switch (donnees_type) {
		default:
		{
			break;
		}
		case type_var::ENT32:
		{
			auto val0 = pile_donnees.charge_entier(inst_courante, pile_insts);
			auto val1 = pile_donnees.charge_entier(inst_courante, pile_insts);
			auto res = fonc(static_cast<float>(val0), static_cast<float>(val1));
			pile_donnees.stocke(inst_courante, pile_insts, static_cast<int>(res));
			break;
		}
		case type_var::DEC:
		{
			auto val0 = pile_donnees.charge_decimal(inst_courante, pile_insts);
			auto val1 = pile_donnees.charge_decimal(inst_courante, pile_insts);
			auto res = fonc(val0, val1);
			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::VEC2:
		{
			auto val0 = pile_donnees.charge_vec2(inst_courante, pile_insts);
			auto val1 = pile_donnees.charge_vec2(inst_courante, pile_insts);
			auto res = dls::math::vec2f();

			res.x = fonc(val0.x, val1.x);
			res.y = fonc(val0.y, val1.y);

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::VEC3:
		{
			auto val0 = pile_donnees.charge_vec3(inst_courante, pile_insts);
			auto val1 = pile_donnees.charge_vec3(inst_courante, pile_insts);
			auto res = dls::math::vec3f();

			res.x = fonc(val0.x, val1.x);
			res.y = fonc(val0.y, val1.y);
			res.z = fonc(val0.z, val1.z);

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::VEC4:
		{
			auto val0 = pile_donnees.charge_vec4(inst_courante, pile_insts);
			auto val1 = pile_donnees.charge_vec4(inst_courante, pile_insts);
			auto res = dls::math::vec4f();

			res.x = fonc(val0.x, val1.x);
			res.y = fonc(val0.y, val1.y);
			res.z = fonc(val0.z, val1.z);
			res.w = fonc(val0.w, val1.w);

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::MAT3:
		{
			auto val0 = pile_donnees.charge_mat3(inst_courante, pile_insts);
			auto val1 = pile_donnees.charge_mat3(inst_courante, pile_insts);
			auto res = dls::math::mat3x3f();

			for (auto i = 0ul; i < 3; ++i) {
				for (auto j = 0ul; j < 3; ++j) {
					res[i][j] = fonc(val0[i][j], val1[i][j]);
				}
			}

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::MAT4:
		{
			auto val0 = pile_donnees.charge_mat4(inst_courante, pile_insts);
			auto val1 = pile_donnees.charge_mat4(inst_courante, pile_insts);
			auto res = dls::math::mat4x4f();

			for (auto i = 0ul; i < 4; ++i) {
				for (auto j = 0ul; j < 4; ++j) {
					res[i][j] = fonc(val0[i][j], val1[i][j]);
				}
			}

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::COULEUR:
		{
			auto v0 = pile_donnees.charge_couleur(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_couleur(inst_courante, pile_insts);

			auto res = dls::phys::couleur32();
			res.r = fonc(v0.r, v1.r);
			res.v = fonc(v0.v, v1.v);
			res.b = fonc(v0.b, v1.b);

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
	}
}

static auto appel_fonction_math_simple(
		pile &pile_donnees,
		pile const &pile_insts,
		int &inst_courante,
		std::function<float(float)> fonc)
{
	auto donnees_type = static_cast<type_var>(pile_insts.charge_entier(inst_courante));

	switch (donnees_type) {
		default:
		{
			break;
		}
		case type_var::ENT32:
		{
			auto val = pile_donnees.charge_entier(inst_courante, pile_insts);
			auto res = fonc(static_cast<float>(val));
			pile_donnees.stocke(inst_courante, pile_insts, static_cast<int>(res));
			break;
		}
		case type_var::DEC:
		{
			auto val = pile_donnees.charge_decimal(inst_courante, pile_insts);
			auto res = fonc(val);
			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::VEC2:
		{
			auto val = pile_donnees.charge_vec2(inst_courante, pile_insts);
			auto res = dls::math::vec2f();

			res.x = fonc(val.x);
			res.y = fonc(val.y);

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::VEC3:
		{
			auto val = pile_donnees.charge_vec3(inst_courante, pile_insts);
			auto res = dls::math::vec3f();

			res.x = fonc(val.x);
			res.y = fonc(val.y);
			res.z = fonc(val.z);

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::VEC4:
		{
			auto val = pile_donnees.charge_vec4(inst_courante, pile_insts);
			auto res = dls::math::vec4f();

			res.x = fonc(val.x);
			res.y = fonc(val.y);
			res.z = fonc(val.z);
			res.w = fonc(val.w);

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::MAT3:
		{
			auto val = pile_donnees.charge_mat3(inst_courante, pile_insts);
			auto res = dls::math::mat3x3f();

			for (auto i = 0ul; i < 3; ++i) {
				for (auto j = 0ul; j < 3; ++j) {
					res[i][j] = fonc(val[i][j]);
				}
			}

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::MAT4:
		{
			auto val = pile_donnees.charge_mat4(inst_courante, pile_insts);
			auto res = dls::math::mat4x4f();

			for (auto i = 0ul; i < 4; ++i) {
				for (auto j = 0ul; j < 4; ++j) {
					res[i][j] = fonc(val[i][j]);
				}
			}

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::COULEUR:
		{
			auto v0 = pile_donnees.charge_couleur(inst_courante, pile_insts);

			auto res = dls::phys::couleur32();
			res.r = fonc(v0.r);
			res.v = fonc(v0.v);
			res.b = fonc(v0.b);

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
	}
}

static auto enligne(float v0, float v1, float v2)
{
	return dls::math::entrepolation_lineaire(v0, v1, v2);
}

static auto restreint(float v0, float v1, float v2)
{
	return dls::math::restreint(v0, v1, v2);
}

static void appel_fonction_3_args(
		pile &pile_donnees,
		pile const &pile_insts,
		int &inst_courante,
		std::function<float(float, float, float)> fonc)
{
	auto donnees_type = static_cast<type_var>(pile_insts.charge_entier(inst_courante));

	switch (donnees_type) {
		default:
		{
			break;
		}
		case type_var::ENT32:
		{
			auto v0 = pile_donnees.charge_entier(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_entier(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_entier(inst_courante, pile_insts);

			auto res = fonc(static_cast<float>(v0), static_cast<float>(v1), static_cast<float>(v2));

			pile_donnees.stocke(inst_courante, pile_insts, static_cast<int>(res));

			break;
		}
		case type_var::DEC:
		{
			auto v0 = pile_donnees.charge_decimal(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_decimal(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_decimal(inst_courante, pile_insts);

			auto res = fonc(v0, v1, v2);

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::VEC2:
		{
			auto v0 = pile_donnees.charge_vec2(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_vec2(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_vec2(inst_courante, pile_insts);

			auto res = dls::math::vec2f();

			for (auto i = 0ul; i < 2; ++i) {
				res[i] = fonc(v0[i], v1[i], v2[i]);
			}

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::VEC3:
		{
			auto v0 = pile_donnees.charge_vec3(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_vec3(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_vec3(inst_courante, pile_insts);

			auto res = dls::math::vec3f();

			for (auto i = 0ul; i < 3; ++i) {
				res[i] = fonc(v0[i], v1[i], v2[i]);
			}

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::VEC4:
		{
			auto v0 = pile_donnees.charge_vec4(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_vec4(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_vec4(inst_courante, pile_insts);

			auto res = dls::math::vec4f();

			for (auto i = 0ul; i < 4; ++i) {
				res[i] = fonc(v0[i], v1[i], v2[i]);
			}

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::MAT3:
		{
			auto v0 = pile_donnees.charge_mat3(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_mat3(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_mat3(inst_courante, pile_insts);

			auto res = dls::math::mat3x3f();

			for (auto i = 0ul; i < 3; ++i) {
				for (auto j = 0ul; j < 3; ++j) {
					res[i][j] = fonc(v0[i][j], v1[i][j], v2[i][j]);
				}
			}

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::MAT4:
		{
			auto v0 = pile_donnees.charge_mat4(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_mat4(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_mat4(inst_courante, pile_insts);

			auto res = dls::math::mat4x4f();

			for (auto i = 0ul; i < 4; ++i) {
				for (auto j = 0ul; j < 4; ++j) {
					res[i][j] = fonc(v0[i][j], v1[i][j], v2[i][j]);
				}
			}

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::COULEUR:
		{
			auto v0 = pile_donnees.charge_couleur(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_couleur(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_couleur(inst_courante, pile_insts);

			auto res = dls::phys::couleur32();
			res.r = fonc(v0.r, v1.r, v2.r);
			res.v = fonc(v0.v, v1.v, v2.v);
			res.b = fonc(v0.b, v1.b, v2.b);

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
	}
}

static auto hermite1(float v0, float v1, float v2, float v3, float v4)
{
	return dls::math::entrepolation_fluide<1>(v0, v1, v2, v3, v4);
}

static auto hermite2(float v0, float v1, float v2, float v3, float v4)
{
	return dls::math::entrepolation_fluide<2>(v0, v1, v2, v3, v4);
}

static auto hermite3(float v0, float v1, float v2, float v3, float v4)
{
	return dls::math::entrepolation_fluide<3>(v0, v1, v2, v3, v4);
}

static auto hermite4(float v0, float v1, float v2, float v3, float v4)
{
	return dls::math::entrepolation_fluide<4>(v0, v1, v2, v3, v4);
}

static auto hermite5(float v0, float v1, float v2, float v3, float v4)
{
	return dls::math::entrepolation_fluide<5>(v0, v1, v2, v3, v4);
}

static auto hermite6(float v0, float v1, float v2, float v3, float v4)
{
	return dls::math::entrepolation_fluide<6>(v0, v1, v2, v3, v4);
}

static auto traduit(float v0, float v1, float v2, float v3, float v4)
{
	return dls::math::traduit(v0, v1, v2, v3, v4);
}

static void appel_fonction_5_args(
		pile &pile_donnees,
		pile const &pile_insts,
		int &inst_courante,
		std::function<float(float, float, float, float, float)> fonc)
{
	auto donnees_type = static_cast<type_var>(pile_insts.charge_entier(inst_courante));

	switch (donnees_type) {
		default:
		{
			break;
		}
		case type_var::ENT32:
		{
			auto v0 = pile_donnees.charge_entier(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_entier(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_entier(inst_courante, pile_insts);
			auto v3 = pile_donnees.charge_entier(inst_courante, pile_insts);
			auto v4 = pile_donnees.charge_entier(inst_courante, pile_insts);

			auto res = fonc(static_cast<float>(v0), static_cast<float>(v1), static_cast<float>(v2), static_cast<float>(v3), static_cast<float>(v4));

			pile_donnees.stocke(inst_courante, pile_insts, static_cast<int>(res));

			break;
		}
		case type_var::DEC:
		{
			auto v0 = pile_donnees.charge_decimal(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_decimal(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_decimal(inst_courante, pile_insts);
			auto v3 = pile_donnees.charge_decimal(inst_courante, pile_insts);
			auto v4 = pile_donnees.charge_decimal(inst_courante, pile_insts);

			auto res = fonc(v0, v1, v2, v3, v4);

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::VEC2:
		{
			auto v0 = pile_donnees.charge_vec2(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_vec2(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_vec2(inst_courante, pile_insts);
			auto v3 = pile_donnees.charge_vec2(inst_courante, pile_insts);
			auto v4 = pile_donnees.charge_vec2(inst_courante, pile_insts);

			auto res = dls::math::vec2f();

			for (auto i = 0ul; i < 2; ++i) {
				res[i] = fonc(v0[i], v1[i], v2[i], v3[i], v4[i]);
			}

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::VEC3:
		{
			auto v0 = pile_donnees.charge_vec3(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_vec3(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_vec3(inst_courante, pile_insts);
			auto v3 = pile_donnees.charge_vec3(inst_courante, pile_insts);
			auto v4 = pile_donnees.charge_vec3(inst_courante, pile_insts);

			auto res = dls::math::vec3f();

			for (auto i = 0ul; i < 3; ++i) {
				res[i] = fonc(v0[i], v1[i], v2[i], v3[i], v4[i]);
			}

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::VEC4:
		{
			auto v0 = pile_donnees.charge_vec4(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_vec4(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_vec4(inst_courante, pile_insts);
			auto v3 = pile_donnees.charge_vec4(inst_courante, pile_insts);
			auto v4 = pile_donnees.charge_vec4(inst_courante, pile_insts);

			auto res = dls::math::vec4f();

			for (auto i = 0ul; i < 4; ++i) {
				res[i] = fonc(v0[i], v1[i], v2[i], v3[i], v4[i]);
			}

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::MAT3:
		{
			auto v0 = pile_donnees.charge_mat3(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_mat3(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_mat3(inst_courante, pile_insts);
			auto v3 = pile_donnees.charge_mat3(inst_courante, pile_insts);
			auto v4 = pile_donnees.charge_mat3(inst_courante, pile_insts);

			auto res = dls::math::mat3x3f();

			for (auto i = 0ul; i < 3; ++i) {
				for (auto j = 0ul; j < 3; ++j) {
					res[i][j] = fonc(v0[i][j], v1[i][j], v2[i][j], v3[i][j], v4[i][j]);
				}
			}

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::MAT4:
		{
			auto v0 = pile_donnees.charge_mat4(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_mat4(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_mat4(inst_courante, pile_insts);
			auto v3 = pile_donnees.charge_mat4(inst_courante, pile_insts);
			auto v4 = pile_donnees.charge_mat4(inst_courante, pile_insts);

			auto res = dls::math::mat4x4f();

			for (auto i = 0ul; i < 4; ++i) {
				for (auto j = 0ul; j < 4; ++j) {
					res[i][j] = fonc(v0[i][j], v1[i][j], v2[i][j], v3[i][j], v4[i][j]);
				}
			}

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
		case type_var::COULEUR:
		{
			auto v0 = pile_donnees.charge_couleur(inst_courante, pile_insts);
			auto v1 = pile_donnees.charge_couleur(inst_courante, pile_insts);
			auto v2 = pile_donnees.charge_couleur(inst_courante, pile_insts);
			auto v3 = pile_donnees.charge_couleur(inst_courante, pile_insts);
			auto v4 = pile_donnees.charge_couleur(inst_courante, pile_insts);

			auto res = dls::phys::couleur32();
			res.r = fonc(v0.r, v1.r, v2.r, v3.r, v4.r);
			res.v = fonc(v0.v, v1.v, v2.v, v3.v, v4.v);
			res.b = fonc(v0.b, v1.b, v2.b, v3.b, v4.b);

			pile_donnees.stocke(inst_courante, pile_insts, res);
			break;
		}
	}
}

static auto charge_param_bruit(
		bruit::parametres &params,
		pile &pile_donnees,
		pile const &insts,
		int &inst_courante)
{
	params.decalage_pos = pile_donnees.charge_vec3(inst_courante, insts);
	params.echelle_pos = pile_donnees.charge_vec3(inst_courante, insts);
	params.decalage_valeur = pile_donnees.charge_decimal(inst_courante, insts);
	params.echelle_valeur = pile_donnees.charge_decimal(inst_courante, insts);
	params.temps_anim = pile_donnees.charge_decimal(inst_courante, insts);
}

static auto charge_param_bruit_turb(
		bruit::param_turbulence &params,
		pile &pile_donnees,
		pile const &insts,
		int &inst_courante)
{
	params.octaves = pile_donnees.charge_decimal(inst_courante, insts);
	params.gain = pile_donnees.charge_decimal(inst_courante, insts);
	params.lacunarite = pile_donnees.charge_decimal(inst_courante, insts);
	params.amplitude = pile_donnees.charge_decimal(inst_courante, insts);
}

auto cree_bruit(
		ctx_local &ctx,
		pile &pile_donnees,
		pile const &insts,
		int &inst_courante,
		bruit::type type_bruit)
{
	auto graine = pile_donnees.charge_entier(inst_courante, insts);

	auto params = bruit::parametres();
	charge_param_bruit(params, pile_donnees, insts, inst_courante);

	bruit::construit(type_bruit, params, graine);

	ctx.params_bruits.pousse(params);
	auto idx = ctx.params_bruits.taille() - 1;

	pile_donnees.stocke(inst_courante, insts, static_cast<int>(idx));
}

static auto evalue_bruit(
		ctx_local &ctx,
		pile &pile_donnees,
		pile const &insts,
		int &inst_courante)
{
	auto idx = pile_donnees.charge_entier(inst_courante, insts);
	auto pos = pile_donnees.charge_vec3(inst_courante, insts);

	auto res = 0.0f;
	auto deriv = dls::math::vec3f();

	if (idx <= ctx.params_bruits.taille()) {
		auto const &params = ctx.params_bruits[idx];

		res = bruit::evalue_derivee(params, pos, deriv);
	}

	auto ptr_sortie = insts.charge_entier(inst_courante);
	pile_donnees.stocke(ptr_sortie, res);
	pile_donnees.stocke(ptr_sortie, deriv);
}

static auto evalue_bruit_turbulence(
		ctx_local &ctx,
		pile &pile_donnees,
		pile const &insts,
		int &inst_courante)
{
	auto idx = pile_donnees.charge_entier(inst_courante, insts);
	auto pos = pile_donnees.charge_vec3(inst_courante, insts);

	auto params_turb = bruit::param_turbulence();
	charge_param_bruit_turb(params_turb, pile_donnees, insts, inst_courante);

	auto res = 0.0f;
	auto deriv = dls::math::vec3f();

	if (idx <= ctx.params_bruits.taille()) {
		auto const &params = ctx.params_bruits[idx];

		res = bruit::evalue_turb_derivee(params, params_turb, pos, deriv);
	}

	auto ptr_sortie = insts.charge_entier(inst_courante);
	pile_donnees.stocke(ptr_sortie, res);
	pile_donnees.stocke(ptr_sortie, deriv);
}

void execute_pile(
		ctx_exec &contexte,
		ctx_local &contexte_local,
		pile &pile_donnees,
		pile const &insts,
		int graine)
{
	auto compteur = 0;
	std::mt19937 gna(graine);

	while (compteur != insts.taille()) {
		auto inst = static_cast<code_inst>(insts.charge_entier(compteur));

		//std::cerr << "code_inst : " << chaine_code_inst(inst) << '\n';

		switch (inst) {
			case code_inst::TERMINE:
			{
				return;
			}
			case code_inst::ASSIGNATION:
			{
				auto donnees_type = static_cast<type_var>(insts.charge_entier(compteur));

				switch (donnees_type) {
					default:
					{
						break;
					}
						/* copie le pointeur du tableau */
					case type_var::TABLEAU:
					case type_var::ENT32:
					{
						auto valeur = pile_donnees.charge_entier(compteur, insts);
						pile_donnees.stocke(compteur, insts, valeur);
						break;
					}
					case type_var::DEC:
					case type_var::VEC2:
					case type_var::VEC3:
					case type_var::VEC4:
					case type_var::MAT3:
					case type_var::MAT4:
					case type_var::COULEUR:
					{
						auto idx_orig = insts.charge_entier(compteur);
						auto idx_dest = insts.charge_entier(compteur);

						auto taille = static_cast<long>(sizeof(float)) * taille_type(donnees_type);

						auto ptr_orig = pile_donnees.donnees() + idx_orig;
						auto ptr_dest = pile_donnees.donnees() + idx_dest;

						std::memcpy(ptr_dest, ptr_orig, static_cast<size_t>(taille));

						break;
					}
				}

				break;
			}
			case code_inst::IN_BRANCHE:
			{
				compteur = insts.charge_entier(compteur);
				break;
			}
			case code_inst::IN_BRANCHE_CONDITION:
			{
				auto ptr = insts.charge_entier(compteur);
				auto si_vrai = insts.charge_entier(compteur);
				auto si_faux = insts.charge_entier(compteur);

				auto val = pile_donnees.charge_decimal(ptr);

				if (val != 0.0f) {
					compteur = si_vrai;
				}
				else {
					compteur = si_faux;
				}

				break;
			}
			case code_inst::IN_INCREMENTE:
			{
				auto type = insts.charge_entier(compteur);
				static_cast<void>(type);

				auto ptr = insts.charge_entier(compteur);

				auto val = pile_donnees.charge_entier(ptr);
				val += 1;
				ptr -= 1;

				pile_donnees.stocke(ptr, val);

				break;
			}
			case code_inst::FN_ALEA:
			{
				auto v0 = pile_donnees.charge_decimal(compteur, insts);
				auto v1 = pile_donnees.charge_decimal(compteur, insts);

				std::uniform_real_distribution<float> dist(v0, v1);
				pile_donnees.stocke(compteur, insts, dist(gna));

				break;
			}
			case code_inst::FN_ECHANTILLONE_SPHERE:
			{
				auto g = static_cast<unsigned>(graine);
				auto gna_loc = GNASimple(g);
				auto res = echantillone_sphere<dls::math::vec3f>(gna_loc);
				graine = static_cast<int>(g);
				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::NIE:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, nie);
				break;
			}
			case code_inst::FN_INVERSE:
			{
				auto donnees_type = static_cast<type_var>(insts.charge_entier(compteur));

				switch (donnees_type) {
					default:
					{
						break;
					}
					case type_var::ENT32:
					{
						/* L'inverse d'un nombre entier est égal à zéro. */
						++compteur;
						pile_donnees.stocke(compteur, insts, 0);
						break;
					}
					case type_var::DEC:
					{
						auto val = pile_donnees.charge_decimal(compteur, insts);
						val = (val != 0.0f) ? 1.0f / val : 0.0f;
						pile_donnees.stocke(compteur, insts, val);
						break;
					}
					case type_var::VEC2:
					{
						auto val = pile_donnees.charge_vec2(compteur, insts);

						for (auto i = 0ul; i < 2; ++i) {
							val[i] = (val[i] != 0.0f) ? 1.0f / val[i] : 0.0f;
						}

						pile_donnees.stocke(compteur, insts, val);
						break;
					}
					case type_var::VEC3:
					{
						auto val = pile_donnees.charge_vec3(compteur, insts);

						for (auto i = 0ul; i < 3; ++i) {
							val[i] = (val[i] != 0.0f) ? 1.0f / val[i] : 0.0f;
						}

						pile_donnees.stocke(compteur, insts, val);
						break;
					}
					case type_var::VEC4:
					{
						auto val = pile_donnees.charge_vec4(compteur, insts);

						for (auto i = 0ul; i < 4; ++i) {
							val[i] = (val[i] != 0.0f) ? 1.0f / val[i] : 0.0f;
						}

						pile_donnees.stocke(compteur, insts, val);
						break;
					}
					case type_var::MAT3:
					{
						auto val = pile_donnees.charge_mat3(compteur, insts);
						val = inverse(val);
						pile_donnees.stocke(compteur, insts, val);
						break;
					}
					case type_var::MAT4:
					{
						auto val = pile_donnees.charge_mat4(compteur, insts);
						val = inverse(val);
						pile_donnees.stocke(compteur, insts, val);
						break;
					}
					case type_var::COULEUR:
					{
						auto val = pile_donnees.charge_couleur(compteur, insts);

						val.r = (val.r != 0.0f) ? 1.0f / val.r : 0.0f;
						val.v = (val.v != 0.0f) ? 1.0f / val.v : 0.0f;
						val.b = (val.b != 0.0f) ? 1.0f / val.b : 0.0f;

						pile_donnees.stocke(compteur, insts, val);
						break;
					}
				}

				break;
			}
			case code_inst::FN_ENLIGNE:
			{
				appel_fonction_3_args(pile_donnees, insts, compteur, enligne);
				break;
			}
			case code_inst::FN_RESTREINT:
			{
				appel_fonction_3_args(pile_donnees, insts, compteur, restreint);
				break;
			}
			case code_inst::FN_TRADUIT:
			{
				appel_fonction_5_args(pile_donnees, insts, compteur, traduit);
				break;
			}
			case code_inst::FN_HERMITE1:
			{
				appel_fonction_5_args(pile_donnees, insts, compteur, hermite1);
				break;
			}
			case code_inst::FN_HERMITE2:
			{
				appel_fonction_5_args(pile_donnees, insts, compteur, hermite2);
				break;
			}
			case code_inst::FN_HERMITE3:
			{
				appel_fonction_5_args(pile_donnees, insts, compteur, hermite3);
				break;
			}
			case code_inst::FN_HERMITE4:
			{
				appel_fonction_5_args(pile_donnees, insts, compteur, hermite4);
				break;
			}
			case code_inst::FN_HERMITE5:
			{
				appel_fonction_5_args(pile_donnees, insts, compteur, hermite5);
				break;
			}
			case code_inst::FN_HERMITE6:
			{
				appel_fonction_5_args(pile_donnees, insts, compteur, hermite6);
				break;
			}
			case code_inst::FN_BASE_ORTHONORMALE:
			{
				auto vec = pile_donnees.charge_vec3(compteur, insts);

				auto b0 = dls::math::vec3f(0.0f);
				auto b1 = dls::math::vec3f(0.0f);

				cree_base_orthonormale(vec, b0, b1);

				auto ptr_sortie = insts.charge_entier(compteur);
				pile_donnees.stocke(ptr_sortie, b0);
				pile_donnees.stocke(ptr_sortie, b1);
				break;
			}
			case code_inst::FN_COMBINE_VEC3:
			{
				dls::math::vec3f vec;
				vec.x = pile_donnees.charge_decimal(compteur, insts);
				vec.y = pile_donnees.charge_decimal(compteur, insts);
				vec.z = pile_donnees.charge_decimal(compteur, insts);

				pile_donnees.stocke(compteur, insts, vec);
				break;
			}
			case code_inst::FN_SEPARE_VEC3:
			{
				auto vec = pile_donnees.charge_vec3(compteur, insts);
				/* les trois sorties sont l'une après l'autre donc on peut
				 * simplement stocker le vecteur directement */
				pile_donnees.stocke(compteur, insts, vec);
				break;
			}
			case code_inst::FN_NORMALISE_VEC3:
			{
				auto vec = pile_donnees.charge_vec3(compteur, insts);
				auto lng = 0.0f;
				vec = normalise(vec, lng);

				auto ptr_sortie = insts.charge_entier(compteur);
				pile_donnees.stocke(ptr_sortie, vec);
				pile_donnees.stocke(ptr_sortie, lng);
				break;
			}
			case code_inst::FN_COMPLEMENT:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, complement);
				break;
			}
			case code_inst::FN_PRODUIT_SCALAIRE_VEC3:
			{
				auto v0 = pile_donnees.charge_vec3(compteur, insts);
				auto v1 = pile_donnees.charge_vec3(compteur, insts);

				auto res = dls::math::produit_scalaire(v0, v1);

				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::FN_LONGUEUR_VEC3:
			{
				auto v0 = pile_donnees.charge_vec3(compteur, insts);

				auto res = dls::math::longueur(v0);

				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::FN_PRODUIT_CROIX_VEC3:
			{
				auto v0 = pile_donnees.charge_vec3(compteur, insts);
				auto v1 = pile_donnees.charge_vec3(compteur, insts);

				auto res = dls::math::produit_croix(v0, v1);

				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::FN_FRESNEL:
			{
				auto I = pile_donnees.charge_vec3(compteur, insts);
				auto N = pile_donnees.charge_vec3(compteur, insts);
				auto idr = pile_donnees.charge_decimal(compteur, insts);

				auto res = fresnel(I, N, idr);

				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::FN_REFLECHI:
			{
				auto I = pile_donnees.charge_vec3(compteur, insts);
				auto N = pile_donnees.charge_vec3(compteur, insts);

				auto res = reflechi(I, N);

				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::FN_REFRACTE:
			{
				auto I = pile_donnees.charge_vec3(compteur, insts);
				auto N = pile_donnees.charge_vec3(compteur, insts);
				auto idr = pile_donnees.charge_decimal(compteur, insts);

				auto res = refracte(I, N, idr);

				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::FN_AJOUTE:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, ajoute);
				break;
			}
			case code_inst::FN_SOUSTRAIT:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, soustrait);
				break;
			}
			case code_inst::FN_MULTIPLIE:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, multiplie);
				break;
			}
			case code_inst::FN_DIVISE:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, divise);
				break;
			}
			case code_inst::FN_MODULO:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, modulo);
				break;
			}
			case code_inst::FN_COSINUS:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, cosinus);
				break;
			}
			case code_inst::FN_SINUS:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, sinus);
				break;
			}
			case code_inst::FN_TANGEANTE:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, tangeante);
				break;
			}
			case code_inst::FN_ARCCOSINUS:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, arccosinus);
				break;
			}
			case code_inst::FN_ARCSINUS:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, arcsinus);
				break;
			}
			case code_inst::FN_ARCTANGEANTE:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, arctangeante);
				break;
			}
			case code_inst::FN_ABSOLU:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, absolu);
				break;
			}
			case code_inst::FN_RACINE_CARREE:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, racine_carre);
				break;
			}
			case code_inst::FN_EXPONENTIEL:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, exponentiel);
				break;
			}
			case code_inst::FN_LOGARITHME:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, logarithme);
				break;
			}
			case code_inst::FN_FRACTION:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, fraction);
				break;
			}
			case code_inst::FN_PLAFOND:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, plafond);
				break;
			}
			case code_inst::FN_SOL:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, sol);
				break;
			}
			case code_inst::FN_ARRONDIS:
			{
				appel_fonction_math_simple(pile_donnees, insts, compteur, arrondis);
				break;
			}
			case code_inst::FN_ARCTAN2:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, arctangeante2);
				break;
			}
			case code_inst::FN_MAX:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, maximum);
				break;
			}
			case code_inst::FN_MIN:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, minimum);
				break;
			}
			case code_inst::FN_PLUS_GRAND_QUE:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, plus_grand_que);
				break;
			}
			case code_inst::FN_PLUS_PETIT_QUE:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, plus_petit_que);
				break;
			}
			case code_inst::FN_EGALITE:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, sont_egaux);
				break;
			}
			case code_inst::FN_INEGALITE:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, sont_inegaux);
				break;
			}
			case code_inst::FN_SUPERIEUR:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, est_superieure);
				break;
			}
			case code_inst::FN_INFERIEUR:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, est_inferieure);
				break;
			}
			case code_inst::FN_SUPERIEUR_EGAL:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, est_superieure_egale);
				break;
			}
			case code_inst::FN_INFERIEUR_EGAL:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, est_inferieure_egale);
				break;
			}
			case code_inst::FN_COMP_OU:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, comp_ou);
				break;
			}
			case code_inst::FN_COMP_ET:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, comp_et);
				break;
			}
			case code_inst::FN_COMP_OUX:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, comp_oux);
				break;
			}
			case code_inst::FN_PUISSANCE:
			{
				appel_fonction_math_double(pile_donnees, insts, compteur, puissance);
				break;
			}
			case code_inst::ENT_VERS_DEC:
			{
				auto ent = pile_donnees.charge_entier(compteur, insts);
				pile_donnees.stocke(compteur, insts, static_cast<float>(ent));
				break;
			}
			case code_inst::DEC_VERS_ENT:
			{
				auto dec = pile_donnees.charge_decimal(compteur, insts);
				pile_donnees.stocke(compteur, insts, static_cast<int>(dec));
				break;
			}
			case code_inst::DEC_VERS_VEC2:
			{
				auto dec = pile_donnees.charge_decimal(compteur, insts);
				auto res = dls::math::vec2f(dec);

				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::ENT_VERS_VEC2:
			{
				auto ent = pile_donnees.charge_entier(compteur, insts);
				auto res = dls::math::vec2f(static_cast<float>(ent));

				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::DEC_VERS_VEC3:
			{
				auto dec = pile_donnees.charge_decimal(compteur, insts);
				auto res = dls::math::vec3f(dec);

				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::ENT_VERS_VEC3:
			{
				auto ent = pile_donnees.charge_entier(compteur, insts);
				auto res = dls::math::vec3f(static_cast<float>(ent));

				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::DEC_VERS_VEC4:
			{
				auto dec = pile_donnees.charge_decimal(compteur, insts);
				auto res = dls::math::vec4f(dec);

				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::ENT_VERS_VEC4:
			{
				auto ent = pile_donnees.charge_entier(compteur, insts);
				auto res = dls::math::vec4f(static_cast<float>(ent));

				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::FN_MULTIPLIE_MAT:
			{
				auto donnees_type = static_cast<type_var>(insts.charge_entier(compteur));

				if (donnees_type == type_var::MAT3) {
					auto mat0 = pile_donnees.charge_mat3(compteur, insts);
					auto mat1 = pile_donnees.charge_mat3(compteur, insts);
					pile_donnees.stocke(compteur, insts, mat0 * mat1);
				}
				else if (donnees_type == type_var::MAT4) {
					auto mat0 = pile_donnees.charge_mat4(compteur, insts);
					auto mat1 = pile_donnees.charge_mat4(compteur, insts);
					pile_donnees.stocke(compteur, insts, mat0 * mat1);
				}

				break;
			}
			case code_inst::FN_AJOUTE_POINT:
			{
				auto pos = pile_donnees.charge_vec3(compteur, insts);

				auto &ptr_corps = contexte.ptr_corps;
				auto index = -1l;

				ptr_corps.accede_ecriture([pos, &index](Corps *corps)
				{
					index = corps->ajoute_point(pos);
				});

				pile_donnees.stocke(compteur, insts, static_cast<int>(index));

				break;
			}
			case code_inst::FN_AJOUTE_PRIMITIVE:
			{
				auto type = pile_donnees.charge_entier(compteur, insts);
				auto &ptr_corps = contexte.ptr_corps;
				auto index = -1l;

				ptr_corps.accede_ecriture([type, &index](Corps *corps)
				{
					auto poly = corps->ajoute_polygone(static_cast<type_polygone>(type));
					index = poly->index;
				});

				pile_donnees.stocke(compteur, insts, static_cast<int>(index));

				break;
			}
			case code_inst::FN_AJOUTE_PRIMITIVE_SOMMETS:
			{
				auto type = pile_donnees.charge_entier(compteur, insts);
				auto idx_tabl = pile_donnees.charge_entier(compteur, insts);
				auto &ptr_corps = contexte.ptr_corps;
				auto &tableau = contexte_local.tableaux.tableau(idx_tabl);
				auto index = -1l;

				ptr_corps.accede_ecriture([type, &index, &tableau](Corps *corps)
				{
					auto poly = corps->ajoute_polygone(static_cast<type_polygone>(type));
					index = poly->index;

					for (auto const &v : tableau) {
						corps->ajoute_sommet(poly, v);
					}
				});

				pile_donnees.stocke(compteur, insts, static_cast<int>(index));

				break;
			}
			case code_inst::FN_AJOUTE_SOMMET:
			{
				auto idx_prim = pile_donnees.charge_entier(compteur, insts);
				auto idx_point = pile_donnees.charge_entier(compteur, insts);
				auto &ptr_corps = contexte.ptr_corps;
				auto idx_sommet = -1l;

				ptr_corps.accede_ecriture([idx_prim, idx_point, &idx_sommet](Corps *corps)
				{
					auto prim = corps->prims()->prim(idx_prim);
					auto poly = dynamic_cast<Polygone *>(prim);
					idx_sommet = corps->ajoute_sommet(poly, idx_point);
				});

				pile_donnees.stocke(compteur, insts, static_cast<int>(idx_sommet));

				break;
			}
			case code_inst::FN_AJOUTE_SOMMETS:
			{
				auto idx_prim = pile_donnees.charge_entier(compteur, insts);
				auto idx_tabl = pile_donnees.charge_entier(compteur, insts);
				auto &ptr_corps = contexte.ptr_corps;
				auto &tableau = contexte_local.tableaux.tableau(idx_tabl);

				ptr_corps.accede_ecriture([idx_prim, &tableau](Corps *corps)
				{
					auto prim = corps->prims()->prim(idx_prim);
					auto poly = dynamic_cast<Polygone *>(prim);

					for (auto const &v : tableau) {
						corps->ajoute_sommet(poly, v);
					}
				});

				// À FAIRE : retourne un tableau d'index des sommets ajoutés
				pile_donnees.stocke(compteur, insts, 0);

				break;
			}
			case code_inst::FN_AJOUTE_LIGNE:
			{
				auto pos = pile_donnees.charge_vec3(compteur, insts);
				auto dir = pile_donnees.charge_vec3(compteur, insts);
				auto &ptr_corps = contexte.ptr_corps;
				auto index = -1;

				ptr_corps.accede_ecriture([&pos, &dir, &index](Corps *corps)
				{
					auto p0 = corps->ajoute_point(pos);
					auto p1 = corps->ajoute_point(pos + dir);

					auto prim = corps->ajoute_polygone(type_polygone::OUVERT, 2);
					corps->ajoute_sommet(prim, p0);
					corps->ajoute_sommet(prim, p1);

					index = static_cast<int>(prim->index);
				});

				pile_donnees.stocke(compteur, insts, index);

				break;
			}
			case code_inst::FN_SATURE:
			{
				auto clr = pile_donnees.charge_couleur(compteur, insts);
				auto l   = pile_donnees.charge_decimal(compteur, insts);
				auto fac = pile_donnees.charge_decimal(compteur, insts);

				auto lum = dls::phys::couleur32(l, l, l, clr.a);

				if (fac != 0.0f) {
					lum = (1.0f - fac) * lum + clr * fac;
				}

				pile_donnees.stocke(compteur, insts, lum);
				break;
			}
			case code_inst::FN_LUMINANCE:
			{
				auto clr = pile_donnees.charge_couleur(compteur, insts);

				auto res = luminance(clr);

				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::FN_CORPS_NOIR:
			{
				auto temp = pile_donnees.charge_decimal(compteur, insts);
				auto res = dls::phys::couleur_depuis_corps_noir(temp);
				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::FN_LONGUEUR_ONDE:
			{
				auto temp = pile_donnees.charge_decimal(compteur, insts);
				auto res = dls::phys::couleur_depuis_longueur_onde(temp);
				pile_donnees.stocke(compteur, insts, res);
				break;
			}
			case code_inst::FN_CONTRASTE:
			{
				auto clr0 = pile_donnees.charge_couleur(compteur, insts);
				auto clr1 = pile_donnees.charge_couleur(compteur, insts);

				auto res = calcul_contraste_local(clr0, clr1);

				pile_donnees.stocke(compteur, insts, res);
				break;
			}				
#define EVALUE_BRUIT(code_, type_) \
			case code_inst::code_: \
			{ \
				cree_bruit(contexte_local, pile_donnees, insts, compteur, type_); \
				break; \
			}
			EVALUE_BRUIT(FN_BRUIT_CELLULE, bruit::type::CELLULE)
			EVALUE_BRUIT(FN_BRUIT_FOURIER, bruit::type::FOURIER)
			EVALUE_BRUIT(FN_BRUIT_ONDELETTE, bruit::type::ONDELETTE)
			EVALUE_BRUIT(FN_BRUIT_PERLIN, bruit::type::PERLIN)
			EVALUE_BRUIT(FN_BRUIT_SIMPLEX, bruit::type::SIMPLEX)
			EVALUE_BRUIT(FN_BRUIT_VALEUR, bruit::type::VALEUR)
			EVALUE_BRUIT(FN_BRUIT_VORONOI_F1, bruit::type::VORONOI_F1)
			EVALUE_BRUIT(FN_BRUIT_VORONOI_F2, bruit::type::VORONOI_F2)
			EVALUE_BRUIT(FN_BRUIT_VORONOI_F3, bruit::type::VORONOI_F3)
			EVALUE_BRUIT(FN_BRUIT_VORONOI_F4, bruit::type::VORONOI_F4)
			EVALUE_BRUIT(FN_BRUIT_VORONOI_F1F2, bruit::type::VORONOI_F1F2)
			EVALUE_BRUIT(FN_BRUIT_VORONOI_CR, bruit::type::VORONOI_CR)
#undef EVALUE_BRUIT
			case code_inst::FN_EVALUE_BRUIT:
			{
				evalue_bruit(contexte_local, pile_donnees, insts, compteur);
				break;
			}
			case code_inst::FN_EVALUE_BRUIT_TURBULENCE:
			{
				evalue_bruit_turbulence(contexte_local, pile_donnees, insts, compteur);
				break;
			}
			case code_inst::CONSTRUIT_TABLEAU:
			{
				auto donnees_type = static_cast<type_var>(insts.charge_entier(compteur));
				auto nombre_donnees = insts.charge_entier(compteur);

				auto pair_tabl_idx = contexte_local.tableaux.cree_tableau();
				auto &tableau = pair_tabl_idx.first;

				tableau.reserve(nombre_donnees);

				switch (donnees_type) {
					default:
					{
						break;
					}
					case type_var::ENT32:
					{
						for (auto i = 0; i < nombre_donnees; ++i) {
							auto val = pile_donnees.charge_entier(compteur, insts);
							tableau.pousse(val);
						}
						break;
					}
					case type_var::DEC:
					{
						break;
					}
					case type_var::VEC2:
					{
						break;
					}
					case type_var::VEC3:
					{
						break;
					}
					case type_var::VEC4:
					{
						break;
					}
					case type_var::MAT3:
					{
						break;
					}
					case type_var::MAT4:
					{
						break;
					}
					case type_var::COULEUR:
					{
						break;
					}
				}

				pile_donnees.stocke(compteur, insts, static_cast<int>(pair_tabl_idx.second));

				break;
			}
			case code_inst::IN_INSERT_TABLEAU:
			{
				// ptr où se trouve le tableau
				// type des données du tableau
				// index où insérer
				// ptr de la valeur à insérer

				auto ptr_tabl = insts.charge_entier(compteur);
				auto type = static_cast<type_var>(insts.charge_entier(compteur));
				auto index = insts.charge_entier(compteur);

				auto &tableau = contexte_local.tableaux.tableau(ptr_tabl);

				/* À FAIRE */
				switch (type) {
					default:
					{
						break;
					}
				}

				tableau[index] = pile_donnees.charge_entier(compteur, insts);

				break;
			}
			case code_inst::IN_EXTRAIT_TABLEAU:
			{
				// ptr où se trouve le tableau
				// type des données du tableau
				// index où extraire
				// ptr où écrire

				auto ptr_tabl = insts.charge_entier(compteur);
				auto type = static_cast<type_var>(insts.charge_entier(compteur));
				auto index = insts.charge_entier(compteur);

				auto &tableau = contexte_local.tableaux.tableau(ptr_tabl);

				/* À FAIRE */
				switch (type) {
					default:
					{
						break;
					}
				}

				pile_donnees.stocke(compteur, insts, tableau[index]);

				break;
			}
			case code_inst::FN_TAILLE_TABLEAU:
			{
				auto ptr_tabl = insts.charge_entier(compteur);
				auto &tableau = contexte_local.tableaux.tableau(ptr_tabl);

				pile_donnees.stocke(compteur, insts, static_cast<int>(tableau.taille()));
				break;
			}
#if 0
			case code_inst::FN_TAILLE_CHAINE:
			{
				auto ptr_chaine = donnees.charge_entier(insts, courant);
				auto chaine = gest_chn.get(ptr_chaine);

				donnees.stocke(insts, courant, chaine->taille());

				break;
			}
			case code_inst::FN_CONCAT_CHAINE:
			{
				auto ptr_chaine1 = donnees.charge_entier(insts, courant);
				auto ptr_chaine2 = donnees.charge_entier(insts, courant);

				auto chaine1 = gest_chn.get(ptr_chaine1);
				auto chaine2 = gest_chn.get(ptr_chaine2);

				auto idx_chn = gest_chn.cree(morceaux[i]);

				donnees.stocke(insts, courant, idx_chn);

				break;
			}
			case code_inst::FN_DECOUPE_CHAINE:
			{
				auto ptr_chaine = donnees.charge_entier(insts, courant);
				auto ptr_seprtr = donnees.charge_entier(insts, courant);

				auto chaine = gest_chn.get(ptr_chaine);
				auto sepa = gest_chn.get(ptr_separa);

				auto morceaux = dls::morcelle(chaine, sepa);

				auto tabl = gest_tabl.cree(morceaux.taille());

				for (auto i = 0; i < morceaux.taille(); ++i) {
					auto idx_chn = gest_chn.cree(morceaux[i]);
					tabl->pousse(idx_chn);
				}

				donnees.stocke(insts, tabl->index());

				break;
			}
			case code_inst::FN_CHAINE_VERS_DECIMAL:
			{
				break;
			}
			case code_inst::FN_SHERE_UV:
			{
				break;
			}
#endif
		}
	}
}

}  /* namespace lcc */
