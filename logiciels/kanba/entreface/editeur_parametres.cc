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

#include "danjo/compilation/assembleuse_disposition.h"

#include "coeur/kanba.h"

static QBoxLayout *crée_disposition_paramètres(danjo::Manipulable *manipulable,
                                               danjo::RepondantBouton *repondant_bouton,
                                               danjo::ConteneurControles *conteneur)
{
    danjo::DonneesInterface données_interface{};
    données_interface.manipulable = manipulable;
    données_interface.repondant_bouton = repondant_bouton;
    données_interface.conteneur = conteneur;

    danjo::AssembleurDisposition assembleuse(données_interface);

    /* Ajout d'une disposition par défaut. */
    assembleuse.ajoute_disposition(danjo::id_morceau::COLONNE);

    auto debut = manipulable->debut();
    auto fin = manipulable->fin();

    for (; debut != fin; ++debut) {
        auto param = *debut;

        assembleuse.ajoute_disposition(danjo::id_morceau::LIGNE);

        dls::chaine nom_param(param.first);
        assembleuse.ajoute_étiquette(nom_param);

        danjo::DonneesControle donnees_controle;
        donnees_controle.nom = nom_param;

        assembleuse.ajoute_controle_pour_propriété(donnees_controle, param.second);
        assembleuse.sors_disposition();
    }

    auto disp = assembleuse.disposition();
    disp->addStretch();
    return disp;
}

/* ************************************************************************** */

VueParametres::VueParametres(KNB::Kanba &kanba) : m_kanba(kanba)
{
    ajoute_propriete("dessine_seaux", danjo::TypePropriete::BOOL, m_kanba.donne_dessine_seaux());
}

void VueParametres::ajourne_donnees()
{
    auto dessine_seaux = evalue_bool("dessine_seaux");
    m_kanba.définis_dessine_seaux(dessine_seaux);
}

bool VueParametres::ajourne_proprietes()
{
    valeur_bool("dessine_seaux", m_kanba.donne_dessine_seaux());
    return true;
}

/* ************************************************************************** */

EditeurParametres::EditeurParametres(KNB::Kanba &kanba, KNB::Éditrice &éditrice, QWidget *parent)
    : BaseEditrice("paramètres", kanba, éditrice, parent), m_vue(new VueParametres(kanba)),
      m_widget(new QWidget()), m_scroll(new QScrollArea()), m_conteneur_disposition(new QWidget()),
      m_disposition_widget(new QVBoxLayout(m_widget))
{
    m_widget->setSizePolicy(m_cadre->sizePolicy());

    m_scroll->setWidget(m_widget);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scroll->setWidgetResizable(true);

    /* Hide scroll area's frame. */
    m_scroll->setFrameStyle(0);

    m_agencement_principal->addWidget(m_scroll);

    m_disposition_widget->addWidget(m_conteneur_disposition);
}

EditeurParametres::~EditeurParametres()
{
    delete m_vue;
    delete m_scroll;
}

void EditeurParametres::ajourne_état(KNB::ChangementÉditrice evenement)
{
    m_vue->ajourne_proprietes();

    auto disposition = crée_disposition_paramètres(m_vue, nullptr, this);
    if (disposition == nullptr) {
        return;
    }

    if (m_conteneur_disposition->layout()) {
        QWidget temp;
        temp.setLayout(m_conteneur_disposition->layout());
    }
    m_conteneur_disposition->setLayout(disposition);
}

void EditeurParametres::ajourne_manipulable()
{
    m_vue->ajourne_donnees();
}
