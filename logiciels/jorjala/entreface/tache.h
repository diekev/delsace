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

#include <memory>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QObject>
#pragma GCC diagnostic pop

class FenetrePrincipale;

/* Apparently we can not have a class derived from both a QObject and a
 * tbb::task so this class is to be used in conjunction with a tbb::task derived
 * class to notify the UI about certain events.
 */
class TaskNotifier : public QObject {
    Q_OBJECT

  public:
    explicit TaskNotifier(FenetrePrincipale *window);

    void signalImageProcessed();
    void signalise_proces(int quoi);

    void signale_debut_tache();
    void signale_ajournement_progres(float progress);
    void signale_fin_tache();
    void signale_debut_evaluation(const QString &message, int execution, int total);

  Q_SIGNALS:
    void image_traitee();

    void signal_proces(int quoi);

    void debut_tache();
    void ajourne_progres(float progress);
    void fin_tache();
    void debut_evaluation(const QString &message, int execution, int total);
};
