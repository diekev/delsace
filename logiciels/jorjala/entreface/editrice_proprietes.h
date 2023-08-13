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

#include "base_editrice.h"

class QScrollArea;

namespace JJL {
class Noeud;
}

/* ------------------------------------------------------------------------- */
/** \name EditriceProprietes
 *  Cette éditrice correspond à l'éditrice de propriétés des noeuds de Jorjala.
 * \{ */

class EditriceProprietes : public BaseEditrice {
    QWidget *m_widget;
    QWidget *m_conteneur_avertissements;
    QWidget *m_conteneur_disposition;
    QScrollArea *m_scroll;
    QVBoxLayout *m_disposition_widget;

  public:
    explicit EditriceProprietes(JJL::Jorjala &jorjala,
                                JJL::Éditrice éditrice,
                                QWidget *parent = nullptr);

    EditriceProprietes(EditriceProprietes const &) = default;
    EditriceProprietes &operator=(EditriceProprietes const &) = default;

    void ajourne_état(JJL::ChangementÉditrice changement) override;

    void ajourne_manipulable() override;

    void obtiens_liste(dls::chaine const &attache, dls::tableau<dls::chaine> &chaines) override;

    void onglet_dossier_change(int index) override;

    void reinitialise_entreface(bool creation_avert);

    void debute_changement_controle() override;

    void termine_changement_controle() override;

  private:
    void ajoute_avertissements(JJL::Noeud &noeud);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name EditriceProprietesNoeudDialogue
 *  Cette éditrice est pour afficher les propriétés des noeuds dans un dialogue.
 * \{ */

class EditriceProprietesNoeudDialogue : public danjo::ConteneurControles {
    QWidget *m_widget;
    QWidget *m_conteneur_avertissements;
    QWidget *m_conteneur_disposition;
    QScrollArea *m_scroll;
    QVBoxLayout *m_disposition;
    QVBoxLayout *m_disposition_widget;

  public:
    explicit EditriceProprietesNoeudDialogue(JJL::Noeud &noeud, QWidget *parent = nullptr);

    EditriceProprietesNoeudDialogue(EditriceProprietesNoeudDialogue const &) = delete;
    EditriceProprietesNoeudDialogue &operator=(EditriceProprietesNoeudDialogue const &) = delete;

    void ajourne_manipulable() override;
};

/** \} */
