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

#include "controle_propriete_chaine.h"

#include <QHBoxLayout>
#include <QLineEdit>

#include "interne/donnees_controle.h"

namespace danjo {

ControleProprieteChaineCaractere::ControleProprieteChaineCaractere(QWidget *parent)
	: ControlePropriete(parent)
	, m_agencement(new QHBoxLayout)
	, m_editeur_ligne(new QLineEdit(this))
	, m_pointeur(nullptr)
{
	m_agencement->addWidget(m_editeur_ligne);
	this->setLayout(m_agencement);

	connect(m_editeur_ligne, &QLineEdit::returnPressed,
			this, &ControleProprieteChaineCaractere::ajourne_valeur_pointee);
}

void ControleProprieteChaineCaractere::finalise(const DonneesControle &donnees)
{
	m_pointeur = static_cast<std::string *>(donnees.pointeur);

	if (donnees.initialisation) {
		*m_pointeur = donnees.valeur_defaut;
	}

	m_editeur_ligne->setText(m_pointeur->c_str());

	setToolTip(donnees.infobulle.c_str());
}

void ControleProprieteChaineCaractere::ajourne_valeur_pointee()
{
	*m_pointeur = m_editeur_ligne->text().toStdString();
	Q_EMIT(controle_change());
}

}  /* namespace danjo */
