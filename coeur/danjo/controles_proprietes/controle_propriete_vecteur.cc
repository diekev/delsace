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

#include <QVBoxLayout>

#include <sstream>

#include "compilation/morceaux.h"

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

SelecteurVec3::SelecteurVec3(QWidget *parent)
	: ControlePropriete(parent)
	, m_x(new SelecteurFloat(this))
	, m_y(new SelecteurFloat(this))
	, m_z(new SelecteurFloat(this))
	, m_agencement(new QVBoxLayout(this))
{
	connect(m_x, SIGNAL(valeur_changee(double)), this, SLOT(xValueChanged(double)));
	connect(m_y, SIGNAL(valeur_changee(double)), this, SLOT(yValueChanged(double)));
	connect(m_z, SIGNAL(valeur_changee(double)), this, SLOT(zValueChanged(double)));

	m_agencement->addWidget(m_x);
	m_agencement->addWidget(m_y);
	m_agencement->addWidget(m_z);

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	m_agencement->setSpacing(0);
}

void SelecteurVec3::xValueChanged(double value)
{
	Q_EMIT(valeur_changee(value, AXIS_X));
}

void SelecteurVec3::yValueChanged(double value)
{
	Q_EMIT(valeur_changee(value, AXIS_Y));
}

void SelecteurVec3::zValueChanged(double value)
{
	Q_EMIT(valeur_changee(value, AXIS_Z));
}

void SelecteurVec3::setValue(float *value)
{
	m_x->valeur(value[AXIS_X]);
	m_y->valeur(value[AXIS_Y]);
	m_z->valeur(value[AXIS_Z]);
}

void SelecteurVec3::getValue(float *value) const
{
	value[AXIS_X] = m_x->valeur();
	value[AXIS_Y] = m_y->valeur();
	value[AXIS_Z] = m_z->valeur();
}

void SelecteurVec3::setMinMax(float min, float max) const
{
	m_x->setRange(min, max);
	m_y->setRange(min, max);
	m_z->setRange(min, max);
}

ControleProprieteVec3::ControleProprieteVec3(QWidget *parent)
	: SelecteurVec3(parent)
	, m_pointeur(nullptr)
{
	connect(this, &SelecteurVec3::valeur_changee, this, &ControleProprieteVec3::ajourne_valeur_pointee);
}

void ControleProprieteVec3::finalise(const DonneesControle &donnees)
{
	m_pointeur = static_cast<float *>(donnees.pointeur);

	auto min = 0.0f;

	if (donnees.valeur_min != "") {
		min = convertie<float>(donnees.valeur_min);
	}

	auto max = min + 1.0f;

	if (donnees.valeur_max != "") {
		max = convertie<float>(donnees.valeur_max);
	}

	setMinMax(min, max);

	auto valeurs = decoupe(donnees.valeur_defaut, ',');
	auto index = 0;

	float valeur_defaut[3] = { 0.0f, 0.0f, 0.0f };

	for (auto v : valeurs) {
		valeur_defaut[index++] = std::atof(v.c_str());
	}

	if (donnees.initialisation) {
		m_pointeur[0] = valeur_defaut[0];
		m_pointeur[1] = valeur_defaut[1];
		m_pointeur[2] = valeur_defaut[2];
	}

	setValue(m_pointeur);

	setToolTip(donnees.infobulle.c_str());
}

void ControleProprieteVec3::ajourne_valeur_pointee(double valeur, int axis)
{
	m_pointeur[axis] = static_cast<float>(valeur);
	Q_EMIT(controle_change());
}

}  /* namespace danjo */
