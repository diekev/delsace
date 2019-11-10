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

#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>

#include <iostream>

#include "donnees_controle.h"

#include "conteneur_controles.h"

namespace danjo {

ControleProprieteListe::ControleProprieteListe(QWidget *parent)
	: ControlePropriete(parent)
	, m_agencement(new QHBoxLayout(this))
	, m_editeur_texte(new QLineEdit(this))
	, m_bouton_liste(new QPushButton("list", this))
	, m_liste(new QMenu(this))
{
	m_agencement->addWidget(m_editeur_texte);
	m_agencement->addWidget(m_bouton_liste);

	setLayout(m_agencement);

	connect(m_bouton_liste, SIGNAL(clicked()), this, SLOT(montre_liste()));
	connect(m_editeur_texte, SIGNAL(returnPressed()), this, SLOT(texte_modifie()));
	connect(m_liste, SIGNAL(aboutToShow()), this, SLOT(ajourne_liste()));
}

void ControleProprieteListe::attache(const dls::chaine &attache)
{
	m_attache = attache;
}

void ControleProprieteListe::conteneur(ConteneurControles *conteneur)
{
	m_conteneur = conteneur;
}

void ControleProprieteListe::finalise(const DonneesControle &donnees)
{
	m_pointeur = static_cast<dls::chaine *>(donnees.pointeur);

	if (donnees.initialisation) {
		*m_pointeur = donnees.valeur_defaut;
	}

	attache(donnees.nom);
	m_editeur_texte->setText(m_pointeur->c_str());

	setToolTip(donnees.infobulle.c_str());
}

void ControleProprieteListe::montre_liste()
{
	/* La liste est positionnée en dessous du bouton, alignée à sa gauche. */
	const auto &rect = m_bouton_liste->geometry();
	const auto &bas_gauche = m_bouton_liste->parentWidget()->mapToGlobal(rect.bottomLeft());

	m_liste->popup(bas_gauche);
}

void ControleProprieteListe::texte_modifie()
{
	ajourne_valeur_pointee(m_editeur_texte->text());
}

void ControleProprieteListe::ajourne_valeur_pointee(const QString &valeur)
{
	if (m_pointeur == nullptr) {
		return;
	}

	Q_EMIT(precontrole_change());
	*m_pointeur = valeur.toStdString();
	Q_EMIT(controle_change());
}

void ControleProprieteListe::ajourne_liste()
{
	if (m_conteneur == nullptr) {
		return;
	}

	dls::tableau<dls::chaine> chaines;
	m_conteneur->obtiens_liste(m_attache, chaines);

	m_liste->clear();

	for (const auto &chaine : chaines) {
		auto action = m_liste->addAction(chaine.c_str());
		connect(action, SIGNAL(triggered()), this, SLOT(repond_clique()));
	}
}

void ControleProprieteListe::repond_clique()
{
	auto action = qobject_cast<QAction *>(sender());

	if (!action) {
		return;
	}

	const auto &texte_action = action->text();
	auto texte_courant = m_editeur_texte->text();

	if (texte_courant.contains(texte_action)) {
		return;
	}

	if (!texte_courant.isEmpty()) {
		texte_courant += ",";
	}

	texte_courant += action->text();

	m_editeur_texte->setText(texte_courant);
	ajourne_valeur_pointee(texte_courant);
}

}  /* namespace danjo */
