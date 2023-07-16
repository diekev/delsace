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

#include "danjo/manipulable.h"

#include "base_editeur.h"

class QGridLayout;
class QScrollArea;

class VueParametres : public danjo::Manipulable {
    KNB::Kanba *m_kanba;

  public:
    explicit VueParametres(KNB::Kanba *kanba);

    EMPECHE_COPIE(VueParametres);

    void ajourne_donnees();
    bool ajourne_proprietes() override;
};

class EditeurParametres final : public BaseEditrice {
    Q_OBJECT

    VueParametres *m_vue;

    QWidget *m_widget;
    QScrollArea *m_scroll;
    QGridLayout *m_glayout;

  public:
    EditeurParametres(KNB::Kanba *kanba, QWidget *parent = nullptr);

    EMPECHE_COPIE(EditeurParametres);

    ~EditeurParametres() override;

    void ajourne_etat(int evenement) override;

    void ajourne_manipulable() override;
};
