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

#include <vector>

struct Point {
	float x;
	float y;
};

enum {
	CONTROLE_CONTRAINT = 0,
	CONTROLE_LIBRE     = 1,
};

enum {
	POINT_CONTROLE1 = 0,
	POINT_CENTRE    = 1,
	POINT_CONTROLE2 = 2,

	NOMBRE_POINT
};

struct PointBezier {
	Point co[NOMBRE_POINT];
	char type_controle = CONTROLE_CONTRAINT;
};

struct CourbeBezier {
	/* les points constituants cette courbe */
	std::vector<PointBezier> points;

	/* extension du point minimum en dehors des limites de la courbe */
	PointBezier extension_min;

	/* extension du point maximum en dehors des limites de la courbe */
	PointBezier extension_max;

	/* Pour le rendu et l'évaluation. */
	std::vector<Point> table;

	/* valeur minimale de la courbe pour l'évaluation */
	float valeur_min = 0.0f;

	/* valeur maximale de la courbe pour l'évaluation */
	float valeur_max = 1.0f;

	CourbeBezier() = default;
	~CourbeBezier() = default;
};

void construit_table_courbe(CourbeBezier &courbe);

void ajoute_point_courbe(CourbeBezier &courbe, float x, float y);
