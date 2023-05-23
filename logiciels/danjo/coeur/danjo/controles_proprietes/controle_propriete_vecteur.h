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

#include "controle_propriete.h"

class ControleEchelleDecimale;
class ControleEchelleEntiere;
class ControleNombreDecimal;
class ControleNombreEntier;
class QHBoxLayout;
class QPushButton;

namespace danjo {

/* ************************************************************************* */

class BaseControleProprieteVecteur : public ControlePropriete {
    Q_OBJECT

  protected:
    static constexpr int DIMENSIONS_MAX = 4;
    int m_dimensions = 0;

    QHBoxLayout *m_agencement{};
    QPushButton *m_bouton_animation{};
    QPushButton *m_bouton_echelle_dim[DIMENSIONS_MAX]{};

  public:
    BaseControleProprieteVecteur(BasePropriete *p, int temps, QWidget *parent = nullptr);

    BaseControleProprieteVecteur(BaseControleProprieteVecteur const &) = default;
    BaseControleProprieteVecteur &operator=(BaseControleProprieteVecteur const &) = default;

  protected Q_SLOTS:
    void montre_echelle_0();
    void montre_echelle_1();
    void montre_echelle_2();
    void montre_echelle_3();

  protected:
    virtual void montre_echelle(int index) = 0;
};

/* ************************************************************************* */

class ControleProprieteVecteurDecimal final : public BaseControleProprieteVecteur {
    Q_OBJECT

    ControleNombreDecimal *m_dim[DIMENSIONS_MAX]{};
    ControleEchelleDecimale *m_echelle[DIMENSIONS_MAX]{};

  public:
    explicit ControleProprieteVecteurDecimal(BasePropriete *p,
                                             int temps,
                                             QWidget *parent = nullptr);
    ~ControleProprieteVecteurDecimal() override;

    ControleProprieteVecteurDecimal(ControleProprieteVecteurDecimal const &) = default;
    ControleProprieteVecteurDecimal &operator=(ControleProprieteVecteurDecimal const &) = default;

    void finalise(const DonneesControle &donnees) override;

  private Q_SLOTS:
    void ajourne_valeur_0(float valeur);
    void ajourne_valeur_1(float valeur);
    void ajourne_valeur_2(float valeur);
    void ajourne_valeur_3(float valeur);
    void bascule_animation();

  private:
    void ajourne_valeur(int index, float valeur);
    void montre_echelle(int index) override;
};

/* ************************************************************************* */

class ControleProprieteVecteurEntier final : public BaseControleProprieteVecteur {
    Q_OBJECT

    ControleNombreEntier *m_dim[DIMENSIONS_MAX]{};
    ControleEchelleEntiere *m_echelle[DIMENSIONS_MAX]{};

  public:
    explicit ControleProprieteVecteurEntier(BasePropriete *p,
                                            int temps,
                                            QWidget *parent = nullptr);
    ~ControleProprieteVecteurEntier() override;

    ControleProprieteVecteurEntier(ControleProprieteVecteurEntier const &) = default;
    ControleProprieteVecteurEntier &operator=(ControleProprieteVecteurEntier const &) = default;

    void finalise(const DonneesControle &donnees) override;

  private Q_SLOTS:
    void ajourne_valeur_0(int valeur);
    void ajourne_valeur_1(int valeur);
    void ajourne_valeur_2(int valeur);
    void ajourne_valeur_3(int valeur);
    void bascule_animation();

  private:
    void ajourne_valeur(int index, int valeur);
    void montre_echelle(int index) override;
};

} /* namespace danjo */
