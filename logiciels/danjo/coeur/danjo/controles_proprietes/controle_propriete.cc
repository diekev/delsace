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

#include "controle_propriete.h"

#include "proprietes.hh"

namespace danjo {

ControlePropriete::ControlePropriete(BasePropriete *p, int temps, QWidget *parent)
    : QWidget(parent), m_propriete(p), m_temps(temps)
{
    if (m_propriete) {
        /* Les étiquettes n'ont pas de propriété. */
        setToolTip(m_propriete->donnne_infobulle().c_str());
        setEnabled(m_propriete->est_visible());
    }
}

void ControlePropriete::emets_debute_changement_controle()
{
    Q_EMIT(debute_changement_controle());
}

void ControlePropriete::emets_termine_changement_controle()
{
    Q_EMIT(termine_changement_controle());
}

} /* namespace danjo */
