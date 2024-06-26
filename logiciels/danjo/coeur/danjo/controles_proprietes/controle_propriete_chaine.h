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

class QHBoxLayout;
class QLineEdit;
class QPushButton;
class QTextEdit;
class QVBoxLayout;

namespace danjo {

class ControleProprieteChaineCaractere final : public ControlePropriete {
    Q_OBJECT

    QHBoxLayout *m_agencement{};
    QLineEdit *m_editeur_ligne{};

  public:
    explicit ControleProprieteChaineCaractere(BasePropriete *p,
                                              int temps,
                                              QWidget *parent = nullptr);
    ~ControleProprieteChaineCaractere() override = default;

    EMPECHE_COPIE(ControleProprieteChaineCaractere);

    void ajourne_depuis_propriété() override;

  private Q_SLOTS:
    void ajourne_valeur_pointee();
};

class ControleProprieteEditeurTexte final : public ControlePropriete {
    Q_OBJECT

    QHBoxLayout *m_agencement{};
    QTextEdit *m_editeur_ligne{};
    QPushButton *m_bouton{};

  public:
    explicit ControleProprieteEditeurTexte(BasePropriete *p, int temps, QWidget *parent = nullptr);
    ~ControleProprieteEditeurTexte() override = default;

    EMPECHE_COPIE(ControleProprieteEditeurTexte);

    void ajourne_depuis_propriété() override;

  private Q_SLOTS:
    void ajourne_valeur_pointee();
};

} /* namespace danjo */
