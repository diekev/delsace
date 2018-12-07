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

#include "couleur.h"

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
	char pad[3];
};

struct CourbeBezier {
	/* les points constituants cette courbe */
	std::vector<PointBezier> points{};

	/* le point courant de cette courbe */
	PointBezier *point_courant = nullptr;

	/* extension du point minimum en dehors des limites de la courbe */
	PointBezier extension_min{};

	/* extension du point maximum en dehors des limites de la courbe */
	PointBezier extension_max{};

	/* Pour le rendu et l'évaluation. */
	std::vector<Point> table{};

	/* valeur minimale de la courbe pour l'évaluation */
	float valeur_min = 0.0f;

	/* valeur maximale de la courbe pour l'évaluation */
	float valeur_max = 1.0f;

	/* défini si oui ou non la table doit être utilisée lors de l'évaluation
	 * de la courbe */
	bool utilise_table = true;

	bool pad[7];

	CourbeBezier() = default;
	~CourbeBezier();

	CourbeBezier(CourbeBezier const &autre);
	CourbeBezier(CourbeBezier &&autre) = default;

	CourbeBezier &operator=(CourbeBezier const &autre) = default;
};

enum {
	COURBE_MAITRESSE = 0,
	COURBE_ROUGE     = 1,
	COURBE_VERTE     = 2,
	COURBE_BLEUE     = 3,
	COURBE_VALEUR    = 4,

	NOMBRE_COURBES = 5,
};

enum {
	COURBE_COULEUR_RGB      = 0,
	COURBE_COULEUR_FILMIQUE = 1,
};

struct CourbeCouleur {
	CourbeBezier courbes[NOMBRE_COURBES];

	int mode = COURBE_MAITRESSE;
	int type = COURBE_COULEUR_RGB;

	CourbeCouleur();
	~CourbeCouleur() = default;

	CourbeCouleur(const CourbeCouleur &autre) = default;
	CourbeCouleur(CourbeCouleur &&autre) = default;
};

void cree_courbe_defaut(CourbeBezier &courbe);

void construit_table_courbe(CourbeBezier &courbe);

void ajoute_point_courbe(CourbeBezier &courbe, float x, float y);

void calcule_controles_courbe(CourbeBezier &courbe);

float evalue_courbe_bezier(const CourbeBezier &courbe, float valeur);

couleur32 evalue_courbe_couleur(const CourbeCouleur &courbe, const couleur32 &valeur);
