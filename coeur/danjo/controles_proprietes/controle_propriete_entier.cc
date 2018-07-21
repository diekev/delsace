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
	, m_echelle(new ControleEchelleEntiere())
	, m_pointeur(nullptr)
{
	auto metriques = this->fontMetrics();
	m_bouton->setFixedWidth(metriques.width("H") * 2.0f);
	m_agencement->addWidget(m_bouton);
	m_agencement->addWidget(m_controle);
	setLayout(m_agencement);

	m_echelle->setWindowFlags(Qt::WindowStaysOnTopHint);

	connect(m_controle, &ControleNombreEntier::valeur_changee, this, &ControleProprieteEntier::ajourne_valeur_pointee);
	connect(m_bouton, &QPushButton::pressed, this, &ControleProprieteEntier::montre_echelle);
	connect(m_echelle, &ControleEchelleEntiere::valeur_changee, m_controle, &ControleNombreEntier::ajourne_valeur);
}

ControleProprieteEntier::~ControleProprieteEntier()
{
	delete m_echelle;
}

void ControleProprieteEntier::montre_echelle()
{
	m_echelle->valeur(*m_pointeur);
	m_echelle->plage(m_controle->min(), m_controle->max());
	m_echelle->show();
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

	m_pointeur = static_cast<int *>(donnees.pointeur);

	const auto valeur_defaut = std::atoi(donnees.valeur_defaut.c_str());

	if (donnees.initialisation) {
		*m_pointeur = valeur_defaut;
	}

	m_controle->valeur(*m_pointeur);
	m_controle->suffixe(donnees.suffixe.c_str());

	setToolTip(donnees.infobulle.c_str());
}

void ControleProprieteEntier::ajourne_valeur_pointee(int valeur)
{
	*m_pointeur = valeur;
	Q_EMIT(controle_change());
}

}  /* namespace danjo */
