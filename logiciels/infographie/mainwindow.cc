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
#include "ui_mainwindow.h"

#include <QComboBox>

#include "colorpickerwidget.h"
#include "scene.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_scene(new Scene)
{
	ui->setupUi(this);
	ui->widget->setScene(m_scene);

	QAction	*act = ui->mainToolBar->addAction("Add Chaikin Curve");
	connect(act, SIGNAL(triggered()), m_scene, SLOT(addChaikinCurve()));

	QAction	*act2 = ui->mainToolBar->addAction("Close Shape");
	connect(act2, SIGNAL(triggered()), m_scene, SLOT(closeShape()));

	QComboBox *box = new QComboBox(ui->mainToolBar);
	box->addItem("Chaikin Curves");
	box->addItem("Sierpinski");
	box->addItem("Polygon");

	ui->mainToolBar->addWidget(box);

	connect(box, SIGNAL(currentIndexChanged(int)), m_scene, SLOT(changeMode(int)));
	connect(ui->curveResolution, SIGNAL(valueChanged(int)), m_scene, SLOT(setCurveResolution(int)));
	connect(m_scene, SIGNAL(redraw()), ui->widget, SLOT(update()));

	ColorPickerWidget *widget = new ColorPickerWidget(this);
	ui->widget_2->layout()->addWidget(widget);
}

MainWindow::~MainWindow()
{
	delete ui;
	delete m_scene;
}
