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

#include "donnees_controle.h"

namespace danjo {

SelecteurListe::SelecteurListe(QWidget *parent)
	: ControlePropriete(parent)
	, m_agencement(new QHBoxLayout(this))
	, m_line_edit(new QLineEdit(this))
	, m_push_button(new QPushButton("list", this))
	, m_list_widget(new QMenu())
{
	m_agencement->addWidget(m_line_edit);
	m_agencement->addWidget(m_push_button);

	connect(m_push_button, SIGNAL(clicked()), this, SLOT(showList()));

	connect(m_line_edit, SIGNAL(returnPressed()), this, SLOT(updateText()));
}

SelecteurListe::~SelecteurListe()
{
	delete m_list_widget;
}

void SelecteurListe::addField(const QString &text)
{
	auto action = m_list_widget->addAction(text);
	connect(action, SIGNAL(triggered()), this, SLOT(handleClick()));
}

void SelecteurListe::setValue(const QString &text)
{
	m_line_edit->setText(text);
}

void SelecteurListe::showList()
{
	/* Figure out where the bottom left corner of the push is located. */
	QRect widgetRect = m_push_button->geometry();
	auto bottom_left = m_push_button->parentWidget()->mapToGlobal(widgetRect.bottomLeft());

	m_list_widget->popup(bottom_left);
}

void SelecteurListe::updateText()
{
	Q_EMIT(valeur_changee(m_line_edit->text()));
}

void SelecteurListe::handleClick()
{
	auto action = qobject_cast<QAction *>(sender());

	if (!action) {
		return;
	}

	auto text = m_line_edit->text();

	if (!text.isEmpty()) {
		text += ",";
	}

	text += action->text();

	this->setValue(text);
	Q_EMIT(valeur_changee(text));
}

ControleProprieteListe::ControleProprieteListe(QWidget *parent)
	: SelecteurListe(parent)
{
	connect(this, &SelecteurListe::valeur_changee, this, &ControleProprieteListe::ajourne_valeur_pointee);
}

void ControleProprieteListe::finalise(const DonneesControle &donnees)
{
	m_pointeur = static_cast<std::string *>(donnees.pointeur);

	if (donnees.initialisation) {
		*m_pointeur = donnees.valeur_defaut;
	}

	setValue(m_pointeur->c_str());

	setToolTip(donnees.infobulle.c_str());
}

void ControleProprieteListe::ajourne_valeur_pointee(const QString &valeur)
{
	*m_pointeur = valeur.toStdString();
	Q_EMIT(controle_change());
}

}  /* namespace danjo */
