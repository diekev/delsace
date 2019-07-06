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

#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QWidget>
#pragma GCC diagnostic pop

class QDoubleSpinBox;
class QHBoxLayout;
class QLineEdit;
class QMenu;
class QPushButton;
class QSlider;
class QSpinBox;
class QVBoxLayout;

/* ************************************************************************** */

class SelecteurFloat : public QWidget {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	QDoubleSpinBox *m_spin_box;
	QSlider *m_slider;

	float m_scale;

public:
	explicit SelecteurFloat(QWidget *parent = nullptr);
	~SelecteurFloat() = default;

	SelecteurFloat(SelecteurFloat const &) = default;
	SelecteurFloat &operator=(SelecteurFloat const &) = default;

	void setValue(float value);
	float value() const;
	void setRange(float min, float max);

Q_SIGNALS:
	void valeur_changee(double value);

private Q_SLOTS:
	void ValueChanged();
	void updateLabel(int value);
};

/* ************************************************************************** */

class SelecteurInt : public QWidget {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	QSpinBox *m_spin_box;
	QSlider *m_slider;

public:
	explicit SelecteurInt(QWidget *parent = nullptr);
	~SelecteurInt() = default;

	SelecteurInt(SelecteurInt const &) = default;
	SelecteurInt &operator=(SelecteurInt const &) = default;

	void setValue(int value);
	int value() const;
	void setRange(int min, int max);

Q_SIGNALS:
	void valeur_changee(int value);

private Q_SLOTS:
	void ValueChanged();
	void updateLabel(int value);
};

/* ************************************************************************** */

class SelecteurVec3 : public QWidget {
	Q_OBJECT

	SelecteurFloat *m_x, *m_y, *m_z;
	QVBoxLayout *m_agencement;

private Q_SLOTS:
	void xValueChanged(double value);
	void yValueChanged(double value);
	void zValueChanged(double value);

Q_SIGNALS:
	void valeur_changee(double value, int axis);

public:
	explicit SelecteurVec3(QWidget *parent = nullptr);
	~SelecteurVec3() = default;

	SelecteurVec3(SelecteurVec3 const &) = default;
	SelecteurVec3 &operator=(SelecteurVec3 const &) = default;

	void setValue(float *value);
	void getValue(float *value) const;
	void setMinMax(float min, float max) const;
};

/* ************************************************************************** */

class SelecteurFichier : public QWidget {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	QLineEdit *m_line_edit;
	QPushButton *m_push_button;

	bool m_input;

public:
	explicit SelecteurFichier(bool input, QWidget *parent = nullptr);

	~SelecteurFichier() = default;

	SelecteurFichier(SelecteurFichier const &) = default;
	SelecteurFichier &operator=(SelecteurFichier const &) = default;

	void setValue(QString const &text);

private Q_SLOTS:
	void setChoosenFile();

Q_SIGNALS:
	void valeur_changee(QString const &text);
};

/* ************************************************************************** */

class SelecteurListe : public QWidget {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	QLineEdit *m_line_edit;
	QPushButton *m_push_button;
	QMenu *m_list_widget;

	bool m_input{};

public:
	explicit SelecteurListe(QWidget *parent = nullptr);

	~SelecteurListe();

	SelecteurListe(SelecteurListe const &) = default;
	SelecteurListe &operator=(SelecteurListe const &) = default;

	void setValue(QString const &text);

	void addField(QString const &text);

private Q_SLOTS:
	void showList();
	void handleClick();
	void updateText();

Q_SIGNALS:
	void valeur_changee(QString const &text);
};

/* ************************************************************************** */

class SelecteurCouleur : public QWidget {
	Q_OBJECT

	float *m_color;

public:
	explicit SelecteurCouleur(QWidget *parent = nullptr);

	~SelecteurCouleur() = default;

	SelecteurCouleur(SelecteurCouleur const &) = default;
	SelecteurCouleur &operator=(SelecteurCouleur const &) = default;

	void setValue(float *value);
	void setMinMax(float min, float max) const;

	void mouseReleaseEvent(QMouseEvent *e) override;
	void paintEvent(QPaintEvent *) override;

Q_SIGNALS:
	void clicked();
	void valeur_changee(double value, int axis);
};
