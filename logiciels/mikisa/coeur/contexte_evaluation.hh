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

#include "biblinternes/geometrie/rectangle.h"

class BaseDeDonnees;
class ChefExecution;
struct GestionnaireFichier;

struct ContexteEvaluation {
	/* Le rectangle définissant l'aire de rendu. */
	Rectangle resolution_rendu{};

	GestionnaireFichier *gestionnaire_fichier = nullptr;

	/* Enveloppe la logique de notification de progression et d'arrêt
	 * des tâches. */
	ChefExecution *chef = nullptr;

	/* Base de données du logiciel */
	BaseDeDonnees const *bdd = nullptr;

	/* données sur le temps */
	int temps_debut = 0;
	int temps_fin = 250;
	int temps_courant = 0;
	double cadence = 0.0;

	ContexteEvaluation(ContexteEvaluation const &) = default;
	ContexteEvaluation &operator=(ContexteEvaluation const &) = default;
};
