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

#include <QDialog>

namespace Ui {
class UserPreferences;
}

enum {
	DELETE_PERMANENTLY = 0,
	MOVE_TO_TRASH      = 1,
	MOVE_TO_FOLDER     = 2,
};

class UserPreferences final : public QDialog {
	Q_OBJECT

	Ui::UserPreferences *ui;

private slots:
	void chooseFolder();
	void updateUI();

public:
	explicit UserPreferences(QWidget *parent = nullptr);
	~UserPreferences();

	auto getRandomMode() const -> bool;
	auto setRandomMode(const bool b) -> void;
	auto diaporamaTime() const -> int;
	auto setDiaporamatime(const int time) -> void;
	auto fileRemovingMethod() const -> int;
	auto fileRemovingMethod(const int index) -> void;
	auto changeFolderPath(const QString &path) -> void;
	auto changeFolderPath() const -> QString;
	auto openSubdirs() const -> bool;
	auto openSubdirs(const bool b) -> void;
};
