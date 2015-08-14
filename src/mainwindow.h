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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <QMainWindow>

#include "glwindow.h"

class QScrollBar;
class UserPreferences;

namespace Ui {
class MainWindow;
}

#define MAX_RECENT_FILES 10

class MainWindow : public QMainWindow {
	Q_OBJECT

	Ui::MainWindow *ui;
	class GLWindow *m_gl_win;
	QTimer *m_timer;

	std::vector<std::string> m_images;
	std::vector<std::string> m_recent_files;
	unsigned m_image_id;

	QImage *m_current_image;
	QAction *m_recent_act[MAX_RECENT_FILES];

	std::mt19937 m_rng;

	float m_scale_factor;
	UserPreferences *m_user_pref;
	bool m_randomize;

public slots:
	void deleteImage();
	void nextImage();
	void openImage();
	void openRecentFile();
	void prevImage();
	void startDiap();
	void stopDiap();
	void scaleUp();
	void scaleDown();
	void fitScreen();
	void normalSize();
	void editPreferences();
	void setRandomize(const bool b);
	void setDiapTime(const int t);

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
	auto currentImage() const -> QImage*;
	auto scaleImage(float scale) -> void;

	void keyPressEvent(QKeyEvent *e);
	void loadImage(const std::string &name);
	void openImageFromDir(const std::string &name, QString dir);
	void adjustScrollBar(QScrollBar *scrollBar, float factor);
	void updateActions();
	void setNormalSize();

	void readSettings();
	void writeSettings();
	void addRecentFile(const std::string &name);
	void updateRecentFilesMenu();
	void closeEvent(QCloseEvent *);
	void getNextImage(const bool forward);
};
