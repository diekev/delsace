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

class ControleCourbeCouleur;
class ControleEchelleDecimale;
class ControleNombreDecimal;
class CourbeBezier;
class CourbeCouleur;
class QCheckBox;
class QComboBox;
class QHBoxLayout;
class QPushButton;
class QVBoxLayout;

namespace danjo {

class ControleProprieteCourbeCouleur final : public ControlePropriete {
	Q_OBJECT

	/* entreface */
	QVBoxLayout *m_agencement_principal;
	QHBoxLayout *m_agencement_nombre;

	/* courbe */
	QComboBox *m_selection_mode;
	QComboBox *m_selection_type;
	QCheckBox *m_utilise_table;
	ControleCourbeCouleur *m_controle_courbe;

	/* controle de la position X du point sélectionné */
	QPushButton *m_bouton_echelle_x;
	ControleEchelleDecimale *m_echelle_x;
	ControleNombreDecimal *m_pos_x;

	/* controle de la position Y du point sélectionné */
	QPushButton *m_bouton_echelle_y;
	ControleEchelleDecimale *m_echelle_y;
	ControleNombreDecimal *m_pos_y;

	/* connexion */
	CourbeCouleur *m_courbe;
	CourbeBezier *m_courbe_active;

public:
	explicit ControleProprieteCourbeCouleur(QWidget *parent = nullptr);
	~ControleProprieteCourbeCouleur();

	void finalise(const DonneesControle &donnees) override;

private Q_SLOTS:
//	void ajourne_valeur_pointee(float valeur);
	void montre_echelle_x();
	void montre_echelle_y();

	void change_mode_courbe(int mode);
	void change_type_courbe(int type);
	void bascule_utilise_table(bool ouinon);

	void ajourne_position(float x, float y);
	void ajourne_position_x(float v);
	void ajourne_position_y(float v);
	void ajourne_point_actif();
};

}  /* namespace danjo */
