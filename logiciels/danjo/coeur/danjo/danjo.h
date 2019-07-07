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

#include <experimental/filesystem>

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/tableau.hh"

class QBoxLayout;
class QMenu;
class QToolBar;

namespace danjo {

class ConteneurControles;
class Manipulable;
class RepondantBouton;

struct DonneesInterface {
	Manipulable *manipulable = nullptr;
	RepondantBouton *repondant_bouton = nullptr;
	ConteneurControles *conteneur = nullptr;

	DonneesInterface(DonneesInterface const &) = default;
	DonneesInterface &operator=(DonneesInterface const &) = default;
};

struct DonneesAction {
	dls::chaine nom{};
	dls::chaine attache{};
	dls::chaine metadonnee{};
	RepondantBouton *repondant_bouton = nullptr;

	DonneesAction(DonneesAction const &) = default;
	DonneesAction &operator=(DonneesAction const &) = default;
};

class GestionnaireInterface {
	dls::dico_desordonne<dls::chaine, QMenu *> m_menus{};
	dls::dico_desordonne<dls::chaine, QMenu *> m_menus_entrerogeables{};
	dls::dico_desordonne<dls::chaine, QBoxLayout *> m_dispositions{};
	dls::tableau<QToolBar *> m_barres_outils{};

public:
	~GestionnaireInterface();

	void ajourne_menu(const dls::chaine &nom);

	void recree_menu(
			const dls::chaine &nom,
			const dls::tableau<DonneesAction> &donnees_actions);

	void ajourne_disposition(const dls::chaine &nom, int temps = 0);

	QMenu *compile_menu(DonneesInterface &donnees, const char *texte_entree);

	QMenu *compile_menu_entrerogeable(DonneesInterface &donnees, const char *texte_entree);

	QBoxLayout *compile_entreface(
			DonneesInterface &donnees,
			const char *texte_entree,
			int temps = 0);

	void initialise_entreface(Manipulable *manipulable, const char *texte_entree);

	QMenu *pointeur_menu(const dls::chaine &nom);

	QToolBar *compile_barre_outils(DonneesInterface &donnees, const char *texte_entree);

	bool montre_dialogue(DonneesInterface &donnees, const char *texte_entree);
};

/**
 * Retourne une chaîne de caractère ayant le contenu du fichier pointé par le
 * chemin spécifié. Si le chemin pointe vers un fichier non-existant, la chaîne
 * retournée sera vide.
 */
dls::chaine contenu_fichier(const std::experimental::filesystem::path &chemin);

QMenu *compile_menu(DonneesInterface &donnees, const char *texte_entree);

QMenu *compile_menu_entrerogeable(DonneesInterface &donnees, const char *texte_entree);

/**
 * Compile le script d'entreface contenu dans texte_entree, et retourne un
 * pointeur vers le QBoxLayout ainsi créé.
 */
QBoxLayout *compile_entreface(
		DonneesInterface &donnees,
		const char *texte_entree,
		int temps = 0);

/**
 * Compile le script d'entreface contenu dans le fichier dont le chemin est
 * spécifié, et retourne un pointeur vers le QBoxLayout ainsi créé.
 */
QBoxLayout *compile_entreface(
		DonneesInterface &donnees,
		const std::experimental::filesystem::path &chemin_texte,
		int temps = 0);

void compile_feuille_logique(const char *texte_entree);

/**
 * Initialise les propriétés d'un manipulable depuis un fichier .jo.
 */
void initialise_entreface(Manipulable *manipulable, const char *texte_entree);

bool montre_dialogue(DonneesInterface &donnees, const char *texte_entree);

}  /* namespace danjo */
