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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "user_preferences.h"

//#define WITH_GL

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	m_timer = new QTimer;
	m_current_image = new QImage();
	m_image_id = 0;
	m_rng.seed(19937);
	m_scale_factor = 1.0f;
	m_recent_files.reserve(MAX_RECENT_FILES);
	m_user_pref = new UserPreferences(this);
	m_randomize = false;

#ifdef WITH_GL
	m_gl_win = new GLWindow(this, *this);

	ui->m_scroll_area->setWidget(m_gl_win);
#endif

	for (int i = 0; i < MAX_RECENT_FILES; ++i) {
		m_recent_act[i] = new QAction(this);
		m_recent_act[i]->setVisible(false);
		ui->m_recent_menu->addAction(m_recent_act[i]);
		connect(m_recent_act[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
	}

	connect(m_timer, SIGNAL(timeout()), this, SLOT(nextImage()));

	resize(1920, 1080);
	setWindowTitle("imago");
	readSettings();
}

MainWindow::~MainWindow()
{
	delete ui;
	delete m_current_image;
	delete m_timer;
}

void MainWindow::closeEvent(QCloseEvent*)
{
	writeSettings();
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
	switch (e->key()) {
		case Qt::Key_F11:
			setWindowState(this->windowState() ^ Qt::WindowFullScreen);

			auto hide = (ui->m_menu_bar->isHidden()) ? false : true;
			ui->m_menu_bar->setHidden(hide);

			break;
	}
}

void MainWindow::loadImage(const QString &filename)
{
#ifndef WITH_GL
	ui->m_label->clear();
#endif
	if (m_current_image->load(filename)) {
		setWindowTitle(QFileInfo(filename).fileName());
		ui->m_scroll_area->setWidgetResizable(true);
#ifndef WITH_GL
		ui->m_label->setPixmap(QPixmap::fromImage(*m_current_image));

		m_scale_factor = 1.0f;

		//fitScreen();
		ui->m_label->adjustSize();
#else
		m_gl_win->update();
#endif
	}
	else {
		std::cerr << "Unable to load image: " << filename.toStdString() << "\n";
	}
#ifndef WITH_GL
	ui->m_label->show();
#endif
}

static QString supported_file_types =
        "*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.png *.pgm *.ppm *.tiff *.xbm *.xpm";

void MainWindow::openImage(const QString &filename)
{
	const auto &dir = QFileInfo(filename).absoluteDir();
	const auto &name_filters = QStringList(supported_file_types.split(' '));

	// TODO: handle subdirectories.
	const auto &filenames = dir.entryList(name_filters, QDir::Files, QDir::Name);

	m_images.clear();
	m_images.reserve(filenames.size());
	for (const auto fname : filenames) {
		m_images.push_back(dir.absoluteFilePath(fname));
	}

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

	if (filename.isEmpty()) {
		return;
	}

	openImage(filename);
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
			ui->m_label->clear();
			setWindowTitle("imago");
		}
		else {
			loadImage(m_images[m_image_id]);
		}
	}
}

void MainWindow::getNextImage(const bool forward)
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
	getNextImage(true);
}

void MainWindow::prevImage()
{
	getNextImage(false);
}

void MainWindow::startDiap()
{
	m_timer->start(m_user_pref->diaporamaTime() * 1000);
}

void MainWindow::stopDiap()
{
	m_timer->stop();
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

	auto w = ui->m_label->pixmap()->width() * m_scale_factor;
	auto h = ui->m_label->pixmap()->height() * m_scale_factor;
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
	auto width = ui->m_label->pixmap()->width();
	auto height = ui->m_label->pixmap()->width();
	auto x = 0;
	auto y = 0;

	x = (ui->centralWidget->width() - width) / 2;
	y = (ui->centralWidget->height() - height) / 2;

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
	if (ui->m_label->height() > ui->m_scroll_area->height()) {
		auto ratio = ui->m_scroll_area->height() / float(ui->m_label->height());
		auto width = ui->m_label->width() * ratio;
		auto height = ui->m_scroll_area->height();
		auto x = (ui->centralWidget->width() - width) / 2;

		ui->m_label->setGeometry(x, 0, width, height);
	}
	else if (ui->m_label->width() > ui->m_scroll_area->width()) {
		auto ratio = ui->m_scroll_area->width() / float(ui->m_label->width());
		auto width = ui->m_scroll_area->width();
		auto height = ui->m_label->height() * ratio;
		auto y = (ui->centralWidget->height() - height) / 2;

		ui->m_label->setGeometry(0, y, width, height);
	}
	else {
		setNormalSize();
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
