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

#include <iostream>

#include <QApplication>
#include <QDir>
#include <QSettings>
#include <QTranslator>

#include "databasemanager.h"
#include "loginform.h"
#include "mainwindow.h"
#include "utilisateur.h"

static void init_save_system()
{
	qRegisterMetaTypeStreamOperators<Utilisateur>("Utilisateur");
	qRegisterMetaTypeStreamOperators<Compte>("Compte");
	qRegisterMetaTypeStreamOperators<Transaction>("Transaction");

	QMetaTypeId<Utilisateur>();
	QMetaTypeId<Compte>();
	QMetaTypeId<Transaction>();
}

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	QCoreApplication::setOrganizationName("giraffeenfeu");
	QCoreApplication::setApplicationName("cresus");

	init_save_system();

	QSettings settings;
	auto databasepath = settings.value("databasepath").toString();

	if (databasepath == "") {
		auto dir_path = QDir::homePath();
		dir_path.append(QDir::separator()).append(".cresus");

		QDir db_dir(dir_path);
		if (!db_dir.exists()) {
			db_dir.mkdir(dir_path);
		}

		dir_path.append(QDir::separator()).append("users.sqlite");
		databasepath = QDir::toNativeSeparators(dir_path);
		settings.setValue("databasepath", databasepath);
	}

	DatabaseManager db_manager;

	MainWindow main_window;
	main_window.resize(1280, 720);

	if (db_manager.openDatabase(databasepath)) {
		db_manager.createTable();

		LoginForm login;

		/* set database after opening it! */
		login.setDatabase(db_manager.getDatabase());
		main_window.setDatabase(db_manager.getDatabase());

		if (login.exec() == QDialog::Accepted) {
			if (login.newUser()) {
				db_manager.createUserTables(login.getUsername());
				main_window.createUser(login.getUser());
			}

			main_window.setUser(login.getUser());
			main_window.show();
		}
		else {
			return 0;
		}
	}
	else {
		return 1;
	}

	return app.exec();
}
