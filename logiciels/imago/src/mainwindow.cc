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

#include <QDateTime>
#include <QFileDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QTimer>

#include <filesystem>
#include "biblinternes/systeme_fichier/utilitaires.h"

#include "glcanvas.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "user_preferences.h"

static constexpr auto MAX_RECENT_FILES = 10;

struct FileTypeInfo {
	const char *name;
	const char *ext;
};

static FileTypeInfo supported_file_types[] = {
    { "BitMap", ".bmp" },
    { "GIF", ".gif"},
    { "JPG", ".jpg"},
    { "JPG", ".jpeg"},
    { "PNG", ".png"},
    { "PBM", ".pbm"},
    { "PNG", ".png"},
    { "PGM", ".pgm"},
    { "PPM", ".ppm"},
    { "TIFF", ".tiff"},
    { "XBM", ".xbm"},
    { "XPM", ".xpm"},
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_canvas(new GLCanvas(this))
    , m_timer(new QTimer)
    , m_current_image(new QImage())
    , m_user_pref(new UserPreferences(this))
{
	ui->setupUi(this);
	ui->m_scroll_area->setWidget(m_canvas);
	ui->m_scroll_area->setAlignment(Qt::AlignCenter);
	ui->m_scroll_area->setWidgetResizable(false);

	m_recent_files.reserve(MAX_RECENT_FILES);
	m_recent_act.resize(MAX_RECENT_FILES);

	for (QAction *&recent_act : m_recent_act) {
		recent_act = new QAction(this);
		recent_act->setVisible(false);
		ui->m_recent_menu->addAction(recent_act);
		connect(recent_act, SIGNAL(triggered()), this, SLOT(openRecentFile()));
	}

	connect(m_timer, SIGNAL(timeout()), this, SLOT(nextImage()));

	readSettings();

	/* Setup filters. */
	m_supported_file_types = "Image Files (";

	for (const FileTypeInfo &file_type : supported_file_types) {
		m_supported_file_types += "*";
		m_supported_file_types += file_type.ext;
		m_supported_file_types += " ";
	}

	m_supported_file_types[m_supported_file_types.size() - 1] = ')';

	for (const FileTypeInfo &file_type : supported_file_types) {
		m_supported_file_types += ";;";
		m_supported_file_types += file_type.name;
		m_supported_file_types += " (*";
		m_supported_file_types += file_type.ext;
		m_supported_file_types += ")";
	}
}

MainWindow::~MainWindow()
{
	delete ui;
	delete m_current_image;
	delete m_timer;
}

/* ********************************* events ********************************* */

void MainWindow::reset()
{
	setWindowTitle(QCoreApplication::applicationName());
	m_canvas->hide();
}

void MainWindow::closeEvent(QCloseEvent *)
{
	writeSettings();
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
	switch (e->key()) {
		case Qt::Key_F11:
			setWindowState(this->windowState() ^ Qt::WindowFullScreen);
			ui->m_menu_bar->setHidden(!ui->m_menu_bar->isHidden());
			break;
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

/* ****************************** image loading ****************************** */

void MainWindow::loadImage(const QString &filename)
{
	if (!m_current_image->load(filename)) {
		std::cerr << "Unable to load image: " << filename.toStdString() << "\n";
		return;
	}

	setWindowTitle(QFileInfo(filename).fileName());

	m_canvas->hide();
	m_canvas->loadImage(*m_current_image);
	m_canvas->update();
	fitScreen();
	m_canvas->show();

	m_scale_factor = 1.0f;
}

void MainWindow::openImage(const QString &filename)
{
	const auto &dir = QFileInfo(filename).absoluteDir();

	getDirectoryContent(dir);
	loadImage(filename);
	addRecentFile(filename, true);
	resetRNG();

	m_image_id = std::find(m_images.begin(), m_images.end(), filename) - m_images.begin();
}

void MainWindow::openImage()
{
	auto filename = QFileDialog::getOpenFileName(
	                    this, tr("Ouvrir fichier image"),
	                    QDir::homePath(),
	                    tr(m_supported_file_types.toLatin1().data()));

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

	openDirectory(dir_path);
}

void MainWindow::openDirectory(const QString &dir_path)
{
	getDirectoryContent(dir_path);

	if (m_images.empty()) {
		reset();
		return;
	}

	loadImage(m_images[0]);
	addRecentFile(m_images[0], true);
	m_image_id = 0;
	resetRNG();
}

void MainWindow::resetRNG()
{
	std::uniform_int_distribution<int>::param_type p(0, m_images.size() - 1);
	m_dist.param(p);
}

static bool is_supported_file(const std::filesystem::path &path)
{
	if (!std::filesystem::is_regular_file(path)) {
		return false;
	}

	const auto &ext = path.extension();
	auto predicate = [&](const FileTypeInfo &info)
	{
		return info.ext == ext;
	};

	auto iter = std::find_if(std::begin(supported_file_types),
	                         std::end(supported_file_types),
	                         predicate);

	return iter != std::end(supported_file_types);
}

template <typename DirIterator>
void get_directory_content(DirIterator iter, QVector<QString> &images)
{
	for (const auto &entry : iter) {
		if (!is_supported_file(entry)) {
			continue;
		}

		images.push_back(entry.path().c_str());
	}
}

void MainWindow::getDirectoryContent(const QDir &dir)
{
	m_images.clear();

	if (m_user_pref->openSubdirs()) {
		std::filesystem::recursive_directory_iterator dir_iter(dir.path().toStdString());
		get_directory_content(dir_iter, m_images);
	}
	else {
		std::filesystem::directory_iterator dir_iter(dir.path().toStdString());
		get_directory_content(dir_iter, m_images);
	}
}

void MainWindow::deleteImage()
{
	if (m_images.size() == 0) {
		return;
	}

	auto name = m_images[static_cast<int>(m_image_id)];
	auto removed = false;
	auto remove_method = m_user_pref->fileRemovingMethod();

	if (remove_method == DELETE_PERMANENTLY) {
		removed = std::filesystem::remove(name.toStdString());
	}
	else if (remove_method == MOVE_TO_TRASH) {
		dls::systeme_fichier::mettre_poubelle(name.toStdString());
		removed = !std::filesystem::exists(name.toStdString());
	}
	else {
		auto old_name = QFileInfo(name).fileName();
		auto new_name = m_user_pref->changeFolderPath().append("/").append(old_name);

		std::filesystem::rename(name.toStdString(), new_name.toStdString());
		removed = !std::filesystem::exists(name.toStdString());
	}

	if (removed) {
		auto iter = std::remove(m_images.begin(), m_images.end(), name);
		m_images.erase(iter, m_images.end());

		if (m_images.size() == 0) {
			reset();
		}
		else {
			loadImage(m_images[static_cast<int>(m_image_id)]);
		}
	}
}

/* ****************************** recent files ******************************* */

void MainWindow::openRecentFile()
{
	auto action = qobject_cast<QAction *>(sender());

	if (action == nullptr) {
		return;
	}

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
	if (m_recent_files.empty()) {
		return;
	}

	ui->m_no_recent_act->setVisible(false);

	for (int i(0); i < m_recent_files.size();  ++i) {
		auto filename = m_recent_files[i];
		auto name = QFileInfo(filename).fileName();

		m_recent_act[i]->setText(name);
		m_recent_act[i]->setData(filename);
		m_recent_act[i]->setVisible(true);
	}
}

/* ******************************* navigation ******************************** */

void MainWindow::nextImage()
{
	nextImage(true);
}

void MainWindow::prevImage()
{
	nextImage(false);
}

void MainWindow::nextImage(const bool forward)
{
	if (m_images.size() == 0) {
		return;
	}

	if (m_randomize) {
		m_image_id = m_dist(m_rng);
	}
	else {
		if (forward) {
			m_image_id = (m_image_id == m_images.size() - 1) ? 0 : m_image_id + 1;
		}
		else {
			m_image_id = (m_image_id == 0) ? m_images.size() - 1 : m_image_id - 1;
		}
	}

	loadImage(m_images[static_cast<int>(m_image_id)]);
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

/* ****************************** image scaling ****************************** */

void MainWindow::scaleUp()
{
	scaleImage(1.25f);
}

void MainWindow::scaleDown()
{
	scaleImage(0.8f);
}

auto MainWindow::scaleImage(const float factor) -> void
{
	m_scale_factor *= factor;

	auto w = static_cast<float>(m_current_width) * m_scale_factor;
	auto h = static_cast<float>(m_current_height) * m_scale_factor;

	m_canvas->resize(static_cast<int>(w), static_cast<int>(h));

	adjustScrollBar(ui->m_scroll_area->horizontalScrollBar(), factor);
	adjustScrollBar(ui->m_scroll_area->verticalScrollBar(), factor);

	ui->m_scale_up->setEnabled(m_scale_factor < 3.0f);
	ui->m_scale_down->setEnabled(m_scale_factor > 0.333f);
}

void MainWindow::adjustScrollBar(QScrollBar *scrollBar, const float factor)
{
	scrollBar->setValue(int(factor * static_cast<float>(scrollBar->value())
							+ ((factor - 1.0f) * static_cast<float>(scrollBar->pageStep()) / 2.0f)));
}

void MainWindow::normalSize()
{
	setNormalSize();
}

void MainWindow::setNormalSize()
{
	m_canvas->resize(m_current_image->width(), m_current_image->height());
	m_scale_factor = 1.0f;
}

void MainWindow::fitScreen()
{
	const auto image_height = m_current_image->height();
	const auto image_width = m_current_image->width();
	const auto screen_height = ui->centralWidget->height() - 2;
	const auto screen_width = ui->centralWidget->width() - 2;

	if (image_height > screen_height) {
		auto ratio = static_cast<float>(screen_height) / static_cast<float>(image_height);
		m_current_width = static_cast<int>(static_cast<float>(image_width) * ratio);
		m_current_height = screen_height;
	}
	else if (image_width > screen_width) {
		auto ratio = static_cast<float>(screen_width) / static_cast<float>(image_width);
		m_current_width = screen_width;
		m_current_height = static_cast<int>(static_cast<float>(image_height) * ratio);
	}
	else {
		m_current_height = image_height;
		m_current_width = image_width;
	}

	m_canvas->resize(m_current_width, m_current_height);
}

/* ******************************* preferences ******************************* */

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

void MainWindow::readSettings()
{
	QSettings settings;

	const auto &recent_files = settings.value("Recent Files").toStringList();

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

	auto remove_method = settings.value("File Removing Mode").toBool();
	m_user_pref->fileRemovingMethod(remove_method);

	auto change_folder = settings.value("Change File Folder").toString();
	m_user_pref->changeFolderPath(change_folder);

	auto open_subdirs = settings.value("Open Subdirs").toBool();
	m_user_pref->openSubdirs(open_subdirs);
}

void MainWindow::writeSettings() const
{
	QSettings settings;
	QStringList recent;

	for (const auto &recent_file : m_recent_files) {
		recent.push_front(recent_file);
	}

	settings.setValue("Recent Files", recent);
	settings.setValue("Random Mode", m_user_pref->getRandomMode());
	settings.setValue("Diaporama Length", m_user_pref->diaporamaTime());
	settings.setValue("File Removing Mode", m_user_pref->fileRemovingMethod());
	settings.setValue("Change File Folder", m_user_pref->changeFolderPath());
	settings.setValue("Open Subdirs", m_user_pref->openSubdirs());
}
