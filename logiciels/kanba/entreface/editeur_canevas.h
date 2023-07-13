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

class VisionneurImage;
class VisionneurScene;
class QScrollArea;

/* ------------------------------------------------------------------------- */
/** \name Vue Canevas 2D
 * \{ */

class VueCanevas2D : public QGLWidget {
    VisionneurImage *m_visionneur_image;

  public:
    explicit VueCanevas2D(Kanba *kanba, QWidget *parent = nullptr);
    ~VueCanevas2D() override;

    VueCanevas2D(VueCanevas2D const &) = delete;
    VueCanevas2D &operator=(VueCanevas2D const &) = delete;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void charge_image(dls::math::matrice_dyn<dls::math::vec4f> const &image);

    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *) override;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Éditrice Canevas 2D
 * \{ */

class EditriceCannevas2D : public BaseEditrice {
    VueCanevas2D *m_vue;

  public:
    explicit EditriceCannevas2D(Kanba &kanba, QWidget *parent = nullptr);

    EditriceCannevas2D(EditriceCannevas2D const &) = delete;
    EditriceCannevas2D &operator=(EditriceCannevas2D const &) = delete;

    void ajourne_etat(int evenement) override;

    void resizeEvent(QResizeEvent *event) override;

    void ajourne_manipulable() override
    {
    }
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Vue Canevas 3D
 * \{ */

class VueCanevas3D : public QGLWidget {
    VisionneurScene *m_visionneur_scene;

  public:
    explicit VueCanevas3D(Kanba *kanba, QWidget *parent = nullptr);
    ~VueCanevas3D() override;

    VueCanevas3D(VueCanevas3D const &) = delete;
    VueCanevas3D &operator=(VueCanevas3D const &) = delete;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *) override;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Éditrice Canevas 3D
 * \{ */

class EditriceCannevas3D : public BaseEditrice {
    VueCanevas3D *m_vue;

  public:
    explicit EditriceCannevas3D(Kanba &kanba, QWidget *parent = nullptr);

    EditriceCannevas3D(EditriceCannevas3D const &) = delete;
    EditriceCannevas3D &operator=(EditriceCannevas3D const &) = delete;

    void ajourne_etat(int evenement) override;

    void resizeEvent(QResizeEvent *event) override;

    void ajourne_manipulable() override
    {
    }
};

/** \} */
