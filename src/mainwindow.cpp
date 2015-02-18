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

/* For use in openImageFromDir(). */
static const char supported_file_formats[] = {
	" -name \\*.bmp -o -name \\*.gif -o -name \\*.jpg -o -name \\*.jpeg "
	" -o -name \\*.png -o -name \\*.pbm -o -name \\*.pgm -o -name \\*.ppm "
	" -o -name \\*.tiff -o -name \\*.xbm -o -name \\*.xpm "
};

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

	//m_gl_win = new GLWindow(this, *this);

	for (int i = 0; i < MAX_RECENT_FILES; ++i) {
		m_recent_act[i] = new QAction(this);
		m_recent_act[i]->setVisible(false);
		ui->m_recent_menu->addAction(m_recent_act[i]);
		connect(m_recent_act[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
	}

	connect(m_timer, SIGNAL(timeout()), this, SLOT(nextImage()));

	resize(1920, 1080);
	readSettings();
}

MainWindow::~MainWindow()
{
	delete ui;
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

			if (ui->m_menu_bar->isHidden()) {
				ui->m_menu_bar->show();
			}
			else {
				ui->m_menu_bar->hide();
			}

			break;
	}
}

void MainWindow::loadImage(const std::string &name)
{
	auto filename = QString::fromStdString(name);

	ui->m_label->clear();

	if (m_current_image->load(filename)) {
		m_orig_image_size = m_current_image->size();
		setWindowTitle(QFileInfo(filename).fileName());
		ui->m_scroll_area->setWidgetResizable(true);
		ui->m_label->setPixmap(QPixmap::fromImage(*m_current_image));

		m_scale_factor = 1.0f;

		//fitScreen();
		ui->m_label->adjustSize();
	}
	else {
		std::cerr << "Unable to load image: " << name << std::endl;
	}

	ui->m_label->show();
}

static auto execBuildImageList(const char *cmd) -> std::vector<std::string>
{
	FILE *pipe = popen(cmd, "r");

	char buffer[128];
	auto result = std::vector<std::string>{};

	while (!feof(pipe)) {
		if (fgets(buffer, 128, pipe) != nullptr) {
			auto str = std::string{buffer};
			str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
			result.push_back(str);
		}
	}

	pclose(pipe);

	return result;
}

void MainWindow::openImageFromDir(const std::string &name, QString dir)
{
	auto cmd = QString{"find \"" + dir + "\" -type f \\("+ supported_file_formats +"\\) | sort"};
	std::cout << cmd.toStdString() << std::endl;

	m_images = execBuildImageList(cmd.toLatin1().data());

	loadImage(name);
	addRecentFile(name);
	updateRecentFilesMenu();

	if (m_images.size() > 1) {
		m_image_id = std::find(m_images.begin(), m_images.end(), name) - m_images.begin();
	}

	for (const auto &name : m_images) {
		std::cout << name << std::endl;
	}
}

void MainWindow::openImage()
{
	auto filename = QFileDialog::getOpenFileName(this, tr("Ouvrir fichier image"),
											 QDir::home().absolutePath(),
											 tr("Image Files (*.png *.jpg *.bmp *.avi *.gif)"));

	if (filename.isEmpty()) {
		return;
	}

	auto dir = QFileInfo(filename).absoluteDir();

	openImageFromDir(filename.toStdString(), dir.path());
}

void MainWindow::openRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());

	if (action) {
		auto filename = action->data().toString().toStdString();

		/* If the image is already in the current session, just load it. */
		if (std::binary_search(m_images.begin(), m_images.end(), filename)) {
			loadImage(filename);
			return;
		}

		// It's not an image we already have loaded
//		add_image (filename);
//		if (m_images.size() > 1) {
//			// Otherwise, add_image already did this for us.
//			current_image (m_images.size() - 1);
//			fitWindowToImage (true, true);
//		}
	}
}

auto MainWindow::currentImage() const -> QImage*
{
	return m_current_image;
}

static auto execRemoveImage(const char *cmd) -> void
{
	FILE *pipe = popen(cmd, "r");
	pclose(pipe);
}

/*! Move an image to the OS trash. */
void MainWindow::deleteImage()
{
	auto name = m_images[m_image_id];
	auto cmd = QString{"gvfs-trash \"" + QString(name.c_str()) + "\""};

	execRemoveImage(cmd.toLatin1().data());

	m_images.erase(std::remove(m_images.begin(), m_images.end(), name), m_images.end());

	name = m_images[m_image_id];

	loadImage(name);
}

void MainWindow::nextImage()
{
	bool randomize = false;
	auto index = 0;

	if (randomize) {
		std::uniform_int_distribution<int> dist(0, m_images.size() - 1);
		index = dist(m_rng);
	}
	else {
		m_image_id = (m_image_id == m_images.size() - 1) ? 0 : m_image_id + 1;
		index = m_image_id;
	}

	auto name = m_images[index];
//	std::cout << "Next image: " << name << std::endl;

	loadImage(name);
}

void MainWindow::prevImage()
{
	m_image_id = (m_image_id == 0) ? m_images.size() - 1 : m_image_id - 1;

	auto index = m_image_id;

	auto name = m_images[index];
//	std::cout << "Prev image: " << name << std::endl;

	loadImage(name);
}

void MainWindow::startDiap()
{
	m_timer->start(2000);
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
	auto width = ui->m_label->pixmap()->width();
	auto height = ui->m_label->pixmap()->width();
	auto x = 0;
	auto y = 0;

	x = (ui->centralWidget->width() - width) / 2;
	y = (ui->centralWidget->height() - height) / 2;

	ui->m_label->setGeometry(x, y, width, height);
	m_scale_factor = 1.0f;
}

void MainWindow::normalSize()
{
	setNormalSize();
}

void MainWindow::fitScreen()
{

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
}

void MainWindow::readSettings()
{
	QSettings settings("Giraffe en feu", "imago");

	auto recent_files = settings.value("Rencent Files").toStringList();

	for (const auto &file : recent_files) {
		addRecentFile(file.toStdString());
		updateRecentFilesMenu();
	}
}

void MainWindow::writeSettings()
{
	QSettings settings("Giraffe en feu", "imago");

	QStringList recent;
	for (const auto &s : m_recent_files) {
		recent.push_front(QString(s.c_str()));
	}

	settings.setValue ("Recent Files", recent);
}

void MainWindow::addRecentFile(const std::string &name)
{
	std::cout << __func__ << " name: " << name << std::endl;
	removeRecentFile(name);
	m_recent_files.insert(m_recent_files.begin(), name);

	if (m_recent_files.size() > MAX_RECENT_FILES) {
		m_recent_files.resize(MAX_RECENT_FILES);
	}
}

void MainWindow::removeRecentFile(const std::string &name)
{
	for (size_t i = 0;  i < m_recent_files.size();  ++i) {
		if (m_recent_files[i] == name) {
			m_recent_files.erase(m_recent_files.begin()+i);
			--i;
		}
	}
}

void MainWindow::updateRecentFilesMenu()
{
	for (size_t i = 0;  i < MAX_RECENT_FILES;  ++i) {
		if (i < m_recent_files.size()) {
			auto filename = QString::fromStdString(m_recent_files[i]);
			auto name = QFileInfo(filename).fileName();

			m_recent_act[i]->setText(name);
			m_recent_act[i]->setData(m_recent_files[i].c_str());
			m_recent_act[i]->setVisible(true);
		}
		else {
			m_recent_act[i]->setVisible(false);
		}
	}
}
