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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "base_controles.h"

#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

#include "donnees_controle.h"
#include "morceaux.h"

namespace danjo {

enum {
	AXIS_X = 0,
	AXIS_Y = 1,
	AXIS_Z = 2,
};

/* ************************************************************************** */

Controle::Controle(QWidget *parent)
	: QWidget(parent)
{}

/* ************************************************************************** */

Etiquette::Etiquette(QWidget *parent)
	: Controle(parent)
	, m_agencement(new QHBoxLayout(this))
	, m_etiquette(new QLabel(this))
{
	m_agencement->addWidget(m_etiquette);
	setLayout(m_agencement);
}

void Etiquette::finalise(const DonneesControle &donnees)
{
	m_etiquette->setText(donnees.valeur_defaut.c_str());
}

/* ************************************************************************** */

SelecteurFloat::SelecteurFloat(QWidget *parent)
	: Controle(parent)
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

void SelecteurFloat::finalise(const DonneesControle &)
{
	setRange(m_min, m_max);
}

void SelecteurFloat::ValueChanged()
{
	const auto value = m_slider->value();
	const float fvalue = value / m_scale;
	m_spin_box->setValue(fvalue);
	Q_EMIT(valeur_changee(fvalue));
}

void SelecteurFloat::updateLabel(int value)
{
	m_spin_box->setValue(value / m_scale);
}

void SelecteurFloat::valeur(float value)
{
	m_spin_box->setValue(value);
	m_slider->setValue(value * m_scale);
}

float SelecteurFloat::valeur() const
{
	return m_spin_box->value();
}

void SelecteurFloat::setRange(float min, float max)
{
	if (min > 0.0f && min < 1.0f) {
		m_scale = 1.0f / min;
	}
	else {
		m_scale = 10000.0f;
	}

	m_slider->setRange(min * m_scale, max * m_scale);
	m_spin_box->setRange(min * m_scale, max * m_scale);
}

/* ************************************************************************** */

SelecteurInt::SelecteurInt(QWidget *parent)
	: Controle(parent)
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

void SelecteurInt::finalise(const DonneesControle &)
{

}

void SelecteurInt::ValueChanged()
{
	const auto value = m_slider->value();
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
	: Controle(parent)
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
	m_x->valeur(value[AXIS_X]);
	m_y->valeur(value[AXIS_Y]);
	m_z->valeur(value[AXIS_Z]);
}

void SelecteurVec3::getValue(float *value) const
{
	value[AXIS_X] = m_x->valeur();
	value[AXIS_Y] = m_y->valeur();
	value[AXIS_Z] = m_z->valeur();
}

void SelecteurVec3::setMinMax(float min, float max) const
{
	m_x->setRange(min, max);
	m_y->setRange(min, max);
	m_z->setRange(min, max);
}

/* ************************************************************************** */

SelecteurFichier::SelecteurFichier(bool input, QWidget *parent)
	: Controle(parent)
	, m_agencement(new QHBoxLayout(this))
	, m_line_edit(new QLineEdit(this))
	, m_push_button(new QPushButton("Choisir Fichier", this))
	, m_input(input)
{
	m_agencement->addWidget(m_line_edit);
	m_agencement->addWidget(m_push_button);

	setLayout(m_agencement);

	connect(m_push_button, SIGNAL(clicked()), this, SLOT(setChoosenFile()));
}

void SelecteurFichier::setValue(const QString &text)
{
	m_line_edit->setText(text);
}

void SelecteurFichier::setChoosenFile()
{
	QString chemin;
	QString caption = "";
	QString dir = "";
	QString filtres = m_filtres;

	if (m_input) {
		chemin = QFileDialog::getOpenFileName(this, caption, dir, filtres);
	}
	else {
		chemin = QFileDialog::getSaveFileName(this, caption, dir, filtres);
	}

	if (!chemin.isEmpty()) {
		m_line_edit->setText(chemin);
		Q_EMIT(valeur_changee(chemin));
	}
}

void SelecteurFichier::ajourne_filtres(const QString &chaine)
{
	m_filtres = chaine;
}

/* ************************************************************************** */

SelecteurListe::SelecteurListe(QWidget *parent)
	: Controle(parent)
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

void SelecteurListe::addField(const QString &text)
{
	auto action = m_list_widget->addAction(text);
	connect(action, SIGNAL(triggered()), this, SLOT(handleClick()));
}

void SelecteurListe::setValue(const QString &text)
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
	: Controle(parent)
	, m_couleur(nullptr)
{}

void SelecteurCouleur::finalise(const DonneesControle &donnees)
{
	m_min = std::atof(donnees.valeur_min.c_str());
	m_max = std::atof(donnees.valeur_max.c_str());
	m_couleur = static_cast<float *>(donnees.pointeur);

	auto valeurs = decoupe(donnees.valeur_defaut, ',');
	auto index = 0;

	m_valeur_defaut[0] = 1.0f;
	m_valeur_defaut[1] = 1.0f;
	m_valeur_defaut[2] = 1.0f;
	m_valeur_defaut[3] = 1.0f;

	for (auto v : valeurs) {
		m_valeur_defaut[index++] = std::atof(v.c_str());
	}

	if (donnees.initialisation) {
		for (int i = 0; i < 4; ++i) {
			m_couleur[i] = m_valeur_defaut[i];
		}
	}

	setToolTip(donnees.infobulle.c_str());
}

void SelecteurCouleur::mouseReleaseEvent(QMouseEvent *e)
{
	if (QRect(QPoint(0, 0), this->size()).contains(e->pos())) {
		Q_EMIT(clicked());

		const auto &color = QColorDialog::getColor(QColor(m_couleur[0] * 255, m_couleur[1] * 255, m_couleur[2] * 255, m_couleur[3] * 255));

		if (color.isValid()) {
			m_couleur[0] = color.redF();
			m_couleur[1] = color.greenF();
			m_couleur[2] = color.blueF();
			m_couleur[3] = color.alphaF();

			for (int i = 0; i < 4; ++i) {
				Q_EMIT(valeur_changee(m_couleur[i], i));
			}
		}
	}
}

void SelecteurCouleur::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	const auto &rect = this->geometry();

	QColor color(m_couleur[0] * 255, m_couleur[1] * 255, m_couleur[2] * 255, m_couleur[3] * 255);

	const auto w = rect.width();
	const auto h = rect.height();

	painter.fillRect(0, 0, w, h, color);
}

}  /* namespace danjo */
