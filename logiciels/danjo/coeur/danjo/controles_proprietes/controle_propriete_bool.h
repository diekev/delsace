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

class QCheckBox;
class QHBoxLayout;

namespace danjo {

class ControleProprieteBool final : public ControlePropriete {
    Q_OBJECT

    char pad[3];
    QHBoxLayout *m_agencement{};
    QCheckBox *m_case_a_cocher{};

  public:
    explicit ControleProprieteBool(BasePropriete *p, int temps, QWidget *parent = nullptr);
    ~ControleProprieteBool() override = default;

    EMPECHE_COPIE(ControleProprieteBool);

  private Q_SLOTS:
    void ajourne_valeur_pointee(bool valeur);
};

} /* namespace danjo */
