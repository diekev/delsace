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

#include <numero7/math/matrice/matrice.h>
#include <delsace/math/vecteur.hh>

#include <QGLWidget>
#include "base_editeur.h"

class QScrollArea;

/* ************************************************************************** */

enum {
	VISIONNAGE_IMAGE = 0,
	VISIONNAGE_SCENE = 1,
};

class VisionneurImage;
class VisionneurScene;

class VueCanevas : public QGLWidget {
	VisionneurImage *m_visionneur_image;
	VisionneurScene *m_visionneur_scene;

	Kanba *m_kanba;

	int m_mode_visionnage = VISIONNAGE_SCENE;

public:
	explicit VueCanevas(Kanba *kanba, QWidget *parent = nullptr);
	~VueCanevas();

	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);

	void charge_image(const numero7::math::matrice<dls::math::vec4f> &image);

	void mode_visionnage(int mode);

	int mode_visionnage() const;

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
	explicit EditeurCanevas(Kanba &kanba, QWidget *parent = nullptr);

	void ajourne_etat(int evenement) override;

	void resizeEvent(QResizeEvent *event) override;

	void ajourne_manipulable() override {}

private Q_SLOTS:
	void change_mode_visionnage(int mode);
};
