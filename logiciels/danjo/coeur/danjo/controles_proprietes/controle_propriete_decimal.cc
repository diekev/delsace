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

#include "controle_propriete_decimal.h"

#include <QHBoxLayout>
#include <QPushButton>

#include <sstream>

#include "controles/controle_echelle_valeur.h"
#include "controles/controle_nombre_decimal.h"

#include "../manipulable.h"

#include "donnees_controle.h"

namespace danjo {

/* Il s'emblerait que std::atof a du mal à convertir les string en float. */
template <typename T>
static T convertie(const dls::chaine &valeur)
{
	std::istringstream ss(valeur.c_str());
	T result;

	ss >> result;

	return result;
}

ControleProprieteDecimal::ControleProprieteDecimal(QWidget *parent)
	: ControlePropriete(parent)
	, m_agencement(new QHBoxLayout(this))
	, m_controle(new ControleNombreDecimal(this))
	, m_bouton(new QPushButton("H", this))
	, m_bouton_animation(new QPushButton("C", this))
	, m_echelle(new ControleEchelleDecimale())
{
	auto metriques = this->fontMetrics();

	m_bouton->setFixedWidth(metriques.width("H") * 2);
	m_bouton_animation->setFixedWidth(metriques.width("C") * 2);

	m_agencement->addWidget(m_bouton_animation);
	m_agencement->addWidget(m_bouton);
	m_agencement->addWidget(m_controle);
	setLayout(m_agencement);

	m_echelle->setWindowFlags(Qt::WindowStaysOnTopHint);

	connect(m_controle, &ControleNombreDecimal::valeur_changee, this, &ControleProprieteDecimal::ajourne_valeur_pointee);
	connect(m_bouton, &QPushButton::pressed, this, &ControleProprieteDecimal::montre_echelle);
	connect(m_echelle, &ControleEchelleDecimale::valeur_changee, m_controle, &ControleNombreDecimal::ajourne_valeur);
	connect(m_bouton_animation, &QPushButton::pressed, this, &ControleProprieteDecimal::bascule_animation);
}

ControleProprieteDecimal::~ControleProprieteDecimal()
{
	delete m_echelle;
}

void ControleProprieteDecimal::ajourne_valeur_pointee(float valeur)
{
	if (m_animation) {
		m_propriete->ajoute_cle(valeur, m_temps);
	}
	else {
		m_propriete->valeur = valeur;
	}

	Q_EMIT(controle_change());
}

void ControleProprieteDecimal::montre_echelle()
{
	m_echelle->valeur(std::experimental::any_cast<float>(m_propriete->valeur));
	m_echelle->plage(m_controle->min(), m_controle->max());
	m_echelle->show();
}

void ControleProprieteDecimal::bascule_animation()
{
	m_animation = !m_animation;

	if (m_animation == false) {
		m_propriete->supprime_animation();
		m_controle->valeur(std::experimental::any_cast<float>(m_propriete->valeur));
		m_bouton_animation->setText("C");
	}
	else {
		m_propriete->ajoute_cle(std::experimental::any_cast<float>(m_propriete->valeur), m_temps);
		m_bouton_animation->setText("c");
	}

	m_controle->marque_anime(m_animation, m_animation);
}

void ControleProprieteDecimal::finalise(const DonneesControle &donnees)
{
	auto min = -std::numeric_limits<float>::max();

	if (donnees.valeur_min != "") {
		min = convertie<float>(donnees.valeur_min.c_str());
	}

	auto max = std::numeric_limits<float>::max();

	if (donnees.valeur_max != "") {
		max = convertie<float>(donnees.valeur_max.c_str());
	}

	m_controle->ajourne_plage(min, max);

	if (donnees.initialisation) {
		m_propriete->valeur = convertie<float>(donnees.valeur_defaut);
	}

	m_animation = m_propriete->est_anime();

	if (m_animation) {
		m_bouton_animation->setText("c");
		m_controle->marque_anime(m_animation, m_propriete->possede_cle(m_temps));
		m_controle->valeur(m_propriete->evalue_decimal(m_temps));
	}
	else {
		m_controle->valeur(std::experimental::any_cast<float>(m_propriete->valeur));
	}

	m_controle->suffixe(donnees.suffixe.c_str());

	setToolTip(donnees.infobulle.c_str());
}

}  /* namespace danjo */
