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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "barre_progres.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#pragma GCC diagnostic pop

#include "coeur/jorjala.hh"

BarreDeProgres::BarreDeProgres(Jorjala &jorjala, QWidget *parent)
	: QWidget(parent)
	, m_jorjala(jorjala)
	, m_barre_progres(new QProgressBar(this))
	, m_label(new QLabel("Évaluation en cours :", this))
	, m_bouton_stop(new QPushButton("STOP", this))
	, m_disposition(new QHBoxLayout(this))
{
	m_barre_progres->setRange(0, 100);

	m_disposition->addWidget(m_label);
	m_disposition->addWidget(m_barre_progres);
	m_disposition->addWidget(m_bouton_stop);

	connect(m_bouton_stop, &QPushButton::pressed, this, &BarreDeProgres::signal_stop);
}

void BarreDeProgres::ajourne_valeur(int valeur)
{
	m_barre_progres->setValue(valeur);
}

void BarreDeProgres::ajourne_message(const char *message, int execution, int total)
{
	auto str = QString("Évaluation en cours ")
			.append(QString::number(execution))
			.append('/')
			.append(QString::number(total))
			.append(" : ")
			.append(message);

	m_label->setText(str);
}

void BarreDeProgres::signal_stop()
{
	m_jorjala.interrompu = true;
}
