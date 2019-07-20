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

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class ProjectDialog;
class ProjectManager;
class QTimer;

class MainWindow : public QMainWindow {
	Q_OBJECT

	Ui::MainWindow *ui = nullptr;
	ProjectManager *m_project_manager = nullptr;
	ProjectDialog *m_project_dialog = nullptr;
	QTimer *m_timer = nullptr;

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

	MainWindow(MainWindow const &) = default;
	MainWindow &operator=(MainWindow const &) = default;

public Q_SLOTS:
	void updateCanvasShader();
	void handleShaderError();
	void saveProject() const;
	void openProject();

	ProjectManager *projectManager() const;

	bool showProjectDialog();
	void showCentralWidget();

private:
	void keyPressEvent(QKeyEvent *e);
	void closeEvent(QCloseEvent *);
	void readSettings();
	void writeSettings();

private Q_SLOTS:
	void tagUpdate();
};
