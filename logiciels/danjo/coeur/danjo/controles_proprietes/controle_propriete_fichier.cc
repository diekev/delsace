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

#include "controle_propriete_fichier.h"

#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>

#include "dialogues_chemins.hh"
#include "donnees_controle.h"
#include "fournisseuse_icones.hh"

namespace danjo {

static QIcon donne_icone_pour_bouton(QStyle *style)
{
    auto &fournisseuse = donne_fournisseuse_icone();
    auto icône = fournisseuse.icone_pour_identifiant("ouvrir_dossier", ÉtatIcône::ACTIF);
    if (icône.has_value()) {
        return icône.value();
    }
    return style->standardIcon(QStyle::SP_DirOpenIcon);
}

SelecteurFichier::SelecteurFichier(BasePropriete *p, int temps, bool input, QWidget *parent)
    : ControlePropriete(p, temps, parent), m_agencement(new QHBoxLayout(this)),
      m_line_edit(new QLineEdit(this)), m_push_button(new QPushButton(this)), m_input(input)
{
    m_agencement->addWidget(m_line_edit);
    m_agencement->addWidget(m_push_button);

    setLayout(m_agencement);

    m_push_button->setIcon(donne_icone_pour_bouton(style()));
    m_push_button->setToolTip("Choisir Fichier");

    connect(m_push_button, SIGNAL(clicked()), this, SLOT(setChoosenFile()));
    connect(m_line_edit, &QLineEdit::editingFinished, this, &SelecteurFichier::lineEditChanged);
}

void SelecteurFichier::setValue(const QString &text)
{
    m_line_edit->setText(text);
}

void SelecteurFichier::setChoosenFile()
{
    QString chemin;
    QString caption = "";
    QString dir = "";
    QString filtres = m_filtres;

    auto chemin_courant = m_line_edit->text();
    if (!chemin_courant.isEmpty()) {
        QFileInfo file_info(chemin_courant);
        if (file_info.exists() && file_info.exists(file_info.dir().path())) {
            dir = file_info.dir().path();
        }
    }

    auto &dialogues = donne_dialogues_chemins();
    if (m_input) {
        chemin = dialogues.donne_chemin_pour_ouverture(chemin_courant, caption, dir, filtres);
    }
    else {
        chemin = dialogues.donne_chemin_pour_écriture(chemin_courant, caption, dir, filtres);
    }

    if (!chemin.isEmpty()) {
        m_line_edit->setText(chemin);
        Q_EMIT(valeur_changee(chemin));
    }
}

void SelecteurFichier::lineEditChanged()
{
    Q_EMIT(valeur_changee(m_line_edit->text()));
}

void SelecteurFichier::ajourne_filtres(const QString &chaine)
{
    m_filtres = chaine;
}

ControleProprieteFichier::ControleProprieteFichier(BasePropriete *p,
                                                   int temps,
                                                   bool input,
                                                   QWidget *parent)
    : SelecteurFichier(p, temps, input, parent)
{
    setValue(p->evalue_chaine(temps).c_str());
    connect(this,
            &SelecteurFichier::valeur_changee,
            this,
            &ControleProprieteFichier::ajourne_valeur_pointee);
}

void ControleProprieteFichier::finalise(const DonneesControle &donnees)
{
    ajourne_filtres(donnees.filtres.c_str());
}

void ControleProprieteFichier::ajourne_valeur_pointee(const QString &valeur)
{
    émets_controle_changé_simple(
        [this, &valeur]() { m_propriete->définis_valeur_chaine(valeur.toStdString().c_str()); });
}

} /* namespace danjo */
