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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

class ReseauNeuronal {
	int m_nombre_entrees = 0;
	int m_nombre_couches = 0;
	int m_nombre_sorties = 0;

	/* Les neurones d'entrées du système. */
	double *m_entrees = nullptr;

	/* Les neurones cachés du système. */
	double *m_couches = nullptr;

	/* Les neurones de sorties du système. */
	double *m_sorties = nullptr;

	/* Matrice gouvernant les poids appliqués aux entrées au niveau des neurones cachés. */
	double **m_entrees_couches = nullptr;

	/* Matrice gouvernant les poids appliqués aux neurones cachés au niveau des sorties. */
	double **m_couches_sorties = nullptr;

public:
	ReseauNeuronal(int entrees, int couches, int sorties);

	ReseauNeuronal(const ReseauNeuronal &rhs) = delete;

	~ReseauNeuronal();

	ReseauNeuronal &operator=(const ReseauNeuronal &);

	void initialise_poids();

	void avance(const double *entrees);

	double *avance_entrees(double *entrees);

	int nombre_entrees() const;

	int nombre_couches() const;

	int nombre_sorties() const;

	double *entrees();

	double *couches();

	double *sorties();

	double **entrees_couches();

	double **couches_sorties();
};
