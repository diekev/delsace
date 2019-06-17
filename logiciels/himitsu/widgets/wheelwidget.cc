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

#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>

#include "wheelwidget.h"

/* ******************************* WheelWidget ******************************* */

WheelWidget::WheelWidget(int num_values, QWidget *parent)
    : QWidget(parent)
    , m_num_values(num_values)
{
	m_spin_box = new QSpinBox(this);
	m_spin_box->setValue(1);
	m_spin_box->setRange(1, num_values);
	m_spin_box->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	m_spin_box->setWrapping(true);

	m_pin_label = new QLabel(this);
	m_pin_label->setText(QString::number(0));
	m_pin_label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

	m_layout = new QHBoxLayout;
	m_layout->addWidget(m_spin_box);
	m_layout->addWidget(m_pin_label);

	setLayout(m_layout);

	connect(m_spin_box, SIGNAL(valueChanged(int)), this, SLOT(updateLabel(int)));
}

WheelWidget::~WheelWidget()
{
	delete m_layout;
}

void WheelWidget::rotate(const int increment)
{
//	std::rotate(m_keys.begin(), m_keys.begin() + ((increment > 0) ? 1 : 25), m_keys.end());
	m_spin_box->setValue(m_spin_box->value() + increment);
}

int WheelWidget::value() const
{
	return m_spin_box->value();
}

void WheelWidget::setValue(const int value)
{
	m_spin_box->setValue(value);
}

void WheelWidget::updateLabel(int value)
{
	m_pin_label->setText(QString::number(value));
}

/* ********************************* PinWheel ******************************** */

PinWheel::PinWheel(int num_values, QWidget *parent)
    : WheelWidget(num_values, parent)
    , m_rng(19337 + num_values)
    , m_pins(0ll)
{
	m_pins_str = "";
	generatePins();
	updateLabel(m_spin_box->value());
}

int PinWheel::pinValue() const
{
	auto pin = m_spin_box->value() - 1;
	return (m_pins & (1ll << pin)) >> pin;
}

QString PinWheel::pinStr() const
{
	return m_pins_str;
}

void PinWheel::generatePins()
{
	std::uniform_int_distribution<u_int64_t> dist(0ll, 1ll);
	m_pins_str.clear();
	m_pins &= 0ll;

	for (u_int64_t i(0ll); i < m_num_values; ++i) {
		auto val = dist(m_rng);
		m_pins |= (val << i);
		m_pins_str.push_back(QString::number(val));
	}
}

void PinWheel::updateLabel(int value)
{
	auto pin = value - 1;
	m_pin_label->setText(QString::number((m_pins & (1ll << pin)) >> pin));
}

void PinWheel::setPins(const QString &pins)
{
	m_pins_str = pins;
	m_pins &= 0ll;

	for (u_int64_t i(0ll); i < (u_int64_t)m_pins_str.size(); ++i) {
		auto val = ((m_pins_str[(int)i] == '0') ? 0ll : 1ll);
		m_pins |= (val << i);
	}

	updateLabel(m_spin_box->value());
}

/* ******************************* LetterWheel ******************************* */

LetterWheel::LetterWheel(int num_values, QWidget *parent)
    : WheelWidget(num_values, parent)
{}

void LetterWheel::setKeys(const std::string &keys)
{
	m_keys = keys;
	updateLabel(m_spin_box->value());
}

char LetterWheel::key()
{
	return m_keys[m_spin_box->value() - 1];
}

char LetterWheel::key(const int pos)
{
	return m_keys[((m_spin_box->value() - 1) + pos) % 26];
}

void LetterWheel::updateLabel(int value)
{
	auto key = value - 1;
	m_pin_label->setText(QChar(m_keys[key]));
}
