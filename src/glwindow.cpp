#include "glwindow.h"

GLWindow::GLWindow(QWidget *parent, MainWindow &window)
	: QGLWidget(parent)
	, m_main_win(window)
{}

void GLWindow::initializeGL()
{
	m_program.addShaderFromSourceFile(QGLShader::Vertex, ":/gpu_shaders/vertex_shader.glsl");
	m_program.addShaderFromSourceFile(QGLShader::Fragment, ":/gpu_shaders/fragment_shader.glsl");
}

void GLWindow::paintGL()
{
	m_data = m_main_win.currentImage();
	m_gl_data = QGLWidget::convertToGLFormat(*m_data);
	resize(m_data->size());
	glDrawPixels(m_data->width(), m_data->height(), GL_RGBA, GL_UNSIGNED_BYTE, m_gl_data.bits());
}

void GLWindow::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, 0, h, -1, 1);
	glMatrixMode(GL_MODELVIEW);
}
