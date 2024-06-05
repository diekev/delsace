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

#include "biblinternes/structures/chaine.hh"

class QHBoxLayout;
class QLineEdit;
class QMenu;
class QPushButton;

namespace danjo {

class ConteneurControles;

class ControleProprieteListe final : public ControlePropriete {
    Q_OBJECT

    char pad[3];

    /* entreface */
    QHBoxLayout *m_agencement{};
    QLineEdit *m_editeur_texte{};
    QPushButton *m_bouton_liste{};
    QMenu *m_liste{};

    /* connexion */
    ConteneurControles *m_conteneur = nullptr;
    dls::chaine m_attache = "";

  public:
    explicit ControleProprieteListe(BasePropriete *p, int temps, QWidget *parent = nullptr);

    EMPECHE_COPIE(ControleProprieteListe);

    ~ControleProprieteListe() override = default;

    void attache(const dls::chaine &attache);

    void conteneur(ConteneurControles *conteneur);

    void finalise(const DonneesControle &donnees) override;

  private Q_SLOTS:
    void montre_liste();
    void repond_clique();
    void texte_modifie();
    void ajourne_liste();
    void ajourne_valeur_pointee(const QString &valeur);
};

} /* namespace danjo */
