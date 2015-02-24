#ifndef GLWINDOW_H
#define GLWINDOW_H

#include <QGLShaderProgram>
#include <QGLWidget>

#include "mainwindow.h"

class GLWindow : public QGLWidget {
	QGLShaderProgram m_program;
	GLuint m_textures[1];
	GLuint m_vaoID;
	GLuint m_vbo_vertsID;
	GLuint m_vbo_indexID;
	QVector2D m_vertices[4];
	GLushort m_indices[6];

protected:
	class MainWindow &m_main_win;
	QImage *m_data, m_gl_data;

public:
	GLWindow(QWidget *parent, MainWindow &window);

	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);
	void loadImage();
};

#endif // GLWINDOW_H
