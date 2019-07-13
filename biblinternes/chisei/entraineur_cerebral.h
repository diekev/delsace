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

#include "biblinternes/structures/tableau.hh"

class ReseauNeuronal;

struct DonneeFormation {
	dls::tableau<double> entrees = {};
	dls::tableau<double> desirees = {};
};

class EntraineurCerebral {
	ReseauNeuronal *m_reseau = nullptr;

	double **m_delta_entrees_couches = nullptr;
	double **m_delta_couches_sorties = nullptr;

	double *m_gradient_erreur_couches = nullptr;
	double *m_gradient_erreur_sorties = nullptr;

	double m_learning_rate = 0.1;
	double m_momentum = 1.0;

public:
	EntraineurCerebral(ReseauNeuronal &reseau);

	EntraineurCerebral(const EntraineurCerebral &rhs) = delete;

	~EntraineurCerebral();

	EntraineurCerebral &operator=(const EntraineurCerebral &);

	template <typename T>
	inline T gradient_erreur_sortie(T valeur_desiree, T valeur_sortie)
	{
		return valeur_sortie * (1.0 - valeur_sortie) * (valeur_desiree - valeur_sortie);
	}

	inline double gradient_erreur_couche(int j);

	void execute_apprentissage(const dls::tableau<DonneeFormation> &donnees);

	void retropojette(const double *desirees);

	void actualise_poids();
};
