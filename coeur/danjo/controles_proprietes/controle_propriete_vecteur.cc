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

#include "controle_propriete_vecteur.h"

#include <QHBoxLayout>

#include <sstream>

#include "compilation/morceaux.h"

#include "controles/controle_nombre_decimal.h"

#include "controle_propriete_decimal.h"
#include "donnees_controle.h"

namespace danjo {

/* Il s'emblerait que std::atof a du mal à convertir les string en float. */
template <typename T>
static T convertie(const std::string &valeur)
{
	std::istringstream ss(valeur);
	T result;

	ss >> result;

	return result;
}

ControleProprieteVec3::ControleProprieteVec3(QWidget *parent)
	: ControlePropriete(parent)
	, m_agencement(new QHBoxLayout(this))
	, m_x(new ControleNombreDecimal(this))
	, m_y(new ControleNombreDecimal(this))
	, m_z(new ControleNombreDecimal(this))
	, m_pointeur(nullptr)
{
	m_agencement->addWidget(m_x);
	m_agencement->addWidget(m_y);
	m_agencement->addWidget(m_z);

	setLayout(m_agencement);

	connect(m_x, &ControleNombreDecimal::valeur_changee, this, &ControleProprieteVec3::ajourne_valeur_x);
	connect(m_y, &ControleNombreDecimal::valeur_changee, this, &ControleProprieteVec3::ajourne_valeur_y);
	connect(m_z, &ControleNombreDecimal::valeur_changee, this, &ControleProprieteVec3::ajourne_valeur_z);
}

void ControleProprieteVec3::finalise(const DonneesControle &donnees)
{
	auto min = -std::numeric_limits<float>::max();

	if (donnees.valeur_min != "") {
		min = convertie<float>(donnees.valeur_min.c_str());
	}

	auto max = std::numeric_limits<float>::max();

	if (donnees.valeur_max != "") {
		max = convertie<float>(donnees.valeur_max.c_str());
	}

	m_x->ajourne_plage(min, max);
	m_y->ajourne_plage(min, max);
	m_z->ajourne_plage(min, max);

	auto valeurs = decoupe(donnees.valeur_defaut, ',');
	auto index = 0;

	float valeur_defaut[3] = { 0.0f, 0.0f, 0.0f };

	for (auto v : valeurs) {
		valeur_defaut[index++] = std::atof(v.c_str());
	}

	m_pointeur = static_cast<float *>(donnees.pointeur);

	if (donnees.initialisation) {
		m_pointeur[0] = valeur_defaut[0];
		m_pointeur[1] = valeur_defaut[1];
		m_pointeur[2] = valeur_defaut[2];
	}

	m_x->valeur(m_pointeur[0]);
	m_y->valeur(m_pointeur[1]);
	m_z->valeur(m_pointeur[2]);

	setToolTip(donnees.infobulle.c_str());
}

void ControleProprieteVec3::ajourne_valeur_x(float valeur)
{
	m_pointeur[0] = valeur;
	Q_EMIT(controle_change());
}

void ControleProprieteVec3::ajourne_valeur_y(float valeur)
{
	m_pointeur[1] = valeur;
	Q_EMIT(controle_change());
}

void ControleProprieteVec3::ajourne_valeur_z(float valeur)
{
	m_pointeur[2] = valeur;
	Q_EMIT(controle_change());
}

}  /* namespace danjo */
