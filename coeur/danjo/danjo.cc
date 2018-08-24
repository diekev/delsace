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

#include "danjo.h"

#include <fstream>
#include <iostream>

#include <QBoxLayout>
#include <QMenu>
#include <QToolBar>

#include "controles/action.h"

#include "compilation/analyseuse_disposition.h"
#include "compilation/analyseuse_logique.h"
#include "compilation/assembleuse_disposition.h"
#include "compilation/decoupeuse.h"

#include "dialogue.h"
#include "erreur.h"
#include "manipulable.h"
#include "menu_interrogeable.h"

namespace danjo {

GestionnaireInterface::~GestionnaireInterface()
{
	for (const auto &donnees : m_menus) {
		auto menu = donnees.second;

		for (auto &action : menu->actions()) {
			delete action;
		}

		delete menu;
	}

	for (const auto &donnees : m_menus_interrogeables) {
		auto menu = donnees.second;

		for (auto &action : menu->actions()) {
			delete action;
		}

		delete menu;
	}

	for (auto &barre_outils : m_barres_outils) {
		for (auto &action : barre_outils->actions()) {
			delete action;
		}

		delete barre_outils;
	}
}

void GestionnaireInterface::ajourne_menu(const std::string &nom)
{
	auto menu = pointeur_menu(nom);

	if (menu == nullptr) {
		return;
	}

	for (auto &pointeur : menu->actions()) {
		Action *action = dynamic_cast<Action *>(pointeur);

		if (action) {
			action->evalue_predicat();
		}
	}
}

/* À FAIRE : passe un script. */
void GestionnaireInterface::recree_menu(
		const std::string &nom,
		const std::vector<DonneesAction> &donnees_actions)
{
	auto menu = pointeur_menu(nom);

	if (menu == nullptr) {
		return;
	}

	menu->clear();

	for (const auto &donnees : donnees_actions) {
		Action *action = new Action;
		action->etablie_valeur(donnees.nom);
		action->etablie_attache(donnees.attache);
		action->etablie_metadonnee(donnees.metadonnee);
		action->installe_repondant(donnees.repondant_bouton);

		menu->addAction(action);
	}
}

QMenu *GestionnaireInterface::compile_menu(
		DonneesInterface &donnees,
		const char *texte_entree)
{
	AssembleurDisposition assembleur(
				donnees.manipulable,
				donnees.repondant_bouton,
				donnees.conteneur);

	AnalyseuseDisposition analyseur;
	analyseur.installe_assembleur(&assembleur);

	Decoupeuse decoupeuse(texte_entree);

	try {
		decoupeuse.decoupe();
		analyseur.lance_analyse(decoupeuse.morceaux());
	}
	catch (const ErreurFrappe &e) {
		std::cerr << e.quoi();
		return nullptr;
	}
	catch (const ErreurSyntactique &e) {
		std::cerr << e.quoi();
		return nullptr;
	}

	for (const auto &pair : assembleur.donnees_menus()) {
		m_menus.insert(pair);
	}

	return assembleur.menu();
}

QMenu *GestionnaireInterface::compile_menu_interrogeable(
		DonneesInterface &donnees,
		const char *texte_entree)
{
	AssembleurDisposition assembleur(
				donnees.manipulable,
				donnees.repondant_bouton,
				donnees.conteneur);

	AnalyseuseDisposition analyseuse;
	analyseuse.installe_assembleur(&assembleur);

	Decoupeuse decoupeuse(texte_entree);

	try {
		decoupeuse.decoupe();
		analyseuse.lance_analyse(decoupeuse.morceaux());
	}
	catch (const ErreurFrappe &e) {
		std::cerr << e.quoi();
		return nullptr;
	}
	catch (const ErreurSyntactique &e) {
		std::cerr << e.quoi();
		return nullptr;
	}

	/* À FAIRE : déplace ça dans l'assembleuse. */
	auto menu_interrogeable = new MenuInterrogeable("");

	for (auto &action : assembleur.menu()->actions()) {
		menu_interrogeable->addAction(action);
	}

	/* À FAIRE : déduplique les menus. */
	for (const auto &pair : assembleur.donnees_menus()) {
		m_menus_interrogeables.insert(pair);
	}

	return menu_interrogeable;
}

QBoxLayout *GestionnaireInterface::compile_interface(DonneesInterface &donnees, const char *texte_entree, int temps)
{
	if (donnees.manipulable == nullptr) {
		return nullptr;
	}

	AssembleurDisposition assembleur(
				donnees.manipulable,
				donnees.repondant_bouton,
				donnees.conteneur,
				temps);

	AnalyseuseDisposition analyseur;
	analyseur.installe_assembleur(&assembleur);

	Decoupeuse decoupeuse(texte_entree);

	try {
		decoupeuse.decoupe();
		analyseur.lance_analyse(decoupeuse.morceaux());
	}
	catch (const ErreurFrappe &e) {
		std::cerr << e.quoi();
		return nullptr;
	}
	catch (const ErreurSyntactique &e) {
		std::cerr << e.quoi();
		return nullptr;
	}

	auto disposition = assembleur.disposition();
	disposition->addStretch();

	auto nom = assembleur.nom_disposition();

	m_dispositions.insert({nom, disposition});

	return disposition;
}

void GestionnaireInterface::initialise_interface(Manipulable *manipulable, const char *texte_entree)
{
	if (manipulable == nullptr) {
		return;
	}

	AssembleurDisposition assembleuse(
				manipulable,
				nullptr,
				nullptr,
				0,
				true);

	AnalyseuseDisposition analyseuse;
	analyseuse.installe_assembleur(&assembleuse);

	Decoupeuse decoupeuse(texte_entree);

	try {
		decoupeuse.decoupe();
		analyseuse.lance_analyse(decoupeuse.morceaux());
	}
	catch (const ErreurFrappe &e) {
		std::cerr << e.quoi();
	}
	catch (const ErreurSyntactique &e) {
		std::cerr << e.quoi();
	}
}

QMenu *GestionnaireInterface::pointeur_menu(const std::string &nom)
{
	auto iter = m_menus.find(nom);

	if (iter == m_menus.end()) {
		std::cerr << "Le menu '" << nom << "' est introuvable !\n";
		return nullptr;
	}

	return (*iter).second;
}

QToolBar *GestionnaireInterface::compile_barre_outils(DonneesInterface &donnees, const char *texte_entree)
{
	AssembleurDisposition assembleur(
				donnees.manipulable,
				donnees.repondant_bouton,
				donnees.conteneur);

	AnalyseuseDisposition analyseur;
	analyseur.installe_assembleur(&assembleur);

	Decoupeuse decoupeuse(texte_entree);

	try {
		decoupeuse.decoupe();
		analyseur.lance_analyse(decoupeuse.morceaux());
	}
	catch (const ErreurFrappe &e) {
		std::cerr << e.quoi();
		return nullptr;
	}
	catch (const ErreurSyntactique &e) {
		std::cerr << e.quoi();
		return nullptr;
	}

	m_barres_outils.push_back(assembleur.barre_outils());

	return assembleur.barre_outils();
}

bool GestionnaireInterface::montre_dialogue(DonneesInterface &donnees, const char *texte_entree)
{
	auto disposition = this->compile_interface(donnees, texte_entree);

	if (disposition == nullptr) {
		return false;
	}

	Dialogue dialogue(disposition);
	dialogue.show();

	return dialogue.exec() == QDialog::Accepted;
}

/* ************************************************************************** */

std::string contenu_fichier(const std::experimental::filesystem::path &chemin)
{
	if (!std::experimental::filesystem::exists(chemin)) {
		std::cerr << "Le chemin de fichier " << chemin << " ne pointe vers aucun fichier !\n";
		return "";
	}

	std::ifstream entree;
	entree.open(chemin.c_str());

	std::string contenu((std::istreambuf_iterator<char>(entree)),
						(std::istreambuf_iterator<char>()));

	return contenu;
}

/* ************************************************************************** */

static GestionnaireInterface __gestionnaire;

QBoxLayout *compile_interface(
		DonneesInterface &donnees,
		const char *texte_entree,
		int temps)
{
	return __gestionnaire.compile_interface(donnees, texte_entree, temps);
}

QBoxLayout *compile_interface(
		DonneesInterface &donnees,
		const std::experimental::filesystem::path &chemin_texte,
		int temps)
{
	if (donnees.manipulable == nullptr) {
		return nullptr;
	}

	const auto texte_entree = contenu_fichier(chemin_texte.c_str());
	return __gestionnaire.compile_interface(donnees, texte_entree.c_str(), temps);
}

QMenu *compile_menu(DonneesInterface &donnees, const char *texte_entree)
{
	return __gestionnaire.compile_menu(donnees, texte_entree);
}

QMenu *compile_menu_interrogeable(DonneesInterface &donnees, const char *texte_entree)
{
	return __gestionnaire.compile_menu_interrogeable(donnees, texte_entree);
}

void compile_feuille_logique(const char *texte_entree)
{
	AnalyseuseLogique analyseuse(nullptr);
	Decoupeuse decoupeuse(texte_entree);

	try {
		decoupeuse.decoupe();
		analyseuse.lance_analyse(decoupeuse.morceaux());
	}
	catch (const ErreurFrappe &e) {
		std::cerr << e.quoi();
	}
	catch (const ErreurSyntactique &e) {
		std::cerr << e.quoi();
	}

	/* À FAIRE */
//	return __gestionnaire.compile_feuille_logique(texte_entree);
}

void initialise_interface(Manipulable *manipulable, const char *texte_entree)
{
	return __gestionnaire.initialise_interface(manipulable, texte_entree);
}

bool montre_dialogue(DonneesInterface &donnees, const char *texte_entree)
{
	return __gestionnaire.montre_dialogue(donnees, texte_entree);
}

}  /* namespace danjo */
