#ifndef GLWINDOW_H
#define GLWINDOW_H

#include <QGLShaderProgram>
#include <QGLWidget>

#include "mainwindow.h"

class GLWindow : public QGLWidget {
	QGLShaderProgram m_program;

protected:
	class MainWindow &m_main_win;
	QImage *m_data, m_gl_data;

public:
	GLWindow(QWidget *parent, MainWindow &window);

	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);
};

#endif // GLWINDOW_H
