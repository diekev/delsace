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

#include <glm/glm.hpp>

#include <image/pixel.h>
#include <math/matrice/matrice.h>

#include <QGLWidget>

#include "bibliotheques/opengl/contexte_rendu.h"
#include "bibliotheques/vision/camera_2d.h"

#include "base_editrice.h"

/* ************************************************************************** */

class EditriceVue2D;
class RenduImage;
class RenduManipulatrice2D;

class Visionneuse2D : public QGLWidget {
	RenduImage *m_rendu_image = nullptr;
	RenduManipulatrice2D *m_rendu_manipulatrice = nullptr;
	Mikisa *m_mikisa;
	EditriceVue2D *m_base;

	ContexteRendu m_contexte;
	glm::mat4 m_matrice_image;

public:
	explicit Visionneuse2D(Mikisa *mikisa, EditriceVue2D *base, QWidget *parent = nullptr);
	~Visionneuse2D();

	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);
	void charge_image(const numero7::math::matrice<numero7::image::Pixel<float> > &image);
	void wheelEvent(QWheelEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
};

/* ************************************************************************** */

class EditriceVue2D : public BaseEditrice {
	Q_OBJECT

	Visionneuse2D *m_vue;

public:
	explicit EditriceVue2D(Mikisa *mikisa, QWidget *parent = nullptr);

	void ajourne_etat(int event) override;

	void ajourne_manipulable() override {}
};
