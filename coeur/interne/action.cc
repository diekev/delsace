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

#include "action.h"

#include "repondant_bouton.h"

Action::Action(QWidget *parent)
	: QAction(parent)
{
	connect(this, SIGNAL(triggered()), this, SLOT(action_presse()));
}

void Action::installe_repondant(RepondantBouton *repondant)
{
	m_repondant = repondant;
}

void Action::etablie_attache(const std::string &attache)
{
	m_attache = attache;
}

void Action::etablie_metadonnee(const std::string &metadonnee)
{
	m_metadonnee = metadonnee;
}

void Action::etablie_valeur(const std::string &valeur)
{
	this->setText(valeur.c_str());
}

void Action::etablie_infobulle(const std::string &valeur)
{
	this->setToolTip(valeur.c_str());
}

void Action::action_presse()
{
	if (m_repondant) {
		m_repondant->repond_clique(m_attache, m_metadonnee);
	}
}
