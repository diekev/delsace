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
#include <QPushButton>
#include <QTextEdit>

#include "donnees_controle.h"

namespace danjo {

ControleProprieteChaineCaractere::ControleProprieteChaineCaractere(BasePropriete *p,
                                                                   int temps,
                                                                   QWidget *parent)
    : ControlePropriete(p, temps, parent), m_agencement(new QHBoxLayout),
      m_editeur_ligne(new QLineEdit(this))
{
    m_agencement->addWidget(m_editeur_ligne);
    this->setLayout(m_agencement);

    m_editeur_ligne->setText(m_propriete->evalue_chaine(m_temps).c_str());

    connect(m_editeur_ligne,
            &QLineEdit::returnPressed,
            this,
            &ControleProprieteChaineCaractere::ajourne_valeur_pointee);
}

void ControleProprieteChaineCaractere::ajourne_valeur_pointee()
{
    Q_EMIT(debute_changement_controle());
    m_propriete->définis_valeur_chaine(m_editeur_ligne->text().toStdString());
    Q_EMIT(controle_change());
    Q_EMIT(termine_changement_controle());
}

ControleProprieteEditeurTexte::ControleProprieteEditeurTexte(BasePropriete *p,
                                                             int temps,
                                                             QWidget *parent)
    : ControlePropriete(p, temps, parent), m_agencement(new QVBoxLayout),
      m_editeur_ligne(new QTextEdit(this)), m_bouton(new QPushButton("Rafraîchis", this))
{
    m_agencement->addWidget(m_editeur_ligne);
    m_agencement->addWidget(m_bouton);
    this->setLayout(m_agencement);

    m_editeur_ligne->setText(m_propriete->evalue_chaine(m_temps).c_str());

    connect(m_bouton,
            &QPushButton::pressed,
            this,
            &ControleProprieteEditeurTexte::ajourne_valeur_pointee);
}

void ControleProprieteEditeurTexte::ajourne_valeur_pointee()
{
    m_propriete->définis_valeur_chaine(m_editeur_ligne->toPlainText().toStdString());
    Q_EMIT(controle_change());
}

} /* namespace danjo */
