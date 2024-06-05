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

#include "controle_propriete_rampe_couleur.h"

#include <QBoxLayout>
#include <QComboBox>
#include <QPushButton>

#include "controles/controle_couleur.h"
#include "controles/controle_echelle_valeur.h"
#include "controles/controle_nombre_decimal.h"
#include "controles/controle_rampe_couleur.h"

#include "types/rampe_couleur.h"

#include "commun.hh"
#include "donnees_controle.h"

namespace danjo {

ControleProprieteRampeCouleur::ControleProprieteRampeCouleur(BasePropriete *p,
                                                             int temps,
                                                             QWidget *parent)
    : ControlePropriete(p, temps, parent), m_agencement_principal(new QVBoxLayout()),
      m_agencement_nombre(crée_hbox_layout()), m_entrepolation(new QComboBox(this)),
      m_controle_rampe(new ControleRampeCouleur(this)),
      m_bouton_echelle(crée_bouton_échelle_valeur(this)), m_pos(new ControleNombreDecimal(this)),
      m_echelle(new ControleEchelleDecimale(m_pos, m_bouton_echelle)),
      m_controle_couleur(new ControleCouleur(this))
{
    m_entrepolation->addItem("RVB");
    m_entrepolation->addItem("HSV");

    m_agencement_principal->addWidget(m_entrepolation);
    m_agencement_principal->addWidget(m_controle_rampe);

    m_agencement_nombre->addWidget(m_bouton_echelle);
    m_agencement_nombre->addWidget(m_pos);
    m_agencement_nombre->addWidget(m_controle_couleur);
    m_agencement_principal->addLayout(m_agencement_nombre);

    connect(
        m_entrepolation, SIGNAL(currentIndexChanged(int)), this, SLOT(ajourne_entrepolation(int)));

    connect(m_controle_rampe,
            &ControleRampeCouleur::position_modifie,
            this,
            &ControleProprieteRampeCouleur::ajourne_position);

    connect(m_controle_rampe,
            &ControleRampeCouleur::point_change,
            this,
            &ControleProprieteRampeCouleur::controle_ajoute);

    connect(m_controle_rampe,
            &ControleRampeCouleur::controle_ajoute,
            this,
            &ControleProprieteRampeCouleur::controle_ajoute);

    connect(m_pos,
            &ControleNombreDecimal::valeur_changee,
            this,
            &ControleProprieteRampeCouleur::ajourne_position_controle);

    connect(m_controle_couleur,
            &ControleCouleur::couleur_changee,
            this,
            &ControleProprieteRampeCouleur::ajourne_couleur);

    setLayout(m_agencement_principal);
}

ControleProprieteRampeCouleur::~ControleProprieteRampeCouleur()
{
    delete m_echelle;
}

void ControleProprieteRampeCouleur::finalise(const DonneesControle &donnees)
{
    m_rampe = static_cast<RampeCouleur *>(donnees.pointeur);
    m_controle_rampe->installe_rampe(m_rampe);
    m_pos->ajourne_plage(0.0f, 1.0f);
    m_entrepolation->setCurrentIndex(m_rampe->entrepolation);

    auto point = trouve_point_selectionne(*m_rampe);

    if (point != nullptr) {
        m_controle_couleur->couleur(point->couleur);
    }
}

void ControleProprieteRampeCouleur::ajourne_position(float x)
{
    m_pos->valeur(x);
    Q_EMIT(controle_change());
}

void ControleProprieteRampeCouleur::ajourne_position_controle(float v)
{
    m_controle_rampe->ajourne_position(v);
    Q_EMIT(controle_change());
}

void ControleProprieteRampeCouleur::ajourne_point_actif()
{
    auto point = trouve_point_selectionne(*m_rampe);
    m_pos->valeur(point->position);
    m_controle_couleur->couleur(point->couleur);
}

void ControleProprieteRampeCouleur::ajourne_entrepolation(int i)
{
    m_rampe->entrepolation = static_cast<char>(i);
    m_controle_rampe->update();
    Q_EMIT(controle_change());
}

void ControleProprieteRampeCouleur::ajourne_couleur()
{
    auto point = trouve_point_selectionne(*m_rampe);
    point->couleur = m_controle_couleur->couleur();
    m_controle_rampe->update();
    Q_EMIT(controle_change());
}

void ControleProprieteRampeCouleur::controle_ajoute()
{
    auto point = trouve_point_selectionne(*m_rampe);
    m_pos->valeur(point->position);
    m_controle_couleur->couleur(point->couleur);
    Q_EMIT(controle_change());
}

} /* namespace danjo */
