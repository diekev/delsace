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

class ProjectiveCamera;
class QScrollArea;

class VueCamera final : public Persona {
	ProjectiveCamera *m_camera;

public:
	explicit VueCamera(ProjectiveCamera *camera);

	VueCamera(VueCamera const &) = default;
	VueCamera &operator=(VueCamera const &) = default;

	void ajourne_donnees();
	bool ajourne_proprietes() override;
};

class EditeurCamera final : public BaseEditrice {
	Q_OBJECT

	VueCamera *m_vue{};

	QWidget *m_widget;
	QScrollArea *m_scroll;
	QGridLayout *m_glayout;
	AssembleurControles m_assembleur_controles;

public:
	EditeurCamera(Koudou *koudou, QWidget *parent = nullptr);

	EditeurCamera(EditeurCamera const &) = default;
	EditeurCamera &operator=(EditeurCamera const &) = default;

	~EditeurCamera() override;

	void ajourne_etat(int /*evenement*/) override;

private Q_SLOTS:
	void ajourne_camera();
};
