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

#include "projectdialog.h"
#include "ui_projectdialog.h"

#include "project.h"

ProjectDialog::ProjectDialog(QDialog *parent)
    : QDialog(parent)
    , ui(new Ui::ProjectDialog)
    , m_open_existing(false)
    , m_create_new(false)
{
	ui->setupUi(this);

	setWindowFlags(Qt::WindowStaysOnTopHint);

	connect(ui->openExisting, SIGNAL(accepted()), this, SLOT(setOpenExisting()));

	addCreateWidget();
}

bool ProjectDialog::openExisting() const
{
	return m_open_existing;
}

bool ProjectDialog::createNew() const
{
	return m_create_new;
}

void ProjectDialog::setOpenExisting()
{
	if (ui->listWidget->currentIndex().row() == 0) {
		if (getSelectedProject() != "Create New") {
			m_create_new = true;
			Q_EMIT accept();
			return;
		}
	}

	m_open_existing = true;
	Q_EMIT accept();
}

void ProjectDialog::reset()
{
	m_create_new = false;
	m_open_existing = false;
}

void ProjectDialog::addCreateWidget() const
{
	ui->listWidget->addItem("Create New");
	const auto &item = ui->listWidget->item(0);
	item->setFlags(item->flags() | Qt::ItemIsEditable);

	/* Add an empty icon */
	QPixmap px(64, 64);
	px.fill(Qt::transparent);
	item->setIcon(QIcon(px));
}

void ProjectDialog::populateProjectList(const QVector<Project> &projects) const
{
	auto view = ui->listWidget;
	view->setIconSize(QSize(64, 64));
	view->clear();
	addCreateWidget();

	int index = 1;
	for (const auto &project : projects) {
		view->addItem(project.name);
		auto item = view->item(index++);
		item->setIcon(project.thumbnail);
	}
}

QString ProjectDialog::getSelectedProject() const
{
	const auto &item = ui->listWidget->currentItem();
	return item->text();
}
