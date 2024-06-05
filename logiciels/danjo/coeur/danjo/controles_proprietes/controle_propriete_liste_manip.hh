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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "controle_propriete.h"

#include "QTreeWidgetItem"

#include "biblinternes/structures/chaine.hh"

class QHBoxLayout;
class QPushButton;
class QVBoxLayout;

namespace danjo {

struct ListeManipulable;
class Manipulable;

/* ************************************************************************** */

class ItemArbreManip : public QTreeWidgetItem {
    const Manipulable *m_manipulable{};

  public:
    explicit ItemArbreManip(const Manipulable *manip, QTreeWidgetItem *parent = nullptr);

    ItemArbreManip(ItemArbreManip const &) = default;
    ItemArbreManip &operator=(ItemArbreManip const &) = default;

    const Manipulable *pointeur() const;
};

/* ************************************************************************** */

class TreeWidget : public QTreeWidget {
  public:
    explicit TreeWidget(QWidget *parent = nullptr);

    TreeWidget(TreeWidget const &) = default;
    TreeWidget &operator=(TreeWidget const &) = default;
};

class ControleProprieteListeManip : public ControlePropriete {
    Q_OBJECT

    char pad[3];

    /* entreface */
    QHBoxLayout *m_disp_horiz = nullptr;
    QVBoxLayout *m_disp_boutons = nullptr;
    QPushButton *m_bouton_ajoute = nullptr;
    QPushButton *m_bouton_enleve = nullptr;
    QPushButton *m_bouton_monte = nullptr;
    QPushButton *m_bouton_descend = nullptr;
    TreeWidget *m_widget_arbre = nullptr;

    /* données */
    dls::chaine m_attache = "";
    ListeManipulable *m_pointeur = nullptr;
    Manipulable *m_manipulable_courant = nullptr;
    long m_index_courant = 0;

  public:
    explicit ControleProprieteListeManip(BasePropriete *p, int temps, QWidget *parent = nullptr);

    EMPECHE_COPIE(ControleProprieteListeManip);

    ~ControleProprieteListeManip() override = default;

    /* entreface */

    void chemin_entreface(const dls::chaine &attache);

    void finalise(const DonneesControle &donnees) override;

  private:
    void init_arbre();

    /* modification liste */
  private Q_SLOTS:
    void ajoute_manipulable();
    void enleve_manipulable();
    void monte_manipulable();
    void descend_manipulable();

    void repond_selection();
};

} /* namespace danjo */
