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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QApplication>
#include <QFile>
#pragma GCC diagnostic pop

#include "coeur/mikisa.h"
#include "coeur/sauvegarde.h"

#include "entreface/fenetre_principale.h"

#include <iostream>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QCoreApplication::setOrganizationName("delsace");
	QCoreApplication::setApplicationName("mikisa");

	QFile file("styles/main.qss");
	file.open(QFile::ReadOnly);
	QString style_sheet = QLatin1String(file.readAll());

	qApp->setStyleSheet(style_sheet);

	Mikisa mikisa;
	mikisa.initialise();

	FenetrePrincipale w(mikisa);
	w.setWindowTitle(QCoreApplication::applicationName());
	w.showMaximized();

	if (argc == 2) {
		auto chemin = argv[1];
		coeur::ouvre_projet(chemin, mikisa);
	}

	return a.exec();
}
