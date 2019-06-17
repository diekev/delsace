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

#include "controle_propriete_courbe_valeur.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QPushButton>

#include "controles/controle_courbe_couleur.h"
#include "controles/controle_echelle_valeur.h"
#include "controles/controle_nombre_decimal.h"

#include "types/courbe_bezier.h"

#include "donnees_controle.h"

namespace danjo {

ControleProprieteCourbeValeur::ControleProprieteCourbeValeur(QWidget *parent)
	: ControlePropriete(parent)
	, m_agencement_principal(new QVBoxLayout())
	, m_agencement_nombre(new QHBoxLayout())
	, m_utilise_table(new QCheckBox("Utilise table", this))
	, m_controle_courbe(new ControleCourbeCouleur(this))
	, m_bouton_echelle_x(new QPushButton("H", this))
	, m_echelle_x(new ControleEchelleDecimale())
	, m_pos_x(new ControleNombreDecimal(this))
	, m_bouton_echelle_y(new QPushButton("H", this))
	, m_echelle_y(new ControleEchelleDecimale())
	, m_pos_y(new ControleNombreDecimal(this))
{
	m_agencement_principal->addWidget(m_utilise_table);
	m_agencement_principal->addWidget(m_controle_courbe);

	auto metriques = this->fontMetrics();
	m_bouton_echelle_x->setFixedWidth(metriques.width("H") * 2);
	m_bouton_echelle_y->setFixedWidth(metriques.width("H") * 2);

	m_agencement_nombre->addWidget(m_bouton_echelle_x);
	m_agencement_nombre->addWidget(m_pos_x);
	m_agencement_nombre->addWidget(m_bouton_echelle_y);
	m_agencement_nombre->addWidget(m_pos_y);
	m_agencement_principal->addLayout(m_agencement_nombre);

	m_echelle_x->setWindowFlags(Qt::WindowStaysOnTopHint);
	m_echelle_y->setWindowFlags(Qt::WindowStaysOnTopHint);

	connect(m_utilise_table, &QCheckBox::clicked,
			this, &ControleProprieteCourbeValeur::bascule_utilise_table);

	connect(m_controle_courbe, &ControleCourbeCouleur::position_changee,
			this, &ControleProprieteCourbeValeur::ajourne_position);

	connect(m_controle_courbe, &ControleCourbeCouleur::point_change,
			this, &ControleProprieteCourbeValeur::ajourne_point_actif);

	connect(m_bouton_echelle_x, &QPushButton::pressed,
			this, &ControleProprieteCourbeValeur::montre_echelle_x);

	connect(m_bouton_echelle_y, &QPushButton::pressed,
			this, &ControleProprieteCourbeValeur::montre_echelle_y);

	connect(m_echelle_x, &ControleEchelleDecimale::valeur_changee,
			m_pos_x, &ControleNombreDecimal::ajourne_valeur);

	connect(m_echelle_y, &ControleEchelleDecimale::valeur_changee,
			m_pos_y, &ControleNombreDecimal::ajourne_valeur);

	connect(m_pos_x, &ControleNombreDecimal::valeur_changee,
			this, &ControleProprieteCourbeValeur::ajourne_position_x);

	connect(m_pos_y, &ControleNombreDecimal::valeur_changee,
			this, &ControleProprieteCourbeValeur::ajourne_position_y);

	setLayout(m_agencement_principal);
}

ControleProprieteCourbeValeur::~ControleProprieteCourbeValeur()
{
	delete m_echelle_x;
	delete m_echelle_y;
}

void ControleProprieteCourbeValeur::finalise(const DonneesControle &donnees)
{
	/* À FAIRE : min/max */
	m_courbe = static_cast<CourbeBezier *>(donnees.pointeur);
	m_controle_courbe->installe_courbe(m_courbe);
	m_utilise_table->setChecked(m_courbe->utilise_table);
	m_pos_x->ajourne_plage(m_courbe->valeur_min, m_courbe->valeur_max);
	m_pos_y->ajourne_plage(m_courbe->valeur_min, m_courbe->valeur_max);
	setToolTip(donnees.infobulle.c_str());
}

void ControleProprieteCourbeValeur::montre_echelle_x()
{
	m_echelle_x->valeur(m_pos_x->valeur());
	m_echelle_x->plage(m_pos_x->min(), m_pos_x->max());
	m_echelle_x->show();
}

void ControleProprieteCourbeValeur::montre_echelle_y()
{
	m_echelle_y->valeur(m_pos_y->valeur());
	m_echelle_y->plage(m_pos_y->min(), m_pos_y->max());
	m_echelle_y->show();
}

void ControleProprieteCourbeValeur::bascule_utilise_table(bool ouinon)
{
	m_courbe->utilise_table = ouinon;
	Q_EMIT(controle_change());
}

void ControleProprieteCourbeValeur::ajourne_position(float x, float y)
{
	m_pos_x->valeur(x);
	m_pos_y->valeur(y);

	Q_EMIT(controle_change());
}

void ControleProprieteCourbeValeur::ajourne_position_x(float v)
{
	m_controle_courbe->ajourne_position_x(v);
	Q_EMIT(controle_change());
}

void ControleProprieteCourbeValeur::ajourne_position_y(float v)
{
	m_controle_courbe->ajourne_position_y(v);
	Q_EMIT(controle_change());
}

void ControleProprieteCourbeValeur::ajourne_point_actif()
{
	m_pos_x->valeur(m_courbe->point_courant->co[POINT_CENTRE].x);
	m_pos_y->valeur(m_courbe->point_courant->co[POINT_CENTRE].y);
}

}  /* namespace danjo */
