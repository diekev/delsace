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

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/opengl/contexte_rendu.h"

#include "coeur/image.hh"

#include "base_editrice.h"

/* ************************************************************************** */

class EditriceVue2D;
class RenduTexte;
class RenduImage;
class RenduManipulatrice2D;

namespace JJL {
class Composite;
}

class Visionneuse2D : public QGLWidget {
    RenduImage *m_rendu_image = nullptr;
    RenduManipulatrice2D *m_rendu_manipulatrice = nullptr;
    JJL::Jorjala &m_jorjala;
    EditriceVue2D *m_base;
    RenduTexte *m_rendu_texte = nullptr;

    ContexteRendu m_contexte{};
    dls::math::mat4x4f m_matrice_image{};
    dls::chrono::metre_seconde m_chrono_rendu{};

  public:
    explicit Visionneuse2D(JJL::Jorjala &jorjala, EditriceVue2D *base, QWidget *parent = nullptr);
    ~Visionneuse2D() override;

    Visionneuse2D(Visionneuse2D const &) = delete;
    Visionneuse2D &operator=(Visionneuse2D const &) = delete;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void charge_image(const grille_couleur &image);
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void charge_composite(JJL::Composite composite);
};

/* ************************************************************************** */

class EditriceVue2D : public BaseEditrice {
    Q_OBJECT

    Visionneuse2D *m_vue;

  public:
    explicit EditriceVue2D(JJL::Jorjala &jorjala, QWidget *parent = nullptr);

    EditriceVue2D(EditriceVue2D const &) = delete;
    EditriceVue2D &operator=(EditriceVue2D const &) = delete;

    void ajourne_état(JJL::TypeÉvènement évènement) override;

    void ajourne_manipulable() override
    {
    }

  private:
    QPointF transforme_position_evenement(QPoint pos) override;
};
