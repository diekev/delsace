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

namespace wlk {

/**
 * Cette structure est équivalente à et duplique essentiellement ChefExecution
 * de Jorjala. On l'utilise pour éviter d'avoir des dépendances étranges entre
 * Jorjala et Wolika.
 */
struct interruptrice {
	virtual ~interruptrice() = default;

	/**
	 * Retourne vrai si l'interruptrice a été interrompue.
	 */
	virtual bool interrompue() const = 0;

	/**
	 * Indique la progression d'un algorithme en série, dans un seul thread.
	 */
	virtual void indique_progression(float progression) = 0;

	/**
	 * Indique la progression depuis le corps d'une boucle parallèle. Le delta
	 * est la quantité de travail effectuée dans le thread du corps.
	 *
	 * Un mutex est verrouillé à chaque appel, et le delta est ajouté à une
	 * progression globale mise à zéro à chaque appel à demarre_evaluation().
	 */
	virtual void indique_progression_parallele(float delta) = 0;
};

}  /* namespace wlk */
