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

#include <QFileDialog>

#include "ui_pref_window.h"
#include "user_preferences.h"

UserPreferences::UserPreferences(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::UserPreferences)
{
	ui->setupUi(this);
}

auto UserPreferences::getRandomMode() const -> bool
{
	return ui->m_random_mode->isChecked();
}

auto UserPreferences::setRandomMode(const bool b) -> void
{
	ui->m_random_mode->setChecked(b);
}

auto UserPreferences::getDiaporamatime() const -> int
{
	return ui->m_diap_dur->value();
}

auto UserPreferences::setDiaporamatime(const int time) -> void
{
	ui->m_diap_dur->setValue(time);
}

auto UserPreferences::deletePermanently() const -> bool
{
	return ui->m_delete_file->isChecked();
}

auto UserPreferences::deletePermanently(const bool b) -> void
{
	ui->m_delete_file->setChecked(b);
}

auto UserPreferences::deleteFolderPath() const -> QString
{
	return ui->m_folder_path->text();
}

auto UserPreferences::deleteFolderPath(const QString &path) -> void
{
	ui->m_folder_path->setText(path);
}

auto UserPreferences::chooseFolder() -> void
{
	const auto &dir = QFileDialog::getExistingDirectory(this,
	                                                    tr("Choisir Dossier"),
	                                                    QDir::homePath(),
	                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	ui->m_folder_path->setText(dir);
}
