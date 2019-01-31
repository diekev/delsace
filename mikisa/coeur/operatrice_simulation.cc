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

#include "operatrice_simulation.hh"

#include "noeud_image.h"

OperatriceSimulation::OperatriceSimulation(Graphe &graphe_parent, Noeud *noeud)
	: OperatriceCorps(graphe_parent, noeud)
{
}

const char *OperatriceSimulation::class_name() const
{
	return NOM;
}

const char *OperatriceSimulation::help_text() const
{
	return AIDE;
}

int OperatriceSimulation::type_entree(int) const
{
	return OPERATRICE_CORPS;
}

int OperatriceSimulation::type_sortie(int) const
{
	return OPERATRICE_CORPS;
}

const char *OperatriceSimulation::chemin_entreface() const
{
	return "";
}

Graphe *OperatriceSimulation::graphe()
{
	return &m_graphe;
}

int OperatriceSimulation::type() const
{
	return OPERATRICE_SIMULATION;
}

int OperatriceSimulation::execute(const Rectangle &rectangle, const int temps)
{
	auto sortie_graphe = m_graphe.dernier_noeud_sortie;

	if (sortie_graphe == nullptr) {
		ajoute_avertissement("Aucune sortie trouvée dans le graphe !");
		return EXECUTION_ECHOUEE;
	}

	/* Si nous sommes au début, réinitialise. */
	if (temps == m_debut) {
		/* copie l'état de base */
		auto corps = input(0)->requiers_corps(rectangle, temps);
		m_corps.reinitialise();
		corps->copie_vers(&m_corps);

		/* Le pointeur des entrées doit correspondre aux données de l'image
		 * précédente, donc du corps de cette opératrice.
		 * À FAIRE : considérer le stockage de plusieurs corps. */
		m_graphe.entrees.clear();
		m_graphe.entrees.push_back(&m_corps);

		m_dernier_temps = m_debut;
		return EXECUTION_REUSSIE;
	}

	/* Ne simule que si l'on a avancé d'une image. */
	if (temps != m_dernier_temps + 1) {
		return EXECUTION_REUSSIE;
	}

	m_dernier_temps = temps;

	execute_noeud(sortie_graphe, rectangle, temps);

	/* exécute graphe */
	auto op_sortie = std::any_cast<OperatriceImage *>(sortie_graphe->donnees());

	m_corps.reinitialise();
	op_sortie->corps()->copie_vers(&m_corps);

	return EXECUTION_REUSSIE;
}
