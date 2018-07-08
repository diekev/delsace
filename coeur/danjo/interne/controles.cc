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

#include <sstream>

#include <QHBoxLayout>

#include "donnees_controle.h"
#include "morceaux.h"

namespace danjo {

/* Il s'emblerait que std::atof a du mal à convertir les string en float. */
template <typename T>
T convertie(const std::string &valeur)
{
	std::istringstream ss(valeur);
	T result;

	ss >> result;

	return result;
}

/* ************************************************************************** */

ControleFloat::ControleFloat(QWidget *parent)
	: SelecteurFloat(parent)
	, m_pointeur(nullptr)
{
	valeur(0.0);
	connect(this, &SelecteurFloat::valeur_changee, this, &ControleFloat::ajourne_valeur_pointee);
}

void ControleFloat::ajourne_valeur_pointee(double valeur)
{
	*m_pointeur = static_cast<float>(valeur);
	Q_EMIT(controle_change());
}

void ControleFloat::finalise(const DonneesControle &donnees)
{
	m_min = convertie<float>(donnees.valeur_min);
	m_max = convertie<float>(donnees.valeur_max);

	setRange(m_min, m_max);

	m_pointeur = static_cast<float *>(donnees.pointeur);

	const auto valeur_defaut = convertie<float>(donnees.valeur_defaut);

	if (donnees.initialisation) {
		*m_pointeur = valeur_defaut;
	}

	valeur(*m_pointeur);

	setToolTip(donnees.infobulle.c_str());
}

/* ************************************************************************** */

ControleInt::ControleInt(QWidget *parent)
	: SelecteurInt(parent)
	, m_pointeur(nullptr)
{
	connect(this, &SelecteurInt::valeur_changee, this, &ControleInt::ajourne_valeur_pointee);
}

void ControleInt::finalise(const DonneesControle &donnees)
{
	m_min = std::atoi(donnees.valeur_min.c_str());
	m_max = std::atoi(donnees.valeur_max.c_str());

	setRange(m_min, m_max);

	m_pointeur = static_cast<int *>(donnees.pointeur);

	const auto valeur_defaut = std::atoi(donnees.valeur_defaut.c_str());

	if (donnees.initialisation) {
		*m_pointeur = valeur_defaut;
	}

	setValue(*m_pointeur);

	setToolTip(donnees.infobulle.c_str());
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

void ControleBool::finalise(const DonneesControle &donnees)
{
	const auto vieil_etat = m_case_a_cocher->blockSignals(true);

	m_pointeur = static_cast<bool *>(donnees.pointeur);

	const auto valeur_defaut = (donnees.valeur_defaut == "vrai");

	if (donnees.initialisation) {
		*m_pointeur = valeur_defaut;
	}

	m_case_a_cocher->setChecked(*m_pointeur);

	setToolTip(donnees.infobulle.c_str());

	m_case_a_cocher->blockSignals(vieil_etat);
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

void ControleEnum::ajourne_valeur_pointee(int /*valeur*/)
{
	*m_pointeur = m_liste_deroulante->currentData().toString().toStdString();
	Q_EMIT(controle_change());
}

void ControleEnum::finalise(const DonneesControle &donnees)
{
	m_pointeur = static_cast<std::string *>(donnees.pointeur);

	auto valeur_defaut = donnees.valeur_defaut;

	if (donnees.initialisation) {
		*m_pointeur = valeur_defaut;
	}
	else {
		valeur_defaut = *m_pointeur;
	}

	const auto vieil_etat = m_liste_deroulante->blockSignals(true);

	auto index_courant = 0;

	for (const auto &pair : donnees.valeur_enum) {
		m_liste_deroulante->addItem(pair.first.c_str(),
									QVariant(pair.second.c_str()));

		if (pair.second == valeur_defaut) {
			m_index_valeur_defaut = index_courant;
		}

		index_courant++;
	}

	m_liste_deroulante->setCurrentIndex(m_index_valeur_defaut);

	m_liste_deroulante->blockSignals(vieil_etat);

	setToolTip(donnees.infobulle.c_str());
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

void ControleChaineCaractere::finalise(const DonneesControle &donnees)
{
	m_pointeur = static_cast<std::string *>(donnees.pointeur);

	if (donnees.initialisation) {
		*m_pointeur = donnees.valeur_defaut;
	}

	m_editeur_ligne->setText(m_pointeur->c_str());

	setToolTip(donnees.infobulle.c_str());
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

void ControleVec3::finalise(const DonneesControle &donnees)
{
	m_pointeur = static_cast<float *>(donnees.pointeur);

	auto valeurs = decoupe(donnees.valeur_defaut, ',');
	auto index = 0;

	float valeur_defaut[3];

	for (auto v : valeurs) {
		valeur_defaut[index++] = std::atof(v.c_str());
	}

	if (donnees.initialisation) {
		m_pointeur[0] = valeur_defaut[0];
		m_pointeur[1] = valeur_defaut[1];
		m_pointeur[2] = valeur_defaut[2];
	}

	setValue(m_pointeur);

	setToolTip(donnees.infobulle.c_str());
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

void ControleFichier::finalise(const DonneesControle &donnees)
{
	m_pointeur = static_cast<std::string *>(donnees.pointeur);

	if (donnees.initialisation) {
		*m_pointeur = donnees.valeur_defaut;
	}

	setValue(m_pointeur->c_str());

	setToolTip(donnees.infobulle.c_str());
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

void ControleListe::finalise(const DonneesControle &donnees)
{
	m_pointeur = static_cast<std::string *>(donnees.pointeur);

	if (donnees.initialisation) {
		*m_pointeur = donnees.valeur_defaut;
	}

	setValue(m_pointeur->c_str());

	setToolTip(donnees.infobulle.c_str());
}

void ControleListe::ajourne_valeur_pointee(const QString &valeur)
{
	*m_pointeur = valeur.toStdString();
	Q_EMIT(controle_change());
}

}  /* namespace danjo */
