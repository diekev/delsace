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

#include "danjo/manipulable.h"

#include "base_editeur.h"

class Monde;
class QScrollArea;
class QGridLayout;

class VueMonde final : public danjo::Manipulable {
	Monde *m_monde;

public:
	explicit VueMonde(Monde *monde);

	VueMonde(VueMonde const &) = default;
	VueMonde &operator=(VueMonde const &) = default;

	void ajourne_donnees();
	bool ajourne_proprietes() override;
};

class EditeurMonde final : public BaseEditrice {
	Q_OBJECT

	VueMonde *m_vue;

	QWidget *m_widget;
	QScrollArea *m_scroll;
	QGridLayout *m_glayout;

public:
	EditeurMonde(Koudou *koudou, QWidget *parent = nullptr);

	EditeurMonde(EditeurMonde const &) = default;
	EditeurMonde &operator=(EditeurMonde const &) = default;

	~EditeurMonde() override;

	void ajourne_etat(int /*evenement*/) override;

private Q_SLOTS:
	void ajourne_monde();
};
