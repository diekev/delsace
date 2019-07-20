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

class Nuanceur;
class QScrollArea;
class QGridLayout;

/* ************************************************************************** */

class VueMaterial {
	danjo::Manipulable *m_persona_diffus;
	danjo::Manipulable *m_persona_angle_vue;
	danjo::Manipulable *m_persona_reflection;
	danjo::Manipulable *m_persona_refraction;
	danjo::Manipulable *m_persona_volume;
	danjo::Manipulable *m_persona_emission;

	Nuanceur *m_nuanceur;

public:
	explicit VueMaterial(Nuanceur *nuaceur);
	~VueMaterial();

	/* pour faire taire cppcheck */
	VueMaterial(VueMaterial const &) = delete;
	VueMaterial &operator=(VueMaterial const &) = delete;

	void nuanceur(Nuanceur *nuanceur);

	void ajourne_donnees();
	bool ajourne_proprietes();

	danjo::Manipulable *persona() const;
};

/* ************************************************************************** */

class EditeurMaterial final : public BaseEditrice {
	Q_OBJECT

	VueMaterial *m_vue = nullptr;

	QWidget *m_widget;
	QScrollArea *m_scroll;
	QGridLayout *m_glayout;

public:
	EditeurMaterial(Koudou *koudou, QWidget *parent = nullptr);

	EditeurMaterial(EditeurMaterial const &) = default;
	EditeurMaterial &operator=(EditeurMaterial const &) = default;

	~EditeurMaterial() override;

	void ajourne_etat(int /*evenement*/) override;

private Q_SLOTS:
	void ajourne_material();
};
