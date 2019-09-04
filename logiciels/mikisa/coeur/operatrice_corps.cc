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

#include "operatrice_corps.h"

#include "biblinternes/structures/flux_chaine.hh"

#include "noeud.hh"

OperatriceCorps::OperatriceCorps(Graphe &graphe_parent, Noeud &noeud_)
	: OperatriceImage(graphe_parent, noeud_)
{
}

int OperatriceCorps::type() const
{
	return OPERATRICE_CORPS;
}

type_prise OperatriceCorps::type_entree(int) const
{
	return type_prise::CORPS;
}

type_prise OperatriceCorps::type_sortie(int) const
{
	return type_prise::CORPS;
}

Corps *OperatriceCorps::corps()
{
	return &m_corps;
}

void OperatriceCorps::donnees_simulation(DonneesSimulation *donnees)
{
	m_donnees_simulation = donnees;
}

void OperatriceCorps::libere_memoire()
{
	m_corps.reinitialise();
	cache_est_invalide = true;
}

/* ************************************************************************** */

bool valide_corps_entree(OperatriceCorps &op,
		Corps const *corps,
		bool besoin_points,
		bool besoin_prims, int index)
{
	if (corps == nullptr) {
		auto flux = dls::flux_chaine();
		flux << "Le corps d'entrée de la prise à l'index" << index << " est nul";
		op.ajoute_avertissement(flux.chn());
		return false;
	}

	if (besoin_points && corps->points_pour_lecture()->taille() == 0) {
		op.ajoute_avertissement("Le corps d'entrée n'a pas de point");
		return false;
	}

	if (besoin_prims && corps->prims()->taille() == 0) {
		op.ajoute_avertissement("Le corps d'entrée n'a pas de primitive");
		return false;
	}

	return true;
}
