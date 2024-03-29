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

#include <QMainWindow>

#include "conteneur_controles.h"
#include "danjo.h"
#include "manipulable.h"
#include "repondant_bouton.h"

class WidgetTest : public danjo::ConteneurControles {
  public:
    explicit WidgetTest(QWidget *parent = nullptr);

    void obtiens_liste(const dls::chaine &attache, dls::tableau<dls::chaine> &chaines) override;

    void ajourne_manipulable() override;
};

class RepondantBoutonTest : public danjo::RepondantBouton {
  public:
    void repond_clique(const dls::chaine &valeur, const dls::chaine &metadonnee) override;
    bool evalue_predicat(const dls::chaine &valeur, const dls::chaine &metadonnee) override;
};

class FenetreTest : public QMainWindow {
    danjo::Manipulable m_manipulable{};
    RepondantBoutonTest m_repondant{};

  public:
    FenetreTest();
};
