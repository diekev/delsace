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

#include "assembleuse_disposition.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>

#include "controles/action.h"
#include "controles/bouton.h"

#include "controles_proprietes/controle_propriete_bool.h"
#include "controles_proprietes/controle_propriete_chaine.h"
#include "controles_proprietes/controle_propriete_couleur.h"
#include "controles_proprietes/controle_propriete_decimal.h"
#include "controles_proprietes/controle_propriete_entier.h"
#include "controles_proprietes/controle_propriete_etiquette.h"
#include "controles_proprietes/controle_propriete_enum.h"
#include "controles_proprietes/controle_propriete_fichier.h"
#include "controles_proprietes/controle_propriete_liste.h"
#include "controles_proprietes/controle_propriete_vecteur.h"

#include "conteneur_controles.h"
#include "manipulable.h"
#include "morceaux.h"
#include "repondant_bouton.h"

namespace danjo {

AssembleurDisposition::AssembleurDisposition(Manipulable *manipulable, RepondantBouton *repondant_bouton, ConteneurControles *conteneur, int temps)
	: m_manipulable(manipulable)
	, m_repondant(repondant_bouton)
	, m_conteneur(conteneur)
	, m_temps(temps)
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
	ControlePropriete *controle = nullptr;

	m_donnees_controle = DonneesControle();

	switch (identifiant) {
		case IDENTIFIANT_ENTIER:
			controle = new ControleProprieteEntier;
			m_donnees_controle.type = TypePropriete::ENTIER;
			break;
		case IDENTIFIANT_DECIMAL:
			controle = new ControleProprieteDecimal;
			m_donnees_controle.type = TypePropriete::DECIMAL;
			break;
		case IDENTIFIANT_ETIQUETTE:
			controle = new ControleProprieteEtiquette;
			break;
		case IDENTIFIANT_ENUM:
			controle = new ControleProprieteEnum;
			m_donnees_controle.type = TypePropriete::ENUM;
			break;
		case IDENTIFIANT_LISTE:
		{
			auto controle_liste = new ControleProprieteListe;
			controle_liste->conteneur(m_conteneur);
			controle = controle_liste;
			m_donnees_controle.type = TypePropriete::CHAINE_CARACTERE;
			break;
		}
		case IDENTIFIANT_CASE:
			controle = new ControleProprieteBool;
			m_donnees_controle.type = TypePropriete::BOOL;
			break;
		case IDENTIFIANT_CHAINE:
			controle = new ControleProprieteChaineCaractere;
			m_donnees_controle.type = TypePropriete::CHAINE_CARACTERE;
			break;
		case IDENTIFIANT_FICHIER_ENTREE:
			controle = new ControleProprieteFichier(true);
			m_donnees_controle.type = TypePropriete::FICHIER_ENTREE;
			break;
		case IDENTIFIANT_FICHIER_SORTIE:
			controle = new ControleProprieteFichier(false);
			m_donnees_controle.type = TypePropriete::FICHIER_SORTIE;
			break;
		case IDENTIFIANT_COULEUR:
			controle = new ControleProprieteCouleur;
			m_donnees_controle.type = TypePropriete::COULEUR;
			break;
		case IDENTIFIANT_VECTEUR:
			controle = new ControleProprieteVec3;
			m_donnees_controle.type = TypePropriete::VECTEUR;
			break;
	}

	m_donnees_controle.initialisation = !m_manipulable->est_initialise();

	m_dernier_controle = controle;
	m_pile_dispositions.top()->addWidget(controle);
}

void AssembleurDisposition::ajoute_item_liste(const std::string &nom, const std::string &valeur)
{
	m_donnees_controle.valeur_enum.push_back({nom, valeur});
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
	if (m_donnees_controle.pointeur == nullptr && m_donnees_controle.nom != "") {
		/* Ajoute la propriété au manipulable. */
		m_manipulable->ajoute_propriete(
					m_donnees_controle.nom, m_donnees_controle.type);

		m_donnees_controle.pointeur = (*m_manipulable)[m_donnees_controle.nom];
		assert(m_donnees_controle.pointeur != nullptr);
		m_donnees_controle.initialisation = true;
	}
	else {
		m_donnees_controle.initialisation = false;
	}

	m_dernier_controle->proriete(m_manipulable->propriete(m_donnees_controle.nom));
	m_dernier_controle->temps(m_temps);
	m_dernier_controle->finalise(m_donnees_controle);

	if (m_conteneur != nullptr) {
		QObject::connect(m_dernier_controle, &ControlePropriete::controle_change,
						 m_conteneur, &ConteneurControles::ajourne_manipulable);
	}
}

void AssembleurDisposition::sort_disposition()
{
	m_pile_dispositions.pop();
}

void AssembleurDisposition::propriete_controle(int identifiant, const std::string &valeur)
{
	switch (identifiant) {
		case IDENTIFIANT_INFOBULLE:
			m_donnees_controle.infobulle = valeur;
			break;
		case IDENTIFIANT_MIN:
			m_donnees_controle.valeur_min = valeur;
			break;
		case IDENTIFIANT_MAX:
			m_donnees_controle.valeur_max = valeur;
			break;
		case IDENTIFIANT_VALEUR:
			m_donnees_controle.valeur_defaut = valeur;
			break;
		case IDENTIFIANT_ATTACHE:		
			m_donnees_controle.nom = valeur;
			m_donnees_controle.pointeur = (*m_manipulable)[valeur];
			break;
		case IDENTIFIANT_PRECISION:
			m_donnees_controle.precision = valeur;
			break;
		case IDENTIFIANT_PAS:
			m_donnees_controle.pas = valeur;
			break;
		case IDENTIFIANT_FILTRES:
			m_donnees_controle.filtres = valeur;
			break;
		case IDENTIFIANT_SUFFIXE:
			m_donnees_controle.suffixe = valeur;
			break;
	}
}

void AssembleurDisposition::propriete_bouton(int identifiant, const std::string &valeur)
{
	switch (identifiant) {
		case IDENTIFIANT_INFOBULLE:
			m_dernier_bouton->etablie_infobulle(valeur);
			break;
		case IDENTIFIANT_VALEUR:
			m_dernier_bouton->etablie_valeur(valeur);
			break;
		case IDENTIFIANT_ATTACHE:
			m_dernier_bouton->etablie_attache(valeur);
			break;
		case IDENTIFIANT_METADONNEE:
			m_dernier_bouton->etablie_metadonnee(valeur);
			break;
		case IDENTIFIANT_ICONE:
			m_dernier_bouton->etablie_icone(valeur);
			break;
	}
}

void AssembleurDisposition::propriete_action(int identifiant, const std::string &valeur)
{
	switch (identifiant) {
		case IDENTIFIANT_INFOBULLE:
			m_derniere_action->etablie_infobulle(valeur.c_str());
			break;
		case IDENTIFIANT_VALEUR:
			m_derniere_action->etablie_valeur(valeur.c_str());
			break;
		case IDENTIFIANT_ATTACHE:
			m_derniere_action->etablie_attache(valeur);
			break;
		case IDENTIFIANT_METADONNEE:
			m_derniere_action->etablie_metadonnee(valeur);
			break;
		case IDENTIFIANT_ICONE:
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

	m_donnees_menus.push_back({nom, menu});

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

	/* À FAIRE : trouve une meilleure manière de procéder. */
	if (m_barre_outils) {
		m_barre_outils->addAction(action);
	}
	else {
		m_pile_menus.top()->addAction(action);
	}

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

void AssembleurDisposition::nom_disposition(const std::string &chaine)
{
	m_nom = chaine;
}

std::string AssembleurDisposition::nom_disposition() const
{
	return m_nom;
}

const std::vector<std::pair<std::string, QMenu *>> &AssembleurDisposition::donnees_menus() const
{
	return m_donnees_menus;
}

void AssembleurDisposition::ajoute_barre_outils()
{
	if (m_barre_outils) {
		delete m_barre_outils;
	}

	m_barre_outils = new QToolBar;
}

QToolBar *AssembleurDisposition::barre_outils() const
{
	return m_barre_outils;
}

}  /* namespace danjo */
