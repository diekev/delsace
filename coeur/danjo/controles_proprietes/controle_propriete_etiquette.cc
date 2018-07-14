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

#include <QHBoxLayout>
#include <QLabel>

#include "interne/donnees_controle.h"

namespace danjo {

ControleProprieteEtiquette::ControleProprieteEtiquette(QWidget *parent)
	: ControlePropriete(parent)
	, m_agencement(new QHBoxLayout(this))
	, m_etiquette(new QLabel(this))
{
	m_agencement->addWidget(m_etiquette);
	setLayout(m_agencement);
}

void ControleProprieteEtiquette::finalise(const DonneesControle &donnees)
{
	m_etiquette->setText(donnees.valeur_defaut.c_str());
}

}  /* namespace danjo */
