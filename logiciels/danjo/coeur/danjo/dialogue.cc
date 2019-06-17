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

#include "dialogue.h"

#include <QBoxLayout>
#include <QPushButton>

Dialogue::Dialogue(QLayout *disposition, QWidget *parent)
	: QDialog(parent)
{
	auto disposition_principale = new QVBoxLayout;
	auto disposition_boutons = new QHBoxLayout;

	auto bouton_accepter = new QPushButton("Accepter");
	auto bouton_annuler  = new QPushButton("Annuler");

	connect(bouton_accepter, &QPushButton::clicked, this, &QDialog::accept);
	connect(bouton_annuler, &QPushButton::clicked, this, &QDialog::reject);

	disposition_boutons->addStretch();
	disposition_boutons->addWidget(bouton_accepter);
	disposition_boutons->addWidget(bouton_annuler);

	disposition_principale->addLayout(disposition);
	disposition_principale->addLayout(disposition_boutons);

	this->setLayout(disposition_principale);
	this->resize(256, this->rect().height());
}
