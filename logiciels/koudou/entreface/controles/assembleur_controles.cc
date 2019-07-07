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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "assembleur_controles.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QGridLayout>
#include <QLabel>
#pragma GCC diagnostic pop

#include "../outils.h"

AssembleurControles::AssembleurControles(QGridLayout *layout)
    : m_agencement(layout)
    , m_dernier_controle(nullptr)
    , m_compte_items(0)
{}

AssembleurControles::~AssembleurControles()
{
	clear();
}

void AssembleurControles::addWidget(QWidget *widget, QString const &name)
{
	auto label = new QLabel(name);
	m_agencement->addWidget(label, m_compte_items, 0);
	m_agencement->addWidget(widget, m_compte_items, 1);

	m_dernier_controle = widget;
	m_controles.push_back(widget);
	m_tableau_controle[name.toStdString()] = std::make_pair(label, widget);

	++m_compte_items;
}

void AssembleurControles::setTooltip(QString const &tooltip)
{
	if (m_dernier_controle) {
		m_dernier_controle->setToolTip(tooltip);
	}
}

void AssembleurControles::clear()
{
	m_tableau_controle.efface();
	m_controles.clear();
	vide_agencement(m_agencement);
}

void AssembleurControles::setVisible(bool yesno)
{
	if (m_dernier_controle) {
		m_dernier_controle->setVisible(yesno);
	}
}

void AssembleurControles::setVisible(QString const &name, bool yesno)
{
	widget_pair widgets = m_tableau_controle[name.toStdString()];
	widgets.first->setVisible(yesno);
	widgets.second->setVisible(yesno);
}

void AssembleurControles::addWarning(QString const &warning)
{
	auto label = new QLabel("Warning");
	label->setPixmap(QPixmap("icons/warning.png"));

	m_agencement->addWidget(label, m_compte_items, 0, Qt::AlignRight);
	m_agencement->addWidget(new QLabel(warning), m_compte_items, 1);

	++m_compte_items;
}
