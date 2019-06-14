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

#include "reseau_neuronal.h"

#include <random>

/* ************************************************************************** */

/**
 * Hyperbolic tangent function, remapped to [0, 1].
 */
template <typename T>
static inline T active_signal(const T x)
{
	return (std::tanh(x) + static_cast<T>(1)) / static_cast<T>(2);
}

/* ************************************************************************** */

ReseauNeuronal::ReseauNeuronal(int entrees, int couches, int sorties)
    : m_nombre_entrees(entrees)
    , m_nombre_couches(couches)
    , m_nombre_sorties(sorties)
    , m_entrees(new double[m_nombre_entrees + 1])
    , m_couches(new double[m_nombre_couches + 1])
    , m_sorties(new double[m_nombre_sorties])
    , m_entrees_couches(new double*[m_nombre_entrees + 1])
    , m_couches_sorties(new double*[m_nombre_couches + 1])
{
	/* -------------------- Neurones -------------------- */

	/* Initialisation des neurones d'entrées. */
	for (int i = 0; i < m_nombre_entrees; ++i) {
		m_entrees[i] = 0.0;
	}

	/* Neurone d'entrée de biais. */
	m_entrees[m_nombre_entrees] = -1.0;

	/* Initialisation des neurones cachés. */
	for (int i = 0; i < m_nombre_couches; ++i) {
		m_couches[i] = 0.0;
	}

	/* Poids caché de biais. */
	m_couches[m_nombre_entrees] = -1.0;

	/* Initialisation des neurones cachés. */
	for (int i = 0; i < m_nombre_sorties; ++i) {
		m_sorties[i] = 0.0;
	}

	/* -------------------- Matrices -------------------- */

	for (int i = 0; i <= m_nombre_entrees; ++i) {
		m_entrees_couches[i] = new double[m_nombre_couches];

		for (int j = 0; j < m_nombre_couches; ++j){
			m_entrees_couches[i][j] = 0.0;
		}
	}

	for (int i = 0; i <= m_nombre_couches; ++i)  {
		m_couches_sorties[i] = new double[m_nombre_sorties];

		for (int j = 0; j < m_nombre_sorties; ++j) {
			m_couches_sorties[i][j] = 0.0;
		}
	}

	initialise_poids();
}

ReseauNeuronal::~ReseauNeuronal()
{
	/* Suppression des neurones d'entrées. */
	delete [] m_entrees;

	/* Suppression des neurones cachés. */
	delete [] m_couches;

	/* Suppression des neurones de sorties. */
	delete [] m_sorties;

	/* Suppression de la matrice entrees/couches. */
	for (int i = 0; i <= m_nombre_entrees; ++i) {
		delete [] m_entrees_couches[i];
	}

	delete [] m_entrees_couches;

	/* Suppression de la matrice couches/sorties. */
	for (int i = 0; i <= m_nombre_couches; ++i) {
		delete [] m_couches_sorties[i];
	}

	delete [] m_couches_sorties;
}

ReseauNeuronal &ReseauNeuronal::operator=(const ReseauNeuronal &)
{
	/* TODO */
	return *this;
}

void ReseauNeuronal::initialise_poids()
{
	const auto plage_couche = 1.0 / std::sqrt(static_cast<double>(m_nombre_entrees));
	const auto plage_sortie = 1.0 / std::sqrt(static_cast<double>(m_nombre_couches));

	std::mt19937 gna(30101991);
	std::uniform_real_distribution<double> dist_couche(-plage_couche, plage_couche);
	std::uniform_real_distribution<double> dist_sortie(-plage_sortie, plage_sortie);

	/* Mise des poids entrée/couche. */
	for (int i = 0; i <= m_nombre_entrees; ++i) {
		for (int j = 0; j < m_nombre_couches; ++j) {
			m_entrees_couches[i][j] = dist_couche(gna);
		}
	}

	/* Mise des poids couche/sortie. */
	for (int i = 0; i <= m_nombre_couches; ++i) {
		for (int j = 0; j < m_nombre_sorties; ++j) {
			m_couches_sorties[i][j] = dist_sortie(gna);
		}
	}
}

void ReseauNeuronal::avance(const double *entrees)
{
	/* Initialisation des neurones d'entrées avec les entrées données. */
	for (int i = 0; i < m_nombre_entrees; ++i) {
		m_entrees[i] = entrees[i];
	}

	/* Calcul des valeurs des neurones cachées, incluant le biais. */
	for (int i = 0; i < m_nombre_couches; ++i) {
		/* clear value */
		m_couches[i] = 0.0f;

		/* perform dot product of the two vector */
		for (int j = 0; j <= m_nombre_entrees; ++j) {
			m_couches[i] += (m_entrees[j] * m_entrees_couches[j][i]);
		}

		/* activate */
		m_couches[i] = active_signal(m_couches[i]);
	}

	/* compute ouput layer values */
	for (int i = 0; i < m_nombre_sorties; ++i) {
		/* clear value */
		m_sorties[i] = 0.0f;

		/* perform dot product of the two vector */
		for (int j = 0; j <= m_nombre_couches; ++j) {
			m_sorties[i] += (m_couches[j] * m_couches_sorties[j][i]);
		}

		/* activate */
		m_sorties[i] = active_signal(m_sorties[i]);
	}
}

double *ReseauNeuronal::avance_entrees(double *entrees)
{
	avance(entrees);
	return m_sorties;
}

int ReseauNeuronal::nombre_entrees() const
{
	return m_nombre_entrees;
}

int ReseauNeuronal::nombre_couches() const
{
	return m_nombre_couches;
}

int ReseauNeuronal::nombre_sorties() const
{
	return m_nombre_sorties;
}

double *ReseauNeuronal::entrees()
{
	return m_entrees;
}

double *ReseauNeuronal::couches()
{
	return m_couches;
}

double *ReseauNeuronal::sorties()
{
	return m_sorties;
}

double **ReseauNeuronal::entrees_couches()
{
	return m_entrees_couches;
}

double **ReseauNeuronal::couches_sorties()
{
	return m_couches_sorties;
}
