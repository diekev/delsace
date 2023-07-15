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

/* Nous devons inclure ces fichiers avant editeur_canevas.h à cause de l'ordre
 * d'inclusion des fichiers OpenGL (gl.h doit être inclu après glew.h, etc.). */
#include "opengl/visionneur_image.h"
#include "opengl/visionneur_scene.h"

#include "editeur_canevas.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QMouseEvent>
#include <QVBoxLayout>
#pragma GCC diagnostic pop

#include "coeur/evenement.h"
#include "coeur/kanba.h"

/* ------------------------------------------------------------------------- */
/** \name Vue Canevas 2D
 * \{ */

VueCanevas2D::VueCanevas2D(Kanba *kanba, QWidget *parent)
    : QGLWidget(parent), m_visionneur_image(new VisionneurImage(this, kanba))
{
    setMouseTracking(true);
}

VueCanevas2D::~VueCanevas2D()
{
    delete m_visionneur_image;
}

void VueCanevas2D::initializeGL()
{
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();

    if (err != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(err) << "\n";
    }

    m_visionneur_image->initialise();
}

void VueCanevas2D::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_visionneur_image->peint_opengl();
}

void VueCanevas2D::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    m_visionneur_image->redimensionne(w, h);
}

void VueCanevas2D::charge_image(dls::math::matrice_dyn<dls::math::vec4f> const &image)
{
    m_visionneur_image->charge_image(image);
}

void VueCanevas2D::mousePressEvent(QMouseEvent *e)
{
    static_cast<EditriceCannevas2D *>(parent())->mousePressEvent(e);
}

void VueCanevas2D::mouseMoveEvent(QMouseEvent *e)
{
    static_cast<EditriceCannevas2D *>(parent())->mouseMoveEvent(e);
}

void VueCanevas2D::mouseReleaseEvent(QMouseEvent *e)
{
    static_cast<EditriceCannevas2D *>(parent())->mouseReleaseEvent(e);
}

void VueCanevas2D::wheelEvent(QWheelEvent *e)
{
    static_cast<EditriceCannevas2D *>(parent())->wheelEvent(e);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Éditrice 2D
 * \{ */

EditriceCannevas2D::EditriceCannevas2D(Kanba &kanba, QWidget *parent)
    : BaseEditrice("vue_2d", kanba, parent), m_vue(new VueCanevas2D(&kanba, this))
{
    m_agencement_principal->addWidget(m_vue);
}

void EditriceCannevas2D::ajourne_etat(int evenement)
{
    if (evenement == (type_evenement::dessin | type_evenement::fini)) {
        m_vue->charge_image(m_kanba->tampon);
    }
    m_vue->update();
}

void EditriceCannevas2D::resizeEvent(QResizeEvent * /*event*/)
{
    m_vue->resize(this->size());
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Vue Canevas 3D
 * \{ */

VueCanevas3D::VueCanevas3D(Kanba *kanba, QWidget *parent)
    : QGLWidget(parent), m_visionneur_scene(new VisionneurScene(this, kanba))
{
    setMouseTracking(true);
}

VueCanevas3D::~VueCanevas3D()
{
    delete m_visionneur_scene;
}

void VueCanevas3D::initializeGL()
{
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();

    if (err != GLEW_OK) {
        std::cerr << "Error: " << glewGetErrorString(err) << "\n";
    }

    m_visionneur_scene->initialise();
}

void VueCanevas3D::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_visionneur_scene->peint_opengl();
}

void VueCanevas3D::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    m_visionneur_scene->redimensionne(w, h);
}

void VueCanevas3D::enterEvent(QEvent * /*event*/)
{
    m_visionneur_scene->affiche_brosse(true);
    this->setCursor(Qt::BlankCursor);
    this->update();
}

void VueCanevas3D::leaveEvent(QEvent * /*event*/)
{
    m_visionneur_scene->affiche_brosse(false);
    this->unsetCursor();
    this->update();
}

void VueCanevas3D::mousePressEvent(QMouseEvent *e)
{
    static_cast<EditriceCannevas3D *>(parent())->mousePressEvent(e);
}

void VueCanevas3D::mouseMoveEvent(QMouseEvent *e)
{
    this->m_visionneur_scene->position_souris(e->pos().x(), e->pos().y());
    this->update();
    static_cast<EditriceCannevas3D *>(parent())->mouseMoveEvent(e);
}

void VueCanevas3D::mouseReleaseEvent(QMouseEvent *e)
{
    static_cast<EditriceCannevas3D *>(parent())->mouseReleaseEvent(e);
}

void VueCanevas3D::wheelEvent(QWheelEvent *e)
{
    static_cast<EditriceCannevas3D *>(parent())->wheelEvent(e);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Éditrice 3D
 * \{ */

EditriceCannevas3D::EditriceCannevas3D(Kanba &kanba, QWidget *parent)
    : BaseEditrice("vue_3d", kanba, parent), m_vue(new VueCanevas3D(&kanba, this))
{
    m_agencement_principal->addWidget(m_vue);
}

void EditriceCannevas3D::ajourne_etat(int evenement)
{
    m_vue->update();
}

void EditriceCannevas3D::resizeEvent(QResizeEvent * /*event*/)
{
    m_vue->resize(this->size());
}

/** \} */
