#include <iostream>

#include <QApplication>
#include <QDir>
#include <QFileInfo>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MainWindow w;

	w.show();

	/* load image passed as argument */
	if (argc > 1) {
		auto dir = QFileInfo(argv[1]).absoluteDir().path();
		auto file = QDir::cleanPath(dir + QDir::separator() + argv[1]);

		w.openImageFromDir(file.toStdString(), dir);
	}

	return a.exec();
}
