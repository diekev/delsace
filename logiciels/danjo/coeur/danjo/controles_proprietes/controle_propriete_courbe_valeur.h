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

class ControleCourbeCouleur;
class ControleEchelleDecimale;
class ControleNombreDecimal;
class CourbeBezier;
class QCheckBox;
class QHBoxLayout;
class QPushButton;
class QVBoxLayout;

namespace danjo {

class ControleProprieteCourbeValeur final : public ControlePropriete {
    Q_OBJECT

    /* entreface */
    QVBoxLayout *m_agencement_principal{};
    QHBoxLayout *m_agencement_nombre{};

    /* courbe */
    QCheckBox *m_utilise_table{};
    ControleCourbeCouleur *m_controle_courbe{};

    /* controle de la position X du point sélectionné */
    QPushButton *m_bouton_echelle_x{};
    ControleNombreDecimal *m_pos_x{};
    ControleEchelleDecimale *m_echelle_x{};

    /* controle de la position Y du point sélectionné */
    QPushButton *m_bouton_echelle_y{};
    ControleNombreDecimal *m_pos_y{};
    ControleEchelleDecimale *m_echelle_y{};

    /* connexion */
    CourbeBezier *m_courbe{};

  public:
    explicit ControleProprieteCourbeValeur(BasePropriete *p, int temps, QWidget *parent = nullptr);
    ~ControleProprieteCourbeValeur() override;

    EMPECHE_COPIE(ControleProprieteCourbeValeur);

    void finalise(const DonneesControle &donnees) override;

  private Q_SLOTS:
    void bascule_utilise_table(bool ouinon);

    void ajourne_position(float x, float y);
    void ajourne_position_x(float v);
    void ajourne_position_y(float v);
    void ajourne_point_actif();
};

} /* namespace danjo */
