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

namespace kangao {

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
	 * Établie le contenu de l'infobulle de ce contrôle.
	 */
	void etablie_infobulle(const std::string &valeur);

	/* Interface pure. */

	/**
	 * Établie l'attache de ce contrôle, c'est-à-dire le pointeur vers la valeur
	 * que le contrôle modifie (int, float, etc.).
	 */
	virtual void etablie_attache(void *pointeur) = 0;

	/**
	 * Établie la valeur par défaut du contôle.
	 */
	virtual void etablie_valeur(const std::string &valeur) = 0;

	/* Interface. */

	/**
	 * Établie la valeur minimum que le contrôle peut avoir.
	 */
	virtual void etablie_valeur_min(const std::string &valeur) {}

	/**
	 * Établie la valeur maximum que le contrôle peut avoir.
	 */
	virtual void etablie_valeur_max(const std::string &valeur) {}

	/**
	 * Établie la précision du contrôle, c'est-à-dire le nombre de chiffres
	 * significatifs pour les nombres décimaux.
	 */
	virtual void etablie_precision(const std::string &valeur) {}

	/**
	 * Établie le pas du contrôle, c'est-à-dire l'interval entre deux valeurs
	 * possibles.
	 */
	virtual void etablie_pas(const std::string &valeur) {}

	/**
	 * Finalise le contrôle. Cette fonction est appelée à la fin de la création
	 * du contrôle par l'assembleur de contrôle.
	 */
	virtual void finalise() {}

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

	void etablie_attache(void *pointeur) override;

	void etablie_valeur(const std::string &valeur) override;
};

/* ************************************************************************** */

class SelecteurFloat : public Controle {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	QDoubleSpinBox *m_spin_box;
	QSlider *m_slider;

	float m_scale;
	float m_min;
	float m_max;

public:
	explicit SelecteurFloat(QWidget *parent = nullptr);
	~SelecteurFloat() = default;

	void etablie_attache(void *pointeur) override {}

	void etablie_valeur(const std::string &valeur) override;

	void etablie_valeur_min(const std::string &valeur) override;
	void etablie_valeur_max(const std::string &valeur) override;

	void finalise() override;

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

	int m_min, m_max;

public:
	explicit SelecteurInt(QWidget *parent = nullptr);
	~SelecteurInt() = default;

	void etablie_valeur(const std::string &valeur) override;

	void etablie_valeur_min(const std::string &valeur) override;
	void etablie_valeur_max(const std::string &valeur) override;

	void finalise() override;

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

	void etablie_valeur(const std::string &valeur) override;

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

	bool m_input;

public:
	explicit SelecteurFichier(bool input, QWidget *parent = nullptr);

	~SelecteurFichier() = default;

	void etablie_valeur(const std::string &valeur) override;

	void setValue(const QString &text);

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

	void etablie_valeur(const std::string &valeur) override;

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
	float *m_couleur = nullptr;

public:
	explicit SelecteurCouleur(QWidget *parent = nullptr);

	~SelecteurCouleur() = default;

	void etablie_attache(void *pointeur) override;

	void etablie_valeur(const std::string &valeur) override;

	void etablie_valeur_min(const std::string &valeur) override;

	void etablie_valeur_max(const std::string &valeur) override;

	void mouseReleaseEvent(QMouseEvent *e) override;

	void paintEvent(QPaintEvent *) override;

Q_SIGNALS:
	void clicked();
	void valeur_changee(double value, int axis);
};

}  /* namespace kangao */
