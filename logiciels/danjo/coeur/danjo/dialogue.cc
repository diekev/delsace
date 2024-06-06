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
#include <QDialogButtonBox>
#include <QPushButton>

Dialogue::Dialogue(QLayout *disposition, QWidget *parent) : QDialog(parent)
{
    auto box = new QDialogButtonBox(this);

    auto bouton_accepter = box->addButton(QDialogButtonBox::StandardButton::Ok);
    auto bouton_annuler = box->addButton(QDialogButtonBox::StandardButton::Cancel);

    connect(bouton_accepter, &QPushButton::clicked, this, &QDialog::accept);
    connect(bouton_annuler, &QPushButton::clicked, this, &QDialog::reject);

    auto disposition_principale = new QVBoxLayout(this);
    disposition_principale->addLayout(disposition);
    disposition_principale->addWidget(box);

    this->resize(256, this->rect().height());
}
