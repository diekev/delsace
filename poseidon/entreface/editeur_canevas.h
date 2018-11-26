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

#include <QGLWidget>
#include "base_editeur.h"

class QScrollArea;

/* ************************************************************************** */

class VisionneurScene;

class VueCanevas : public QGLWidget {
	VisionneurScene *m_visionneur_scene;

	Poseidon *m_poseidon;

public:
	explicit VueCanevas(Poseidon *poseidon, QWidget *parent = nullptr);
	~VueCanevas();

	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);

	void mousePressEvent(QMouseEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void wheelEvent(QWheelEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *) override;
};

/* ************************************************************************** */

class EditeurCanevas : public BaseEditrice {
	Q_OBJECT

	VueCanevas *m_vue;

public:
	explicit EditeurCanevas(Poseidon &poseidon, QWidget *parent = nullptr);

	void ajourne_etat(int evenement) override;

	void resizeEvent(QResizeEvent *event) override;

	void ajourne_manipulable() override {}
};
