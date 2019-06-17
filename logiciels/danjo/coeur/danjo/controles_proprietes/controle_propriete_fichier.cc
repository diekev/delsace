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

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

#include "donnees_controle.h"

namespace danjo {

SelecteurFichier::SelecteurFichier(bool input, QWidget *parent)
	: ControlePropriete(parent)
	, m_agencement(new QHBoxLayout(this))
	, m_line_edit(new QLineEdit(this))
	, m_push_button(new QPushButton("Choisir Fichier", this))
	, m_input(input)
{
	m_agencement->addWidget(m_line_edit);
	m_agencement->addWidget(m_push_button);

	setLayout(m_agencement);

	connect(m_push_button, SIGNAL(clicked()), this, SLOT(setChoosenFile()));
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

	if (m_input) {
		chemin = QFileDialog::getOpenFileName(this, caption, dir, filtres);
	}
	else {
		chemin = QFileDialog::getSaveFileName(this, caption, dir, filtres);
	}

	if (!chemin.isEmpty()) {
		m_line_edit->setText(chemin);
		Q_EMIT(valeur_changee(chemin));
	}
}

void SelecteurFichier::ajourne_filtres(const QString &chaine)
{
	m_filtres = chaine;
}

ControleProprieteFichier::ControleProprieteFichier(bool input, QWidget *parent)
	: SelecteurFichier(input, parent)
	, m_pointeur(nullptr)
{
	connect(this, &SelecteurFichier::valeur_changee, this, &ControleProprieteFichier::ajourne_valeur_pointee);
}

void ControleProprieteFichier::finalise(const DonneesControle &donnees)
{
	m_pointeur = static_cast<std::string *>(donnees.pointeur);

	if (donnees.initialisation) {
		*m_pointeur = donnees.valeur_defaut;
	}

	setValue(m_pointeur->c_str());

	setToolTip(donnees.infobulle.c_str());

	ajourne_filtres(donnees.filtres.c_str());
}

void ControleProprieteFichier::ajourne_valeur_pointee(const QString &valeur)
{
	*m_pointeur = valeur.toStdString();
	Q_EMIT(controle_change());
}

}  /* namespace danjo */
