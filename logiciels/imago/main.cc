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

#include <QApplication>
#include <QFileInfo>

#include "src/mainwindow.h"

#include <iostream>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QCoreApplication::setOrganizationName("giraffeenfeu");
	QCoreApplication::setApplicationName("imago");

	MainWindow w;
	w.showMaximized();
	w.reset();

	/* load image or directory passed as argument */
	if (argc > 1) {
		auto file_info = QFileInfo(argv[1]);

		if (file_info.isDir()) {
			w.openDirectory(file_info.filePath());
		}
		else {
			w.openImage(file_info.filePath());
		}
	}

	return a.exec();
}
