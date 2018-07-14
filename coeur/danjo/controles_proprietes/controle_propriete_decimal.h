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

#pragma once

#include "controle_propriete.h"

class QHBoxLayout;
class QDoubleSpinBox;
class QSlider;

namespace danjo {

class SelecteurFloat : public ControlePropriete {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	QDoubleSpinBox *m_spin_box;
	QSlider *m_slider;

protected:
	float m_scale;
	float m_min;
	float m_max;

public:
	explicit SelecteurFloat(QWidget *parent = nullptr);
	~SelecteurFloat() = default;

	void finalise(const DonneesControle &) override;

	void valeur(float value);
	float valeur() const;

	void setRange(float min, float max);

Q_SIGNALS:
	void valeur_changee(double value);

private Q_SLOTS:
	void ValueChanged();
	void updateLabel(int value);
};

class ControleProprieteDecimal final : public SelecteurFloat {
	Q_OBJECT

	float *m_pointeur;

public:
	explicit ControleProprieteDecimal(QWidget *parent = nullptr);
	~ControleProprieteDecimal() = default;

	void finalise(const DonneesControle &donnees) override;

private Q_SLOTS:
	void ajourne_valeur_pointee(double valeur);
};

}  /* namespace danjo */
