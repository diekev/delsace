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

#include "biblinternes/outils/definitions.h"

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

    EMPECHE_COPIE(ControlePropriete);

    /**
     * Finalise le contrôle. Cette fonction est appelée à la fin de la création
     * du contrôle par l'assembleur de contrôle.
     */
    virtual void finalise(const DonneesControle & /*donnees*/) {};

    template <typename TypeFonction>
    void émets_controle_changé_simple(TypeFonction &&fonction)
    {
        Q_EMIT(debute_changement_controle());
        fonction();
        Q_EMIT(controle_change());
        Q_EMIT(termine_changement_controle());
    }

    template <typename TypeControle, typename TypeControlePropriete>
    static void connecte_signaux_début_fin_changement(TypeControle *controle,
                                                      TypeControlePropriete *controle_propriete)
    {
        connect(controle,
                &TypeControle::debute_changement_controle,
                controle_propriete,
                &TypeControlePropriete::emets_debute_changement_controle);

        connect(controle,
                &TypeControle::termine_changement_controle,
                controle_propriete,
                &TypeControlePropriete::emets_termine_changement_controle);
    }

    virtual void ajourne_depuis_propriété()
    {
    }

    BasePropriete *donne_propriete() const
    {
        return m_propriete;
    }

  Q_SIGNALS:
    /**
     * Signal émis avant que la valeur du contrôle ne soit changée dans l'entreface.
     */
    void debute_changement_controle();

    /**
     * Signal émis quand la valeur du contrôle est changée dans l'entreface.
     */
    void controle_change();

    /**
     * Signal émis après que la valeur du contrôle fut changée dans l'entreface.
     */
    void termine_changement_controle();

  public Q_SLOTS:
    void emets_debute_changement_controle();

    void emets_termine_changement_controle();
};

} /* namespace danjo */
