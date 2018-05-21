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

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QWidget>

#include "base_controles.h"

class QGridLayout;

namespace danjo {

/* Les contrôles sont des sousclasses de leur QWidgets ou contrôle personnalisé
 * correspondants. Pour éviter des APIs étranges, ils stockent un pointeur vers
 * la valeur externe à laquelle ils sont 'connectés'.
 */

/* ************************************************************************** */

class ControleFloat final : public SelecteurFloat {
	Q_OBJECT

	float *m_pointeur;

public:
	explicit ControleFloat(QWidget *parent = nullptr);
	~ControleFloat() = default;

	void finalise(const DonneesControle &donnees) override;

private Q_SLOTS:
	void ajourne_valeur_pointee(double valeur);
};

/* ************************************************************************** */

class ControleInt final : public SelecteurInt {
	Q_OBJECT

	int *m_pointeur;

public:
	explicit ControleInt(QWidget *parent = nullptr);
	~ControleInt() = default;

	void finalise(const DonneesControle &donnees) override;

private Q_SLOTS:
	void ajourne_valeur_pointee(int valeur);
};

/* ************************************************************************** */

class ControleBool final : public Controle {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	QCheckBox *m_case_a_cocher;

	bool *m_pointeur;

public:
	explicit ControleBool(QWidget *parent = nullptr);
	~ControleBool() = default;

	void finalise(const DonneesControle &donnees) override;

private Q_SLOTS:
	void ajourne_valeur_pointee(bool valeur);
};

/* ************************************************************************** */

class ControleEnum final : public Controle {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	QComboBox *m_liste_deroulante;

	std::string *m_pointeur;
	std::string m_valeur_defaut;
	int m_index_valeur_defaut;
	int m_index_courant;

public:
	explicit ControleEnum(QWidget *parent = nullptr);
	~ControleEnum() = default;

	void finalise(const DonneesControle &donnees) override;

private Q_SLOTS:
	void ajourne_valeur_pointee(int valeur);
};

/* ************************************************************************** */

class ControleChaineCaractere final : public Controle {
	Q_OBJECT

	QHBoxLayout *m_agencement;
	QLineEdit *m_editeur_ligne;

	std::string *m_pointeur;

public:
	explicit ControleChaineCaractere(QWidget *parent = nullptr);
	~ControleChaineCaractere() = default;

	void finalise(const DonneesControle &donnees) override;

private Q_SLOTS:
	void ajourne_valeur_pointee();
};

/* ************************************************************************** */

class ControleVec3 final : public SelecteurVec3 {
	Q_OBJECT

	float *m_pointeur;

public:
	explicit ControleVec3(QWidget *parent = nullptr);
	~ControleVec3() = default;

	void finalise(const DonneesControle &donnees) override;

private Q_SLOTS:
	void ajourne_valeur_pointee(double valeur, int axis);
};

/* ************************************************************************** */

class ControleCouleur final : public SelecteurCouleur {
	Q_OBJECT

	float *m_pointeur;

public:
	explicit ControleCouleur(QWidget *parent = nullptr);
	~ControleCouleur() = default;

private Q_SLOTS:
	void ajourne_valeur_pointee(double valeur, int axis);
};

/* ************************************************************************** */

class ControleFichier final : public SelecteurFichier {
	Q_OBJECT

	std::string *m_pointeur;

public:
	explicit ControleFichier(bool input, QWidget *parent = nullptr);
	~ControleFichier() = default;

	void finalise(const DonneesControle &donnees) override;

private Q_SLOTS:
	void ajourne_valeur_pointee(const QString &valeur);
};

/* ************************************************************************** */

class ControleListe final : public SelecteurListe {
	Q_OBJECT

	std::string *m_pointeur;

public:
	explicit ControleListe(QWidget *parent = nullptr);

	~ControleListe() = default;

	void finalise(const DonneesControle &donnees) override;

private Q_SLOTS:
	void ajourne_valeur_pointee(const QString &valeur);
};

}  /* namespace danjo */
