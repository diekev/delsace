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

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QGLWidget>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "base_editrice.h"

class EditriceVue3D;
class QComboBox;
class QToolButton;
class VisionneurScene;

/* ************************************************************************** */

class VueCanevas3D : public QGLWidget {
    JJL::Jorjala &m_jorjala;
    VisionneurScene *m_visionneur_scene;
    EditriceVue3D *m_base;

  public:
    explicit VueCanevas3D(JJL::Jorjala &jorjala, EditriceVue3D *base, QWidget *parent = nullptr);

    VueCanevas3D(VueCanevas3D const &) = delete;
    VueCanevas3D &operator=(VueCanevas3D const &) = delete;

    ~VueCanevas3D() override;

    void initializeGL() override;

    void paintGL() override;

    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent *e) override;

    void mouseMoveEvent(QMouseEvent *e) override;

    void wheelEvent(QWheelEvent *e) override;

    void mouseReleaseEvent(QMouseEvent *) override;

    void reconstruit_scene() const;
    void change_moteur_rendu(const dls::chaine &id) const;
};

/* ************************************************************************** */

class EditriceVue3D : public BaseEditrice {
    Q_OBJECT

    VueCanevas3D *m_vue{};
    QToolButton *m_bouton_position{};
    QToolButton *m_bouton_rotation{};
    QToolButton *m_bouton_echelle{};
    QToolButton *m_bouton_actif{};
    QComboBox *m_selecteur_rendu{};

  public:
    explicit EditriceVue3D(JJL::Jorjala &jorjala,
                           JJL::Éditrice éditrice,
                           QWidget *parent = nullptr);

    EditriceVue3D(EditriceVue3D const &) = delete;
    EditriceVue3D &operator=(EditriceVue3D const &) = delete;

    void ajourne_état(JJL::ChangementÉditrice changement) override;

    void ajourne_manipulable() override
    {
    }

  private Q_SLOTS:
    void bascule_manipulation();
    void manipule_rotation();
    void manipule_position();
    void manipule_echelle();
    void change_moteur_rendu(int idx);
};
