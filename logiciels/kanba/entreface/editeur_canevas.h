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

/* ------------------------------------------------------------------------- */
/** \name Vue Canevas 2D
 * \{ */

class VueCanevas2D : public QGLWidget {
    VisionneurImage *m_visionneur_image;

  public:
    explicit VueCanevas2D(KNB::Kanba &kanba, QWidget *parent = nullptr);
    ~VueCanevas2D() override;

    EMPECHE_COPIE(VueCanevas2D);

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void charge_image();

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
    explicit EditriceCannevas2D(KNB::Kanba &kanba,
                                KNB::Éditrice &éditrice,
                                QWidget *parent = nullptr);

    EMPECHE_COPIE(EditriceCannevas2D);

    void ajourne_état(KNB::ChangementÉditrice evenement) override;

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
    explicit VueCanevas3D(KNB::Kanba &kanba, QWidget *parent = nullptr);
    ~VueCanevas3D() override;

    EMPECHE_COPIE(VueCanevas3D);

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
    explicit EditriceCannevas3D(KNB::Kanba &kanba,
                                KNB::Éditrice &éditrice,
                                QWidget *parent = nullptr);

    EMPECHE_COPIE(EditriceCannevas3D);

    void ajourne_état(KNB::ChangementÉditrice evenement) override;

    void resizeEvent(QResizeEvent *event) override;

    void ajourne_manipulable() override
    {
    }
};

/** \} */
