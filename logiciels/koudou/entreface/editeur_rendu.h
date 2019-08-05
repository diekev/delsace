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

#pragma once

#include "base_editeur.h"

class ImageNode;
class AssembleurControles;
class QFrame;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QScrollArea;

class EditriceRendu : public BaseEditrice {
	Q_OBJECT

	QWidget *m_widget;
	QScrollArea *m_scroll;
	QGridLayout *m_glayout;

	QLabel *m_info_temps_ecoule, *m_info_temps_restant, *m_info_temps_echantillon;
	QLabel *m_info_echantillon;

public:
	explicit EditriceRendu(kdo::Koudou &koudou, QWidget *parent = nullptr);

	EditriceRendu(EditriceRendu const &) = default;
	EditriceRendu &operator=(EditriceRendu const &) = default;

	void ajourne_etat(int event) override;

private Q_SLOTS:
	void demarre_rendu();
	void arrete_rendu();
};
