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

#include "mainwindow.h"
#include "project.h"
#include "projectdialog.h"

#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QSettings>
#include <QTextStream>
#include <QTimer>

class scope_timer {
	QTimer *m_timer;

public:
	scope_timer(QTimer *timer)
	    : m_timer(timer)
	{
		m_timer->stop();
	}

	~scope_timer()
	{
		m_timer->start();
	}
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_project_manager(new ProjectManager)
    , m_project_dialog(new ProjectDialog)
    , m_timer(new QTimer(this))
{
	ui->setupUi(this);

	/* set up default sizes */
	this->resize(1280, 720);
	ui->openGLWidget->resize(0.75f * 1280, 0.75f * 720);

	/* catch shader errors */
	connect(ui->openGLWidget, SIGNAL(shaderErrorOccurred()),
	        this, SLOT(handleShaderError()));

	/* update shader action */
	QAction	*shader_act = ui->mainToolBar->addAction("Update Fragment Shader");
	shader_act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Return));

	connect(shader_act, SIGNAL(triggered()), this, SLOT(updateCanvasShader()));

	/* set up text editor and console font and metrics */

	QFont font;
	font.setFamily("Courier");
	font.setStyleHint(QFont::Monospace);
	font.setFixedPitch(true);
	font.setPointSize(10);

	QFontMetrics metrics(font);
	const auto tab_stop = 4;

	ui->textEditor->setFont(font);
	ui->textEditor->setTabStopWidth(tab_stop * metrics.width(' '));

	ui->console->setFont(font);

	/* set up text editor and console palette */

	QPalette p = ui->textEditor->palette();
	p.setColor(QPalette::Base, QColor(128, 128, 128));
	ui->textEditor->setPalette(p);

	p.setColor(QPalette::Text, QColor(223, 32, 32));
	ui->console->setPalette(p);

	readSettings();

	ui->centralWidget->hide();

	/* set up timer */
	m_timer->setInterval(1000);
	connect(m_timer, SIGNAL(timeout()), this, SLOT(saveProject()));

	connect(ui->textEditor, SIGNAL(textChanged()), this, SLOT(tagUpdate()));
}

MainWindow::~MainWindow()
{
	delete ui;
	delete m_project_manager;
	delete m_project_dialog;
}

/* ********************************* Events ********************************* */

void MainWindow::keyPressEvent(QKeyEvent *e)
{
	switch (e->key()) {
		case Qt::Key_F11:
			setWindowState(this->windowState() ^ Qt::WindowFullScreen);
			ui->menuBar->setHidden(!ui->menuBar->isHidden());
			break;
	}
}

void MainWindow::closeEvent(QCloseEvent *)
{
	m_project_dialog->close();

	auto thumb = ui->openGLWidget->makeThumbnail();
	m_project_manager->tagUpdate();
	m_project_manager->saveProject(ui->textEditor->toPlainText(), &thumb);

	writeSettings();
}

void MainWindow::updateCanvasShader()
{
	const auto &text = ui->textEditor->toPlainText();
	ui->openGLWidget->loadProgram(text);
}

void MainWindow::handleShaderError()
{
	ui->console->setPlainText(ui->openGLWidget->errorLog());
}

/* ******************************** File I/O ******************************** */

void MainWindow::saveProject() const
{
	m_project_manager->saveProject(ui->textEditor->toPlainText(), nullptr);
}

void MainWindow::openProject()
{
	showProjectDialog();
	updateCanvasShader();
}

ProjectManager *MainWindow::projectManager() const
{
	return m_project_manager;
}

bool MainWindow::showProjectDialog()
{
	scope_timer st(m_timer);
	Q_ASSERT(!m_timer->isActive());

	m_project_dialog->reset();
	m_project_dialog->populateProjectList(m_project_manager->projects());
	m_project_dialog->show();

	if (m_project_dialog->exec() == QDialog::Rejected) {
		return false;
	}

	bool ok = false;

	QString text = "";

	if (m_project_dialog->openExisting()) {
		m_project_manager->currentProject(m_project_dialog->getSelectedProject());
		text = m_project_manager->openProject();
		ok = true;
	}

	if (m_project_dialog->createNew()) {
		m_project_manager->createProject(m_project_dialog->getSelectedProject());
		text = basic_fragment;
		ok = true;
	}

	if (ok) {
		ui->textEditor->setPlainText(text);
		ui->openGLWidget->loadProgram(text);
	}

	return ok;
}

void MainWindow::readSettings()
{
	QSettings settings;
	m_project_manager->projectFolder(settings.value("project_folder").toString());
}

void MainWindow::writeSettings()
{
	QSettings settings;
	settings.setValue("project_folder", m_project_manager->projectFolder());
}

void MainWindow::tagUpdate()
{
	m_project_manager->tagUpdate();
}

void MainWindow::showCentralWidget()
{
	ui->centralWidget->show();
	updateCanvasShader();
}
