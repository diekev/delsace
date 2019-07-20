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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#pragma once

#include <QDialog>
#include <QSqlDatabase>

namespace Ui {
class LoginForm;
}

class LoginForm : public QDialog {
	Q_OBJECT

	QSqlDatabase m_database = {};
	QString m_user = "";
	QString m_username = "";

	Ui::LoginForm *ui = nullptr;

	bool m_new_user = false;

private Q_SLOTS:
	void onLoginButClicked();	
	void onTextEditChange();

public:
	explicit LoginForm(QWidget *parent = nullptr);
	~LoginForm();

	LoginForm(LoginForm const &) = default;
	LoginForm &operator=(LoginForm const &) = default;

	void setDatabase(const QSqlDatabase &db);
	void setUser(const QString &name);
	auto getUser() const -> QString;
	void setUsername(const QString &name);
	auto getUsername() const -> QString;
	bool newUser() const;
};
