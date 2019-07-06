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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "editeur_rendu.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#pragma GCC diagnostic pop

#include "biblinternes/commandes/repondant_commande.h"

#include "coeur/evenement.h"
#include "coeur/koudou.h"

#include "outils.h"

EditriceRendu::EditriceRendu(Koudou &koudou, QWidget *parent)
	: BaseEditrice(koudou, parent)
    , m_widget(new QWidget())
    , m_scroll(new QScrollArea())
    , m_glayout(new QGridLayout(m_widget))
	, m_assembleur_controles(m_glayout)
	, m_info_temps_ecoule(new QLabel("0 s", this))
	, m_info_temps_restant(new QLabel("0 s", this))
	, m_info_temps_echantillon(new QLabel("0 s", this))
	, m_info_echantillon(new QLabel("0 / 0", this))
{
	m_widget->setSizePolicy(m_cadre->sizePolicy());

	m_scroll->setWidget(m_widget);
	m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_scroll->setWidgetResizable(true);

	/* Hide scroll area's frame. */
	m_scroll->setFrameStyle(0);

	m_agencement_principal->addWidget(m_scroll);

	QPushButton *bouton = new QPushButton("Démarrer rendu", this);
	m_glayout->addWidget(bouton, 0, 0);

	connect(bouton, &QPushButton::clicked, this, &EditriceRendu::demarre_rendu);

	bouton = new QPushButton("Arrêter rendu", this);
	m_glayout->addWidget(bouton, 0, 1);

	connect(bouton, &QPushButton::clicked, this, &EditriceRendu::arrete_rendu);

	/* Temps restant. */

	auto label_echantillon = new QLabel("Échantillon", this);
	m_glayout->addWidget(label_echantillon, 1, 0);

	m_glayout->addWidget(m_info_echantillon, 1, 1);

	/* Temps échantillon. */

	auto label_temps_echantillon = new QLabel("Temps échantillon", this);
	m_glayout->addWidget(label_temps_echantillon, 2, 0);

	m_glayout->addWidget(m_info_temps_echantillon, 2, 1);

	/* Temps écoulé. */

	auto label_temps_ecoule = new QLabel("Temps écoulé", this);
	m_glayout->addWidget(label_temps_ecoule, 3, 0);

	m_glayout->addWidget(m_info_temps_ecoule, 3, 1);

	/* Temps restant. */

	auto label_temps_restant = new QLabel("Temps restant", this);
	m_glayout->addWidget(label_temps_restant, 4, 0);

	m_glayout->addWidget(m_info_temps_restant, 4, 1);
}

void EditriceRendu::ajourne_etat(int event)
{
	if (event != type_evenement::rafraichissement) {
		return;
	}

	auto informations_rendu = m_koudou->informations_rendu;

	auto texte_echantillon = QString::number(informations_rendu.echantillon);
	texte_echantillon += " / ";
	texte_echantillon += QString::number(m_koudou->parametres_rendu.nombre_echantillons);

	m_info_echantillon->setText(texte_echantillon);

	m_info_temps_echantillon->setText(QString::number(informations_rendu.temps_echantillon) + " s");
	m_info_temps_ecoule->setText(QString::number(informations_rendu.temps_ecoule) + " s");
	m_info_temps_restant->setText(QString::number(informations_rendu.temps_restant) + " s");
}

void EditriceRendu::demarre_rendu()
{
	m_koudou->repondant_commande->repond_clique("demarre_rendu", "");
}

void EditriceRendu::arrete_rendu()
{
	m_koudou->repondant_commande->repond_clique("arrete_rendu", "");
}
