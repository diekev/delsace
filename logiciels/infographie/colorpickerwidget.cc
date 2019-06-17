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

#include "colorpickerwidget.h"

#include <QComboBox>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

enum {
	COLOR_R = 0,
	COLOR_G = 1,
	COLOR_B = 2,
	COLOR_A = 3,
};

template <typename T>
inline T clamp(const T x, const T min, const T max)
{
	return (x < min) ? min : (x > max) ? max : x;
}

ColorWidget::ColorWidget(QWidget *parent)
    : QWidget(parent)
{
	QPalette palette(ColorWidget::palette());
	palette.setColor(backgroundRole(), Qt::white);
	this->setPalette(palette);

	this->setMaximumSize(256, 256);

	setCurvesPos();
}

void ColorWidget::setRole(int role)
{
	m_role = role;
	repaint();
}

void ColorWidget::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	QColor color(255, 255, 255);

	painter.setRenderHint(QPainter::Antialiasing);

	const auto size = this->size();
	const auto sx = size.width();
	const auto sy = size.height();

	switch (m_role) {
		case COLOR_R:
			for (int x = 0; x < sx; ++x) {
				for (int y = 0; y < sy; ++y) {
					const auto cx = x / static_cast<float>(sx) * 255;
					const auto cy = y / static_cast<float>(sy) * 255;
					color.setRgb(255 - cy, cx, cx);
					painter.setPen(color);
					painter.drawPoint(x, y);
				}
			}
			break;
		case COLOR_G:
			for (int x = 0; x < sx; ++x) {
				for (int y = 0; y < sy; ++y) {
					const auto cx = x / static_cast<float>(sx) * 255;
					const auto cy = y / static_cast<float>(sy) * 255;
					color.setRgb(cx, 255 - cy, cx);
					painter.setPen(color);
					painter.drawPoint(x, y);
				}
			}
			break;
		case COLOR_B:
			for (int x = 0; x < sx; ++x) {
				for (int y = 0; y < sy; ++y) {
					const auto cx = x / static_cast<float>(sx) * 255;
					const auto cy = y / static_cast<float>(sy) * 255;
					color.setRgb(cx, cx, 255 - cy);
					painter.setPen(color);
					painter.drawPoint(x, y);
				}
			}
			break;
		case COLOR_A:
			for (int x = 0; x < sx; ++x) {
				for (int y = 0; y < sy; ++y) {
					const auto cx = x / static_cast<float>(sx) * 255;
					const auto cy = y / static_cast<float>(sy) * 255;
					int col = (cx + cy) / 2;
					color.setRgb(255 - col, 255 - col, 255 - col);
					painter.setPen(color);
					painter.drawPoint(x, y);
				}
			}
			break;
	}

	painter.setPen(QColor(192, 192, 192));

	const auto stride_x = sx / 4;
	const auto stride_y = sy / 4;

	for (int i = 0; i < 3; ++i) {
		const auto stride = stride_x * (i + 1);
		painter.drawLine(stride, 0, stride, sy);
	}

	for (int i = 0; i < 3; ++i) {
		const auto stride = stride_y * (i + 1);
		painter.drawLine(0, stride, sx, stride);
	}

	painter.setPen(QPen(Qt::black, 5));
	for (const auto &point : m_curves[m_role]) {
		painter.drawPoint(point.x() * sx, point.y() * sy);
	}

	painter.setPen(QPen(Qt::black, 1));
	painter.drawLine(m_curves[m_role][0].x() * sx, m_curves[m_role][0].y() * sy,
	                 m_curves[m_role][1].x() * sx, m_curves[m_role][1].y() * sy);
	painter.drawLine(m_curves[m_role][1].x() * sx, m_curves[m_role][1].y() * sy,
	                 m_curves[m_role][2].x() * sx, m_curves[m_role][2].y() * sy);

	painter.setPen(Qt::black);
	painter.drawRect(this->rect());
}

void ColorWidget::resizeEvent(QResizeEvent */*e*/)
{
	//setCurvesPos();
}

void ColorWidget::mousePressEvent(QMouseEvent *e)
{
	qreal distance = std::numeric_limits<qreal>::max();
	const auto size = this->size();
	const auto sx = size.width();
	const auto sy = size.height();

	for (int i = 0; i < 3; ++i) {
		QPointF point(m_curves[m_role][i].x() * sx, m_curves[m_role][i].y() * sy);

		QVector2D vec(e->pos() - point);
		auto length = vec.length();

		if (length < distance) {
			distance = length;
			m_cur_point = &m_curves[m_role][i];
		}
	}

	if (m_cur_point) {
		m_mouse_down = true;
	}
}

void ColorWidget::mouseMoveEvent(QMouseEvent *e)
{
	if (m_mouse_down) {
		auto x = e->pos().x() / static_cast<float>(this->size().width());
		auto y = e->pos().y() / static_cast<float>(this->size().height());

		x = clamp(x, 0.0f, 1.0f);
		y = clamp(y, 0.0f, 1.0f);

		*m_cur_point = { x, y };
		repaint();
	}
}

void ColorWidget::mouseReleaseEvent(QMouseEvent *)
{
	m_mouse_down = false;
	m_cur_point = nullptr;
}

void ColorWidget::setCurvesPos()
{
	QPointF bottom(0.0f, 1.0f);
	QPointF middle(0.5f, 0.5f);
	QPointF top(1.0f, 0.0f);

	for (int i = 0; i < COLOR_A + 1; ++i) {
		m_curves[i].clear();
		m_curves[i].push_back(bottom);
		m_curves[i].push_back(middle);
		m_curves[i].push_back(top);
	}
}

ColorPickerWidget::ColorPickerWidget(QWidget *parent)
    : QWidget(parent)
    , m_frame(new ColorWidget(this))
    , m_layout(new QVBoxLayout(this))
    , m_role(new QComboBox(this))
{
	m_role->addItem("R", COLOR_R);
	m_role->addItem("G", COLOR_G);
	m_role->addItem("B", COLOR_B);
	m_role->addItem("A", COLOR_A);

	connect(m_role, SIGNAL(currentIndexChanged(int)), m_frame, SLOT(setRole(int)));

	m_layout->addWidget(m_role);
	m_layout->addWidget(m_frame);

	setLayout(m_layout);
}
