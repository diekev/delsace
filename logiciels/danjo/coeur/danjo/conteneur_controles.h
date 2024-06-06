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

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/tableau.hh"

namespace danjo {

class ControlePropriete;

/**
 * La classe ConteneurControles sert de classe de base pour faire le pont entre
 * l'entreface et le programme.
 *
 * Cette classe contiendra la disposition provenant de la compilation d'un
 * script, et à chaque fois qu'un contrôle est modifié dans l'entreface,
 * le Q_SLOT ajourne_manipulable est appelé pour que le programme répondent au
 * changement dans l'entreface.
 */
class ConteneurControles : public QWidget {
    Q_OBJECT

    dls::dico_desordonne<dls::chaine, ControlePropriete *> m_controles{};

  public:
    explicit ConteneurControles(QWidget *parent = nullptr);

    void ajoute_controle(dls::chaine identifiant, ControlePropriete *controle);

    void ajourne_controles();

  protected:
    void efface_controles();

  public Q_SLOTS:
    /**
     * Cette méthode est appelée à chaque qu'un contrôle associé est modifiée
     * dans l'entreface.
     */
    virtual void ajourne_manipulable() = 0;

    virtual void debute_changement_controle()
    {
    }

    virtual void termine_changement_controle()
    {
    }

    virtual void onglet_dossier_change(int index);

    /**
     * Cette méthode est appelée à chaque fois qu'un controle de liste a besoin
     * de mettre à jour sa liste de chaînes disponible pour l'attache spécifiée.
     */
    virtual void obtiens_liste(const dls::chaine &attache, dls::tableau<dls::chaine> &chaines);
};

} /* namespace danjo */
