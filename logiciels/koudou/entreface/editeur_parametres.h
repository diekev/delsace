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

#include "coeur/persona.h"

#include "controles/assembleur_controles.h"
#include "base_editeur.h"

class Koudou;
class QScrollArea;

class VueParametres : public Persona {
	Koudou *m_koudou;

public:
	explicit VueParametres(Koudou *koudou);

	VueParametres(VueParametres const &) = default;
	VueParametres &operator=(VueParametres const &) = default;

	void ajourne_donnees();
	bool ajourne_proprietes() override;
};

class EditeurParametres final : public BaseEditrice {
	Q_OBJECT

	VueParametres *m_vue;

	QWidget *m_widget;
	QScrollArea *m_scroll;
	QGridLayout *m_glayout;
	AssembleurControles m_assembleur_controles;

public:
	EditeurParametres(Koudou *koudou, QWidget *parent = nullptr);

	EditeurParametres(EditeurParametres const &) = default;
	EditeurParametres &operator=(EditeurParametres const &) = default;

	~EditeurParametres() override;

	void ajourne_etat(int evenement) override;

private Q_SLOTS:
	void ajourne_vue();
};
