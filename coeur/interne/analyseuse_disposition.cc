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

#include "analyseuse_disposition.h"

#include <iostream>

#include "assembleuse_disposition.h"

//#define DEBOGUE_ANALYSEUR

namespace kangao {

/* ************************************************************************** */

static bool est_identifiant_controle(int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_ENTIER:
		case IDENTIFIANT_DECIMAL:
		case IDENTIFIANT_ETIQUETTE:
		case IDENTIFIANT_LISTE:
		case IDENTIFIANT_CASE:
		case IDENTIFIANT_CHAINE:
		case IDENTIFIANT_FICHIER_ENTREE:
		case IDENTIFIANT_FICHIER_SORTIE:
		case IDENTIFIANT_COULEUR:
		case IDENTIFIANT_VECTEUR:
			return true;
		default:
			return false;
	}
}

static bool est_identifiant_propriete(int identifiant)
{
	switch (identifiant) {
		case IDENTIFIANT_INFOBULLE:
		case IDENTIFIANT_MIN:
		case IDENTIFIANT_MAX:
		case IDENTIFIANT_VALEUR:
		case IDENTIFIANT_ATTACHE:
		case IDENTIFIANT_PRECISION:
		case IDENTIFIANT_PAS:
		case IDENTIFIANT_ITEMS:
		case IDENTIFIANT_METADONNEE:
		case IDENTIFIANT_ICONE:
			return true;
		default:
			return false;
	}
}

/* ************************************************************************** */

void AnalyseuseDisposition::installe_assembleur(AssembleurDisposition *assembleur)
{
	m_assembleur = assembleur;
}

void AnalyseuseDisposition::lance_analyse(const std::vector<DonneesMorceaux> &identifiants)
{
	m_identifiants = identifiants;
	m_position = 0;

	if (m_assembleur == nullptr) {
		throw "Un assembleur doit être installé avant de générer l'interface !";
	}

	if (est_identifiant(IDENTIFIANT_DISPOSITION)) {
		analyse_script_disposition();
	}
	else if (est_identifiant(IDENTIFIANT_MENU)) {
		analyse_script_menu();
	}
	else if (est_identifiant(IDENTIFIANT_BARRE_OUTILS)) {
		analyse_script_barre_outils();
	}
}

void AnalyseuseDisposition::analyse_script_disposition()
{
#ifdef DEBOGUE_ANALYSEUR
	std::cout << __func__ << " début\n";
#endif

	if (!requiers_identifiant(IDENTIFIANT_DISPOSITION)) {
		lance_erreur("Le script doit commencer par 'disposition' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_CHAINE_LITTERALE)) {
		lance_erreur("Attendu le nom de la disposition après 'disposition' !");
	}

	m_assembleur->nom_disposition(m_identifiants[position()].contenu);

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante après le nom de la disposition !");
	}

	/* Ajout d'une disposition par défaut. */
	m_assembleur->ajoute_disposition(IDENTIFIANT_COLONNE);

	analyse_disposition();

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin du script !");
	}

#ifdef DEBOGUE_ANALYSEUR
	std::cout << __func__ << " fin\n";
#endif
}

void AnalyseuseDisposition::analyse_script_menu()
{
	if (!requiers_identifiant(IDENTIFIANT_MENU)) {
		lance_erreur("Attendu la déclaration menu !");
	}

	if (!requiers_identifiant(IDENTIFIANT_CHAINE_LITTERALE)) {
		lance_erreur("Attendu le nom du menu après 'menu' !");
	}

	const auto nom = m_identifiants[position()].contenu;

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante après le nom du menu !");
	}

	m_assembleur->ajoute_menu(nom);

	analyse_menu();

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin du script !");
	}

	m_assembleur->sort_menu();
}

void AnalyseuseDisposition::analyse_script_barre_outils()
{
	if (!requiers_identifiant(IDENTIFIANT_BARRE_OUTILS)) {
		lance_erreur("Attendu la déclaration 'barre_outils' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante après 'barre_outils' !");
	}

	m_assembleur->ajoute_barre_outils();

	analyse_barre_outils();

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante à la fin du script !");
	}
}

void AnalyseuseDisposition::analyse_barre_outils()
{
	if (est_identifiant(IDENTIFIANT_ACTION)) {
		analyse_action();
	}
	else {
		return;
	}

	analyse_barre_outils();
}

void AnalyseuseDisposition::analyse_menu()
{
	if (est_identifiant(IDENTIFIANT_ACTION)) {
		analyse_action();
	}
	else if (est_identifiant(IDENTIFIANT_MENU)) {
		analyse_script_menu();
	}
	else if (est_identifiant(IDENTIFIANT_SEPARATEUR)) {
		m_assembleur->ajoute_separateur();
		avance();
	}
	else {
		return;
	}

	analyse_menu();
}

void AnalyseuseDisposition::analyse_action()
{
	if (!requiers_identifiant(IDENTIFIANT_ACTION)) {
		lance_erreur("Attendu la déclaration d'une action !");
	}

	if (!requiers_identifiant(IDENTIFIANT_PARENTHESE_OUVRANTE)) {
		lance_erreur("Attendu l'ouverture d'une paranthèse après 'action' !");
	}

	m_assembleur->ajoute_action();

	analyse_propriete(IDENTIFIANT_ACTION);

	if (!requiers_identifiant(IDENTIFIANT_PARENTHESE_FERMANTE)) {
		lance_erreur("Attendu la fermeture d'une paranthèse !");
	}
}

void AnalyseuseDisposition::analyse_disposition()
{
	if (est_identifiant(IDENTIFIANT_LIGNE)) {
		analyse_ligne();
	}
	else if (est_identifiant(IDENTIFIANT_COLONNE)) {
		analyse_colonne();
	}
	else if (est_identifiant(IDENTIFIANT_DOSSIER)) {
		analyse_dossier();
	}
	else if (est_identifiant(IDENTIFIANT_BOUTON)) {
		analyse_bouton();
	}
	else if (est_identifiant_controle(identifiant_courant())) {
		analyse_controle();
	}
	else {
		return;
	}

	analyse_disposition();
}

void AnalyseuseDisposition::analyse_ligne()
{
#ifdef DEBOGUE_ANALYSEUR
	std::cout << __func__ << '\n';
#endif

	if (!requiers_identifiant(IDENTIFIANT_LIGNE)) {
		lance_erreur("Attendu la déclaration 'ligne' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante après la déclaration 'ligne' !");
	}

	m_assembleur->ajoute_disposition(IDENTIFIANT_LIGNE);

	analyse_disposition();

	m_assembleur->sort_disposition();

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante après la déclaration du contenu de la 'ligne' !");
	}

	analyse_disposition();
}

void AnalyseuseDisposition::analyse_colonne()
{
#ifdef DEBOGUE_ANALYSEUR
	std::cout << __func__ << '\n';
#endif

	if (!requiers_identifiant(IDENTIFIANT_COLONNE)) {
		lance_erreur("Attendu la déclaration 'colonne' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante après la déclaration 'colonne' !");
	}

	m_assembleur->ajoute_disposition(IDENTIFIANT_COLONNE);

	analyse_disposition();

	m_assembleur->sort_disposition();

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante après la déclaration du contenu de la 'colonne' !");
	}

	analyse_disposition();
}

void AnalyseuseDisposition::analyse_dossier()
{
	if (!requiers_identifiant(IDENTIFIANT_DOSSIER)) {
		lance_erreur("Attendu la déclaration 'dossier' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante après la déclaration 'dossier' !");
	}

	m_assembleur->ajoute_dossier();

	analyse_onglet();

	m_assembleur->finalise_dossier();

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante après la déclaration du contenu de 'dossier' !");
	}

	analyse_disposition();
}

void AnalyseuseDisposition::analyse_onglet()
{
	/* Soit le dossier est vide, soit il n'y a plus d'onglets. */
	if (!est_identifiant(IDENTIFIANT_ONGLET)) {
		return;
	}

	if (!requiers_identifiant(IDENTIFIANT_ONGLET)) {
		lance_erreur("Attendu la déclaration 'onglet' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_CHAINE_LITTERALE)) {
		lance_erreur("Attendu une chaîne de caractère après 'onglet' !");
	}

	const auto &nom = m_identifiants[position()].contenu;

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante après le nom de 'onglet' !");
	}

	m_assembleur->ajoute_onglet(nom);

	analyse_disposition();

	m_assembleur->finalise_onglet();

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante après la déclaration du contenu de 'onglet' !");
	}

	analyse_onglet();
}

void AnalyseuseDisposition::analyse_controle()
{
#ifdef DEBOGUE_ANALYSEUR
	std::cout << __func__ << '\n';
#endif

	if (!est_identifiant_controle(identifiant_courant())) {
		lance_erreur("Attendu la déclaration d'un contrôle !");
	}

	const auto identifiant_controle = identifiant_courant();

	avance();

	if (!requiers_identifiant(IDENTIFIANT_PARENTHESE_OUVRANTE)) {
		lance_erreur("Attendu une parenthèse ouvrante après la déclaration du contrôle !");
	}

	m_assembleur->ajoute_controle(identifiant_controle);

	analyse_propriete(identifiant_controle);

	if (!requiers_identifiant(IDENTIFIANT_PARENTHESE_FERMANTE)) {
		lance_erreur("Attendu une parenthèse fermante après la déclaration du contenu du contrôle !");
	}

	m_assembleur->finalise_controle();
}

void AnalyseuseDisposition::analyse_bouton()
{
	if (!requiers_identifiant(IDENTIFIANT_BOUTON)) {
		lance_erreur("Attendu la déclaration d'un bouton !");
	}

	if (!requiers_identifiant(IDENTIFIANT_PARENTHESE_OUVRANTE)) {
		lance_erreur("Attendu une parenthèse ouvrante après la déclaration 'bouton' !");
	}

	m_assembleur->ajoute_bouton();

	analyse_propriete(IDENTIFIANT_BOUTON);

	if (!requiers_identifiant(IDENTIFIANT_PARENTHESE_FERMANTE)) {
		lance_erreur("Attendu une parenthèse fermante après la déclaration du contenu du 'bouton' !");
	}
}

void AnalyseuseDisposition::analyse_propriete(int type_controle)
{
#ifdef DEBOGUE_ANALYSEUR
	std::cout << __func__ << '\n';
#endif

	if (!est_identifiant_propriete(identifiant_courant())) {
		lance_erreur("Attendu la déclaration d'un identifiant !");
	}

	const auto identifiant_propriete = identifiant_courant();

	avance();

	if (!requiers_identifiant(IDENTIFIANT_EGAL)) {
		lance_erreur("Attendu la déclaration '=' !");
	}

	if (identifiant_propriete == IDENTIFIANT_ITEMS) {
		analyse_liste_item();
	}
	else {
		if (!requiers_identifiant(IDENTIFIANT_CHAINE_LITTERALE)) {
			lance_erreur("Attendu une chaine de caractère !");
		}

		if (type_controle == IDENTIFIANT_BOUTON) {
			m_assembleur->propriete_bouton(
						identifiant_propriete,
						m_identifiants[position()].contenu);
		}
		else if (type_controle == IDENTIFIANT_ACTION) {
			m_assembleur->propriete_action(
						identifiant_propriete,
						m_identifiants[position()].contenu);
		}
		else {
			m_assembleur->propriete_controle(
						identifiant_propriete,
						m_identifiants[position()].contenu);
		}
	}

	if (!requiers_identifiant(IDENTIFIANT_POINT_VIRGULE)) {
		/* Fin des identifiants propriétés. */
		recule();
		return;
	}

	analyse_propriete(type_controle);
}

void AnalyseuseDisposition::analyse_liste_item()
{
	if (!requiers_identifiant(IDENTIFIANT_CROCHET_OUVRANT)) {
		lance_erreur("Attendu un crochet ouvert !");
	}

	analyse_item();

	if (!requiers_identifiant(IDENTIFIANT_CROCHET_FERMANT)) {
		lance_erreur("Attendu un crochet fermé !");
	}
}

/* { nom="", valeur=""}, */
void AnalyseuseDisposition::analyse_item()
{
	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouverte !");
	}

	if (!requiers_identifiant(IDENTIFIANT_NOM)) {
		lance_erreur("Attendu la déclaration 'nom' après l'accolade ouverte !");
	}

	if (!requiers_identifiant(IDENTIFIANT_EGAL)) {
		lance_erreur("Attendu la déclaration '=' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_CHAINE_LITTERALE)) {
		lance_erreur("Attendu une chaîne de caractère après '=' !");
	}

	const auto nom = m_identifiants[position()].contenu;

	if (!requiers_identifiant(IDENTIFIANT_VIRGULE)) {
		lance_erreur("Attendu une virgule !");
	}

	if (!requiers_identifiant(IDENTIFIANT_VALEUR)) {
		lance_erreur("Attendu la déclaration 'nom' après l'accolade ouverte !");
	}

	if (!requiers_identifiant(IDENTIFIANT_EGAL)) {
		lance_erreur("Attendu la déclaration '=' !");
	}

	if (!requiers_identifiant(IDENTIFIANT_CHAINE_LITTERALE)) {
		lance_erreur("Attendu une chaîne de caractère après '=' !");
	}

	const auto valeur = m_identifiants[position()].contenu;

	if (!requiers_identifiant(IDENTIFIANT_ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermée !");
	}

	m_assembleur->ajoute_item_liste(nom, valeur);

	if (!requiers_identifiant(IDENTIFIANT_VIRGULE)) {
		/* Fin des identifiants item. */
		recule();
		return;
	}

	analyse_item();
}

}  /* namespace kangao */
