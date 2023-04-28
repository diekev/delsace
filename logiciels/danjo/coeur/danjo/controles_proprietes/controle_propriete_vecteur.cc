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

#include <QHBoxLayout>
#include <QPushButton>

#include "biblinternes/outils/chaine.hh"

#include "compilation/morceaux.h"

#include "controles/controle_echelle_valeur.h"
#include "controles/controle_nombre_decimal.h"

#include "controle_propriete_decimal.h"
#include "donnees_controle.h"

#include <sstream>

namespace danjo {

ControleProprieteVec3::ControleProprieteVec3(BasePropriete *p, int temps, QWidget *parent)
    : ControlePropriete(p, temps, parent)
	, m_agencement(new QHBoxLayout(this))
	, m_x(new ControleNombreDecimal(this))
	, m_y(new ControleNombreDecimal(this))
	, m_z(new ControleNombreDecimal(this))
	, m_bouton_animation(new QPushButton("C", this))
	, m_bouton_x(new QPushButton("H", this))
	, m_bouton_y(new QPushButton("H", this))
	, m_bouton_z(new QPushButton("H", this))
	, m_echelle_x(new ControleEchelleDecimale())
	, m_echelle_y(new ControleEchelleDecimale())
	, m_echelle_z(new ControleEchelleDecimale())
{
	auto metriques = this->fontMetrics();
	m_bouton_x->setFixedWidth(metriques.horizontalAdvance("H") * 2);
	m_bouton_y->setFixedWidth(metriques.horizontalAdvance("H") * 2);
	m_bouton_z->setFixedWidth(metriques.horizontalAdvance("H") * 2);
	m_bouton_animation->setFixedWidth(metriques.horizontalAdvance("C") * 2);

	m_agencement->addWidget(m_bouton_animation);
	m_agencement->addWidget(m_bouton_x);
	m_agencement->addWidget(m_x);
	m_agencement->addWidget(m_bouton_y);
	m_agencement->addWidget(m_y);
	m_agencement->addWidget(m_bouton_z);
	m_agencement->addWidget(m_z);
	setLayout(m_agencement);

	m_echelle_x->setWindowFlags(Qt::WindowStaysOnTopHint);
	m_echelle_y->setWindowFlags(Qt::WindowStaysOnTopHint);
	m_echelle_z->setWindowFlags(Qt::WindowStaysOnTopHint);

	connect(m_x, &ControleNombreDecimal::valeur_changee, this, &ControleProprieteVec3::ajourne_valeur_x);
	connect(m_y, &ControleNombreDecimal::valeur_changee, this, &ControleProprieteVec3::ajourne_valeur_y);
	connect(m_z, &ControleNombreDecimal::valeur_changee, this, &ControleProprieteVec3::ajourne_valeur_z);
	connect(m_x, &ControleNombreDecimal::prevaleur_changee, this, &ControleProprieteVec3::emet_precontrole_change);
	connect(m_y, &ControleNombreDecimal::prevaleur_changee, this, &ControleProprieteVec3::emet_precontrole_change);
	connect(m_z, &ControleNombreDecimal::prevaleur_changee, this, &ControleProprieteVec3::emet_precontrole_change);

	connect(m_bouton_x, &QPushButton::pressed, this, &ControleProprieteVec3::montre_echelle_x);
	connect(m_bouton_y, &QPushButton::pressed, this, &ControleProprieteVec3::montre_echelle_y);
	connect(m_bouton_z, &QPushButton::pressed, this, &ControleProprieteVec3::montre_echelle_z);

	connect(m_echelle_x, &ControleEchelleDecimale::valeur_changee, m_x, &ControleNombreDecimal::ajourne_valeur);
	connect(m_echelle_y, &ControleEchelleDecimale::valeur_changee, m_y, &ControleNombreDecimal::ajourne_valeur);
	connect(m_echelle_z, &ControleEchelleDecimale::valeur_changee, m_z, &ControleNombreDecimal::ajourne_valeur);
	connect(m_echelle_x, &ControleEchelleDecimale::prevaleur_changee, this, &ControleProprieteVec3::emet_precontrole_change);
	connect(m_echelle_y, &ControleEchelleDecimale::prevaleur_changee, this, &ControleProprieteVec3::emet_precontrole_change);
	connect(m_echelle_z, &ControleEchelleDecimale::prevaleur_changee, this, &ControleProprieteVec3::emet_precontrole_change);

	connect(m_bouton_animation, &QPushButton::pressed, this, &ControleProprieteVec3::bascule_animation);
}

ControleProprieteVec3::~ControleProprieteVec3()
{
	delete m_echelle_x;
	delete m_echelle_y;
	delete m_echelle_z;
}

void ControleProprieteVec3::finalise(const DonneesControle &donnees)
{
    auto plage = m_propriete->plage_valeur_vecteur();
    m_x->ajourne_plage(plage.min, plage.max);
    m_y->ajourne_plage(plage.min, plage.max);
    m_z->ajourne_plage(plage.min, plage.max);

    if (!m_propriete->est_animable()) {
		m_bouton_animation->hide();
    }

	m_animation = m_propriete->est_animee();

	if (m_animation) {
		m_bouton_animation->setText("c");
		auto temps_exacte = m_propriete->possede_cle(m_temps);
		m_x->marque_anime(m_animation, temps_exacte);
		m_y->marque_anime(m_animation, temps_exacte);
		m_z->marque_anime(m_animation, temps_exacte);
		const auto &valeur = m_propriete->evalue_vecteur(m_temps);
		m_x->valeur(valeur[0]);
		m_y->valeur(valeur[1]);
		m_z->valeur(valeur[2]);
	}
	else {
        const auto &valeur = m_propriete->evalue_vecteur(m_temps);
		m_x->valeur(valeur[0]);
		m_y->valeur(valeur[1]);
		m_z->valeur(valeur[2]);
	}

	setToolTip(donnees.infobulle.c_str());
}

void ControleProprieteVec3::montre_echelle_x()
{
	m_echelle_x->valeur(m_x->valeur());
	m_echelle_x->plage(m_x->min(), m_x->max());
	m_echelle_x->show();
}

void ControleProprieteVec3::montre_echelle_y()
{
	m_echelle_y->valeur(m_y->valeur());
	m_echelle_y->plage(m_y->min(), m_y->max());
	m_echelle_y->show();
}

void ControleProprieteVec3::montre_echelle_z()
{
	m_echelle_z->valeur(m_z->valeur());
	m_echelle_z->plage(m_z->min(), m_z->max());
	m_echelle_z->show();
}

void ControleProprieteVec3::bascule_animation()
{
	m_animation = !m_animation;

	if (m_animation == false) {
		m_propriete->supprime_animation();
        const auto &valeur = m_propriete->evalue_vecteur(m_temps);
		m_x->valeur(valeur[0]);
		m_y->valeur(valeur[1]);
		m_z->valeur(valeur[2]);
		m_bouton_animation->setText("C");
	}
	else {
        m_propriete->ajoute_cle(m_propriete->evalue_vecteur(m_temps), m_temps);
		m_bouton_animation->setText("c");
	}

	m_x->marque_anime(m_animation, m_animation);
	m_y->marque_anime(m_animation, m_animation);
	m_z->marque_anime(m_animation, m_animation);
}

void ControleProprieteVec3::ajourne_valeur_x(float valeur)
{
	auto vec = dls::math::vec3f(valeur, m_y->valeur(), m_z->valeur());

	if (m_animation) {
		m_propriete->ajoute_cle(vec, m_temps);
	}
	else {
        m_propriete->définit_valeur_vec3(vec);
	}

	Q_EMIT(controle_change());
}

void ControleProprieteVec3::ajourne_valeur_y(float valeur)
{
	auto vec = dls::math::vec3f(m_x->valeur(), valeur, m_z->valeur());

	if (m_animation) {
		m_propriete->ajoute_cle(vec, m_temps);
	}
    else {
        m_propriete->définit_valeur_vec3(vec);
	}

	Q_EMIT(controle_change());
}

void ControleProprieteVec3::ajourne_valeur_z(float valeur)
{
	auto vec = dls::math::vec3f(m_x->valeur(), m_y->valeur(), valeur);

	if (m_animation) {
		m_propriete->ajoute_cle(vec, m_temps);
	}
    else {
        m_propriete->définit_valeur_vec3(vec);
	}

	Q_EMIT(controle_change());
}

}  /* namespace danjo */
