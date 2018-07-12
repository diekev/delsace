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

#pragma once

#include <QWidget>

class QDoubleSpinBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QMenu;
class QPushButton;
class QSlider;
class QSpinBox;
class QVBoxLayout;

/* ************************************************************************** */

namespace danjo {

struct DonneesControle;

/**
 * La classe Controle donne l'interface nécessaire pour les contrôles à afficher
 * dans l'interface utilisateur. Dès que le contrôle est changé le signal
 * Controle::controle_change() est émis.
 */
class Controle : public QWidget {
	Q_OBJECT

public:
	explicit Controle(QWidget *parent = nullptr);

	/**
	 * Finalise le contrôle. Cette fonction est appelée à la fin de la création
	 * du contrôle par l'assembleur de contrôle.
	 */
	virtual void finalise(const DonneesControle &donnees) = 0;

Q_SIGNALS:
	/**
	 * Signal émis quand la valeur du contrôle est changée dans l'interface.
	 */
	void controle_change();
};

/* ************************************************************************** */

class Etiquette final : public Controle {
	QHBoxLayout *m_agencement;
	QLabel *m_etiquette;

public:
	explicit Etiquette(QWidget *parent = nullptr);

	void finalise(const DonneesControle &donnees) override;
};

/* ************************************************************************** */

class SelecteurFloat : public Controle {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	QDoubleSpinBox *m_spin_box;
	QSlider *m_slider;

protected:
	float m_scale;
	float m_min;
	float m_max;

public:
	explicit SelecteurFloat(QWidget *parent = nullptr);
	~SelecteurFloat() = default;

	void finalise(const DonneesControle &) override;

	void valeur(float value);
	float valeur() const;

	void setRange(float min, float max);

Q_SIGNALS:
	void valeur_changee(double value);

private Q_SLOTS:
	void ValueChanged();
	void updateLabel(int value);
};

/* ************************************************************************** */

class SelecteurInt : public Controle {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	QSpinBox *m_spin_box;
	QSlider *m_slider;

protected:
	int m_min, m_max;

public:
	explicit SelecteurInt(QWidget *parent = nullptr);
	~SelecteurInt() = default;

	void finalise(const DonneesControle &) override;

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

class SelecteurVec3 : public Controle {
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

	void setValue(float *value);
	void getValue(float *value) const;
	void setMinMax(float min, float max) const;
};

/* ************************************************************************** */

class SelecteurFichier : public Controle {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	QLineEdit *m_line_edit;
	QPushButton *m_push_button;

	QString m_filtres;
	bool m_input;

public:
	explicit SelecteurFichier(bool input, QWidget *parent = nullptr);

	~SelecteurFichier() = default;

	void setValue(const QString &text);

	void ajourne_filtres(const QString &chaine);

private Q_SLOTS:
	void setChoosenFile();

Q_SIGNALS:
	void valeur_changee(const QString &text);
};

/* ************************************************************************** */

class SelecteurListe : public Controle {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	QLineEdit *m_line_edit;
	QPushButton *m_push_button;
	QMenu *m_list_widget;

	bool m_input;

public:
	explicit SelecteurListe(QWidget *parent = nullptr);

	~SelecteurListe();

	void setValue(const QString &text);

	void addField(const QString &text);

private Q_SLOTS:
	void showList();
	void handleClick();
	void updateText();

Q_SIGNALS:
	void valeur_changee(const QString &text);
};

/* ************************************************************************** */

class SelecteurCouleur : public Controle {
	Q_OBJECT

	float m_min = 0.0f;
	float m_max = 0.0f;

protected:
	float m_valeur_defaut[4];
	float *m_couleur = nullptr;

public:
	explicit SelecteurCouleur(QWidget *parent = nullptr);

	~SelecteurCouleur() = default;

	void mouseReleaseEvent(QMouseEvent *e) override;

	void finalise(const DonneesControle &donnees) override;

	void paintEvent(QPaintEvent *) override;

Q_SIGNALS:
	void clicked();
	void valeur_changee(double value, int axis);
};

}  /* namespace danjo */
