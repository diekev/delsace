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
class QPushButton;

namespace danjo {

class SelecteurFichier : public ControlePropriete {
    Q_OBJECT

    char pad[3];

    QHBoxLayout *m_agencement{};
    QLineEdit *m_line_edit{};
    QPushButton *m_push_button{};

    QString m_filtres{};
    bool m_input{};
    char pad1[7];

  public:
    explicit SelecteurFichier(BasePropriete *p, int temps, bool input, QWidget *parent = nullptr);

    EMPECHE_COPIE(SelecteurFichier);

    ~SelecteurFichier() = default;

    void setValue(const QString &text);

    void ajourne_filtres(const QString &chaine);

  private Q_SLOTS:
    void setChoosenFile();
    void lineEditChanged();

  Q_SIGNALS:
    void valeur_changee(const QString &text);
};

class ControleProprieteFichier final : public SelecteurFichier {
    Q_OBJECT

  public:
    explicit ControleProprieteFichier(BasePropriete *p,
                                      int temps,
                                      bool input,
                                      QWidget *parent = nullptr);
    ~ControleProprieteFichier() override = default;

    EMPECHE_COPIE(ControleProprieteFichier);

    void finalise(const DonneesControle &donnees) override;

  private Q_SLOTS:
    void ajourne_valeur_pointee(const QString &valeur);
};

} /* namespace danjo */
