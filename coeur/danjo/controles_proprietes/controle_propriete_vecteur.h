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

class QVBoxLayout;

namespace danjo {

class SelecteurFloat;

class SelecteurVec3 : public ControlePropriete {
	Q_OBJECT

	SelecteurFloat *m_x, *m_y, *m_z;
	QVBoxLayout *m_agencement;

private Q_SLOTS:
	void xValueChanged(double value);
	void yValueChanged(double value);
	void zValueChanged(double value);

Q_SIGNALS:
	void valeur_changee(double value, int axis);

public:
	explicit SelecteurVec3(QWidget *parent = nullptr);
	~SelecteurVec3() = default;

	void setValue(float *value);
	void getValue(float *value) const;
	void setMinMax(float min, float max) const;
};

class ControleProprieteVec3 final : public SelecteurVec3 {
	Q_OBJECT

	float *m_pointeur;

public:
	explicit ControleProprieteVec3(QWidget *parent = nullptr);
	~ControleProprieteVec3() = default;

	void finalise(const DonneesControle &donnees) override;

private Q_SLOTS:
	void ajourne_valeur_pointee(double valeur, int axis);
};

}  /* namespace danjo */
