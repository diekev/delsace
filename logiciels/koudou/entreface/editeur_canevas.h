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

#include "biblinternes/image/pixel.h"
#include "biblinternes/math/matrice/matrice.hh"
#include "biblinternes/math/vecteur.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QGLWidget>
#pragma GCC diagnostic pop

#include "base_editeur.h"

class QScrollArea;

/* ************************************************************************** */

class VisionneurImage;
class VisionneurScene;

class VueCanevas : public QGLWidget {
	kdo::Koudou *m_koudou;
	VisionneurImage *m_visionneur_image;

public:
	explicit VueCanevas(kdo::Koudou *koudou, QWidget *parent = nullptr);
	~VueCanevas() override;

	VueCanevas(VueCanevas const &) = default;
	VueCanevas &operator=(VueCanevas const &) = default;

	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int w, int h) override;

	void charge_image(dls::math::matrice_dyn<dls::math::vec3d> const &image);

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
	explicit EditeurCanevas(kdo::Koudou &koudou, QWidget *parent = nullptr);

	EditeurCanevas(EditeurCanevas const &) = default;
	EditeurCanevas &operator=(EditeurCanevas const &) = default;

	~EditeurCanevas() override;

	void ajourne_etat(int event) override;
};

/* ************************************************************************** */

class VueCanevas3D : public QGLWidget {
	kdo::Koudou *m_koudou;
	VisionneurScene *m_visionneur_scene;

public:
	explicit VueCanevas3D(kdo::Koudou *koudou, QWidget *parent = nullptr);

	VueCanevas3D(VueCanevas3D const &) = default;
	VueCanevas3D &operator=(VueCanevas3D const &) = default;

	~VueCanevas3D() override;

	void initializeGL() override;

	void paintGL() override;

	void resizeGL(int w, int h) override;

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
	explicit EditriceVue3D(kdo::Koudou &koudou, QWidget *parent = nullptr);

	EditriceVue3D(EditriceVue3D const &) = default;
	EditriceVue3D &operator=(EditriceVue3D const &) = default;

	void ajourne_etat(int event) override;

	void resizeEvent(QResizeEvent *event) override;
};
