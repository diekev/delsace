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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <QWidget>

class QComboBox;
class QVBoxLayout;

class ColorWidget final : public QWidget {
	Q_OBJECT
	int m_role = 0;
	QVector<QPointF> m_curves[4];
	QPointF *m_cur_point = nullptr;
	bool m_mouse_down = false;

	void setCurvesPos();

public:
	ColorWidget(QWidget *parent = nullptr);
	~ColorWidget() = default;

public Q_SLOTS:
	void setRole(int role);

protected:
	void paintEvent(QPaintEvent *);
	void resizeEvent(QResizeEvent *);
	void mousePressEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
};

class ColorPickerWidget final : public QWidget {
	ColorWidget *m_frame;
	QVBoxLayout *m_layout;
	QComboBox *m_role;

public:
	ColorPickerWidget(QWidget *parent = nullptr);
	~ColorPickerWidget() = default;
};
