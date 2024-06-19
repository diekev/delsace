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

#include "controle_propriete_liste.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>

#include <iostream>

#include "donnees_controle.h"
#include "fournisseuse_icones.hh"

#include "commun.hh"
#include "conteneur_controles.h"

namespace danjo {

ControleProprieteListe::ControleProprieteListe(BasePropriete *p, int temps, QWidget *parent)
    : ControlePropriete(p, temps, parent), m_agencement(crée_hbox_layout(this)),
      m_combobox(new QComboBox(this))
{
    m_agencement->addWidget(m_combobox);

    setLayout(m_agencement);

    m_combobox->setInsertPolicy(QComboBox::NoInsert);
    m_combobox->setEditable(true);

    m_combobox->setCurrentText(m_propriete->evalue_chaine(m_temps).c_str());

    connect(m_combobox->lineEdit(),
            &QLineEdit::editingFinished,
            this,
            &ControleProprieteListe::texte_modifie);
    connect(m_combobox,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &ControleProprieteListe::index_modifie);
}

void ControleProprieteListe::attache(const dls::chaine &attache)
{
    m_attache = attache;
}

void ControleProprieteListe::conteneur(ConteneurControles *conteneur)
{
    m_conteneur = conteneur;
    ajourne_liste();
}

void ControleProprieteListe::finalise(const DonneesControle &donnees)
{
    ajourne_liste();
    attache(donnees.nom);
}

void ControleProprieteListe::ajourne_depuis_propriété()
{
    QSignalBlocker blocke(m_combobox);
    m_combobox->setCurrentText(m_propriete->evalue_chaine(m_temps).c_str());
}

void ControleProprieteListe::texte_modifie()
{
    ajourne_valeur_pointee(m_combobox->currentText());
}

void ControleProprieteListe::index_modifie(int)
{
    texte_modifie();
}

void ControleProprieteListe::ajourne_valeur_pointee(const QString &valeur)
{
    émets_controle_changé_simple(
        [this, &valeur]() { m_propriete->définis_valeur_chaine(valeur.toStdString()); });
}

void ControleProprieteListe::ajourne_liste()
{
    if (m_conteneur == nullptr) {
        return;
    }

    QSignalBlocker blocke(m_combobox);

    dls::tableau<dls::chaine> chaines;
    m_conteneur->obtiens_liste(m_attache, chaines);

    m_combobox->clear();

    for (const auto &chaine : chaines) {
        m_combobox->addItem(chaine.c_str());
    }
    ajourne_depuis_propriété();
}

} /* namespace danjo */
