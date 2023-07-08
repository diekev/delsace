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

#include "base_controle.hh"

class RampeCouleur;
class PointRampeCouleur;

class ControleRampeCouleur : public BaseControle {
    Q_OBJECT

    RampeCouleur *m_rampe = nullptr;
    PointRampeCouleur *m_point_courant = nullptr;
    bool m_point_selectionne = false;
    char pad[7];

  public:
    explicit ControleRampeCouleur(QWidget *parent = nullptr);

    ControleRampeCouleur(ControleRampeCouleur const &) = default;
    ControleRampeCouleur &operator=(ControleRampeCouleur const &) = default;

    void installe_rampe(RampeCouleur *rampe);

    void paintEvent(QPaintEvent *event) override;

    RéponseÉvènement gère_clique_souris(QMouseEvent *event) override;

    void gère_mouvement_souris(QMouseEvent *event) override;

    void gère_fin_clique_souris(QMouseEvent *event) override;

    RéponseÉvènement gère_double_clique_souris(QMouseEvent *event) override;

    void ajourne_position(float x);

  private:
    float position_degrade(float x);

  Q_SIGNALS:
    void point_change();
    void controle_ajoute();

    void position_modifie(float x);
};
