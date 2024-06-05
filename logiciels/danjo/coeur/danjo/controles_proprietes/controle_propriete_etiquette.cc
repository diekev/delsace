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

#include "controle_propriete_etiquette.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>

#include "donnees_controle.h"

namespace danjo {

ControleProprieteEtiquette::ControleProprieteEtiquette(QString const &texte, QWidget *parent)
    : ControlePropriete(nullptr, 0, parent), m_agencement(new QHBoxLayout(this)),
      m_etiquette(new QLabel(this))
{
    m_agencement->addWidget(m_etiquette);
    m_etiquette->setText(texte);
    setLayout(m_agencement);
}

ControleProprieteEtiquetteActivable::ControleProprieteEtiquetteActivable(QString const &texte,
                                                                         BasePropriete *p,
                                                                         int temps,
                                                                         QWidget *parent)
    : ControlePropriete(p, temps, parent), m_agencement(new QHBoxLayout(this)),
      m_checkbox(new QCheckBox(texte, this))
{
    m_agencement->addWidget(m_checkbox);
    setLayout(m_agencement);

    m_checkbox->setChecked(p->est_visible());

    QObject::connect(m_checkbox, &QCheckBox::stateChanged, [=](int state) {
        p->definit_visibilité(state == Qt::Checked);
    });

    QObject::connect(
        m_checkbox, &QCheckBox::stateChanged, this, &ControlePropriete::controle_change);

    /* Ceci doit toujours être activé. */
    setEnabled(true);
}

} /* namespace danjo */
