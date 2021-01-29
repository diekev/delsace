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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

struct Compilatrice;
struct ConstructriceRI;
struct EspaceDeTravail;

struct Coulisse {
	double temps_generation_code = 0.0;
	double temps_fichier_objet = 0.0;
	double temps_executable = 0.0;

	virtual ~Coulisse() = default;

	virtual bool cree_fichier_objet(Compilatrice &compilatrice, EspaceDeTravail &espace, ConstructriceRI &constructrice_ri) = 0;

	virtual bool cree_executable(Compilatrice &compilatrice, EspaceDeTravail &espace) = 0;
};
