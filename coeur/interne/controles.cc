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

#include "controles.h"

#include <QHBoxLayout>

namespace kangao {

/* ************************************************************************** */

ControleFloat::ControleFloat(QWidget *parent)
	: SelecteurFloat(parent)
	, m_pointeur(nullptr)
{
	valeur(0.0);
	connect(this, &SelecteurFloat::valeur_changee, this, &ControleFloat::ajourne_valeur_pointee);
}

void ControleFloat::etablie_attache(void *pointeur)
{
	m_pointeur = static_cast<float *>(pointeur);
	valeur(*m_pointeur);
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

void ControleInt::etablie_attache(void *pointeur)
{
	m_pointeur = static_cast<int *>(pointeur);
	setValue(*m_pointeur);
}

void ControleInt::ajourne_valeur_pointee(int valeur)
{
	*m_pointeur = valeur;
	Q_EMIT(controle_change());
}

/* ************************************************************************** */

ControleBool::ControleBool(QWidget *parent)
	: Controle(parent)
	, m_agencement(new QHBoxLayout)
	, m_case_a_cocher(new QCheckBox(this))
	, m_pointeur(nullptr)
{
	m_agencement->addWidget(m_case_a_cocher);
	m_case_a_cocher->setChecked(false);

	this->setLayout(m_agencement);

	connect(m_case_a_cocher, &QAbstractButton::toggled, this, &ControleBool::ajourne_valeur_pointee);
}

void ControleBool::etablie_valeur(const std::string &valeur)
{
	const auto vieil_etat = m_case_a_cocher->blockSignals(true);
	m_case_a_cocher->setChecked(valeur == "vrai");
	m_case_a_cocher->blockSignals(vieil_etat);
}

void ControleBool::etablie_attache(void *pointeur)
{
	m_pointeur = static_cast<bool *>(pointeur);
	m_case_a_cocher->setChecked(*m_pointeur);
}

void ControleBool::ajourne_valeur_pointee(bool valeur)
{
	*m_pointeur = valeur;
	Q_EMIT(controle_change());
}

/* ************************************************************************** */

ControleEnum::ControleEnum(QWidget *parent)
	: Controle(parent)
	, m_agencement(new QHBoxLayout)
	, m_liste_deroulante(new QComboBox(this))
	, m_pointeur(nullptr)
	, m_index_valeur_defaut(0)
	, m_index_courant(0)
{
	m_agencement->addWidget(m_liste_deroulante);
	this->setLayout(m_agencement);

	/* On ne peut pas utilisé le nouveau style de connection de Qt5 :
	 *     connect(m_liste_deroulante, &QComboBox::currentIndexChanged,
	 *             this, &ControleEnum::ajourne_valeur_pointee);
	 * car currentIndexChanged a deux signatures possibles et le compileur
	 * ne sait pas laquelle choisir. */
	connect(m_liste_deroulante, SIGNAL(currentIndexChanged(int)),
			this, SLOT(ajourne_valeur_pointee(int)));
}

void ControleEnum::etablie_valeur(const std::string &valeur)
{
	m_valeur_defaut = valeur;
}

void ControleEnum::etablie_attache(void *pointeur)
{
	m_pointeur = static_cast<std::string *>(pointeur);
}

void ControleEnum::ajoute_item(const std::string &nom, const std::string &valeur)
{
	const auto vieil_etat = m_liste_deroulante->blockSignals(true);
	m_liste_deroulante->addItem(nom.c_str(), QVariant(valeur.c_str()));
	m_liste_deroulante->blockSignals(vieil_etat);

	if (valeur == m_valeur_defaut) {
		m_index_valeur_defaut = m_index_courant;
	}

	++m_index_courant;
}

void ControleEnum::ajourne_valeur_pointee(int /*valeur*/)
{
	*m_pointeur = m_liste_deroulante->currentData().toString().toStdString();
	Q_EMIT(controle_change());
}

void ControleEnum::finalise()
{
	m_liste_deroulante->setCurrentIndex(m_index_valeur_defaut);
}

/* ************************************************************************** */

ControleChaineCaractere::ControleChaineCaractere(QWidget *parent)
	: Controle(parent)
	, m_agencement(new QHBoxLayout)
	, m_editeur_ligne(new QLineEdit(this))
	, m_pointeur(nullptr)
{
	m_agencement->addWidget(m_editeur_ligne);
	this->setLayout(m_agencement);

	connect(m_editeur_ligne, &QLineEdit::returnPressed,
			this, &ControleChaineCaractere::ajourne_valeur_pointee);
}

void ControleChaineCaractere::etablie_valeur(const std::string &valeur)
{
	m_editeur_ligne->setText(valeur.c_str());
}

void ControleChaineCaractere::etablie_attache(void *pointeur)
{
	m_pointeur = static_cast<std::string *>(pointeur);
}

void ControleChaineCaractere::ajourne_valeur_pointee()
{
	*m_pointeur = m_editeur_ligne->text().toStdString();
	Q_EMIT(controle_change());
}

/* ************************************************************************** */

ControleVec3::ControleVec3(QWidget *parent)
	: SelecteurVec3(parent)
	, m_pointeur(nullptr)
{
	connect(this, &SelecteurVec3::valeur_changee, this, &ControleVec3::ajourne_valeur_pointee);
}

void ControleVec3::etablie_attache(void *pointeur)
{
	m_pointeur = static_cast<float *>(pointeur);
	setValue(m_pointeur);
}

void ControleVec3::ajourne_valeur_pointee(double valeur, int axis)
{
	m_pointeur[axis] = static_cast<float>(valeur);
	Q_EMIT(controle_change());
}

/* ************************************************************************** */

ControleCouleur::ControleCouleur(QWidget *parent)
	: SelecteurCouleur(parent)
{
	connect(this, &SelecteurCouleur::valeur_changee, this, &ControleCouleur::ajourne_valeur_pointee);
}

void ControleCouleur::ajourne_valeur_pointee(double valeur, int axis)
{
	m_couleur[axis] = static_cast<float>(valeur);
	Q_EMIT(controle_change());
}

/* ************************************************************************** */

ControleFichier::ControleFichier(bool input, QWidget *parent)
	: SelecteurFichier(input, parent)
	, m_pointeur(nullptr)
{
	connect(this, &SelecteurFichier::valeur_changee, this, &ControleFichier::ajourne_valeur_pointee);
}

void ControleFichier::etablie_attache(void *pointeur)
{
	m_pointeur = static_cast<std::string *>(pointeur);
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

void ControleListe::etablie_attache(void *pointeur)
{
	m_pointeur = static_cast<std::string *>(pointeur);
}

void ControleListe::ajourne_valeur_pointee(const QString &valeur)
{
	*m_pointeur = valeur.toStdString();
	Q_EMIT(controle_change());
}

}  /* namespace kangao */
