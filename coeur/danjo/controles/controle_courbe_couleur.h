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

struct CourbeBezier;
struct PointBezier;

class ControleCourbeCouleur : public QWidget {
	Q_OBJECT

	CourbeBezier *m_courbe = nullptr;
	PointBezier *m_point_courant = nullptr;
	int m_type_point = 0;
	int m_mode = 0;
	bool m_point_selectionne = false;

public:
	explicit ControleCourbeCouleur(QWidget *parent = nullptr);

	ControleCourbeCouleur(ControleCourbeCouleur const &) = default;
	ControleCourbeCouleur &operator=(ControleCourbeCouleur const &) = default;

	void change_mode(int mode);

	void installe_courbe(CourbeBezier *courbe);

	void ajourne_position_x(float v);
	void ajourne_position_y(float v);

	void paintEvent(QPaintEvent */*event*/) override;

	void mousePressEvent(QMouseEvent *event) override;

	void mouseMoveEvent(QMouseEvent *event) override;

	void mouseReleaseEvent(QMouseEvent */*event*/) override;

	void mouseDoubleClickEvent(QMouseEvent *event) override;

Q_SIGNALS:
	void position_changee(float x, float y);
	void point_change();
};
