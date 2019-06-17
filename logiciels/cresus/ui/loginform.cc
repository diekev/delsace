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

#include "loginform.h"

#include <QDialogButtonBox>
#include <QDebug>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSqlQuery>
#include <QVBoxLayout>

#include "ui_login_form.h"

#include "util/util_bool.h"

LoginForm::LoginForm(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::LoginForm)
    , m_new_user(false)
{
	ui->setupUi(this);
	setWindowTitle(tr("S'identifier"));
	ui->m_error_label->setStyleSheet("QLabel { color : red; }");
}

LoginForm::~LoginForm()
{
	delete ui;
}

void LoginForm::setDatabase(const QSqlDatabase &db)
{
	m_database = db;
}

void LoginForm::setUser(const QString &name)
{
	m_user = name;
}

auto LoginForm::getUser() const -> QString
{
	return m_user;
}

void LoginForm::setUsername(const QString &name)
{
	m_username = name;
}

auto LoginForm::getUsername() const -> QString
{
	return m_username;
}

auto LoginForm::newUser() const -> bool
{
	return m_new_user;
}

auto LoginForm::onTextEditChange() -> void
{
	ui->m_error_label->clear();
}

void LoginForm::onLoginButClicked()
{
	if (!m_database.open()) {
		qDebug() << "Impossible d'ouvrir la base de données.";
		return;
	}

	QSqlQuery query;
	auto username = ui->m_user_edit->text();
	auto password = ui->m_pass_edit->text();

	/* create new user if the above two fields are blank */
	if (is_equal("", username, password)) {
		auto fullname = ui->m_fullname_create->text();
		auto username = ui->m_username_create->text();
		auto password = ui->m_password_create->text();
		auto password_check = ui->m_password_check_create->text();

		if (is_equal("", fullname, username, password, password_check)) {
			ui->m_error_label->setText(tr("Les champs sont tous vident !"));
			return;
		}

		if (password_check != password) {
			ui->m_error_label->setText(tr("Les mot de passes ne correpondent pas !"));
			ui->m_password_check_create->setFocus();
			return;
		}

		if (query.exec("insert into users (username, password, fullname) values('" + username + "','" + password + "','" + fullname + "')")) {
			setUser(fullname);
			setUsername(username);
			m_new_user = true;
			Q_EMIT accept();
			return;
		}
	}

	if (query.exec("select fullname from users where username='" + username + "' and password='" + password + "'")) {
		auto count = 0;

		while (query.next()) {
			count++;
			setUser(query.value("fullname").toString());
			setUsername(username);
		}

		if (count == 1) {
			Q_EMIT accept();
		}
		else {
			if (count > 1) {
				/* multiple username/passwords */
				ui->m_error_label->setText(tr("Cet utilisateur est enregistré plusieurs fois"));
			}
			else {
				/* incorrect */
				ui->m_error_label->setText(tr("Nom d'utilisateur ou mot de passe invalide"));
			}
		}
	}
}
