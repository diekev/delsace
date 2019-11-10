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

#include "biblinternes/math/transformation.hh"

#include "listes.h"

using type_point = dls::math::vec3f;

struct AccesseusePointLecture {
private:
	ListePoints3D const &m_points;
	math::transformation const &m_transformation;

public:
	AccesseusePointLecture(ListePoints3D const &pnts, math::transformation const &transform);

	type_point point_local(long idx) const;

	/**
	 * Retourne le point à l'index précisé transformé pour être dans l'espace
	 * mondiale. Aucune vérification de limite n'est effectuée sur l'index. Si
	 * l'index est hors de limite, le programme crashera sans doute.
	 */
	type_point point_monde(long idx) const;

	long taille() const;
};

struct AccesseusePointEcriture {
private:
	Corps &m_corps;
	ListePoints3D &m_points;
	math::transformation const &m_transformation;

public:
	AccesseusePointEcriture(Corps &corps, ListePoints3D &pnts, math::transformation const &transform);

	type_point point_local(long idx) const;

	/**
	 * Retourne le point à l'index précisé transformé pour être dans l'espace
	 * mondiale. Aucune vérification de limite n'est effectuée sur l'index. Si
	 * l'index est hors de limite, le programme crashera sans doute.
	 */
	type_point point_monde(long idx) const;

	long ajoute_point(type_point const &pnt);

	long ajoute_point(float x, float y, float z);

	void point(long idx, type_point const &pnt);

	void redimensionne(long nombre);

	void reserve(long nombre);

	long taille() const;
};
