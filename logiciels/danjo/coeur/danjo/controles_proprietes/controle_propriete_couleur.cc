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

#include "controle_propriete_couleur.h"

#include <QHBoxLayout>

#include "biblinternes/outils/chaine.hh"

#include "compilation/morceaux.h"

#include "controles/controle_couleur.h"

#include "donnees_controle.h"

#include "commun.hh"

#include <sstream>

namespace danjo {

ControleProprieteCouleur::ControleProprieteCouleur(BasePropriete *p, int temps, QWidget *parent)
    : ControlePropriete(p, temps, parent), m_agencement(crée_hbox_layout(this)),
      m_controle_couleur(new ControleCouleur(this))
{
    m_agencement->addWidget(m_controle_couleur);

    auto plage = m_propriete->plage_valeur_couleur();
    m_controle_couleur->ajourne_plage(plage.min, plage.max);
    m_controle_couleur->couleur(m_propriete->evalue_couleur(m_temps));

    connect(m_controle_couleur,
            &ControleCouleur::couleur_changee,
            this,
            &ControleProprieteCouleur::ajourne_couleur);
}

void ControleProprieteCouleur::ajourne_couleur()
{
    émets_controle_changé_simple(
        [this]() { m_propriete->définis_valeur_couleur(m_controle_couleur->couleur()); });
}

} /* namespace danjo */
