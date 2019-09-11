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
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>

#include "controles/action.h"
#include "controles/bouton.h"
#include "controles/menu_filtrable.hh"

#include "controles_proprietes/controle_propriete_bool.h"
#include "controles_proprietes/controle_propriete_chaine.h"
#include "controles_proprietes/controle_propriete_couleur.h"
#include "controles_proprietes/controle_propriete_courbe_couleur.h"
#include "controles_proprietes/controle_propriete_courbe_valeur.h"
#include "controles_proprietes/controle_propriete_decimal.h"
#include "controles_proprietes/controle_propriete_entier.h"
#include "controles_proprietes/controle_propriete_etiquette.h"
#include "controles_proprietes/controle_propriete_enum.h"
#include "controles_proprietes/controle_propriete_fichier.h"
#include "controles_proprietes/controle_propriete_liste.h"
#include "controles_proprietes/controle_propriete_liste_manip.hh"
#include "controles_proprietes/controle_propriete_rampe_couleur.h"
#include "controles_proprietes/controle_propriete_vecteur.h"

#include "conteneur_controles.h"
#include "manipulable.h"
#include "repondant_bouton.h"

namespace danjo {

AssembleurDisposition::AssembleurDisposition(Manipulable *manipulable, RepondantBouton *repondant_bouton, ConteneurControles *conteneur, int temps, bool initialisation_seule)
	: m_manipulable(manipulable)
	, m_repondant(repondant_bouton)
	, m_conteneur(conteneur)
	, m_temps(temps)
	, m_initialisation_seule(initialisation_seule)
{}

void AssembleurDisposition::ajoute_disposition(id_morceau identifiant)
{
	if (m_initialisation_seule) {
		return;
	}

	QBoxLayout *disposition = nullptr;

	switch (identifiant) {
		default:
		case id_morceau::LIGNE:
			disposition = new QHBoxLayout;
			break;
		case id_morceau::COLONNE:
			disposition = new QVBoxLayout;
			break;
	}

	if (!m_pile_dispositions.est_vide()) {
		m_pile_dispositions.haut()->addLayout(disposition);
	}

	m_pile_dispositions.empile(disposition);
}

void AssembleurDisposition::ajoute_controle(id_morceau identifiant)
{
	ControlePropriete *controle = nullptr;

	m_donnees_controle = DonneesControle();

	switch (identifiant) {
		case id_morceau::ENTIER:
			controle = new ControleProprieteEntier;
			m_donnees_controle.type = TypePropriete::ENTIER;
			break;
		case id_morceau::DECIMAL:
			controle = new ControleProprieteDecimal;
			m_donnees_controle.type = TypePropriete::DECIMAL;
			break;
		case id_morceau::ETIQUETTE:
			controle = new ControleProprieteEtiquette;
			break;
		case id_morceau::ENUM:
			controle = new ControleProprieteEnum;
			m_donnees_controle.type = TypePropriete::ENUM;
			break;
		case id_morceau::LISTE:
		{
			auto controle_liste = new ControleProprieteListe;
			controle_liste->conteneur(m_conteneur);
			controle = controle_liste;
			m_donnees_controle.type = TypePropriete::CHAINE_CARACTERE;
			break;
		}
		case id_morceau::CASE:
			controle = new ControleProprieteBool;
			m_donnees_controle.type = TypePropriete::BOOL;
			break;
		case id_morceau::CHAINE:
			controle = new ControleProprieteChaineCaractere;
			m_donnees_controle.type = TypePropriete::CHAINE_CARACTERE;
			break;
		case id_morceau::FICHIER_ENTREE:
			controle = new ControleProprieteFichier(true);
			m_donnees_controle.type = TypePropriete::FICHIER_ENTREE;
			break;
		case id_morceau::FICHIER_SORTIE:
			controle = new ControleProprieteFichier(false);
			m_donnees_controle.type = TypePropriete::FICHIER_SORTIE;
			break;
		case id_morceau::COULEUR:
			controle = new ControleProprieteCouleur;
			m_donnees_controle.type = TypePropriete::COULEUR;
			break;
		case id_morceau::VECTEUR:
			controle = new ControleProprieteVec3;
			m_donnees_controle.type = TypePropriete::VECTEUR;
			break;
		case id_morceau::COURBE_COULEUR:
			controle = new ControleProprieteCourbeCouleur;
			m_donnees_controle.type = TypePropriete::COURBE_COULEUR;
			break;
		case id_morceau::COURBE_VALEUR:
			controle = new ControleProprieteCourbeValeur;
			m_donnees_controle.type = TypePropriete::COURBE_VALEUR;
			break;
		case id_morceau::RAMPE_COULEUR:
			controle = new ControleProprieteRampeCouleur;
			m_donnees_controle.type = TypePropriete::RAMPE_COULEUR;
			break;
		case id_morceau::TEXTE:
			controle = new ControleProprieteEditeurTexte;
			m_donnees_controle.type = TypePropriete::TEXTE;
			break;
		case id_morceau::LISTE_MANIP:
			controle = new ControleProprieteListeManip;
			m_donnees_controle.type = TypePropriete::LISTE_MANIP;
			break;
		default:
			break;
	}

	m_dernier_controle = controle;

	if (!m_initialisation_seule) {
		m_pile_dispositions.haut()->addWidget(controle);
	}
}

void AssembleurDisposition::cree_controles_proprietes_extra()
{
	if (m_initialisation_seule) {
		return;
	}

	auto debut = m_manipulable->debut();
	auto fin = m_manipulable->fin();

	for (auto iter = debut; iter != fin; ++iter) {
		const Propriete &prop = iter->second;

		if (!prop.est_extra) {
			continue;
		}

		auto identifiant = id_morceau::INCONNU;

		switch (prop.type) {
			case TypePropriete::ENTIER:
				identifiant = id_morceau::ENTIER;
				break;
			case TypePropriete::DECIMAL:
				identifiant = id_morceau::DECIMAL;
				break;
			case TypePropriete::ENUM:
				identifiant = id_morceau::ENUM;
				break;
			case TypePropriete::CHAINE_CARACTERE:
				identifiant = id_morceau::CHAINE;
				break;
			case TypePropriete::FICHIER_ENTREE:
				identifiant = id_morceau::FICHIER_ENTREE;
				break;
			case TypePropriete::FICHIER_SORTIE:
				identifiant = id_morceau::FICHIER_SORTIE;
				break;
			case TypePropriete::COULEUR:
				identifiant = id_morceau::COULEUR;
				break;
			case TypePropriete::VECTEUR:
				identifiant = id_morceau::VECTEUR;
				break;
			default:
				break;
		}

		if (identifiant == id_morceau::INCONNU) {
			continue;
		}

		ajoute_disposition(id_morceau::LIGNE);

		auto etiquette = new ControleProprieteEtiquette;
		auto donnees_etiquette = DonneesControle();
		donnees_etiquette.valeur_defaut = iter->first;
		etiquette->finalise(donnees_etiquette);

		m_pile_dispositions.haut()->addWidget(etiquette);

		ajoute_controle(identifiant);
		m_donnees_controle.nom = iter->first;
		m_donnees_controle.pointeur = (*m_manipulable)[m_donnees_controle.nom];
		finalise_controle();

		sors_disposition();
	}
}

void AssembleurDisposition::ajoute_item_liste(const dls::chaine &nom, const dls::chaine &valeur)
{
	if (m_initialisation_seule) {
		return;
	}

	m_donnees_controle.valeur_enum.pousse({nom, valeur});
}

void AssembleurDisposition::ajoute_bouton()
{
	if (m_initialisation_seule) {
		return;
	}

	Bouton *bouton = new Bouton;
	bouton->installe_repondant(m_repondant);

	m_dernier_bouton = bouton;
	m_pile_dispositions.haut()->addWidget(bouton);
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

	/* NOTE : l'initialisation de la valeur par défaut de la propriété se fait
	 * dans la méthode 'finalise' du controle. */
	auto prop = m_manipulable->propriete(m_donnees_controle.nom);

	/* Les étiquettes n'ont pas de pointeur dans le Manipulable. */
	if (prop != nullptr) {
		prop->etat = m_donnees_controle.etat;
	}

	m_dernier_controle->proriete(prop);
	m_dernier_controle->temps(m_temps);
	m_dernier_controle->finalise(m_donnees_controle);

	if (m_initialisation_seule) {
		/* Si on ne fait qu'initialiser les propriétés du manipulable, on peut
		 * directement supprimer le controle. */
		delete m_dernier_controle;
		m_dernier_controle = nullptr;
	}

	if (m_conteneur != nullptr) {
		QObject::connect(m_dernier_controle, &ControlePropriete::precontrole_change,
						 m_conteneur, &ConteneurControles::precontrole_change);

		QObject::connect(m_dernier_controle, &ControlePropriete::controle_change,
						 m_conteneur, &ConteneurControles::ajourne_manipulable);
	}
}

void AssembleurDisposition::sors_disposition()
{
	m_pile_dispositions.depile();
}

void AssembleurDisposition::propriete_controle(id_morceau identifiant, const dls::chaine &valeur)
{
	switch (identifiant) {
		case id_morceau::INFOBULLE:
			m_donnees_controle.infobulle = valeur;
			break;
		case id_morceau::MIN:
			m_donnees_controle.valeur_min = valeur;
			break;
		case id_morceau::MAX:
			m_donnees_controle.valeur_max = valeur;
			break;
		case id_morceau::VALEUR:
			m_donnees_controle.valeur_defaut = valeur;
			break;
		case id_morceau::ATTACHE:		
			m_donnees_controle.nom = valeur;
			m_donnees_controle.pointeur = (*m_manipulable)[valeur];
			break;
		case id_morceau::PRECISION:
			m_donnees_controle.precision = valeur;
			break;
		case id_morceau::PAS:
			m_donnees_controle.pas = valeur;
			break;
		case id_morceau::FILTRES:
			m_donnees_controle.filtres = valeur;
			break;
		case id_morceau::SUFFIXE:
			m_donnees_controle.suffixe = valeur;
			break;
		case id_morceau::ANIMABLE:
			m_donnees_controle.etat |= EST_ANIMABLE;
			break;
		case id_morceau::ACTIVABLE:
			m_donnees_controle.etat |= EST_ACTIVABLE;
			break;
		default:
			break;
	}
}

void AssembleurDisposition::propriete_bouton(id_morceau identifiant, const dls::chaine &valeur)
{
	if (m_initialisation_seule) {
		return;
	}

	switch (identifiant) {
		case id_morceau::INFOBULLE:
			m_dernier_bouton->etablie_infobulle(valeur);
			break;
		case id_morceau::VALEUR:
			m_dernier_bouton->etablie_valeur(valeur);
			break;
		case id_morceau::ATTACHE:
			m_dernier_bouton->etablie_attache(valeur);
			break;
		case id_morceau::METADONNEE:
			m_dernier_bouton->etablie_metadonnee(valeur);
			break;
		case id_morceau::ICONE:
			m_dernier_bouton->etablie_icone(valeur);
			break;
		default:
			break;
	}
}

void AssembleurDisposition::propriete_action(id_morceau identifiant, const dls::chaine &valeur)
{
	if (m_initialisation_seule) {
		return;
	}

	switch (identifiant) {
		case id_morceau::INFOBULLE:
			m_derniere_action->etablie_infobulle(valeur.c_str());
			break;
		case id_morceau::VALEUR:
			m_derniere_action->etablie_valeur(valeur.c_str());
			break;
		case id_morceau::ATTACHE:
			m_derniere_action->etablie_attache(valeur);
			break;
		case id_morceau::METADONNEE:
			m_derniere_action->etablie_metadonnee(valeur);
			break;
		case id_morceau::ICONE:
			m_derniere_action->etablie_icone(valeur);
			break;
		default:
			break;
	}
}

QBoxLayout *AssembleurDisposition::disposition()
{
	if (m_pile_dispositions.est_vide()) {
		return nullptr;
	}

	return m_pile_dispositions.haut();
}

QMenu *AssembleurDisposition::menu()
{
	return m_menu_racine;
}

void AssembleurDisposition::ajoute_menu(const dls::chaine &nom)
{
	if (m_initialisation_seule) {
		return;
	}

	auto menu = new MenuFiltrable(nom.c_str());

	if (!m_pile_menus.est_vide()) {
		m_pile_menus.haut()->addMenu(menu);
	}
	else {
		m_menu_racine = menu;
	}

	m_donnees_menus.pousse({nom, menu});

	m_pile_menus.empile(menu);
}

void AssembleurDisposition::sort_menu()
{
	m_pile_menus.depile();
}

void AssembleurDisposition::ajoute_action()
{
	if (m_initialisation_seule) {
		return;
	}

	auto action = new Action;
	action->installe_repondant(m_repondant);

	/* À FAIRE : trouve une meilleure manière de procéder. */
	if (m_barre_outils) {
		m_barre_outils->addAction(action);
	}
	else {
		m_pile_menus.haut()->addAction(action);
	}

	m_derniere_action = action;
}

void AssembleurDisposition::ajoute_separateur()
{
	if (m_initialisation_seule) {
		return;
	}

	m_pile_menus.haut()->addSeparator();
}

void AssembleurDisposition::ajoute_dossier()
{
	if (m_initialisation_seule) {
		return;
	}

	QTabWidget *dossier = new QTabWidget;

	m_pile_dispositions.haut()->addWidget(dossier);

	m_dernier_dossier = dossier;
}

void AssembleurDisposition::finalise_dossier()
{
	m_dernier_dossier->setCurrentIndex(m_manipulable->onglet_courant);

	if (m_conteneur != nullptr) {
		QObject::connect(m_dernier_dossier, &QTabWidget::currentChanged,
						 m_conteneur, &ConteneurControles::onglet_dossier_change);
	}

	m_dernier_dossier = nullptr;
}

void AssembleurDisposition::ajoute_onglet(const dls::chaine &nom)
{
	if (m_initialisation_seule) {
		return;
	}

	/* À FAIRE : expose contrôle de la direction. */
	auto disp_onglet = new QVBoxLayout;

	auto onglet = new QWidget;
	onglet->setLayout(disp_onglet);

	m_pile_dispositions.empile(disp_onglet);

	m_dernier_dossier->addTab(onglet, nom.c_str());
}

void AssembleurDisposition::finalise_onglet()
{
	m_pile_dispositions.depile();
}

void AssembleurDisposition::nom_disposition(const dls::chaine &chaine)
{
	m_nom = chaine;
}

dls::chaine AssembleurDisposition::nom_disposition() const
{
	return m_nom;
}

const dls::tableau<std::pair<dls::chaine, QMenu *>> &AssembleurDisposition::donnees_menus() const
{
	return m_donnees_menus;
}

void AssembleurDisposition::ajoute_barre_outils()
{
	if (m_initialisation_seule) {
		return;
	}

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
