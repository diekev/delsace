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

#include "normaux.hh"

#include "biblinternes/outils/constantes.h"

#include "corps/corps.h"
#include "corps/iteration_corps.hh"

template <typename T>
static auto acos(T fac)
{
	if (fac <= static_cast<T>(-1)) {
		return constantes<T>::PI;
	}

	if (fac >= static_cast<T>(1)) {
		return static_cast<T>(0);
	}

	return std::acos(fac);
}

/* calcul les normaux pour chaque polygone, ne les normalise pas */
static auto rappel_calcul_normal_poly(Corps const &corps, Polygone *poly)
{
	auto attr_N = corps.attribut("N_poly");
	auto points = corps.points_pour_lecture();

	switch (poly->nombre_sommets()) {
		case 3:
		{
			/* Calcul du normal en croisant deux cotés. */

			auto const &p0 = points->point(poly->index_point(0));
			auto const &p1 = points->point(poly->index_point(1));
			auto const &p2 = points->point(poly->index_point(2));

			auto const c0 = p1 - p0;
			auto const c1 = p2 - p0;

			auto nor = produit_croix(c0, c1);
			attr_N->valeur(static_cast<long>(poly->index), nor);

			break;
		}
		case 4:
		{
			/* Calcul du normal en croisant les sommets 0-2 x 1-3. */

			auto const &p0 = points->point(poly->index_point(0));
			auto const &p1 = points->point(poly->index_point(1));
			auto const &p2 = points->point(poly->index_point(2));
			auto const &p3 = points->point(poly->index_point(3));

			auto const c0 = p0 - p2;
			auto const c1 = p1 - p3;

			auto nor = produit_croix(c0, c1);
			attr_N->valeur(static_cast<long>(poly->index), nor);

			break;
		}
		default:
		{
			/* Calcul du normal selon la méthode de Newell, voir Graphics Gems. */

			auto i0 = poly->nombre_sommets() - 1;
			auto i1 = 0l;

			auto nor = dls::math::vec3f(0.0f);

			for (auto i = 0; i < poly->nombre_sommets(); ++i) {
				auto idx_prev = poly->index_point(i0);
				auto idx_cour = poly->index_point(i1);

				auto const &p_prev = points->point(idx_prev);
				auto const &p_cour = points->point(idx_cour);

				nor[0] += (p_prev[1] - p_cour[1]) * (p_prev[2] + p_cour[2]);
				nor[1] += (p_prev[2] - p_cour[2]) * (p_prev[0] + p_cour[0]);
				nor[2] += (p_prev[0] - p_cour[0]) * (p_prev[1] + p_cour[1]);

				i0 = i1;
				i1 = (i1 + 1) % poly->nombre_sommets();
			}

			attr_N->valeur(static_cast<long>(poly->index), nor);

			break;
		}
	}
}

/* accumule les normaux non "normalisés" des faces adjacentes */
static auto rappel_pesee_normal_aire(Corps const &corps, Polygone *poly)
{
	auto attr_N_poly = corps.attribut("N_poly");
	auto attr_N = corps.attribut("N");

	auto nor = attr_N_poly->vec3(static_cast<long>(poly->index));

	for (auto i = 0; i < poly->nombre_sommets(); ++i) {
		auto idx_s = poly->index_point(i);
		auto nor_s = attr_N->vec3(idx_s);
		attr_N->valeur(idx_s, nor_s + nor);
	}
}

/* normalise les normaux des faces adjacentes et pondère selon l'angle au niveau
 * du vertex */
static auto rappel_pesee_normal_angle(Corps const &corps, Polygone *poly)
{
	auto points = corps.points_pour_lecture();
	auto attr_N_poly = corps.attribut("N_poly");
	auto attr_N = corps.attribut("N");

	auto nor = normalise(attr_N_poly->vec3(static_cast<long>(poly->index)));

	auto i0 = poly->nombre_sommets() - 1;
	auto i1 = 0l;
	auto i2 = 1l;

	for (auto i = 0; i < poly->nombre_sommets(); ++i) {
		auto idx_prev = poly->index_point(i0);
		auto idx_cour = poly->index_point(i1);
		auto idx_suiv = poly->index_point(i2);

		auto const &p_prev = points->point(idx_prev);
		auto const &p_cour = points->point(idx_cour);
		auto const &p_suiv = points->point(idx_suiv);

		auto c0 = normalise(p_cour - p_prev);
		auto c1 = normalise(p_suiv - p_cour);

		auto cos_angle = -produit_scalaire(c0, c1);

		auto poids = acos(cos_angle);

		auto nor_s = attr_N->vec3(idx_cour);
		attr_N->valeur(idx_cour, nor_s + nor * poids);

		i0 = i1;
		i1 = i2;
		i2 = (i2 + 1) % poly->nombre_sommets();
	}
}

/* accumule les normaux "normalisés" des faces adjacentes */
static auto rappel_pesee_normal_moyenne(Corps const &corps, Polygone *poly)
{
	auto attr_N_poly = corps.attribut("N_poly");
	auto attr_N = corps.attribut("N");

	auto nor = normalise(attr_N_poly->vec3(static_cast<long>(poly->index)));

	for (auto i = 0; i < poly->nombre_sommets(); ++i) {
		auto idx_s = poly->index_point(i);
		auto nor_s = attr_N->vec3(idx_s);
		attr_N->valeur(idx_s, nor_s + nor);
	}
}

/* accumule les normaux des faces adjacentes et pondère selon l'inverse du
 * produit de la taille des côtés adjacents au vertex */
static auto rappel_pesee_normal_max(Corps const &corps, Polygone *poly)
{
	auto points = corps.points_pour_lecture();

	auto attr_N_poly = corps.attribut("N_poly");
	auto attr_N = corps.attribut("N");

	auto nor = attr_N_poly->vec3(static_cast<long>(poly->index));

	auto i0 = poly->nombre_sommets() - 1;
	auto i1 = 0l;
	auto i2 = 1l;

	for (auto i = 0; i < poly->nombre_sommets(); ++i) {
		auto idx_prev = poly->index_point(i0);
		auto idx_cour = poly->index_point(i1);
		auto idx_suiv = poly->index_point(i2);

		auto const &p_prev = points->point(idx_prev);
		auto const &p_cour = points->point(idx_cour);
		auto const &p_suiv = points->point(idx_suiv);

		auto l0 = longueur(p_prev - p_cour);
		auto l1 = longueur(p_suiv - p_cour);

		if (l0 != 0.0f && l1 != 0.0f) {
			auto nor_s = attr_N->vec3(idx_cour);
			attr_N->valeur(idx_cour, nor_s + nor * (1.0f / (l0 * l1)));
		}

		i0 = i1;
		i1 = i2;
		i2 = (i2 + 1) % poly->nombre_sommets();
	}
}

void calcul_normaux(Corps &corps, location_normal location, pesee_normal pesee, bool inverse_normaux)
{
	corps.supprime_attribut("N");
	corps.supprime_attribut("N_poly");

	auto attr_N_poly = corps.ajoute_attribut("N_poly", type_attribut::VEC3, portee_attr::PRIMITIVE);
	auto attr_N = static_cast<Attribut *>(nullptr);

	pour_chaque_polygone_ferme(corps, rappel_calcul_normal_poly);

	switch (location) {
		case location_normal::POINT:
		{
			attr_N = corps.ajoute_attribut("N", type_attribut::VEC3, portee_attr::POINT);

			switch (pesee) {
				case pesee_normal::AIRE:
				{
					pour_chaque_polygone_ferme(corps, rappel_pesee_normal_aire);
					break;
				}
				case pesee_normal::ANGLE:
				{
					pour_chaque_polygone_ferme(corps, rappel_pesee_normal_angle);
					break;
				}
				case pesee_normal::MOYENNE:
				{
					pour_chaque_polygone_ferme(corps, rappel_pesee_normal_moyenne);
					break;
				}
				case pesee_normal::MAX:
				{
					pour_chaque_polygone_ferme(corps, rappel_pesee_normal_max);
					break;
				}
			}

			break;
		}
		case location_normal::PRIMITIVE:
		{
			/* ne fait rien, les normaux seront normalisés avant de sortir */
			attr_N = attr_N_poly;
			attr_N->nom("N");
			break;
		}
		case location_normal::CORPS:
		{
			attr_N = corps.ajoute_attribut("N", type_attribut::VEC3, portee_attr::CORPS);

			/* accumule les normaux de tous les polygones */
			auto nor = accumule_attr<dls::math::vec3f>(*attr_N_poly);
			attr_N->valeur(0, nor);

			break;
		}
	}

	/* normalise et inverse au besoin */
	transforme_attr<dls::math::vec3f>(*attr_N, [&](dls::math::vec3f const &v)
	{
		auto res = normalise(v);

		if (inverse_normaux) {
			return -res;
		}

		return res;
	});

	if (attr_N != attr_N_poly) {
		corps.supprime_attribut("N_poly");
	}
}

void calcul_normaux(Corps &corps, bool plats, bool inverse_normaux)
{
	calcul_normaux(
				corps,
				(plats == true) ? location_normal::PRIMITIVE : location_normal::POINT,
				pesee_normal::AIRE,
				inverse_normaux);
}
