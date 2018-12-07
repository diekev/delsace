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
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QWidget>
#pragma GCC diagnostic pop

#include "base_controles.h"

class QGridLayout;

/* Les contrôles sont des sousclasses de leur QWidgets ou contrôle personnalisé
 * correspondants. Pour éviter des APIs étranges, ils stockent un pointeur vers
 * la valeur externe à laquelle ils sont 'connectés'.
 */

/* ************************************************************************** */

class ControleFloat final : public SelecteurFloat {
	Q_OBJECT

	float *m_pointeur{};

public:
	explicit ControleFloat(QWidget *parent = nullptr);
	~ControleFloat() = default;

	ControleFloat(ControleFloat const &) = default;
	ControleFloat &operator=(ControleFloat const &) = default;

	void pointeur(float *pointeur);

private Q_SLOTS:
	void ajourne_valeur_pointee(double valeur);

Q_SIGNALS:
	void controle_change();
};

/* ************************************************************************** */

class ControleInt final : public SelecteurInt {
	Q_OBJECT

	int *m_pointeur{};

public:
	explicit ControleInt(QWidget *parent = nullptr);
	~ControleInt() = default;

	ControleInt(ControleInt const &) = default;
	ControleInt &operator=(ControleInt const &) = default;

	void pointeur(int *pointeur);

private Q_SLOTS:
	void ajourne_valeur_pointee(int valeur);

Q_SIGNALS:
	void controle_change();
};

/* ************************************************************************** */

class ControleBool final : public QCheckBox {
	Q_OBJECT

	bool *m_pointeur{};

public:
	explicit ControleBool(QWidget *parent = nullptr);
	~ControleBool() = default;

	ControleBool(ControleBool const &) = default;
	ControleBool &operator=(ControleBool const &) = default;

	void pointeur(bool *pointeur);

private Q_SLOTS:
	void ajourne_valeur_pointee(bool valeur);

Q_SIGNALS:
	void controle_change();
};

/* ************************************************************************** */

class ControleEnum final : public QComboBox {
	Q_OBJECT

	int *m_pointeur{};

public:
	explicit ControleEnum(QWidget *parent = nullptr);
	~ControleEnum() = default;

	ControleEnum(ControleEnum const &) = default;
	ControleEnum &operator=(ControleEnum const &) = default;

	void pointeur(int *pointeur);

private Q_SLOTS:
	void ajourne_valeur_pointee(int valeur);

Q_SIGNALS:
	void controle_change();
};

/* ************************************************************************** */

class ControleChaineCaractere final : public QLineEdit {
	Q_OBJECT

	std::string *m_pointeur{};

public:
	explicit ControleChaineCaractere(QWidget *parent = nullptr);
	~ControleChaineCaractere() = default;

	ControleChaineCaractere(ControleChaineCaractere const &) = default;
	ControleChaineCaractere &operator=(ControleChaineCaractere const &) = default;

	void pointeur(std::string *pointeur);

private Q_SLOTS:
	void ajourne_valeur_pointee();

Q_SIGNALS:
	void controle_change();
};

/* ************************************************************************** */

class ControleVec3 final : public SelecteurVec3 {
	Q_OBJECT

	float *m_pointeur{};

public:
	explicit ControleVec3(QWidget *parent = nullptr);
	~ControleVec3() = default;

	ControleVec3(ControleVec3 const &) = default;
	ControleVec3 &operator=(ControleVec3 const &) = default;

	void pointeur(float pointeur[3]);

private Q_SLOTS:
	void ajourne_valeur_pointee(double valeur, int axis);

Q_SIGNALS:
	void controle_change();
};

/* ************************************************************************** */

class ControleCouleur final : public SelecteurCouleur {
	Q_OBJECT

	float *m_pointeur{};

public:
	explicit ControleCouleur(QWidget *parent = nullptr);
	~ControleCouleur() = default;

	ControleCouleur(ControleCouleur const &) = default;
	ControleCouleur &operator=(ControleCouleur const &) = default;

	void pointeur(float pointeur[4]);

private Q_SLOTS:
	void ajourne_valeur_pointee(double valeur, int axis);

Q_SIGNALS:
	void controle_change();
};

/* ************************************************************************** */

class ControleFichier final : public SelecteurFichier {
	Q_OBJECT

	std::string *m_pointeur{};

public:
	explicit ControleFichier(bool input, QWidget *parent = nullptr);
	~ControleFichier() = default;

	ControleFichier(ControleFichier const &) = default;
	ControleFichier &operator=(ControleFichier const &) = default;

	void pointeur(std::string *pointeur);

private Q_SLOTS:
	void ajourne_valeur_pointee(const QString &valeur);

Q_SIGNALS:
	void controle_change();
};

/* ************************************************************************** */

class ControleListe final : public SelecteurListe {
	Q_OBJECT

	std::string *m_pointeur{};

public:
	explicit ControleListe(QWidget *parent = nullptr);

	~ControleListe() = default;

	ControleListe(ControleListe const &) = default;
	ControleListe &operator=(ControleListe const &) = default;

	void pointeur(std::string *pointeur);

private Q_SLOTS:
	void ajourne_valeur_pointee(const QString &valeur);

Q_SIGNALS:
	void controle_change();
};
