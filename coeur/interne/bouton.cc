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

#include "bouton.h"

#include "repondant_bouton.h"

Bouton::Bouton(QWidget *parent)
	: QPushButton(parent)
{
	connect(this, SIGNAL(pressed()), this, SLOT(bouton_presse()));
}

void Bouton::installe_repondant(RepondantBouton *repondant)
{
	m_repondant = repondant;
}

void Bouton::etablie_attache(const std::string &attache)
{
	m_attache = attache;
}

void Bouton::etablie_valeur(const std::string &valeur)
{
	this->setText(valeur.c_str());
}

void Bouton::etablie_infobulle(const std::string &valeur)
{
	this->setToolTip(valeur.c_str());
}

void Bouton::bouton_presse()
{
	if (m_repondant) {
		m_repondant->repond_clique(m_attache);
	}
}
