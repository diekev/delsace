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

class Silvatheque;
class QGridLayout;
class QScrollArea;

class VueArbre : public danjo::Manipulable {
	Silvatheque *m_silvatheque;

public:
	explicit VueArbre(Silvatheque *silvatheque);

	VueArbre(VueArbre const &) = default;
	VueArbre &operator=(VueArbre const &) = default;

	void ajourne_donnees();

	bool ajourne_proprietes() override;
};

class EditeurArbre final : public BaseEditrice {
	Q_OBJECT

	VueArbre *m_vue;

	QWidget *m_widget;
	QWidget *m_conteneur_disposition;
	QScrollArea *m_scroll;
	QVBoxLayout *m_glayout;

public:
	EditeurArbre(Silvatheque *silvatheque, QWidget *parent = nullptr);

	EditeurArbre(EditeurArbre const &) = default;
	EditeurArbre &operator=(EditeurArbre const &) = default;

	~EditeurArbre() override;

	void ajourne_etat(int evenement) override;

	void ajourne_manipulable() override;
};
