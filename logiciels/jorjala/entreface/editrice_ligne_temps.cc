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

#include "editrice_ligne_temps.h"

#include "danjo/danjo.h"
#include "danjo/manipulable.h"

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QSlider>
#include <QSpinBox>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "biblinternes/patrons_conception/repondant_commande.h"

#include "coeur/jorjala.hh"

EditriceLigneTemps::EditriceLigneTemps(JJL::Jorjala &jorjala, QWidget *parent)
    : BaseEditrice("scene", jorjala, parent), m_slider(new QSlider(m_frame)),
      m_tc_layout(new QHBoxLayout()), m_num_layout(new QHBoxLayout()),
      m_vbox_layout(new QVBoxLayout()), m_end_frame(new QSpinBox(m_frame)),
      m_start_frame(new QSpinBox(m_frame)), m_cur_frame(new QSpinBox(m_frame)),
      m_fps(new QDoubleSpinBox(m_frame))
{
    m_main_layout->addLayout(m_vbox_layout);

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

    auto données = cree_donnees_interface_danjo(m_jorjala, &dummy);
    auto gestionnaire = gestionnaire_danjo(m_jorjala);

    auto disp_controles = gestionnaire->compile_entreface_fichier(
        données, "entreface/disposition_ligne_temps.jo");

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
}

void EditriceLigneTemps::ajourne_état(JJL::TypeEvenement évènement)
{
    auto creation = (évènement == (JJL::TypeEvenement::TEMPS | JJL::TypeEvenement::MODIFIÉ));
    creation |= (évènement == (JJL::TypeEvenement::RAFRAICHISSEMENT));

    if (!creation) {
        return;
    }

    /* déconnexion pour deux raison :
     * - on peut se retrouver dans une boucle infinie à cause de setCurrentFrame
     *   étant appelé par m_slider qui cause une notification de toutes les
     *   observatrices
     * - lors des animations, ajourner l'état lance tout un tas de signaux qui
     *   dans les slots ci-dessous changent quelle éditrice est active
     */
    disconnect(m_start_frame, SIGNAL(valueChanged(int)), this, SLOT(setStartFrame(int)));
    disconnect(m_end_frame, SIGNAL(valueChanged(int)), this, SLOT(setEndFrame(int)));
    disconnect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(setCurrentFrame(int)));
    disconnect(m_fps, SIGNAL(valueChanged(double)), this, SLOT(setFPS(double)));

    m_slider->setMinimum(m_jorjala.temps_début());
    m_start_frame->setValue(m_jorjala.temps_début());

    m_slider->setMaximum(m_jorjala.temps_fin());
    m_end_frame->setValue(m_jorjala.temps_fin());

    m_cur_frame->setValue(m_jorjala.temps_courant());

    m_slider->setValue(m_jorjala.temps_courant());

    m_fps->setValue(m_jorjala.cadence());

    connect(m_start_frame, SIGNAL(valueChanged(int)), this, SLOT(setStartFrame(int)));
    connect(m_end_frame, SIGNAL(valueChanged(int)), this, SLOT(setEndFrame(int)));
    connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(setCurrentFrame(int)));
    connect(m_fps, SIGNAL(valueChanged(double)), this, SLOT(setFPS(double)));
}

void EditriceLigneTemps::setStartFrame(int value)
{
    this->rend_actif();
    m_jorjala.temps_début(value);
}

void EditriceLigneTemps::setEndFrame(int value)
{
    this->rend_actif();
    m_jorjala.temps_fin(value);
}

void EditriceLigneTemps::setCurrentFrame(int value)
{
    this->rend_actif();
    m_jorjala.temps_courant(value);
    m_jorjala.ajourne_pour_nouveau_temps("éditrice temps");

    m_jorjala.notifie_observatrices(JJL::TypeEvenement::TEMPS | JJL::TypeEvenement::MODIFIÉ);
}

void EditriceLigneTemps::setFPS(double value)
{
    this->rend_actif();
    m_jorjala.cadence(value);
}
