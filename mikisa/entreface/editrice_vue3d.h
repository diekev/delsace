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

#include "base_editrice.h"

class EditriceVue3D;
class Mikisa;
class QToolButton;
class VisionneurScene;

/* ************************************************************************** */

class VueCanevas3D : public QGLWidget {
	Mikisa *m_mikisa;
	VisionneurScene *m_visionneur_scene;
	EditriceVue3D *m_base;

public:
	explicit VueCanevas3D(Mikisa *mikisa, EditriceVue3D *base, QWidget *parent = nullptr);

	~VueCanevas3D();

	void initializeGL();

	void paintGL();

	void resizeGL(int w, int h);

	void mousePressEvent(QMouseEvent *e) override;

	void mouseMoveEvent(QMouseEvent *e) override;

	void wheelEvent(QWheelEvent *e) override;

	void mouseReleaseEvent(QMouseEvent *) override;

	void reconstruit_scene() const;
};

/* ************************************************************************** */

class EditriceVue3D : public BaseEditrice {
	Q_OBJECT

	VueCanevas3D *m_vue;
	QToolButton *m_bouton_position;
	QToolButton *m_bouton_rotation;
	QToolButton *m_bouton_echelle;
	QToolButton *m_bouton_actif;

public:
	explicit EditriceVue3D(Mikisa *mikisa, QWidget *parent = nullptr);

	void ajourne_etat(int event) override;

	void ajourne_manipulable() override {}

private Q_SLOTS:
	void bascule_manipulation();
	void manipule_rotation();
	void manipule_position();
	void manipule_echelle();
};
