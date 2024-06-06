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

#include "controle_propriete_enum.h"

#include <QComboBox>
#include <QHBoxLayout>

#include "commun.hh"
#include "donnees_controle.h"

namespace danjo {

ControleProprieteEnum::ControleProprieteEnum(BasePropriete *p, int temps, QWidget *parent)
    : ControlePropriete(p, temps, parent), m_agencement(crée_hbox_layout()),
      m_liste_deroulante(new QComboBox(this))
{
    m_agencement->addWidget(m_liste_deroulante);
    this->setLayout(m_agencement);

    QObject::connect(m_liste_deroulante,
                     qOverload<int>(&QComboBox::currentIndexChanged),
                     this,
                     &ControleProprieteEnum::ajourne_valeur_pointee);
}

void ControleProprieteEnum::ajourne_valeur_pointee(int /*valeur*/)
{
    émets_controle_changé_simple([this]() {
        m_propriete->définis_valeur_énum(
            m_liste_deroulante->currentData().toString().toStdString());
    });
}

void ControleProprieteEnum::finalise(const DonneesControle &donnees)
{
    const QSignalBlocker blocker(m_liste_deroulante);

    for (const auto &pair : donnees.valeur_enum) {
        m_liste_deroulante->addItem(pair.first.c_str(), QVariant(pair.second.c_str()));
    }

    auto valeur_defaut = m_propriete->evalue_énum(0);
    m_liste_deroulante->setCurrentText(valeur_defaut.c_str());
}

void ControleProprieteEnum::ajourne_depuis_propriété()
{
    const QSignalBlocker blocker(m_liste_deroulante);
    m_liste_deroulante->setCurrentText(m_propriete->evalue_énum(m_temps).c_str());
}

} /* namespace danjo */
