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

#include <mutex>

class Mikisa;

class ChefExecution {
	Mikisa &m_mikisa;
	float m_progression_parallele = 0.0f;
	std::mutex m_mutex_progression{};

public:
	explicit ChefExecution(Mikisa &mikisa);

	bool interrompu() const;

	/**
	 * Indique la progression d'un algorithme en série, dans un seul thread.
	 */
	void indique_progression(float progression);

	/**
	 * Indique la progression depuis le corps d'une boucle parallèle. Le delta
	 * est la quantité de travail effectuée dans le thread du corps.
	 *
	 * Un mutex est verrouillé à chaque appel, et le delta est ajouté à une
	 * progression globale mise à zéro à chaque appel à demarre_evaluation().
	 */
	void indique_progression_parallele(float delta);

	void demarre_evaluation(const char *message);
};
