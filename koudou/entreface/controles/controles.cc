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

#include "controles.h"

/* ************************************************************************** */

ControleFloat::ControleFloat(QWidget *parent)
    : SelecteurFloat(parent)
	, m_pointeur(nullptr)
{
	setValue(0.0);
	connect(this, &SelecteurFloat::valeur_changee, this, &ControleFloat::ajourne_valeur_pointee);
}

void ControleFloat::pointeur(float *pointeur)
{
	m_pointeur = pointeur;
	setValue(*pointeur);
}

void ControleFloat::ajourne_valeur_pointee(double valeur)
{
	*m_pointeur = static_cast<float>(valeur);
	Q_EMIT(controle_change());
}

/* ************************************************************************** */

ControleInt::ControleInt(QWidget *parent)
    : SelecteurInt(parent)
	, m_pointeur(nullptr)
{
	setValue(0);
	connect(this, &SelecteurInt::valeur_changee, this, &ControleInt::ajourne_valeur_pointee);
}

void ControleInt::pointeur(int *pointeur)
{
	m_pointeur = pointeur;
	setValue(*pointeur);
}

void ControleInt::ajourne_valeur_pointee(int valeur)
{
	*m_pointeur = valeur;
	Q_EMIT(controle_change());
}

/* ************************************************************************** */

ControleBool::ControleBool(QWidget *parent)
    : QCheckBox(parent)
	, m_pointeur(nullptr)
{
	setChecked(false);
	connect(this, &QAbstractButton::toggled, this, &ControleBool::ajourne_valeur_pointee);
}

void ControleBool::pointeur(bool *pointeur)
{
	m_pointeur = pointeur;
	setChecked(*pointeur);
}

void ControleBool::ajourne_valeur_pointee(bool valeur)
{
	*m_pointeur = valeur;
	Q_EMIT(controle_change());
}

/* ************************************************************************** */

ControleEnum::ControleEnum(QWidget *parent)
    : QComboBox(parent)
	, m_pointeur(nullptr)
{
	/* On ne peut pas utilisé le nouveau style de connexion de Qt5 :
	 *     connect(this, &QComboBox::currentIndexChanged,
	 *             this, &ControleEnum::ajourne_valeur_pointee);
	 * car currentIndexChanged a deux signatures possibles et le compileur
	 * ne sait pas laquelle choisir. */
	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(ajourne_valeur_pointee(int)));
}

void ControleEnum::pointeur(int *pointeur)
{
	m_pointeur = pointeur;
}

void ControleEnum::ajourne_valeur_pointee(int /*valeur*/)
{
	*m_pointeur = this->currentData().toInt();
	Q_EMIT(controle_change());
}

/* ************************************************************************** */

ControleChaineCaractere::ControleChaineCaractere(QWidget *parent)
    : QLineEdit(parent)
	, m_pointeur(nullptr)
{
	connect(this, &QLineEdit::returnPressed, this, &ControleChaineCaractere::ajourne_valeur_pointee);
}

void ControleChaineCaractere::pointeur(std::string *pointeur)
{
	m_pointeur = pointeur;
}

void ControleChaineCaractere::ajourne_valeur_pointee()
{
	*m_pointeur = this->text().toStdString();
	Q_EMIT(controle_change());
}

/* ************************************************************************** */

ControleVec3::ControleVec3(QWidget *parent)
    : SelecteurVec3(parent)
	, m_pointeur(nullptr)
{
	connect(this, &SelecteurVec3::valeur_changee, this, &ControleVec3::ajourne_valeur_pointee);
}

void ControleVec3::pointeur(float pointeur[3])
{
	m_pointeur = pointeur;
	setValue(pointeur);
}

void ControleVec3::ajourne_valeur_pointee(double valeur, int axis)
{
	m_pointeur[axis] = static_cast<float>(valeur);
	Q_EMIT(controle_change());
}

/* ************************************************************************** */

ControleCouleur::ControleCouleur(QWidget *parent)
    : SelecteurCouleur(parent)
	, m_pointeur(nullptr)
{
	connect(this, &SelecteurCouleur::valeur_changee, this, &ControleCouleur::ajourne_valeur_pointee);
}

void ControleCouleur::pointeur(float pointeur[4])
{
	m_pointeur = pointeur;
	setValue(pointeur);
}

void ControleCouleur::ajourne_valeur_pointee(double valeur, int axis)
{
	m_pointeur[axis] = static_cast<float>(valeur);
	Q_EMIT(controle_change());
}

/* ************************************************************************** */

ControleFichier::ControleFichier(bool input, QWidget *parent)
    : SelecteurFichier(input, parent)
	, m_pointeur(nullptr)
{
	connect(this, &SelecteurFichier::valeur_changee, this, &ControleFichier::ajourne_valeur_pointee);
}

void ControleFichier::pointeur(std::string *pointeur)
{
	m_pointeur = pointeur;
}

void ControleFichier::ajourne_valeur_pointee(const QString &valeur)
{
	*m_pointeur = valeur.toStdString();
	Q_EMIT(controle_change());
}

/* ************************************************************************** */

ControleListe::ControleListe(QWidget *parent)
    : SelecteurListe(parent)
{
	connect(this, &SelecteurListe::valeur_changee, this, &ControleListe::ajourne_valeur_pointee);
}

void ControleListe::pointeur(std::string *pointeur)
{
	m_pointeur = pointeur;
}

void ControleListe::ajourne_valeur_pointee(const QString &valeur)
{
	*m_pointeur = valeur.toStdString();
	Q_EMIT(controle_change());
}
