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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "editeur_ligne_temps.h"

#include "danjo/danjo.h"
#include "danjo/manipulable.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QTimer>
#pragma GCC diagnostic pop

#include "biblinternes/patrons_conception/repondant_commande.h"
#include "biblinternes/outils/fichier.hh"

#include "coeur/evenement.h"
#include "coeur/fluide.h"
#include "coeur/poseidon.h"

EditriceLigneTemps::EditriceLigneTemps(Poseidon *poseidon, QWidget *parent)
	: BaseEditrice(*poseidon, parent)
	, m_slider(new QSlider(m_cadre))
	, m_tc_layout(new QHBoxLayout())
	, m_num_layout(new QHBoxLayout())
	, m_vbox_layout(new QVBoxLayout())
	, m_end_frame(new QSpinBox(m_cadre))
	, m_start_frame(new QSpinBox(m_cadre))
	, m_cur_frame(new QSpinBox(m_cadre))
	, m_fps(new QDoubleSpinBox(m_cadre))
	, m_timer(new QTimer(this))
{;

	m_agencement_principal->addLayout(m_vbox_layout);

	m_num_layout->setSizeConstraint(QLayout::SetMinimumSize);

	/* ------------------------------ jog bar ------------------------------- */

	m_slider->setMouseTracking(false);
	m_slider->setValue(0);
	m_slider->setOrientation(Qt::Horizontal);
	m_slider->setTickPosition(QSlider::TicksBothSides);
	m_slider->setTickInterval(0);
	m_slider->setMaximum(250);

	m_start_frame->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	m_start_frame->setMaximum(500000);
	m_start_frame->setValue(0);
	m_start_frame->setToolTip("Start Frame");

	m_end_frame->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	m_end_frame->setMaximum(500000);
	m_end_frame->setValue(250);
	m_start_frame->setToolTip("End Frame");

	m_num_layout->addWidget(m_start_frame);
	m_num_layout->addWidget(m_slider);
	m_num_layout->addWidget(m_end_frame);

	m_vbox_layout->addLayout(m_num_layout);

	/* ------------------------- current selection -------------------------- */

	m_cur_frame->setAlignment(Qt::AlignCenter);
	m_cur_frame->setReadOnly(true);
	m_cur_frame->setButtonSymbols(QAbstractSpinBox::NoButtons);
	m_cur_frame->setProperty("showGroupSeparator", QVariant(false));
	m_cur_frame->setMaximum(500000);
	m_cur_frame->setToolTip("Current Frame");

	m_tc_layout->addWidget(m_cur_frame);

	/* ------------------------- transport controls ------------------------- */

	m_tc_layout->addStretch();

	danjo::Manipulable dummy;

	danjo::DonneesInterface donnees{};
	donnees.conteneur = nullptr;
	donnees.manipulable = &dummy;
	donnees.repondant_bouton = poseidon->repondant_commande;

	auto text_entree = dls::contenu_fichier("scripts/disposition_ligne_temps.jo");
	auto disp_controles = danjo::compile_entreface(donnees, text_entree.c_str());

	m_tc_layout->addLayout(disp_controles);

	m_tc_layout->addStretch();

	/* --------------------------------- fps -------------------------------- */

	m_fps->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	m_fps->setValue(24);
	m_fps->setToolTip("Frame Rate");

	m_tc_layout->addWidget(m_fps);

	m_vbox_layout->addLayout(m_tc_layout);

	/* ------------------------------ finalize ------------------------------ */

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	connect(m_start_frame, SIGNAL(valueChanged(int)), this, SLOT(setStartFrame(int)));
	connect(m_end_frame, SIGNAL(valueChanged(int)), this, SLOT(setEndFrame(int)));
	connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(setCurrentFrame(int)));
	connect(m_fps, SIGNAL(valueChanged(double)), this, SLOT(setFPS(double)));

	connect(m_timer, SIGNAL(timeout()), this, SLOT(updateFrame()));
}

void EditriceLigneTemps::ajourne_etat(int event)
{
	if (event != (type_evenement::temps | type_evenement::modifie)) {
		return;
	}

	auto fluide = m_poseidon->fluide;

	m_slider->setMinimum(fluide->temps_debut);
	m_start_frame->setValue(fluide->temps_debut);

	m_slider->setMaximum(fluide->temps_fin);
	m_end_frame->setValue(fluide->temps_fin);

	m_cur_frame->setValue(fluide->temps_courant);

	m_slider->setValue(fluide->temps_courant);

	m_fps->setValue(24); /* À FAIRE */

	/* Start or stop the animation. */
	if (m_poseidon->animation) {
		m_timer->start(static_cast<int>(1000.0 / m_fps->value()));
	}
	else {
		m_timer->stop();
	}
}

void EditriceLigneTemps::setStartFrame(int value)
{
	this->rend_actif();
	auto fluide = m_poseidon->fluide;
	fluide->temps_debut = value;
}

void EditriceLigneTemps::setEndFrame(int value)
{
	this->rend_actif();
	auto fluide = m_poseidon->fluide;
	fluide->temps_fin = value;
}

void EditriceLigneTemps::setCurrentFrame(int value)
{
	this->rend_actif();
	auto fluide = m_poseidon->fluide;
	fluide->temps_precedent = fluide->temps_courant;
	fluide->temps_courant = value;
	fluide->ajourne_pour_nouveau_temps();

	m_poseidon->notifie_observatrices(type_evenement::temps | type_evenement::modifie);
}

void EditriceLigneTemps::setFPS(double value)
{
	this->rend_actif();
//	auto scene = m_context->scene;
//	scene->framesPerSecond(value);
}

void EditriceLigneTemps::updateFrame() const
{
	auto fluide = m_poseidon->fluide;
	fluide->temps_precedent = fluide->temps_courant;
	auto value = fluide->temps_courant;

	++value;

	if (value > fluide->temps_fin) {
		value = fluide->temps_debut;
	}

	fluide->temps_courant = value;
	fluide->ajourne_pour_nouveau_temps();

	m_poseidon->notifie_observatrices(type_evenement::temps | type_evenement::modifie);
}
