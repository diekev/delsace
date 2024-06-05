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
class ControleRampeCouleur;
class RampeCouleur;
class QComboBox;
class QHBoxLayout;
class QPushButton;
class QVBoxLayout;

namespace danjo {

class ControleCouleur;

class ControleProprieteRampeCouleur final : public ControlePropriete {
    Q_OBJECT

    /* entreface */
    QVBoxLayout *m_agencement_principal{};
    QHBoxLayout *m_agencement_nombre{};

    QComboBox *m_entrepolation{};

    /* rampe */
    ControleRampeCouleur *m_controle_rampe{};

    /* controle de la position du point sélectionné */
    QPushButton *m_bouton_echelle{};
    ControleNombreDecimal *m_pos{};
    ControleEchelleDecimale *m_echelle{};

    /* controle de la couleur */
    ControleCouleur *m_controle_couleur{};

    /* connexion */
    RampeCouleur *m_rampe{};

  public:
    explicit ControleProprieteRampeCouleur(BasePropriete *p, int temps, QWidget *parent = nullptr);
    ~ControleProprieteRampeCouleur() override;

    EMPECHE_COPIE(ControleProprieteRampeCouleur);

    void finalise(const DonneesControle &donnees) override;

  private Q_SLOTS:
    void controle_ajoute();

    void ajourne_position(float x);
    void ajourne_position_controle(float v);
    void ajourne_point_actif();

    void ajourne_entrepolation(int i);

    void ajourne_couleur();
};

} /* namespace danjo */
