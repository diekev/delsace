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

#include "entraineur_cerebral.h"

#include "reseau_neuronal.h"

#if 0
EntraineurCerebral::EntraineurCerebral(ReseauNeuronal &reseau)
    : m_reseau(&reseau)
{
	/* Création des matrices delta. */

	m_delta_entrees_couches = new double*[m_reseau->nombre_entrees() + 1];

	for (int i = 0; i <= m_reseau->nombre_entrees(); ++i) {
		m_delta_entrees_couches[i] = new double[m_reseau->nombre_couches()];

		for (int j = 0; j < m_reseau->nombre_couches(); ++j) {
			m_delta_entrees_couches[i][j] = 0.0;
		}
	}

	m_delta_couches_sorties = new double*[m_reseau->nombre_couches() + 1];

	for (int i = 0; i <= m_reseau->nombre_couches(); ++i) {
		m_delta_couches_sorties[i] = new double[m_reseau->nombre_sorties()];

		for (int j = 0; j < m_reseau->nombre_sorties(); ++j) {
			m_delta_couches_sorties[i][j] = 0.0;
		}
	}

	/* Création du stockage de gradient. */

	m_gradient_erreur_couches = new double[m_reseau->nombre_couches() + 1];

	for (int i = 0; i <= m_reseau->nombre_couches(); ++i) {
		m_gradient_erreur_couches[i] = 0.0;
	}

	m_gradient_erreur_sorties = new double[m_reseau->nombre_sorties() + 1];

	for (int i = 0; i <= m_reseau->nombre_sorties(); ++i) {
		m_gradient_erreur_sorties[i] = 0.0;
	}
}

EntraineurCerebral::~EntraineurCerebral()
{
	delete [] m_gradient_erreur_couches;
	delete [] m_gradient_erreur_sorties;

	for (int i = 0; i <= m_reseau->nombre_couches(); ++i) {
		delete [] m_delta_couches_sorties[i];
	}

	delete [] m_delta_couches_sorties;

	for (int i = 0; i <= m_reseau->nombre_entrees(); ++i) {
		delete [] m_delta_entrees_couches[i];
	}

	delete [] m_delta_entrees_couches;
}

EntraineurCerebral &EntraineurCerebral::operator=(const EntraineurCerebral &)
{
	/* TODO */
	return *this;
}

double EntraineurCerebral::gradient_erreur_couche(int j)
{
	/* get sum of hidden->output weights * output error gradients  */
	auto somme_pesee = 0.0;

	for (int k = 0; k < m_reseau->nombre_sorties(); ++k) {
		somme_pesee += m_reseau->couches_sorties()[j][k] * m_gradient_erreur_sorties[k];
	}

	return m_reseau->couches()[j] * (1.0 - m_reseau->couches()[j]) * somme_pesee;
}

void EntraineurCerebral::execute_apprentissage(const std::vector<DonneeFormation> &donnees)
{
	for (const DonneeFormation &donnee : donnees) {
		m_reseau->avance(donnee.entrees.data());
		retropojette(donnee.desirees.data());
	}
}

void EntraineurCerebral::retropojette(const double *desirees)
{
	/* Modify deltas between output and hidden. */
	for (int k = 0; k < m_reseau->nombre_sorties(); ++k) {
		const auto gradient = gradient_erreur_sortie(desirees[k], m_reseau->sorties()[k]);
		m_gradient_erreur_sorties[k] = gradient;

		for (int j = 0; j <= m_reseau->nombre_couches(); ++j) {
			const auto delta = m_learning_rate * m_reseau->couches()[j] * gradient
			                   + m_momentum * m_delta_couches_sorties[j][k];

			m_delta_couches_sorties[j][k] = delta;
		}
	}

	/* Modify deltas between hidden and input. */
	for (int j = 0; j < m_reseau->nombre_couches(); ++j) {
		const auto gradient = gradient_erreur_couche(j);
		m_gradient_erreur_couches[j] = gradient;

		for (int i = 0; i <= m_reseau->nombre_entrees(); ++i) {
			const auto delta = m_learning_rate * m_reseau->entrees()[i] * gradient
			                   + m_momentum * m_delta_entrees_couches[i][j];

			m_delta_entrees_couches[i][j] = delta;
		}
	}

	actualise_poids();
}

void EntraineurCerebral::actualise_poids()
{
	for (int i = 0; i <= m_reseau->nombre_entrees(); ++i) {
		for (int j = 0; j < m_reseau->nombre_couches(); ++j) {
			m_reseau->entrees_couches()[i][j] += m_delta_entrees_couches[i][j];
		}
	}

	for (int j = 0; j <= m_reseau->nombre_couches(); ++j) {
		for (int k = 0; k < m_reseau->nombre_sorties(); ++k) {
			m_reseau->couches_sorties()[j][k] += m_delta_couches_sorties[j][k];
		}
	}
}
#endif
