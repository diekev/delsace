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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QMainWindow>
#pragma GCC diagnostic pop

#include "biblinternes/outils/definitions.h"

#include "coeur/kanba.h"

class BaseDialogue;
class ProjectSettingsDialog;
class VueRegion;
class QProgressBar;

class FenetrePrincipale : public QMainWindow {
    Q_OBJECT

    QProgressBar *m_progress_bar{};

    KNB::Kanba m_kanba{};

    QVector<VueRegion *> m_régions{};

  public:
    explicit FenetrePrincipale(QWidget *parent = nullptr);

    EMPECHE_COPIE(FenetrePrincipale);

  public Q_SLOTS:
    void tache_commence();
    void tache_fini();

    void progres_avance(float progres);
    void progres_temps(int echantillon,
                       float temps_echantillon,
                       float temps_ecoule,
                       float temps_restant);

    void repond_action();

  private:
    void construit_interface_depuis_kanba();
};
