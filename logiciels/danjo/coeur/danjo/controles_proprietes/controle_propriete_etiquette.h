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
class QLabel;
class QCheckBox;

namespace danjo {

class ControleProprieteEtiquette final : public ControlePropriete {
    char pad[3];
    QHBoxLayout *m_agencement{};
    QLabel *m_etiquette{};

  public:
    explicit ControleProprieteEtiquette(QString const &texte, QWidget *parent = nullptr);

    EMPECHE_COPIE(ControleProprieteEtiquette);
};

class ControleProprieteEtiquetteActivable final : public ControlePropriete {
    char pad[3];
    QHBoxLayout *m_agencement{};
    QCheckBox *m_checkbox{};

  public:
    ControleProprieteEtiquetteActivable(QString const &texte,
                                        BasePropriete *p,
                                        int temps,
                                        QWidget *parent = nullptr);

    EMPECHE_COPIE(ControleProprieteEtiquetteActivable);
};

class ControleProprieteEtiquettePropriete final : public ControlePropriete {
    char pad[3];
    QHBoxLayout *m_agencement{};
    QLabel *m_etiquette{};

  public:
    ControleProprieteEtiquettePropriete(BasePropriete *p, int temps, QWidget *parent = nullptr);

    EMPECHE_COPIE(ControleProprieteEtiquettePropriete);

    void ajourne_depuis_propriété() override;
};

} /* namespace danjo */
