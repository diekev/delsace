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

#include "user_preferences.h"

#include <QFileDialog>

#include "ui_pref_window.h"

UserPreferences::UserPreferences(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::UserPreferences)
{
	ui->setupUi(this);
	updateUI();
}

UserPreferences::~UserPreferences()
{
	delete ui;
}

auto UserPreferences::getRandomMode() const -> bool
{
	return ui->m_random_mode->isChecked();
}

auto UserPreferences::setRandomMode(const bool b) -> void
{
	ui->m_random_mode->setChecked(b);
}

auto UserPreferences::diaporamaTime() const -> int
{
	return ui->m_diap_dur->value();
}

auto UserPreferences::setDiaporamatime(const int time) -> void
{
	ui->m_diap_dur->setValue(time);
	ui->m_diap_dur_label->setText(QString::number(time));
}

auto UserPreferences::fileRemovingMethod() const -> int
{
	return ui->m_delete_file_bbox->currentIndex();
}

auto UserPreferences::fileRemovingMethod(const int index) -> void
{
	ui->m_delete_file_bbox->setCurrentIndex(index);
}

auto UserPreferences::changeFolderPath() const -> QString
{
	return ui->m_folder_path->text();
}

auto UserPreferences::changeFolderPath(const QString &path) -> void
{
	ui->m_folder_path->setText(path);
}

auto UserPreferences::openSubdirs() const -> bool
{
	return ui->m_open_subdirs->isChecked();
}

auto UserPreferences::openSubdirs(const bool b) -> void
{
	return ui->m_open_subdirs->setChecked(b);
}

auto UserPreferences::chooseFolder() -> void
{
	const auto &dir = QFileDialog::getExistingDirectory(
	                      this, tr("Choisir Dossier"), QDir::homePath(),
	                      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	ui->m_folder_path->setText(dir);
}

auto UserPreferences::updateUI() -> void
{
	auto to_enable = (ui->m_delete_file_bbox->currentIndex() == MOVE_TO_FOLDER);
	ui->m_choose_folder_but->setEnabled(to_enable);
	ui->m_delete_file_label->setEnabled(to_enable);
	ui->m_folder_path->setEnabled(to_enable);
}
