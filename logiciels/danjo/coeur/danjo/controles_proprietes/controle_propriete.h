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

#include <QWidget>

namespace danjo {

struct DonneesControle;
class BasePropriete;

/**
 * La classe Controle donne l'entreface nécessaire pour les contrôles à afficher
 * dans l'entreface utilisateur. Dès que le contrôle est changé le signal
 * Controle::controle_change() est émis.
 */
class ControlePropriete : public QWidget {
    Q_OBJECT

  protected:
    BasePropriete *m_propriete = nullptr;
    int m_temps = 0;
    bool m_animation = false;

  public:
    explicit ControlePropriete(BasePropriete *p, int temps, QWidget *parent = nullptr);

    ControlePropriete(ControlePropriete const &) = default;
    ControlePropriete &operator=(ControlePropriete const &) = default;

    /**
     * Finalise le contrôle. Cette fonction est appelée à la fin de la création
     * du contrôle par l'assembleur de contrôle.
     */
    virtual void finalise(const DonneesControle & /*donnees*/){};

  Q_SIGNALS:
    /**
     * Signal émis quand la valeur du contrôle est changée dans l'entreface.
     */
    void precontrole_change();

    /**
     * Signal émis quand la valeur du contrôle est changée dans l'entreface.
     */
    void controle_change();

  public Q_SLOTS:
    void emet_precontrole_change();
};

} /* namespace danjo */
