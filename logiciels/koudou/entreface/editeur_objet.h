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

#include "base_editeur.h"
#include "danjo/manipulable.h"

namespace kdo {
class Objet;
}
class QScrollArea;
class QGridLayout;

class VueObjet final : public danjo::Manipulable {
	kdo::Objet *m_objet = nullptr;

public:
	VueObjet();

	VueObjet(VueObjet const &) = default;
	VueObjet &operator=(VueObjet const &) = default;

	void objet(kdo::Objet *o);

	void ajourne_donnees();
	bool ajourne_proprietes() override;
};

class EditeurObjet final : public BaseEditrice {
	Q_OBJECT

	VueObjet *m_vue;

	QWidget *m_widget;
	QScrollArea *m_scroll;
	QGridLayout *m_glayout;

public:
	EditeurObjet(kdo::Koudou *koudou, QWidget *parent = nullptr);

	EditeurObjet(EditeurObjet const &) = default;
	EditeurObjet &operator=(EditeurObjet const &) = default;

	~EditeurObjet() override;

	void ajourne_etat(int evenement) override;

private Q_SLOTS:
	void ajourne_maillage();
};
