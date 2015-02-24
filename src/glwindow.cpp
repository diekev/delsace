#include "glwindow.h"

GLWindow::GLWindow(QWidget *parent, MainWindow &window)
	: QGLWidget(parent)
	, m_main_win(window)
{}

void GLWindow::initializeGL()
{
	glEnable(GL_TEXTURE_2D);
	m_program.addShaderFromSourceFile(QGLShader::Vertex, ":/gpu_shaders/vertex_shader.glsl");
	m_program.addShaderFromSourceFile(QGLShader::Fragment, ":/gpu_shaders/fragment_shader.glsl");

	m_vertices[0] = QVector2D(0.0f, 0.0f);
	m_vertices[1] = QVector2D(1.0f, 0.0f);
	m_vertices[2] = QVector2D(1.0f, 1.0f);
	m_vertices[3] = QVector2D(0.0f, 1.0f);

	GLushort *id = &m_indices[0];
	*id++ = 0;
	*id++ = 1;
	*id++ = 2;
	*id++ = 0;
	*id++ = 2;
	*id++ = 3;
}

void GLWindow::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	loadImage();

	if (m_data != nullptr) {
		resize(m_data->size());

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, m_vertices);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
		glDisableClientState(GL_VERTEX_ARRAY);
	}

//	glDrawPixels(m_data->width(), m_data->height(), GL_RGBA, GL_UNSIGNED_BYTE, m_gl_data.bits());
//	m_program.bind();
//	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
//	m_program.release();
}

void GLWindow::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, 0, h, -1, 1);
	glMatrixMode(GL_MODELVIEW);
}

void GLWindow::loadImage()
{
	m_data = m_main_win.currentImage();
	m_gl_data = QGLWidget::convertToGLFormat(*m_data);

	glGenTextures(1, &m_textures[0]);
	glBindTexture(GL_TEXTURE_2D, m_textures[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_gl_data.width(), m_gl_data.height(),
				 0, GL_RGB, GL_UNSIGNED_BYTE, m_gl_data.bits());
}
