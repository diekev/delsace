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
class ControleNombreDecimal;
class QHBoxLayout;
class QPushButton;

namespace danjo {

class ControleProprieteVecteurDecimal final : public ControlePropriete {
    Q_OBJECT

    static constexpr int DIMENSIONS_MAX = 4;

    QHBoxLayout *m_agencement{};
    ControleNombreDecimal *m_dim[DIMENSIONS_MAX]{};

    QPushButton *m_bouton_animation{};
    QPushButton *m_bouton_echelle_dim[DIMENSIONS_MAX]{};
    ControleEchelleDecimale *m_echelle[DIMENSIONS_MAX]{};

    int m_dimensions = 0;

  public:
    explicit ControleProprieteVecteurDecimal(BasePropriete *p, int temps, QWidget *parent = nullptr);
    ~ControleProprieteVecteurDecimal() override;

    ControleProprieteVecteurDecimal(ControleProprieteVecteurDecimal const &) = default;
    ControleProprieteVecteurDecimal &operator=(ControleProprieteVecteurDecimal const &) = default;

    void finalise(const DonneesControle &donnees) override;

  private Q_SLOTS:
    void ajourne_valeur_x(float valeur);
    void ajourne_valeur_y(float valeur);
    void ajourne_valeur_z(float valeur);
    void montre_echelle_x();
    void montre_echelle_y();
    void montre_echelle_z();
    void bascule_animation();

  private:
    void montre_echelle(int index);
    void ajourne_valeur(int index, float valeur);
};

} /* namespace danjo */
