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

#include "base_dialogue.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QDialogButtonBox>
#include <QGridLayout>
#pragma GCC diagnostic pop

#include "coeur/configuration.h"
#include "coeur/koudou.h"

/* ************************************************************************** */

BaseDialogue::BaseDialogue(Koudou &koudou, QWidget *parent)
    : QDialog(parent)
	, m_agencement(new QVBoxLayout(this))
	, m_agencement_grille(new QGridLayout())
	, m_koudou(&koudou)
{
	this->setWindowTitle("Preferences");

	auto button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

	connect(button_box, SIGNAL(accepted()), this, SLOT(accept()));
	connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));

	m_agencement->addLayout(m_agencement_grille);
	m_agencement->addWidget(button_box);
}

void BaseDialogue::montre()
{
//	cree_controles(m_assembleur_controles, m_koudou->configuration);
	this->show();
}

void BaseDialogue::ajourne()
{
	auto config = m_koudou->configuration;
	config->ajourne();
}

/* ************************************************************************** */

ProjectSettingsDialog::ProjectSettingsDialog(Koudou &koudou, QWidget *parent)
    : QDialog(parent)
	, m_agencement(new QVBoxLayout(this))
	, m_agencement_grille(new QGridLayout())
	, m_koudou(&koudou)
{
	this->setWindowTitle("Project Settings");

	auto button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

	connect(button_box, SIGNAL(accepted()), this, SLOT(accept()));
	connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));

	m_agencement->addLayout(m_agencement_grille);
	m_agencement->addWidget(button_box);
}

void ProjectSettingsDialog::montre()
{
//	cree_controles(m_assembleur_controles, m_koudou->parametres_projet);
	this->show();
}

void ProjectSettingsDialog::ajourne()
{
	auto settings = m_koudou->parametres_projet;
	settings->ajourne();
}
