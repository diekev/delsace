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

#include "controle_propriete_entier.h"

#include <QHBoxLayout>
#include <QSlider>
#include <QSpinBox>

#include "donnees_controle.h"

namespace danjo {

SelecteurInt::SelecteurInt(QWidget *parent)
	: ControlePropriete(parent)
	, m_agencement(new QHBoxLayout(this))
	, m_spin_box(new QSpinBox(this))
	, m_slider(new QSlider(Qt::Orientation::Horizontal, this))
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

void SelecteurInt::finalise(const DonneesControle &)
{

}

void SelecteurInt::ValueChanged()
{
	const auto value = m_slider->value();
	m_spin_box->setValue(value);
	Q_EMIT(valeur_changee(value));
}

void SelecteurInt::updateLabel(int value)
{
	m_spin_box->setValue(value);
}

void SelecteurInt::setValue(int value)
{
	m_spin_box->setValue(value);
	m_slider->setValue(value);
}

int SelecteurInt::value() const
{
	return m_spin_box->value();
}

void SelecteurInt::setRange(int min, int max)
{
	m_slider->setRange(min, max);
	m_spin_box->setRange(min, max);
}

ControleProprieteEntier::ControleProprieteEntier(QWidget *parent)
	: SelecteurInt(parent)
	, m_pointeur(nullptr)
{
	connect(this, &SelecteurInt::valeur_changee, this, &ControleProprieteEntier::ajourne_valeur_pointee);
}

void ControleProprieteEntier::finalise(const DonneesControle &donnees)
{
	m_min = std::atoi(donnees.valeur_min.c_str());
	m_max = std::atoi(donnees.valeur_max.c_str());

	setRange(m_min, m_max);

	m_pointeur = static_cast<int *>(donnees.pointeur);

	const auto valeur_defaut = std::atoi(donnees.valeur_defaut.c_str());

	if (donnees.initialisation) {
		*m_pointeur = valeur_defaut;
	}

	setValue(*m_pointeur);

	setToolTip(donnees.infobulle.c_str());
}

void ControleProprieteEntier::ajourne_valeur_pointee(int valeur)
{
	*m_pointeur = valeur;
	Q_EMIT(controle_change());
}

}  /* namespace danjo */
