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

#include <stack>
#include <string>
#include <vector>

class Action;
class Bouton;
class ConteneurControles;
class Controle;
class Manipulable;
class RepondantBouton;
class QBoxLayout;
class QLabel;
class QMenu;

/**
 * La classe AssembleurDisposition s'occupe de créer l'interface en fonction de
 * ce que l'analyseur lui dit.
 *
 * L'assembleur met en place les connections entre signaux et slots des
 * contrôles et de leur conteneur.
 */
class AssembleurDisposition {
	std::stack<QBoxLayout *, std::vector<QBoxLayout *>> m_pile_dispositions;
	std::stack<QMenu *, std::vector<QMenu *>> m_pile_menus;

	Action *m_derniere_action = nullptr;
	Controle *m_dernier_controle = nullptr;
	Bouton *m_dernier_bouton = nullptr;
	QMenu *m_menu_racine = nullptr;

	Manipulable *m_manipulable = nullptr;
	RepondantBouton *m_repondant = nullptr;
	ConteneurControles *m_conteneur = nullptr;

public:
	/**
	 * Construit une instance d'un assembleur avec les paramètres spécifiés.
	 *
	 * Le manipulable est l'objet qui contient les propriétés exposées dans
	 * l'interface.
	 *
	 * Le repondant_bouton est l'objet qui doit répondre des cliques de bouton.
	 *
	 * Le conteneur est l'objet qui soit détient le manipulable, soit répond
	 * aux changements des contrôles exposés dans l'interface.
	 */
	explicit AssembleurDisposition(
			Manipulable *manipulable,
			RepondantBouton *repondant_bouton,
			ConteneurControles *conteneur);

	/**
	 * Ajoute une disposition (ligne ou colonne) à la pile de disposition.
	 */
	void ajoute_disposition(int identifiant);

	/**
	 * Ajoute un contrôle à la disposition se trouvant au sommet de la pile.
	 */
	void ajoute_controle(int identifiant);

	/**
	 * Ajoute une item à un contrôle de type liste déroulante. Le dernier
	 * contrôle ajouté via ajoute_controle est considéré comme étant une liste
	 * déroulante.
	 */
	void ajoute_item_liste(const std::string &nom, const std::string &valeur);

	/**
	 * Ajoute un bouton à la disposition se trouvant au sommet de la pile.
	 */
	void ajoute_bouton();

	/**
	 * Finalise le dernier contrôle ajouté en appelant Controle::finalise().
	 *
	 * Le signal Controle::controle_change du contrôle est connecté au slot
	 * ConteneurControles::ajourne_manipulable du conteneur spécifié en
	 * paramètre du constructeur de l'assembleur.
	 */
	void finalise_controle();

	/**
	 * Enlève la disposition courante du sommet de la pile. La disposition se
	 * trouvant en dessous devient la disposition active.
	 */
	void sort_disposition();

	/**
	 * Ajoute une propriété au dernier contrôle ajouté.
	 */
	void propriete_controle(int identifiant, const std::string &valeur);

	/**
	 * Ajoute une propriété au dernier bouton ajouté.
	 */
	void propriete_bouton(int indentifiant, const std::string &valeur);

	/**
	 * Retourne la disposition se trouvant au sommet de la pile de dispositions.
	 * Si aucune disposition n'existe dans la pile, retourne nullptr.
	 */
	QBoxLayout *disposition();

	QMenu *menu();

	void ajoute_menu(const std::string &nom);

	void sort_menu();

	void ajoute_action();

	void propriete_action(int identifiant, const std::string &valeur);

	void ajoute_separateur();
};
