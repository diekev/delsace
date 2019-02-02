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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "base_controles.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#pragma GCC diagnostic pop

enum {
	AXIS_X = 0,
	AXIS_Y = 1,
	AXIS_Z = 2,
};

/* ************************************************************************** */

SelecteurFloat::SelecteurFloat(QWidget *parent)
    : QWidget(parent)
	, m_agencement(new QHBoxLayout(this))
    , m_spin_box(new QDoubleSpinBox(this))
    , m_slider(new QSlider(Qt::Orientation::Horizontal, this))
    , m_scale(1.0f)
{
	m_agencement->addWidget(m_spin_box);
	m_agencement->addWidget(m_slider);

	setLayout(m_agencement);

	m_spin_box->setAlignment(Qt::AlignRight);
	m_spin_box->setButtonSymbols(QAbstractSpinBox::NoButtons);
	m_spin_box->setReadOnly(true);

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	connect(m_slider, SIGNAL(sliderReleased()), this, SLOT(ValueChanged()));
	connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(updateLabel(int)));
}

void SelecteurFloat::ValueChanged()
{
	auto const value = m_slider->value();
	auto const fvalue = static_cast<double>(static_cast<float>(value) / m_scale);
	m_spin_box->setValue(fvalue);
	Q_EMIT(valeur_changee(fvalue));
}

void SelecteurFloat::updateLabel(int value)
{
	m_spin_box->setValue(static_cast<double>(static_cast<float>(value) / m_scale));
}

void SelecteurFloat::setValue(float value)
{
	m_spin_box->setValue(static_cast<double>(value));
	m_slider->setValue(static_cast<int>(value * m_scale));
}

float SelecteurFloat::value() const
{
	return static_cast<float>(m_spin_box->value());
}

void SelecteurFloat::setRange(float min, float max)
{
	if (min > 0.0f && min < 1.0f) {
		m_scale = 1.0f / min;
	}
	else {
		m_scale = 10000.0f;
	}

	m_slider->setRange(static_cast<int>(min * m_scale), static_cast<int>(max * m_scale));
	m_spin_box->setRange(static_cast<double>(min * m_scale), static_cast<double>(max * m_scale));
}

/* ************************************************************************** */

SelecteurInt::SelecteurInt(QWidget *parent)
    : QWidget(parent)
	, m_agencement(new QHBoxLayout(this))
    , m_spin_box(new QSpinBox(this))
    , m_slider(new QSlider(Qt::Orientation::Horizontal, this))
{
	m_agencement->addWidget(m_spin_box);
	m_agencement->addWidget(m_slider);

	setLayout(m_agencement);

	m_spin_box->setAlignment(Qt::AlignRight);
	m_spin_box->setButtonSymbols(QAbstractSpinBox::NoButtons);
	m_spin_box->setReadOnly(true);

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	connect(m_slider, SIGNAL(sliderReleased()), this, SLOT(ValueChanged()));
	connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(updateLabel(int)));
}

void SelecteurInt::ValueChanged()
{
	auto const value = m_slider->value();
	m_spin_box->setValue(value);
	Q_EMIT(valeur_changee(value));
}

void SelecteurInt::updateLabel(int value)
{
	m_spin_box->setValue(value);
}

void SelecteurInt::setValue(int value)
{
	m_spin_box->setValue(value);
	m_slider->setValue(value);
}

int SelecteurInt::value() const
{
	return m_spin_box->value();
}

void SelecteurInt::setRange(int min, int max)
{
	m_slider->setRange(min, max);
	m_spin_box->setRange(min, max);
}

/* ************************************************************************** */

SelecteurVec3::SelecteurVec3(QWidget *parent)
    : QWidget(parent)
    , m_x(new SelecteurFloat(this))
    , m_y(new SelecteurFloat(this))
    , m_z(new SelecteurFloat(this))
	, m_agencement(new QVBoxLayout(this))
{
	connect(m_x, SIGNAL(valeur_changee(double)), this, SLOT(xValueChanged(double)));
	connect(m_y, SIGNAL(valeur_changee(double)), this, SLOT(yValueChanged(double)));
	connect(m_z, SIGNAL(valeur_changee(double)), this, SLOT(zValueChanged(double)));

	m_agencement->addWidget(m_x);
	m_agencement->addWidget(m_y);
	m_agencement->addWidget(m_z);

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

	m_agencement->setSpacing(0);
}

void SelecteurVec3::xValueChanged(double value)
{
	Q_EMIT(valeur_changee(value, AXIS_X));
}

void SelecteurVec3::yValueChanged(double value)
{
	Q_EMIT(valeur_changee(value, AXIS_Y));
}

void SelecteurVec3::zValueChanged(double value)
{
	Q_EMIT(valeur_changee(value, AXIS_Z));
}

void SelecteurVec3::setValue(float *value)
{
	m_x->setValue(value[AXIS_X]);
	m_y->setValue(value[AXIS_Y]);
	m_z->setValue(value[AXIS_Z]);
}

void SelecteurVec3::getValue(float *value) const
{
	value[AXIS_X] = m_x->value();
	value[AXIS_Y] = m_y->value();
	value[AXIS_Z] = m_z->value();
}

void SelecteurVec3::setMinMax(float min, float max) const
{
	m_x->setRange(min, max);
	m_y->setRange(min, max);
	m_z->setRange(min, max);
}

/* ************************************************************************** */

SelecteurFichier::SelecteurFichier(bool input, QWidget *parent)
    : QWidget(parent)
	, m_agencement(new QHBoxLayout(this))
    , m_line_edit(new QLineEdit(this))
    , m_push_button(new QPushButton("Open File", this))
    , m_input(input)
{
	m_agencement->addWidget(m_line_edit);
	m_agencement->addWidget(m_push_button);

	setLayout(m_agencement);

	connect(m_push_button, SIGNAL(clicked()), this, SLOT(setChoosenFile()));
}

void SelecteurFichier::setValue(QString const &text)
{
	m_line_edit->setText(text);
}

void SelecteurFichier::setChoosenFile()
{
	auto const filename = m_input ? QFileDialog::getOpenFileName(this)
	                              : QFileDialog::getSaveFileName(this);

	if (!filename.isEmpty()) {
		m_line_edit->setText(filename);
		Q_EMIT(valeur_changee(filename));
	}
}

/* ************************************************************************** */

SelecteurListe::SelecteurListe(QWidget *parent)
    : QWidget(parent)
	, m_agencement(new QHBoxLayout(this))
    , m_line_edit(new QLineEdit(this))
    , m_push_button(new QPushButton("list", this))
    , m_list_widget(new QMenu())
{
	m_agencement->addWidget(m_line_edit);
	m_agencement->addWidget(m_push_button);

	connect(m_push_button, SIGNAL(clicked()), this, SLOT(showList()));

	connect(m_line_edit, SIGNAL(returnPressed()), this, SLOT(updateText()));
}

SelecteurListe::~SelecteurListe()
{
	delete m_list_widget;
}

void SelecteurListe::addField(QString const &text)
{
	auto action = m_list_widget->addAction(text);
	connect(action, SIGNAL(triggered()), this, SLOT(handleClick()));
}

void SelecteurListe::setValue(QString const &text)
{
	m_line_edit->setText(text);
}

void SelecteurListe::showList()
{
	/* Figure out where the bottom left corner of the push is located. */
	QRect widgetRect = m_push_button->geometry();
	auto bottom_left = m_push_button->parentWidget()->mapToGlobal(widgetRect.bottomLeft());

	m_list_widget->popup(bottom_left);
}

void SelecteurListe::updateText()
{
	Q_EMIT(valeur_changee(m_line_edit->text()));
}

void SelecteurListe::handleClick()
{
	auto action = qobject_cast<QAction *>(sender());

	if (!action) {
		return;
	}

	auto text = m_line_edit->text();

	if (!text.isEmpty()) {
		text += ",";
	}

	text += action->text();

	this->setValue(text);
	Q_EMIT(valeur_changee(text));
}

/* ************************************************************************** */

SelecteurCouleur::SelecteurCouleur(QWidget *parent)
    : QWidget(parent)
    , m_color(nullptr)
{}

void SelecteurCouleur::setValue(float *value)
{
	m_color = value;
}

void SelecteurCouleur::setMinMax(float /*min*/, float /*max*/) const
{

}

[[nodiscard]] auto converti_couleur(float couleur[4])
{
	auto resultat = QColor{};
	resultat.setRed(static_cast<int>(couleur[0] * 255.0f));
	resultat.setGreen(static_cast<int>(couleur[1] * 255.0f));
	resultat.setBlue(static_cast<int>(couleur[2] * 255.0f));
	resultat.setAlpha(static_cast<int>(couleur[3] * 255.0f));

	return resultat;
}

void SelecteurCouleur::mouseReleaseEvent(QMouseEvent *e)
{
	if (QRect(QPoint(0, 0), this->size()).contains(e->pos())) {
		Q_EMIT(clicked());

		auto const &color = QColorDialog::getColor(converti_couleur(m_color));

		if (color.isValid()) {
			m_color[0] = static_cast<float>(color.redF());
			m_color[1] = static_cast<float>(color.greenF());
			m_color[2] = static_cast<float>(color.blueF());
			m_color[3] = static_cast<float>(color.alphaF());

			for (int i = 0; i < 4; ++i) {
				Q_EMIT(valeur_changee(static_cast<double>(m_color[i]), i));
			}
		}
	}
}

void SelecteurCouleur::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	auto const &rect = this->geometry();

	auto color = converti_couleur(m_color);

	auto const w = rect.width();
	auto const h = rect.height();

	painter.fillRect(0, 0, w, h, color);
}
