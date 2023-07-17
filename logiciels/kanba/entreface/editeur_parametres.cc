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

#include "editeur_parametres.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QGridLayout>
#include <QScrollArea>
#pragma GCC diagnostic pop

#include "coeur/kanba.h"

/* ************************************************************************** */

VueParametres::VueParametres(KNB::Kanba *kanba) : m_kanba(kanba)
{
}

void VueParametres::ajourne_donnees()
{
}

bool VueParametres::ajourne_proprietes()
{
    return true;
}

/* ************************************************************************** */

EditeurParametres::EditeurParametres(KNB::Kanba *kanba, QWidget *parent)
    : BaseEditrice("paramètres", *kanba, parent), m_vue(new VueParametres(kanba)),
      m_widget(new QWidget()), m_scroll(new QScrollArea()), m_glayout(new QGridLayout(m_widget))
{
    m_widget->setSizePolicy(m_cadre->sizePolicy());

    m_scroll->setWidget(m_widget);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scroll->setWidgetResizable(true);

    /* Hide scroll area's frame. */
    m_scroll->setFrameStyle(0);

    m_agencement_principal->addWidget(m_scroll);
}

EditeurParametres::~EditeurParametres()
{
    delete m_vue;
    delete m_scroll;
}

void EditeurParametres::ajourne_état(KNB::TypeÉvènement evenement)
{
    m_vue->ajourne_proprietes();
    //	cree_controles(m_assembleur_controles, m_vue);
    //	m_assembleur_controles.setContext(this, SLOT(ajourne_vue()));
}

void EditeurParametres::ajourne_manipulable()
{
    m_vue->ajourne_donnees();
}
