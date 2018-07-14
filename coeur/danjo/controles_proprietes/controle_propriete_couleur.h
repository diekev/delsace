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

namespace danjo {

class SelecteurCouleur : public ControlePropriete {
	Q_OBJECT

	float m_min = 0.0f;
	float m_max = 0.0f;

protected:
	float m_valeur_defaut[4];
	float *m_couleur = nullptr;

public:
	explicit SelecteurCouleur(QWidget *parent = nullptr);

	~SelecteurCouleur() = default;

	void mouseReleaseEvent(QMouseEvent *e) override;

	void finalise(const DonneesControle &donnees) override;

	void paintEvent(QPaintEvent *) override;

Q_SIGNALS:
	void clicked();
	void valeur_changee(double value, int axis);
};

class ControleProprieteCouleur final : public SelecteurCouleur {
	Q_OBJECT

	float *m_pointeur;

public:
	explicit ControleProprieteCouleur(QWidget *parent = nullptr);
	~ControleProprieteCouleur() = default;

private Q_SLOTS:
	void ajourne_valeur_pointee(double valeur, int axis);
};

}  /* namespace danjo */
