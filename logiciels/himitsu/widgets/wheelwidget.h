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

#pragma once

#include <QWidget>
#include <random>

#include "biblinternes/structures/chaine.hh"

class QHBoxLayout;
class QLabel;
class QSpinBox;

class WheelWidget : public QWidget {
	Q_OBJECT

protected:
	QHBoxLayout *m_layout = nullptr;
	QLabel *m_pin_label = nullptr;
	QSpinBox *m_spin_box = nullptr;
	u_int64_t m_num_values = 0;

private Q_SLOTS:
	virtual void updateLabel(int value);

public:
	WheelWidget(int num_values, QWidget *parent = nullptr);
	~WheelWidget();

	WheelWidget(WheelWidget const &) = default;
	WheelWidget &operator=(WheelWidget const &) = default;

	virtual void rotate(const int increment);
	virtual int value() const;
	virtual void setValue(const int value);
};

class PinWheel : public WheelWidget {
	Q_OBJECT

	std::mt19937 m_rng{};
	u_int64_t m_pins = 0;
	QString m_pins_str = "";
	dls::chaine m_keys = "";

private Q_SLOTS:
	virtual void updateLabel(int value);
	void setPins(const QString &pins);

public:
	PinWheel(int num_values, QWidget *parent = nullptr);
	~PinWheel() = default;

	PinWheel(PinWheel const &) = default;
	PinWheel &operator=(PinWheel const &) = default;

	int pinValue() const;
	QString pinStr() const;
	void generatePins();
};

class LetterWheel : public WheelWidget {
	Q_OBJECT

	dls::chaine m_keys = "";

private Q_SLOTS:
	virtual void updateLabel(int value);

public:
	LetterWheel(int num_values, QWidget *parent = nullptr);
	~LetterWheel() = default;

	LetterWheel(LetterWheel const &) = default;
	LetterWheel &operator=(LetterWheel const &) = default;

	void setKeys(const dls::chaine &keys);
	char key();
	char key(const int pos);
};
