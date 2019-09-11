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
#include <QPushButton>

#include "controles/controle_echelle_valeur.h"
#include "controles/controle_nombre_entier.h"

#include "donnees_controle.h"

namespace danjo {

ControleProprieteEntier::ControleProprieteEntier(QWidget *parent)
	: ControlePropriete(parent)
	, m_agencement(new QHBoxLayout(this))
	, m_controle(new ControleNombreEntier(this))
	, m_bouton(new QPushButton("H", this))
	, m_bouton_animation(new QPushButton("C", this))
	, m_echelle(new ControleEchelleEntiere())
{
	auto metriques = this->fontMetrics();

	m_bouton->setFixedWidth(metriques.width("H") * 2);
	m_bouton_animation->setFixedWidth(metriques.width("C") * 2);

	m_agencement->addWidget(m_bouton_animation);
	m_agencement->addWidget(m_bouton);
	m_agencement->addWidget(m_controle);
	setLayout(m_agencement);

	m_echelle->setWindowFlags(Qt::WindowStaysOnTopHint);

	connect(m_controle, &ControleNombreEntier::valeur_changee, this, &ControleProprieteEntier::ajourne_valeur_pointee);
	connect(m_controle, &ControleNombreEntier::prevaleur_changee, this, &ControleProprieteEntier::emet_precontrole_change);
	connect(m_bouton, &QPushButton::pressed, this, &ControleProprieteEntier::montre_echelle);
	connect(m_echelle, &ControleEchelleEntiere::valeur_changee, m_controle, &ControleNombreEntier::ajourne_valeur);
	connect(m_echelle, &ControleEchelleEntiere::prevaleur_changee, this, &ControleProprieteEntier::emet_precontrole_change);
	connect(m_bouton_animation, &QPushButton::pressed, this, &ControleProprieteEntier::bascule_animation);
}

ControleProprieteEntier::~ControleProprieteEntier()
{
	delete m_echelle;
}

void ControleProprieteEntier::montre_echelle()
{
	m_echelle->valeur(std::any_cast<int>(m_propriete->valeur));
	m_echelle->plage(m_controle->min(), m_controle->max());
	m_echelle->show();
}

void ControleProprieteEntier::bascule_animation()
{
	Q_EMIT(precontrole_change());
	m_animation = !m_animation;

	if (m_animation == false) {
		m_propriete->supprime_animation();
		m_controle->valeur(std::any_cast<int>(m_propriete->valeur));
		m_bouton_animation->setText("C");
	}
	else {
		m_propriete->ajoute_cle(std::any_cast<int>(m_propriete->valeur), m_temps);
		m_bouton_animation->setText("c");
	}

	m_controle->marque_anime(m_animation, m_animation);
	Q_EMIT(controle_change());
}

void ControleProprieteEntier::finalise(const DonneesControle &donnees)
{
	auto min = std::numeric_limits<int>::min();

	if (donnees.valeur_min != "") {
		min = std::atoi(donnees.valeur_min.c_str());
	}

	auto max = std::numeric_limits<int>::max();

	if (donnees.valeur_max != "") {
		max = std::atoi(donnees.valeur_max.c_str());
	}

	m_controle->ajourne_plage(min, max);

	if (donnees.initialisation) {
		m_propriete->valeur = std::atoi(donnees.valeur_defaut.c_str());
	}

	if ((m_propriete->etat & etat_propriete::EST_ANIMABLE) == etat_propriete::VIERGE) {
		m_bouton_animation->hide();
	}

	m_animation = m_propriete->est_animee();

	if (m_animation) {
		m_bouton_animation->setText("c");
		m_controle->marque_anime(m_animation, m_propriete->possede_cle(m_temps));
		m_controle->valeur(m_propriete->evalue_entier(m_temps));
	}
	else {
		m_controle->valeur(std::any_cast<int>(m_propriete->valeur));
	}

	m_controle->suffixe(donnees.suffixe.c_str());

	setToolTip(donnees.infobulle.c_str());
}

void ControleProprieteEntier::ajourne_valeur_pointee(int valeur)
{
	if (m_animation) {
		m_propriete->ajoute_cle(valeur, m_temps);
	}
	else {
		m_propriete->valeur = valeur;
	}

	Q_EMIT(controle_change());
}

}  /* namespace danjo */
