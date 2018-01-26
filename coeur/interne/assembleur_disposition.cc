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

#include "assembleur_disposition.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QVBoxLayout>

#include "action.h"
#include "bouton.h"
#include "conteneur_controles.h"
#include "controles.h"
#include "manipulable.h"
#include "morceaux.h"
#include "repondant_bouton.h"

namespace kangao {

AssembleurDisposition::AssembleurDisposition(Manipulable *manipulable, RepondantBouton *repondant_bouton, ConteneurControles *conteneur)
	: m_manipulable(manipulable)
	, m_repondant(repondant_bouton)
	, m_conteneur(conteneur)
{}

void AssembleurDisposition::ajoute_disposition(int identifiant)
{
	QBoxLayout *disposition = nullptr;

	switch (identifiant) {
		default:
		case IDENTIFIANT_LIGNE:
			disposition = new QHBoxLayout;
			break;
		case IDENTIFIANT_COLONNE:
			disposition = new QVBoxLayout;
			break;
	}

	if (!m_pile_dispositions.empty()) {
		m_pile_dispositions.top()->addLayout(disposition);
	}

	m_pile_dispositions.push(disposition);
}

void AssembleurDisposition::ajoute_controle(int identifiant)
{
	Controle *controle = nullptr;

	switch (identifiant) {
		case IDENTIFIANT_CONTROLE_CURSEUR:
			controle = new ControleInt;
			break;
		case IDENTIFIANT_CONTROLE_CURSEUR_DECIMAL:
			controle = new ControleFloat;
			break;
		case IDENTIFIANT_CONTROLE_ETIQUETTE:
			controle = new Etiquette;
			break;
		case IDENTIFIANT_CONTROLE_LISTE:
			controle = new ControleEnum;
			break;
		case IDENTIFIANT_CONTROLE_CASE_COCHER:
			controle = new ControleBool;
			break;
		case IDENTIFIANT_CONTROLE_CHAINE:
			controle = new ControleChaineCaractere;
			break;
		case IDENTIFIANT_CONTROLE_FICHIER_ENTREE:
			controle = new ControleFichier(true);
			break;
		case IDENTIFIANT_CONTROLE_FICHIER_SORTIE:
			controle = new ControleFichier(false);
			break;
		case IDENTIFIANT_CONTROLE_COULEUR:
			controle = new ControleCouleur;
			break;
		case IDENTIFIANT_CONTROLE_VECTEUR:
			controle = new ControleVec3;
			break;
	}

	m_dernier_controle = controle;
	m_pile_dispositions.top()->addWidget(controle);
}

void AssembleurDisposition::ajoute_item_liste(const std::string &nom, const std::string &valeur)
{
	auto controle = dynamic_cast<ControleEnum *>(m_dernier_controle);

	controle->ajoute_item(nom, valeur);
}

void AssembleurDisposition::ajoute_bouton()
{
	Bouton *bouton = new Bouton;
	bouton->installe_repondant(m_repondant);

	m_dernier_bouton = bouton;
	m_pile_dispositions.top()->addWidget(bouton);
}

void AssembleurDisposition::finalise_controle()
{
	m_dernier_controle->finalise();
	QObject::connect(m_dernier_controle, &Controle::controle_change,
					 m_conteneur, &ConteneurControles::ajourne_manipulable);
}

void AssembleurDisposition::sort_disposition()
{
	m_pile_dispositions.pop();
}

void AssembleurDisposition::propriete_controle(int identifiant, const std::string &valeur)
{
	switch (identifiant) {
		case IDENTIFIANT_PROPRIETE_INFOBULLE:
			m_dernier_controle->etablie_infobulle(valeur);
			break;
		case IDENTIFIANT_PROPRIETE_MIN:
			m_dernier_controle->etablie_valeur_min(valeur);
			break;
		case IDENTIFIANT_PROPRIETE_MAX:
			m_dernier_controle->etablie_valeur_max(valeur);
			break;
		case IDENTIFIANT_PROPRIETE_VALEUR:
			m_dernier_controle->etablie_valeur(valeur);
			break;
		case IDENTIFIANT_PROPRIETE_ATTACHE:
			m_dernier_controle->etablie_attache((*m_manipulable)[valeur]);
			break;
		case IDENTIFIANT_PROPRIETE_PRECISION:
			m_dernier_controle->etablie_precision(valeur);
			break;
		case IDENTIFIANT_PROPRIETE_PAS:
			m_dernier_controle->etablie_pas(valeur);
			break;
	}
}

void AssembleurDisposition::propriete_bouton(int identifiant, const std::string &valeur)
{
	switch (identifiant) {
		case IDENTIFIANT_PROPRIETE_INFOBULLE:
			m_dernier_bouton->etablie_infobulle(valeur);
			break;
		case IDENTIFIANT_PROPRIETE_VALEUR:
			m_dernier_bouton->etablie_valeur(valeur);
			break;
		case IDENTIFIANT_PROPRIETE_ATTACHE:
			m_dernier_bouton->etablie_attache(valeur);
			break;
		case IDENTIFIANT_PROPRIETE_METADONNEE:
			m_dernier_bouton->etablie_metadonnee(valeur);
			break;
		case IDENTIFIANT_PROPRIETE_ICONE:
			m_dernier_bouton->etablie_icone(valeur);
			break;
	}
}

void AssembleurDisposition::propriete_action(int identifiant, const std::string &valeur)
{
	switch (identifiant) {
		case IDENTIFIANT_PROPRIETE_INFOBULLE:
			m_derniere_action->etablie_infobulle(valeur.c_str());
			break;
		case IDENTIFIANT_PROPRIETE_VALEUR:
			m_derniere_action->etablie_valeur(valeur.c_str());
			break;
		case IDENTIFIANT_PROPRIETE_ATTACHE:
			m_derniere_action->etablie_attache(valeur);
			break;
		case IDENTIFIANT_PROPRIETE_METADONNEE:
			m_derniere_action->etablie_metadonnee(valeur);
			break;
		case IDENTIFIANT_PROPRIETE_ICONE:
			m_derniere_action->etablie_icone(valeur);
			break;
	}
}

QBoxLayout *AssembleurDisposition::disposition()
{
	if (m_pile_dispositions.empty()) {
		return nullptr;
	}

	return m_pile_dispositions.top();
}

QMenu *AssembleurDisposition::menu()
{
	return m_menu_racine;
}

void AssembleurDisposition::ajoute_menu(const std::string &nom)
{
	auto menu = new QMenu(nom.c_str());

	if (!m_pile_menus.empty()) {
		m_pile_menus.top()->addMenu(menu);
	}
	else {
		m_menu_racine = menu;
	}

	m_pile_menus.push(menu);
}

void AssembleurDisposition::sort_menu()
{
	m_pile_menus.pop();
}

void AssembleurDisposition::ajoute_action()
{
	auto action = new Action;
	action->installe_repondant(m_repondant);

	m_pile_menus.top()->addAction(action);

	m_derniere_action = action;
}

void AssembleurDisposition::ajoute_separateur()
{
	m_pile_menus.top()->addSeparator();
}

void AssembleurDisposition::ajoute_dossier()
{
	QTabWidget *dossier = new QTabWidget;

	m_pile_dispositions.top()->addWidget(dossier);

	m_dernier_dossier = dossier;
}

void AssembleurDisposition::finalise_dossier()
{
	m_dernier_dossier = nullptr;
}

void AssembleurDisposition::ajoute_onglet(const std::string &nom)
{
	/* À FAIRE : expose contrôle de la direction. */
	auto disp_onglet = new QVBoxLayout;

	auto onglet = new QWidget;
	onglet->setLayout(disp_onglet);

	m_pile_dispositions.push(disp_onglet);

	m_dernier_dossier->addTab(onglet, nom.c_str());
}

void AssembleurDisposition::finalise_onglet()
{
	m_pile_dispositions.pop();
}

}  /* namespace kangao */
