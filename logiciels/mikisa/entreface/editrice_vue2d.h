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

#include "biblinternes/image/pixel.h"
#include "biblinternes/math/matrice/matrice.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QGLWidget>
#pragma GCC diagnostic pop

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/opengl/contexte_rendu.h"
#include "biblinternes/vision/camera_2d.h"

#include "base_editrice.h"

/* ************************************************************************** */

class EditriceVue2D;
class RenduTexte;
class RenduImage;
class RenduManipulatrice2D;

class Visionneuse2D : public QGLWidget {
	RenduImage *m_rendu_image = nullptr;
	RenduManipulatrice2D *m_rendu_manipulatrice = nullptr;
	Mikisa &m_mikisa;
	EditriceVue2D *m_base;
	RenduTexte *m_rendu_texte = nullptr;

	ContexteRendu m_contexte{};
	dls::math::mat4x4f m_matrice_image{};
	dls::chrono::metre_seconde m_chrono_rendu{};

public:
	explicit Visionneuse2D(Mikisa &mikisa, EditriceVue2D *base, QWidget *parent = nullptr);
	~Visionneuse2D() override;

	Visionneuse2D(Visionneuse2D const &) = default;
	Visionneuse2D &operator=(Visionneuse2D const &) = default;

	void initializeGL() override;
	void paintGL() override;
	void resizeGL(int w, int h) override;
	void charge_image(const dls::math::matrice_dyn<dls::image::Pixel<float> > &image);
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
	explicit EditriceVue2D(Mikisa &mikisa, QWidget *parent = nullptr);

	EditriceVue2D(EditriceVue2D const &) = default;
	EditriceVue2D &operator=(EditriceVue2D const &) = default;

	void ajourne_etat(int event) override;

	void ajourne_manipulable() override {}
};
