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

#include <image/pixel.h>
#include <math/matrice/matrice.h>
#include <math/vec3.h>

#include <QGLWidget>
#include "base_editeur.h"

class QScrollArea;

/* ************************************************************************** */

class VisionneurImage;
class VisionneurScene;

class VueCanevas : public QGLWidget {
	Koudou *m_koudou;
	VisionneurImage *m_visionneur_image;

public:
	explicit VueCanevas(Koudou *koudou, QWidget *parent = nullptr);
	~VueCanevas();

	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);

	void charge_image(const numero7::math::matrice<numero7::math::vec3d> &image);

	void mousePressEvent(QMouseEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void wheelEvent(QWheelEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *) override;
};

/* ************************************************************************** */

class EditeurCanevas : public BaseEditrice {
	Q_OBJECT

	VueCanevas *m_vue;

	QScrollArea *m_zone_defilement;

public:
	explicit EditeurCanevas(Koudou &koudou, QWidget *parent = nullptr);

	~EditeurCanevas();

	void ajourne_etat(int event) override;
};

/* ************************************************************************** */

class VueCanevas3D : public QGLWidget {
	Koudou *m_koudou;
	VisionneurScene *m_visionneur_scene;

public:
	explicit VueCanevas3D(Koudou *koudou, QWidget *parent = nullptr);

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

public:
	explicit EditriceVue3D(Koudou &koudou, QWidget *parent = nullptr);

	void ajourne_etat(int event) override;

	void resizeEvent(QResizeEvent *event) override;
};
