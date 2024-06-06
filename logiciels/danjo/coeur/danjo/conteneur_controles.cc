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

#include "conteneur_controles.h"

#include "biblinternes/outils/definitions.h"

#include "controles_proprietes/controle_propriete.h"
#include "proprietes.hh"

namespace danjo {

ConteneurControles::ConteneurControles(QWidget *parent) : QWidget(parent)
{
}

void ConteneurControles::ajoute_controle(dls::chaine identifiant, ControlePropriete *controle)
{
    m_controles.insere({identifiant, controle});
}

void ConteneurControles::ajourne_controles()
{
    for (auto const &paire : m_controles) {
        auto controle = paire.second;
        auto prop = controle->donne_propriete();
        controle->ajourne_depuis_propriété();
        controle->setEnabled(prop->est_visible());
    }
}

void ConteneurControles::efface_controles()
{
    m_controles.efface();
}

void ConteneurControles::onglet_dossier_change(int index)
{
    INUTILISE(index);
}

void ConteneurControles::obtiens_liste(const dls::chaine & /*attache*/,
                                       dls::tableau<dls::chaine> &chaines)
{
    chaines.efface();
}

} /* namespace danjo */
