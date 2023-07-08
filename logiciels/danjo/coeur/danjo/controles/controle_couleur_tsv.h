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

#include "biblinternes/phys/couleur.hh"

#include "base_controle.hh"

namespace danjo {

/* ************************************************************************** */

class ControleSatVal final : public BaseControle {
    int pad0 = 0;
    dls::phys::couleur32 m_hsv{};
    double m_pos_x = 0.0;
    double m_pos_y = 0.0;

  public:
    explicit ControleSatVal(QWidget *parent = nullptr);

    void couleur(const dls::phys::couleur32 &c);

    float saturation() const;

    float valeur() const;

    void paintEvent(QPaintEvent * /*event*/) override;

    RéponseÉvènement gère_clique_souris(QMouseEvent *event) override;

    void gère_mouvement_souris(QMouseEvent *event) override;
};

/* ************************************************************************** */

class SelecteurTeinte final : public BaseControle {
    double m_teinte = 0.0;
    double m_pos_x = 0.0;

  public:
    explicit SelecteurTeinte(QWidget *parent = nullptr);

    void teinte(float t);

    float teinte() const;

    void paintEvent(QPaintEvent * /*event*/) override;

    RéponseÉvènement gère_clique_souris(QMouseEvent *event) override;

    void gère_mouvement_souris(QMouseEvent *event) override;
};

/* ************************************************************************** */

class ControleValeurCouleur final : public BaseControle {
    double m_valeur = 0.0;
    double m_pos_y = 0.0;

  public:
    explicit ControleValeurCouleur(QWidget *parent = nullptr);

    void valeur(float t);

    float valeur() const;

    void paintEvent(QPaintEvent * /*event*/) override;

    RéponseÉvènement gère_clique_souris(QMouseEvent *event) override;

    void gère_mouvement_souris(QMouseEvent *event) override;
};

} /* namespace danjo */
