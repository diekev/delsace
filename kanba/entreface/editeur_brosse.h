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

#pragma once

#include <danjo/manipulable.h>

#include "base_editeur.h"

class Kanba;
class QGridLayout;
class QScrollArea;

class VueBrosse : public danjo::Manipulable {
	Kanba *m_kanba;

public:
	explicit VueBrosse(Kanba *kanba);

	void ajourne_donnees();

	bool ajourne_proprietes() override;
};

class EditeurBrosse final : public BaseEditrice {
	Q_OBJECT

	VueBrosse *m_vue;

	QWidget *m_widget;
	QWidget *m_conteneur_disposition;
	QScrollArea *m_scroll;
	QVBoxLayout *m_glayout;

public:
	EditeurBrosse(Kanba *kanba, QWidget *parent = nullptr);

	~EditeurBrosse();

	void ajourne_etat(int evenement) override;

	void ajourne_manipulable() override;
};
