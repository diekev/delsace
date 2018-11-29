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

#include "base_editrice.h"

#include <danjo/manipulable.h>

class QFrame;
class QGridLayout;
class QHBoxLayout;
class QScrollArea;

class EditriceRendu : public BaseEditrice {
	Q_OBJECT

	danjo::Manipulable m_manipulable;

	QWidget *m_widget;
	QWidget *m_conteneur_disposition;
	QScrollArea *m_scroll;
	QVBoxLayout *m_disposition_widget;

public:
	explicit EditriceRendu(Mikisa *mikisa, QWidget *parent = nullptr);

	void ajourne_etat(int evenement) override;

	void ajourne_manipulable() override;

	void obtiens_liste(const std::string &attache, std::vector<std::string> &chaines) override;
};
