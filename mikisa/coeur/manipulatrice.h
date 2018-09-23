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

#include <math/point3.h>
#include <math/vec3.h>

enum {
	ETAT_DEFAUT         = 0,
	ETAT_SELECTION_X    = 1,
	ETAT_SELECTION_Y    = 2,
	ETAT_SELECTION_Z    = 3,
	ETAT_INTERSECTION_X = 4,
	ETAT_INTERSECTION_Y = 5,
	ETAT_INTERSECTION_Z = 6,
	ETAT_INTERSECTION_XYZ = 7,
};

enum {
	MANIPULATION_POSITION      = 0,
	MANIPULATION_ROTATION      = 1,
	MANIPULATION_ECHELLE       = 2,
	MANIPULATION_PERSONNALISEE = 3,
};

/* ************************************************************************** */

struct Plan {
	numero7::math::point3f pos;
	numero7::math::vec3f   nor;
};

enum {
	PLAN_XY,
	PLAN_XZ,
	PLAN_YZ,
	PLAN_XYZ,
};

bool entresecte_plan(const Plan &plan, const numero7::math::point3f &orig, const numero7::math::vec3f &dir, float &t);

/* ************************************************************************** */

class Manipulatrice3D {
protected:
	numero7::math::point3f m_position = numero7::math::point3f(0.0f);
	numero7::math::point3f m_taille = numero7::math::point3f(1.0f);
	numero7::math::point3f m_taille_originale = numero7::math::point3f(1.0f);
	numero7::math::point3f m_rotation = numero7::math::point3f(0.0f);
	numero7::math::point3f m_rotation_originale = numero7::math::point3f(0.0f);
	int m_etat = 0;

public:
	virtual ~Manipulatrice3D() = default;

	const numero7::math::point3f &pos() const;

	void pos(const numero7::math::point3f &pos);

	const numero7::math::point3f &taille() const;

	void taille(const numero7::math::point3f &t);

	const numero7::math::point3f &rotation() const;

	void rotation(const numero7::math::point3f &r);

	int etat() const;

	virtual	bool entresecte(const numero7::math::point3f &orig, const numero7::math::vec3f &dir) = 0;

	virtual	void repond_manipulation(const numero7::math::vec3f &delta) = 0;
};

/* ************************************************************************** */

class ManipulatricePosition3D final : public Manipulatrice3D {
	/* boite englobante */
	numero7::math::vec3f m_min = numero7::math::vec3f(0.0f);
	numero7::math::vec3f m_max = numero7::math::vec3f(1.0f);

	/* baton axe x */
	numero7::math::vec3f m_min_baton_x;
	numero7::math::vec3f m_max_baton_x;

	/* baton axe y */
	numero7::math::vec3f m_min_baton_y;
	numero7::math::vec3f m_max_baton_y;

	/* baton axe z */
	numero7::math::vec3f m_min_baton_z;
	numero7::math::vec3f m_max_baton_z;

	/* poignée axe x */
	numero7::math::vec3f m_min_poignee_x;
	numero7::math::vec3f m_max_poignee_x;

	/* poignée axe y */
	numero7::math::vec3f m_min_poignee_y;
	numero7::math::vec3f m_max_poignee_y;

	/* poignée axe z */
	numero7::math::vec3f m_min_poignee_z;
	numero7::math::vec3f m_max_poignee_z;

	/* poignée axe xyz */
	numero7::math::vec3f m_min_poignee_xyz;
	numero7::math::vec3f m_max_poignee_xyz;

public:
	ManipulatricePosition3D();

	bool entresecte(const numero7::math::point3f &orig, const numero7::math::vec3f &dir) override;

	void repond_manipulation(const numero7::math::vec3f &delta) override;
};

/* ************************************************************************** */

class ManipulatriceEchelle3D final : public Manipulatrice3D {
	/* boite englobante */
	numero7::math::vec3f m_min = numero7::math::vec3f(0.0f);
	numero7::math::vec3f m_max = numero7::math::vec3f(1.0f);

	/* baton axe x */
	numero7::math::vec3f m_min_baton_x;
	numero7::math::vec3f m_max_baton_x;

	/* baton axe y */
	numero7::math::vec3f m_min_baton_y;
	numero7::math::vec3f m_max_baton_y;

	/* baton axe z */
	numero7::math::vec3f m_min_baton_z;
	numero7::math::vec3f m_max_baton_z;

	/* poignée axe x */
	numero7::math::vec3f m_min_poignee_x;
	numero7::math::vec3f m_max_poignee_x;

	/* poignée axe y */
	numero7::math::vec3f m_min_poignee_y;
	numero7::math::vec3f m_max_poignee_y;

	/* poignée axe z */
	numero7::math::vec3f m_min_poignee_z;
	numero7::math::vec3f m_max_poignee_z;

	/* poignée axe xyz */
	numero7::math::vec3f m_min_poignee_xyz;
	numero7::math::vec3f m_max_poignee_xyz;

public:
	ManipulatriceEchelle3D();

	bool entresecte(const numero7::math::point3f &orig, const numero7::math::vec3f &dir) override;

	void repond_manipulation(const numero7::math::vec3f &delta) override;
};

/* ************************************************************************** */

class ManipulatriceRotation3D final : public Manipulatrice3D {
	/* boite englobante */
	numero7::math::vec3f m_min = numero7::math::vec3f(0.0f);
	numero7::math::vec3f m_max = numero7::math::vec3f(1.0f);

public:
	ManipulatriceRotation3D();

	bool entresecte(const numero7::math::point3f &orig, const numero7::math::vec3f &dir) override;

	void repond_manipulation(const numero7::math::vec3f &delta) override;
};
