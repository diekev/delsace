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

class ControleEchelleDecimale;
class ControleNombreDecimal;
class QHBoxLayout;
class QPushButton;

namespace danjo {

class ControleProprieteVec3 final : public ControlePropriete {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	ControleNombreDecimal *m_x;
	ControleNombreDecimal *m_y;
	ControleNombreDecimal *m_z;

	QPushButton *m_bouton_x;
	QPushButton *m_bouton_y;
	QPushButton *m_bouton_z;
	ControleEchelleDecimale *m_echelle_x;
	ControleEchelleDecimale *m_echelle_y;
	ControleEchelleDecimale *m_echelle_z;

	float *m_pointeur;

public:
	explicit ControleProprieteVec3(QWidget *parent = nullptr);
	~ControleProprieteVec3();

	void finalise(const DonneesControle &donnees) override;

private Q_SLOTS:
	void ajourne_valeur_x(float valeur);
	void ajourne_valeur_y(float valeur);
	void ajourne_valeur_z(float valeur);
	void montre_echelle_x();
	void montre_echelle_y();
	void montre_echelle_z();
};

}  /* namespace danjo */
