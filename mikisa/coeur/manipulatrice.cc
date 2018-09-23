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

#include "manipulatrice.h"

static constexpr auto RAYON_BRANCHE = 0.02f;
static constexpr auto RAYON_POIGNEE = 0.10f;
static constexpr auto TAILLE = 1.0f;

static bool entresecte_min_max(
		const numero7::math::vec3f &min,
		const numero7::math::vec3f &max,
		const numero7::math::point3f &origine,
		const numero7::math::vec3f &inverse_direction)
{
	auto t1 = (min[0] - origine[0]) * inverse_direction[0];
	auto t2 = (max[0] - origine[0]) * inverse_direction[0];

	auto tmin = std::min(t1, t2);
	auto tmax = std::max(t1, t2);

	for (int i = 1; i < 3; ++i) {
		t1 = (min[i] - origine[i]) * inverse_direction[i];
		t2 = (max[i] - origine[i]) * inverse_direction[i];

		tmin = std::max(tmin, std::min(t1, t2));
		tmax = std::min(tmax, std::max(t1, t2));
	}

	return tmax > std::max(tmin, 0.0f);
}

bool entresecte_plan(const Plan &plan, const numero7::math::point3f &orig, const numero7::math::vec3f &dir, float &t)
{
	const auto denom = produit_scalaire(plan.nor, dir);

	if (std::abs(denom) > 0.0001f) {
		t = produit_scalaire(plan.pos - orig, plan.nor) / denom;

		if (t >= 0) {
			return true;
		}
	}

	return false;
}

/* ************************************************************************** */

int Manipulatrice3D::etat() const
{
	return m_etat;
}

void Manipulatrice3D::pos(const numero7::math::point3f &pos)
{
	m_position = pos;
}

const numero7::math::point3f &Manipulatrice3D::taille() const
{
	return m_taille;
}

void Manipulatrice3D::taille(const numero7::math::point3f &t)
{
	m_taille = t;
	m_taille_originale = t;
}

const numero7::math::point3f &Manipulatrice3D::rotation() const
{
	return m_rotation;
}

void Manipulatrice3D::rotation(const numero7::math::point3f &r)
{
	m_rotation_originale = r;
	m_rotation = r;
}

const numero7::math::point3f &Manipulatrice3D::pos() const
{
	return m_position;
}

/* ************************************************************************** */

ManipulatricePosition3D::ManipulatricePosition3D()
{
	m_min = numero7::math::vec3f(-RAYON_POIGNEE);
	m_max = numero7::math::vec3f(1.0f + RAYON_POIGNEE);

	m_min_baton_x = numero7::math::vec3f(RAYON_POIGNEE * 2, -RAYON_BRANCHE, -RAYON_BRANCHE);
	m_max_baton_x = numero7::math::vec3f(TAILLE,  RAYON_BRANCHE,  RAYON_BRANCHE);

	m_min_poignee_x = numero7::math::vec3f(TAILLE - RAYON_POIGNEE, -RAYON_POIGNEE, -RAYON_POIGNEE);
	m_max_poignee_x = numero7::math::vec3f(TAILLE + RAYON_POIGNEE,  RAYON_POIGNEE,  RAYON_POIGNEE);

	m_min_baton_y = numero7::math::vec3f(-RAYON_BRANCHE, RAYON_POIGNEE * 2, -RAYON_BRANCHE);
	m_max_baton_y = numero7::math::vec3f( RAYON_BRANCHE, TAILLE,  RAYON_BRANCHE);

	m_min_poignee_y = numero7::math::vec3f(-RAYON_POIGNEE, TAILLE - RAYON_POIGNEE, -RAYON_POIGNEE);
	m_max_poignee_y = numero7::math::vec3f( RAYON_POIGNEE, TAILLE + RAYON_POIGNEE,  RAYON_POIGNEE);

	m_min_baton_z = numero7::math::vec3f(-RAYON_BRANCHE, -RAYON_BRANCHE, RAYON_POIGNEE * 2);
	m_max_baton_z = numero7::math::vec3f( RAYON_BRANCHE,  RAYON_BRANCHE, TAILLE);

	m_min_poignee_z = numero7::math::vec3f(-RAYON_POIGNEE, -RAYON_POIGNEE, TAILLE - RAYON_POIGNEE);
	m_max_poignee_z = numero7::math::vec3f( RAYON_POIGNEE,  RAYON_POIGNEE, TAILLE + RAYON_POIGNEE);

	m_min_poignee_xyz = numero7::math::vec3f(-RAYON_POIGNEE, -RAYON_POIGNEE, -RAYON_POIGNEE);
	m_max_poignee_xyz = numero7::math::vec3f( RAYON_POIGNEE,  RAYON_POIGNEE,  RAYON_POIGNEE);
}

bool ManipulatricePosition3D::entresecte(const numero7::math::point3f &orig, const numero7::math::vec3f &dir)
{
	auto dir_inverse = numero7::math::vec3f(0.0f);

	for (int i = 0; i < 3; ++i) {
		if (dir[i] != 0.0f) {
			dir_inverse[i] = 1.0f / dir[i];
		}
	}

	auto orig_local = orig;
	orig_local -= m_position;

	if (!entresecte_min_max(m_min, m_max, orig_local, dir_inverse)) {
		m_etat = ETAT_DEFAUT;
		return false;
	}

	if (entresecte_min_max(m_min_baton_x, m_max_baton_x, orig_local, dir_inverse)) {
		m_etat = ETAT_INTERSECTION_X;
		return true;
	}

	if (entresecte_min_max(m_min_poignee_x, m_max_poignee_x, orig_local, dir_inverse)) {
		m_etat = ETAT_INTERSECTION_X;
		return true;
	}

	if (entresecte_min_max(m_min_baton_y, m_max_baton_y, orig_local, dir_inverse)) {
		m_etat = ETAT_INTERSECTION_Y;
		return true;
	}

	if (entresecte_min_max(m_min_poignee_y, m_max_poignee_y, orig_local, dir_inverse)) {
		m_etat = ETAT_INTERSECTION_Y;
		return true;
	}

	if (entresecte_min_max(m_min_baton_z, m_max_baton_z, orig_local, dir_inverse)) {
		m_etat = ETAT_INTERSECTION_Z;
		return true;
	}

	if (entresecte_min_max(m_min_poignee_z, m_max_poignee_z, orig_local, dir_inverse)) {
		m_etat = ETAT_INTERSECTION_Z;
		return true;
	}

	if (entresecte_min_max(m_min_poignee_xyz, m_max_poignee_xyz, orig_local, dir_inverse)) {
		m_etat = ETAT_INTERSECTION_XYZ;
		return true;
	}

	m_etat = ETAT_DEFAUT;
	return false;
}

void ManipulatricePosition3D::repond_manipulation(const numero7::math::vec3f &delta)
{
	if (m_etat == ETAT_INTERSECTION_X) {
		m_position.x = delta.x;
	}
	else if (m_etat == ETAT_INTERSECTION_Y) {
		m_position.y = delta.y;
	}
	else if (m_etat == ETAT_INTERSECTION_Z) {
		m_position.z = delta.z;
	}
	else if (m_etat == ETAT_INTERSECTION_XYZ) {
		m_position.x = delta.x;
		m_position.y = delta.y;
		m_position.z = delta.z;
	}
}

/* ************************************************************************** */

ManipulatriceEchelle3D::ManipulatriceEchelle3D()
{
	m_min = numero7::math::vec3f(-RAYON_POIGNEE);
	m_max = numero7::math::vec3f(1.0f + RAYON_POIGNEE);

	m_min_baton_x = numero7::math::vec3f(  0.0f, -RAYON_BRANCHE, -RAYON_BRANCHE);
	m_max_baton_x = numero7::math::vec3f(TAILLE,  RAYON_BRANCHE,  RAYON_BRANCHE);

	m_min_poignee_x = numero7::math::vec3f(TAILLE - RAYON_POIGNEE, -RAYON_POIGNEE, -RAYON_POIGNEE);
	m_max_poignee_x = numero7::math::vec3f(TAILLE + RAYON_POIGNEE,  RAYON_POIGNEE,  RAYON_POIGNEE);

	m_min_baton_y = numero7::math::vec3f(-RAYON_BRANCHE,   0.0f, -RAYON_BRANCHE);
	m_max_baton_y = numero7::math::vec3f( RAYON_BRANCHE, TAILLE,  RAYON_BRANCHE);

	m_min_poignee_y = numero7::math::vec3f(-RAYON_POIGNEE, TAILLE - RAYON_POIGNEE, -RAYON_POIGNEE);
	m_max_poignee_y = numero7::math::vec3f( RAYON_POIGNEE, TAILLE + RAYON_POIGNEE,  RAYON_POIGNEE);

	m_min_baton_z = numero7::math::vec3f(-RAYON_BRANCHE, -RAYON_BRANCHE,   0.0f);
	m_max_baton_z = numero7::math::vec3f( RAYON_BRANCHE,  RAYON_BRANCHE, TAILLE);

	m_min_poignee_z = numero7::math::vec3f(-RAYON_POIGNEE, -RAYON_POIGNEE, TAILLE - RAYON_POIGNEE);
	m_max_poignee_z = numero7::math::vec3f( RAYON_POIGNEE,  RAYON_POIGNEE, TAILLE + RAYON_POIGNEE);

	m_min_poignee_xyz = numero7::math::vec3f(-RAYON_POIGNEE, -RAYON_POIGNEE, -RAYON_POIGNEE);
	m_max_poignee_xyz = numero7::math::vec3f( RAYON_POIGNEE,  RAYON_POIGNEE,  RAYON_POIGNEE);
}

bool ManipulatriceEchelle3D::entresecte(const numero7::math::point3f &orig, const numero7::math::vec3f &dir)
{
	auto dir_inverse = numero7::math::vec3f(0.0f);

	for (int i = 0; i < 3; ++i) {
		if (dir[i] != 0.0f) {
			dir_inverse[i] = 1.0f / dir[i];
		}
	}

	auto orig_local = orig;
	orig_local -= m_position;

	if (!entresecte_min_max(m_min, m_max, orig_local, dir_inverse)) {
		m_etat = ETAT_DEFAUT;
		return false;
	}

	if (entresecte_min_max(m_min_baton_x, m_max_baton_x, orig_local, dir_inverse)) {
		m_etat = ETAT_INTERSECTION_X;
		return true;
	}

	if (entresecte_min_max(m_min_poignee_x, m_max_poignee_x, orig_local, dir_inverse)) {
		m_etat = ETAT_INTERSECTION_X;
		return true;
	}

	if (entresecte_min_max(m_min_baton_y, m_max_baton_y, orig_local, dir_inverse)) {
		m_etat = ETAT_INTERSECTION_Y;
		return true;
	}

	if (entresecte_min_max(m_min_poignee_y, m_max_poignee_y, orig_local, dir_inverse)) {
		m_etat = ETAT_INTERSECTION_Y;
		return true;
	}

	if (entresecte_min_max(m_min_baton_z, m_max_baton_z, orig_local, dir_inverse)) {
		m_etat = ETAT_INTERSECTION_Z;
		return true;
	}

	if (entresecte_min_max(m_min_poignee_z, m_max_poignee_z, orig_local, dir_inverse)) {
		m_etat = ETAT_INTERSECTION_Z;
		return true;
	}

	if (entresecte_min_max(m_min_poignee_xyz, m_max_poignee_xyz, orig_local, dir_inverse)) {
		m_etat = ETAT_INTERSECTION_XYZ;
		return true;
	}

	m_etat = ETAT_DEFAUT;
	return false;
}

void ManipulatriceEchelle3D::repond_manipulation(const numero7::math::vec3f &delta)
{
	if (m_etat == ETAT_INTERSECTION_X) {
//		m_max.x = delta.x;
//		m_max_baton_x.x = delta.x;
//		m_min_poignee_x.x = delta.x;
//		m_max_poignee_x.x = delta.x;
		m_taille.x = m_taille_originale.x + delta.x;
	}
	else if (m_etat == ETAT_INTERSECTION_Y) {
//		m_max.y = std::max(m_max.y, delta.y);
//		m_max_baton_y.y = delta.y;
//		m_min_poignee_y.y = delta.y;
//		m_max_poignee_y.y = delta.y;
		m_taille.y = m_taille_originale.y + delta.y;
	}
	else if (m_etat == ETAT_INTERSECTION_Z) {
//		m_max.z = std::max(m_max.z, delta.z);
//		m_max_baton_z.z = delta.z;
//		m_min_poignee_z.z = delta.z;
//		m_max_poignee_z.z = delta.z;
		m_taille.z = m_taille_originale.z + delta.z;
	}
	else if (m_etat == ETAT_INTERSECTION_XYZ) {
		m_taille.x = m_taille_originale.x + delta.x;
		m_taille.y = m_taille_originale.y + delta.y;
		m_taille.z = m_taille_originale.z + delta.z;
	}
}

/* ************************************************************************** */

static Plan plans_roues[] = {
	{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	{{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
	{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
};

ManipulatriceRotation3D::ManipulatriceRotation3D()
{
	m_min = numero7::math::vec3f(-1.0f - RAYON_POIGNEE);
	m_max = numero7::math::vec3f( 1.0f + RAYON_POIGNEE);
}

bool ManipulatriceRotation3D::entresecte(const numero7::math::point3f &orig, const numero7::math::vec3f &dir)
{
	auto dir_inverse = numero7::math::vec3f(0.0f);

	for (int i = 0; i < 3; ++i) {
		if (dir[i] != 0.0f) {
			dir_inverse[i] = 1.0f / dir[i];
		}
	}

	auto orig_local = orig;
	orig_local -= m_position;

	if (!entresecte_min_max(m_min, m_max, orig_local, dir_inverse)) {
		m_etat = ETAT_DEFAUT;
		return false;
	}

	plans_roues[0].pos = m_position;
	plans_roues[1].pos = m_position;
	plans_roues[2].pos = m_position;
	m_etat = ETAT_DEFAUT;

	float dist_max = std::numeric_limits<float>::infinity();
	float t;

	if (entresecte_plan(plans_roues[PLAN_YZ], orig, dir, t)) {
		if (t < dist_max) {
			const auto pos = orig + t * dir;
			const auto vec = m_position - pos;
			const auto lon = (longueur(vec));

			if (lon >= 0.95f && lon <= 1.05f) {
				dist_max = t;
				m_etat = ETAT_INTERSECTION_X;
			}
		}
	}

	if (entresecte_plan(plans_roues[PLAN_XZ], orig, dir, t)) {
		if (t < dist_max) {
			const auto pos = orig + t * dir;
			const auto vec = m_position - pos;
			const auto lon = longueur(vec);

			if (lon >= 0.95f && lon <= 1.05f) {
				dist_max = t;
				m_etat = ETAT_INTERSECTION_Y;
			}
		}
	}

	if (entresecte_plan(plans_roues[PLAN_XY], orig, dir, t)) {
		if (t < dist_max) {
			const auto pos = orig + t * dir;
			const auto vec = m_position - pos;
			const auto lon = longueur(vec);

			if (lon >= 0.95f && lon <= 1.05f) {
				m_etat = ETAT_INTERSECTION_Z;
			}
		}
	}

	return m_etat != ETAT_DEFAUT;
}

void ManipulatriceRotation3D::repond_manipulation(const numero7::math::vec3f &delta)
{
	if (m_etat == ETAT_INTERSECTION_X) {
		m_rotation.x = m_rotation_originale.x + delta.x;
	}
	else if (m_etat == ETAT_INTERSECTION_Y) {
		m_rotation.y = m_rotation_originale.y + delta.y;
	}
	else if (m_etat == ETAT_INTERSECTION_Z) {
		m_rotation.z = m_rotation_originale.z + delta.z;
	}
}
