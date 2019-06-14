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

#include <QDir>
#include <QMainWindow>

class GLCanvas;
class QScrollBar;
class UserPreferences;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT

	Ui::MainWindow *ui = nullptr;
	GLCanvas *m_canvas = nullptr;
	QTimer *m_timer = nullptr;

	int m_current_width = 0;
	int m_current_height = 0;

	QVector<QString> m_images = {};
	QVector<QString> m_recent_files = {};
	int m_image_id = 0;

	QImage *m_current_image = nullptr;
	QVector<QAction *> m_recent_act = {};

	std::mt19937 m_rng = std::mt19937(19337);
	std::uniform_int_distribution<int> m_dist = std::uniform_int_distribution<int>(0, 0);

	float m_scale_factor = 1.0f;
	UserPreferences *m_user_pref = nullptr;
	bool m_randomize = false;
	bool m_diaporama_started = false;
	bool m_trash_initialized = false;

	QString m_supported_file_types;

	/* Event handling */
	void closeEvent(QCloseEvent *) const;
	void keyPressEvent(QKeyEvent *e);

	void getDirectoryContent(const QDir &dir);

	void adjustScrollBar(QScrollBar *scrollBar, const float factor);

private slots:
	/* Image operations */
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

	/* Settings, configuration file */
	void setRandomize(const bool b);
	void setDiapTime(const int t);

	void openDirectory();

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

	/* Image operations */
	auto loadImage(const QString &filename) -> void;
	auto openImage(const QString &filename) -> void;
	void openDirectory(const QString &dir_path);
	auto nextImage(const bool forward) -> void;
	auto scaleImage(const float scale) -> void;

	/* Settings, configuration file */
	auto readSettings() -> void;
	auto writeSettings() const -> void;

	auto addRecentFile(const QString &name, const bool update_menu) -> void;
	auto updateRecentFilesMenu() -> void;
	auto setNormalSize() -> void;
	void reset();
	void resetRNG();
};
