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

#include "controle_propriete_bool.h"

#include <QCheckBox>
#include <QHBoxLayout>

#include "commun.hh"
#include "donnees_controle.h"

namespace danjo {

ControleProprieteBool::ControleProprieteBool(BasePropriete *p, int temps, QWidget *parent)
    : ControlePropriete(p, temps, parent), m_agencement(crée_hbox_layout()),
      m_case_a_cocher(new QCheckBox(this))
{
    m_agencement->addWidget(m_case_a_cocher);

    m_case_a_cocher->setChecked(m_propriete->evalue_bool(temps));

    this->setLayout(m_agencement);

    connect(m_case_a_cocher,
            &QAbstractButton::toggled,
            this,
            &ControleProprieteBool::ajourne_valeur_pointee);
}

void ControleProprieteBool::ajourne_valeur_pointee(bool valeur)
{
    émets_controle_changé_simple([this, valeur]() { m_propriete->définis_valeur_bool(valeur); });
}

} /* namespace danjo */
