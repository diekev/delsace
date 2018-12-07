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

#include <QWidget>

#include "types/couleur.h"

namespace danjo {

class DialogueCouleur;

class ControleCouleur final : public QWidget {
	Q_OBJECT

	DialogueCouleur *m_dialogue;
	couleur32 m_couleur{};

public:
	explicit ControleCouleur(QWidget *parent = nullptr);

	ControleCouleur(ControleCouleur const &) = default;
	ControleCouleur &operator=(ControleCouleur const &) = default;

	couleur32 couleur();

	void couleur(const couleur32 &c);

	void ajourne_plage(const float min, const float max);

	void mouseReleaseEvent(QMouseEvent *e) override;

	void paintEvent(QPaintEvent *) override;

private Q_SLOTS:
	void ajourne_couleur();

Q_SIGNALS:
	void couleur_changee();
};

}  /* namespace danjo */
