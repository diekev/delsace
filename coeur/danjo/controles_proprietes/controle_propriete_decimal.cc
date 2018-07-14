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

#include "controle_propriete_decimal.h"

#include <QHBoxLayout>
#include <QSlider>
#include <QDoubleSpinBox>

#include <sstream>

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

SelecteurFloat::SelecteurFloat(QWidget *parent)
	: ControlePropriete(parent)
	, m_agencement(new QHBoxLayout(this))
	, m_spin_box(new QDoubleSpinBox(this))
	, m_slider(new QSlider(Qt::Orientation::Horizontal, this))
	, m_scale(1.0f)
{
	m_agencement->addWidget(m_spin_box);
	m_agencement->addWidget(m_slider);

	setLayout(m_agencement);

	m_spin_box->setAlignment(Qt::AlignRight);
	m_spin_box->setButtonSymbols(QAbstractSpinBox::NoButtons);
	m_spin_box->setReadOnly(true);

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	connect(m_slider, SIGNAL(sliderReleased()), this, SLOT(ValueChanged()));
	connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(updateLabel(int)));
}

void SelecteurFloat::finalise(const DonneesControle &)
{
	setRange(m_min, m_max);
}

void SelecteurFloat::ValueChanged()
{
	const auto value = m_slider->value();
	const float fvalue = value / m_scale;
	m_spin_box->setValue(fvalue);
	Q_EMIT(valeur_changee(fvalue));
}

void SelecteurFloat::updateLabel(int value)
{
	m_spin_box->setValue(value / m_scale);
}

void SelecteurFloat::valeur(float value)
{
	m_spin_box->setValue(value);
	m_slider->setValue(value * m_scale);
}

float SelecteurFloat::valeur() const
{
	return m_spin_box->value();
}

void SelecteurFloat::setRange(float min, float max)
{
	if (min > 0.0f && min < 1.0f) {
		m_scale = 1.0f / min;
	}
	else {
		m_scale = 10000.0f;
	}

	m_slider->setRange(min * m_scale, max * m_scale);
	m_spin_box->setRange(min * m_scale, max * m_scale);
}

ControleProprieteDecimal::ControleProprieteDecimal(QWidget *parent)
	: SelecteurFloat(parent)
	, m_pointeur(nullptr)
{
	valeur(0.0);
	connect(this, &SelecteurFloat::valeur_changee, this, &ControleProprieteDecimal::ajourne_valeur_pointee);
}

void ControleProprieteDecimal::ajourne_valeur_pointee(double valeur)
{
	*m_pointeur = static_cast<float>(valeur);
	Q_EMIT(controle_change());
}

void ControleProprieteDecimal::finalise(const DonneesControle &donnees)
{
	m_min = convertie<float>(donnees.valeur_min);
	m_max = convertie<float>(donnees.valeur_max);

	setRange(m_min, m_max);

	m_pointeur = static_cast<float *>(donnees.pointeur);

	const auto valeur_defaut = convertie<float>(donnees.valeur_defaut);

	if (donnees.initialisation) {
		*m_pointeur = valeur_defaut;
	}

	valeur(*m_pointeur);

	setToolTip(donnees.infobulle.c_str());
}

}  /* namespace danjo */
