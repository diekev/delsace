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
class QSpinBox;
class QSlider;

namespace danjo {

class SelecteurInt : public ControlePropriete {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	QSpinBox *m_spin_box;
	QSlider *m_slider;

protected:
	int m_min, m_max;

public:
	explicit SelecteurInt(QWidget *parent = nullptr);
	~SelecteurInt() = default;

	void finalise(const DonneesControle &) override;

	void setValue(int value);
	int value() const;
	void setRange(int min, int max);

Q_SIGNALS:
	void valeur_changee(int value);

private Q_SLOTS:
	void ValueChanged();
	void updateLabel(int value);
};

class ControleProprieteEntier final : public SelecteurInt {
	Q_OBJECT

	int *m_pointeur;

public:
	explicit ControleProprieteEntier(QWidget *parent = nullptr);
	~ControleProprieteEntier() = default;

	void finalise(const DonneesControle &donnees) override;

private Q_SLOTS:
	void ajourne_valeur_pointee(int valeur);
};

}  /* namespace danjo */
