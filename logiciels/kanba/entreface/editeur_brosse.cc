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

#include "editeur_brosse.h"

#include "danjo/danjo.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QScrollArea>
#include <QVBoxLayout>
#pragma GCC diagnostic pop

#include "biblinternes/outils/fichier.hh"

#include "coeur/kanba.h"

#include "conversion_types.hh"

VueBrosse::VueBrosse(KNB::Kanba &kanba) : m_kanba(kanba)
{
    ajoute_propriete("couleur_pinceau", danjo::TypePropriete::COULEUR, dls::phys::couleur32(1.0f));
    ajoute_propriete("rayon", danjo::TypePropriete::ENTIER, 35);
    ajoute_propriete("opacité", danjo::TypePropriete::DECIMAL, 1.0f);
    ajoute_propriete("mode_fusion", danjo::TypePropriete::ENUM);
}

void VueBrosse::ajourne_donnees()
{
    auto couleur = evalue_couleur("couleur_pinceau");
    auto pinceau = m_kanba.donne_pinceau();
    pinceau.définis_couleur(convertis_couleur(couleur));
    pinceau.définis_rayon(uint32_t(evalue_entier("rayon")));
    pinceau.définis_opacité(evalue_decimal("opacité"));
    // À FAIRE
    // pinceau.définis_mode_de_peinture(KNB::mode_fusion_depuis_nom(evalue_enum("mode_fusion")));
}

bool VueBrosse::ajourne_proprietes()
{
    auto pinceau = m_kanba.donne_pinceau();
    auto couleur = pinceau.donne_couleur();
    valeur_couleur("couleur_pinceau", convertis_couleur(couleur));
    valeur_decimal("opacité", pinceau.donne_opacité());
    valeur_entier("rayon", int32_t(pinceau.donne_rayon()));
    // À FAIRE
    // valeur_chaine("mode_fusion", KNB::nom_mode_fusion(pinceau.donne_mode_de_peinture()));

    return true;
}

EditeurBrosse::EditeurBrosse(KNB::Kanba &kanba, KNB::Éditrice &éditrice, QWidget *parent)
    : BaseEditrice("pinceau", kanba, éditrice, parent), m_vue(new VueBrosse(kanba)),
      m_widget(new QWidget()), m_conteneur_disposition(new QWidget()), m_scroll(new QScrollArea()),
      m_glayout(new QVBoxLayout(m_widget))
{
    m_widget->setSizePolicy(m_cadre->sizePolicy());

    m_scroll->setWidget(m_widget);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scroll->setWidgetResizable(true);

    /* Hide scroll area's frame. */
    m_scroll->setFrameStyle(0);

    m_agencement_principal->addWidget(m_scroll);

    m_glayout->addWidget(m_conteneur_disposition);
}

EditeurBrosse::~EditeurBrosse()
{
    delete m_vue;
    delete m_glayout;
    delete m_scroll;
}

void EditeurBrosse::ajourne_état(KNB::TypeÉvènement evenement)
{
    if (evenement != KNB::TypeÉvènement::RAFRAICHISSEMENT) {
        return;
    }

    m_vue->ajourne_proprietes();

    danjo::DonneesInterface donnees{};
    donnees.conteneur = this;
    donnees.manipulable = m_vue;
    donnees.repondant_bouton = nullptr;

    auto const contenu_fichier = dls::contenu_fichier("scripts/brosse.jo");
    auto disposition = danjo::compile_entreface(donnees, contenu_fichier.c_str());

    if (m_conteneur_disposition->layout()) {
        QWidget tmp;
        tmp.setLayout(m_conteneur_disposition->layout());
    }

    m_conteneur_disposition->setLayout(disposition);
}

void EditeurBrosse::ajourne_manipulable()
{
    m_vue->ajourne_donnees();
}
