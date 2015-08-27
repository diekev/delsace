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

#include <iostream>

#include <QFileDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QTimer>

#include "glwindow.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "user_preferences.h"

//#define WITH_GL

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
    , m_timer(new QTimer)
    , m_image_id(0)
    , m_current_image(new QImage())
    , m_rng(19937)
    , m_scale_factor(1.0f)
    , m_user_pref(new UserPreferences(this))
    , m_randomize(false)
    , m_diaporama_started(false)
{
	ui->setupUi(this);

#ifdef WITH_GL
	m_gl_win = new GLWindow(this, *this);

	ui->m_scroll_area->setWidget(m_gl_win);
#endif

	m_recent_files.reserve(MAX_RECENT_FILES);

	for (int i = 0; i < MAX_RECENT_FILES; ++i) {
		m_recent_act[i] = new QAction(this);
		m_recent_act[i]->setVisible(false);
		ui->m_recent_menu->addAction(m_recent_act[i]);
		connect(m_recent_act[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
	}

	connect(m_timer, SIGNAL(timeout()), this, SLOT(nextImage()));

	readSettings();
}

MainWindow::~MainWindow()
{
	delete ui;
	delete m_current_image;
	delete m_timer;
}

void MainWindow::reset()
{
	setWindowTitle(QCoreApplication::applicationName());
	ui->m_label->clear();
}

void MainWindow::closeEvent(QCloseEvent*)
{
	writeSettings();
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
	switch (e->key()) {
		case Qt::Key_F11:
		{
			setWindowState(this->windowState() ^ Qt::WindowFullScreen);
			auto hide = (ui->m_menu_bar->isHidden()) ? false : true;
			ui->m_menu_bar->setHidden(hide);
			break;
		}
		case Qt::Key_Space:
			if (!m_diaporama_started) {
				startDiap();
			}
			else {
				stopDiap();
			}
			break;
	}
}

#ifndef WITH_GL
void MainWindow::loadImage(const QString &filename)
{
	if (m_current_image->load(filename)) {
		setWindowTitle(QFileInfo(filename).fileName());
		ui->m_scroll_area->setWidgetResizable(true);
		ui->m_label->setPixmap(QPixmap::fromImage(*m_current_image));
		ui->m_label->adjustSize();
		m_scale_factor = 1.0f;
	}
	else {
		std::cerr << "Unable to load image: " << filename.toStdString() << "\n";
	}
	setNormalSize();
}
#else
void MainWindow::loadImage(const QString &filename)
{
	if (m_current_image->load(filename)) {
		setWindowTitle(QFileInfo(filename).fileName());
		ui->m_scroll_area->setWidgetResizable(true);
		m_gl_win->update();
	}
	else {
		std::cerr << "Unable to load image: " << filename.toStdString() << "\n";
	}
}
#endif

static QString supported_file_types =
        "*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.png *.pgm *.ppm *.tiff *.xbm *.xpm";

void MainWindow::openImage(const QString &filename)
{
	const auto &dir = QFileInfo(filename).absoluteDir();

	getDirectoryContent(dir);
	loadImage(filename);
	addRecentFile(filename, true);

	m_image_id = std::find(m_images.begin(), m_images.end(), filename) - m_images.begin();
}

void MainWindow::openImage()
{
	auto filters = "Image Files (" + supported_file_types + ")";

	auto filename = QFileDialog::getOpenFileName(
	                    this, tr("Ouvrir fichier image"),
	                    QDir::homePath(),
	                    tr(filters.toLatin1().data()));

	if (!filename.isEmpty()) {
		openImage(filename);
	}
}

void MainWindow::openDirectory()
{
	auto dir_path = QFileDialog::getExistingDirectory(
	                    this, tr("Choisir Dossier"),
	                    QDir::homePath(),
	                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (dir_path.isEmpty()) {
		return;
	}

	getDirectoryContent(dir_path);

	if (m_images.size() > 0) {
		loadImage(m_images[0]);
		addRecentFile(m_images[0], true);
		m_image_id = 0;
	}
	else {
		reset();
	}
}

void MainWindow::getDirectoryContent(const QDir &dir)
{
	const auto &name_filters = QStringList(supported_file_types.split(' '));
	m_images.clear();

	if (m_user_pref->openSubdirs()) {
		const auto &subdirs = dir.entryList(QDir::Dirs, QDir::Name);
		const auto &path = dir.path();

		for (const auto &subdir : subdirs) {
			if (subdir == "." || subdir == "..") {
				continue;
			}

			getDirectoryFiles(QDir(path + '/' + subdir), name_filters);
		}
	}

	getDirectoryFiles(dir, name_filters);
}

void MainWindow::getDirectoryFiles(const QDir &dir, const QStringList &filters)
{
	const auto &filenames = dir.entryList(filters, QDir::Files, QDir::Name);
	m_images.reserve(m_images.size() + filenames.size());

	for (const auto filename : filenames) {
		m_images.push_back(dir.absoluteFilePath(filename));
	}
}

/* TODO (kevin): this function isn't that nice */
void MainWindow::openRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());

	if (action) {
		auto filename = action->data().toString();

		/* If the image is already in the current session, just load it. */
		if (std::binary_search(m_images.begin(), m_images.end(), filename)) {
			loadImage(filename);
			addRecentFile(filename, true);
		}
		else {
			openImage(filename);
		}
	}
}

auto MainWindow::currentImage() const -> QImage*
{
	return m_current_image;
}

void MainWindow::deleteImage()
{
	if (m_images.size() == 0) {
		return;
	}

	auto name = m_images[m_image_id];
	auto removed = false;

	if (m_user_pref->deletePermanently()) {
		removed = QFile::remove(name);
	}
	else {
		auto old_name = QFileInfo(name).fileName();
		auto new_name = m_user_pref->deleteFolderPath().append("/").append(old_name);

		removed = QFile::rename(name, new_name);
	}

	if (removed) {
		auto iter = std::remove(m_images.begin(), m_images.end(), name);
		m_images.erase(iter, m_images.end());

		if (m_images.size() == 0) {
			reset();
		}
		else {
			loadImage(m_images[m_image_id]);
		}
	}
}

void MainWindow::nextImage(const bool forward)
{
	if (m_images.size() == 0) {
		return;
	}

	if (m_randomize) {
		std::uniform_int_distribution<int> dist(0, m_images.size() - 1);
		m_image_id = dist(m_rng);
	}
	else {
		if (forward) {
			m_image_id = (m_image_id == m_images.size() - 1) ? 0 : m_image_id + 1;
		}
		else {
			m_image_id = (m_image_id == 0) ? m_images.size() - 1 : m_image_id - 1;
		}
	}

	loadImage(m_images[m_image_id]);
}

void MainWindow::nextImage()
{
	nextImage(true);
}

void MainWindow::prevImage()
{
	nextImage(false);
}

void MainWindow::startDiap()
{
	m_timer->start(m_user_pref->diaporamaTime() * 1000);
	m_diaporama_started = true;
}

void MainWindow::stopDiap()
{
	m_timer->stop();
	m_diaporama_started = false;
}

void MainWindow::adjustScrollBar(QScrollBar *scrollBar, float factor)
{
	scrollBar->setValue(int(factor * scrollBar->value()
							+ ((factor - 1.0f) * scrollBar->pageStep() / 2)));
}

auto MainWindow::scaleImage(float factor) -> void
{
	m_scale_factor *= factor;
#ifndef WITH_GL
	//ui->m_label->resize(ui->m_label->size() * m_scale_factor);

	auto w = m_current_image->width() * m_scale_factor;
	auto h = m_current_image->height() * m_scale_factor;
	auto x = (ui->centralWidget->width() - w) / 2;
	auto y = (ui->centralWidget->height() - h) / 2;

	ui->m_label->setGeometry(x, y, w, h);

	adjustScrollBar(ui->m_scroll_area->horizontalScrollBar(), factor);
	adjustScrollBar(ui->m_scroll_area->verticalScrollBar(), factor);

	ui->m_scale_up->setEnabled(m_scale_factor < 3.0f);
	ui->m_scale_down->setEnabled(m_scale_factor > 0.333f);
#endif
}

void MainWindow::scaleUp()
{
	scaleImage(1.25f);
}

void MainWindow::scaleDown()
{
	scaleImage(0.8f);
}

void MainWindow::setNormalSize()
{
#ifndef WITH_GL
	auto width = m_current_image->width();
	auto height = m_current_image->height();
	auto x = (ui->centralWidget->width() - width) / 2;
	auto y = (ui->centralWidget->height() - height) / 2;

	ui->m_label->setGeometry(x, y, width, height);
	m_scale_factor = 1.0f;
#endif
}

void MainWindow::normalSize()
{
	setNormalSize();
}

void MainWindow::editPreferences()
{
	m_user_pref->show();

	if (m_user_pref->exec() == QDialog::Accepted) {
		setRandomize(m_user_pref->getRandomMode());
		setDiapTime(m_user_pref->diaporamaTime());
	}
}

void MainWindow::setRandomize(const bool b)
{
	m_randomize = b;
}

void MainWindow::setDiapTime(const int t)
{
	m_timer->setInterval(t * 1000);
}

void MainWindow::fitScreen()
{
#ifndef WITH_GL
	const auto label_height = m_current_image->height();
	const auto label_width = m_current_image->width();
	const auto scroll_area_height = ui->m_scroll_area->height();
	const auto scroll_area_width = ui->m_scroll_area->width();

	if (label_height > scroll_area_height) {
		auto ratio = scroll_area_height / float(label_height);
		auto width = label_width * ratio;
		auto height = scroll_area_height;
		auto x = (ui->centralWidget->width() - width) / 2;

		ui->m_label->setGeometry(x, 0, width, height);
	}
	else if (label_width > scroll_area_width) {
		auto ratio = scroll_area_width / float(label_width);
		auto width = scroll_area_width;
		auto height = label_height * ratio;
		auto y = (ui->centralWidget->height() - height) / 2;

		ui->m_label->setGeometry(0, y, width, height);
	}
#endif
}

void MainWindow::readSettings()
{
	QSettings settings;

	auto recent_files = settings.value("Recent Files").toStringList();

	for (const auto &file : recent_files) {
		if (QFile(file).exists()) {
			addRecentFile(file, false);
		}
	}

	updateRecentFilesMenu();	

	auto randomise = settings.value("Random Mode").toBool();
	setRandomize(randomise);
	m_user_pref->setRandomMode(randomise);

	auto time = settings.value("Diaporama Length").toInt();
	setDiapTime(time);
	m_user_pref->setDiaporamatime(time);

	auto delete_permanently = settings.value("Delete File Permanently").toBool();
	m_user_pref->deletePermanently(delete_permanently);

	auto delete_folder = settings.value("Delete File Folder").toString();
	m_user_pref->deleteFolderPath(delete_folder);

	auto open_subdirs = settings.value("Open Subdirs").toBool();
	m_user_pref->openSubdirs(open_subdirs);
}

void MainWindow::writeSettings()
{
	QSettings settings;
	QStringList recent;

	for (const auto &recent_file : m_recent_files) {
		recent.push_front(recent_file);
	}

	settings.setValue("Recent Files", recent);
	settings.setValue("Random Mode", m_user_pref->getRandomMode());
	settings.setValue("Diaporama Length", m_user_pref->diaporamaTime());
	settings.setValue("Delete File Permanently", m_user_pref->deletePermanently());
	settings.setValue("Delete File Folder", m_user_pref->deleteFolderPath());
	settings.setValue("Open Subdirs", m_user_pref->openSubdirs());
}

void MainWindow::addRecentFile(const QString &name, const bool update_menu)
{
	auto index = std::find(m_recent_files.begin(), m_recent_files.end(), name);

	if (index != m_recent_files.end()) {
		std::rotate(m_recent_files.begin(), index, index + 1);
	}
	else {
		m_recent_files.insert(m_recent_files.begin(), name);

		if (m_recent_files.size() > MAX_RECENT_FILES) {
			m_recent_files.resize(MAX_RECENT_FILES);
		}
	}

	if (update_menu) {
		updateRecentFilesMenu();
	}
}

void MainWindow::updateRecentFilesMenu()
{
	if (m_recent_files.size() > 0) {
		ui->m_no_recent_act->setVisible(false);

		for (auto i = 0u; i < m_recent_files.size();  ++i) {
			auto filename = m_recent_files[i];
			auto name = QFileInfo(filename).fileName();

			m_recent_act[i]->setText(name);
			m_recent_act[i]->setData(filename);
			m_recent_act[i]->setVisible(true);
		}
	}
}
