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

#include "coeur/kanba.h"

class BaseDialogue;
class ProjectSettingsDialog;
class QProgressBar;

class FenetrePrincipale : public QMainWindow {
	Q_OBJECT

	QDockWidget *m_viewer_dock = nullptr;

	QProgressBar *m_progress_bar;

	Kanba m_kanba;

public:
	explicit FenetrePrincipale(QWidget *parent = nullptr);
	~FenetrePrincipale();

	void ajoute_visionneur_image();
	void ajoute_editeur_proprietes();

public Q_SLOTS:
	void rendu_fini();
	void tache_commence();
	void tache_fini();

	void progres_avance(float progres);
	void progres_temps(int echantillon, float temps_echantillon, float temps_ecoule, float temps_restant);

	void repond_action();
};
