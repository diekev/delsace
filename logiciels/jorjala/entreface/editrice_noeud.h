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

class QComboBox;
class QGraphicsScene;
class QMenu;
class VueEditeurNoeud;

class EditriceGraphe : public BaseEditrice {
    Q_OBJECT

    QGraphicsScene *m_scene;
    VueEditeurNoeud *m_vue;

    QLineEdit *m_barre_chemin;
    QComboBox *m_selecteur_graphe;

    std::map<std::string, QMenu *> m_menus{};

  public:
    explicit EditriceGraphe(JJL::Jorjala &jorjala, QWidget *parent = nullptr);

    EditriceGraphe(EditriceGraphe const &) = default;
    EditriceGraphe &operator=(EditriceGraphe const &) = default;

    ~EditriceGraphe() override;

    void ajourne_etat(int evenement) override;

    void ajourne_manipulable() override
    {
    }

    void keyPressEvent(QKeyEvent *event) override;

  private Q_SLOTS:
    void sors_noeud();
    void change_contexte(int index);

  private:
    QMenu *menu_pour_graphe();

    QPointF transforme_position_evenement(QPoint pos) override;

    void ajourne_sélecteur_graphe();
};
