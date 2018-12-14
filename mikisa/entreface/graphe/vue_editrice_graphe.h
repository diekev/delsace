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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QGraphicsView>
#include <QMenu>
#pragma GCC diagnostic pop

class EditriceGraphe;
class Mikisa;
class QMenu;
class RepondantCommande;

namespace danjo {
class GestionnaireInterface;
}  /* namespace danjo */

class VueEditeurNoeud : public QGraphicsView {
	Mikisa &m_mikisa;
	EditriceGraphe *m_base;

	QMenu *m_menu_ajout_noeud_composite = nullptr;
	QMenu *m_menu_ajout_noeud_point3d = nullptr;
	QMenu *m_menu_ajout_noeud_objet = nullptr;
	QMenu *m_menu_ajout_noeud_scene = nullptr;

public:
	VueEditeurNoeud(
			Mikisa &mikisa,
			EditriceGraphe *base,
			QWidget *parent = nullptr);

	VueEditeurNoeud(VueEditeurNoeud const &) = default;
	VueEditeurNoeud &operator=(VueEditeurNoeud const &) = default;

	~VueEditeurNoeud() override;

	void mousePressEvent(QMouseEvent *event) override;

	void mouseDoubleClickEvent(QMouseEvent *event) override;

	void mouseReleaseEvent(QMouseEvent *event) override;

	/* Réimplémentation pour éviter les conflits entre le zoom et les barres de
	 * défilement. */
	void wheelEvent(QWheelEvent *event) override;

	void mouseMoveEvent(QMouseEvent *event) override;

	void keyPressEvent(QKeyEvent *event) override;

	bool focusNextPrevChild(bool) override;
};

