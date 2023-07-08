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

#include "commun.hh"
#include "donnees_controle.h"

namespace danjo {

ControleProprieteDecimal::ControleProprieteDecimal(BasePropriete *p, int temps, QWidget *parent)
    : ControlePropriete(p, temps, parent), m_agencement(new QHBoxLayout(this)),
      m_controle(new ControleNombreDecimal(this)), m_bouton(crée_bouton_échelle_valeur(this)),
      m_bouton_animation(crée_bouton_animation_controle(this)),
      m_echelle(new ControleEchelleDecimale(m_controle, m_bouton))
{
    m_agencement->addWidget(m_bouton_animation);
    m_agencement->addWidget(m_bouton);
    m_agencement->addWidget(m_controle);
    setLayout(m_agencement);

    connecte_signaux_début_fin_changement(m_controle, this);
    connecte_signaux_début_fin_changement(m_echelle, this);

    connect(m_controle,
            &ControleNombreDecimal::valeur_changee,
            this,
            &ControleProprieteDecimal::ajourne_valeur_pointee);
    connect(m_bouton_animation,
            &QPushButton::pressed,
            this,
            &ControleProprieteDecimal::bascule_animation);
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
        m_propriete->définit_valeur_décimal(valeur);
    }

    Q_EMIT(controle_change());
}

void ControleProprieteDecimal::bascule_animation()
{
    émets_controle_changé_simple([this]() {
        m_animation = !m_animation;

        if (m_animation == false) {
            m_propriete->supprime_animation();
            m_controle->valeur(m_propriete->evalue_decimal(m_temps));
        }
        else {
            m_propriete->ajoute_cle(m_propriete->evalue_decimal(m_temps), m_temps);
        }

        définit_état_bouton_animation(m_bouton_animation, m_animation);
        m_controle->marque_anime(m_animation, m_animation);
    });
}

void ControleProprieteDecimal::finalise(const DonneesControle &donnees)
{
    if (!m_propriete->est_animable()) {
        m_bouton_animation->hide();
    }

    m_animation = m_propriete->est_animee();
    définit_état_bouton_animation(m_bouton_animation, m_animation);

    if (m_animation) {
        m_controle->marque_anime(m_animation, m_propriete->possede_cle(m_temps));
    }

    auto plage = m_propriete->plage_valeur_decimal();
    m_controle->ajourne_plage(plage.min, plage.max);
    m_controle->valeur(m_propriete->evalue_decimal(m_temps));
    m_controle->suffixe(donnees.suffixe.c_str());
}

} /* namespace danjo */
