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
 * The Original Code is Copyright (C) 2024 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "controle_propriete_bouton.h"

#include <QHBoxLayout>
#include <QPushButton>

#include "commun.hh"
#include "proprietes.hh"

namespace danjo {

ControleProprieteBouton::ControleProprieteBouton(BasePropriete *p, int temps, QWidget *parent)
    : ControlePropriete(p, temps, parent)
{
    m_agencement = crée_hbox_layout(this);

    m_bouton = new QPushButton(this);

    auto texte = p->donne_texte_bouton();
    if (texte.empty()) {
        texte = "Pressez-moi";
    }
    m_bouton->setText(texte.c_str());

    QObject::connect(m_bouton, &QPushButton::pressed, [=]() { p->sur_pression(); });

    m_agencement->addWidget(m_bouton);
}

void ControleProprieteBouton::ajourne_depuis_propriété()
{
}

} /* namespace danjo */
