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

#include "biblinternes/langage/tampon_source.hh"
#include "biblinternes/outils/fichier.hh"

#include "controles/action.h"

#include "compilation/analyseuse_disposition.h"
#include "compilation/analyseuse_logique.h"
#include "compilation/assembleuse_disposition.h"
#include "compilation/decoupeuse.h"

#include "dialogue.h"
#include "erreur.h"
#include "manipulable.h"
#include "menu_entrerogeable.h"

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

	for (const auto &donnees : m_menus_entrerogeables) {
		auto menu = donnees.second;

		for (auto &action : menu->actions()) {
			delete action;
		}

		delete menu;
	}

	/* crash lors de la sortie des programmes */
//	for (auto &barre_outils : m_barres_outils) {
//		for (auto &action : barre_outils->actions()) {
//			delete action;
//		}

//		delete barre_outils;
//	}
}

void GestionnaireInterface::parent_dialogue(QWidget *p)
{
	m_parent_dialogue = p;
}

void GestionnaireInterface::ajourne_menu(const dls::chaine &nom)
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
		const dls::chaine &nom,
		const dls::tableau<DonneesAction> &donnees_actions)
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
	AssembleurDisposition assembleuse(
				donnees.manipulable,
				donnees.repondant_bouton,
				donnees.conteneur);

	auto tampon = lng::tampon_source(texte_entree);
	auto decoupeuse = Decoupeuse(tampon);

	try {
		decoupeuse.decoupe();

		auto analyseuse = AnalyseuseDisposition(tampon, decoupeuse.morceaux());
		analyseuse.installe_assembleur(&assembleuse);
		analyseuse.lance_analyse(std::cerr);
	}
	catch (const ErreurFrappe &e) {
		std::cerr << e.quoi();
		return nullptr;
	}
	catch (const ErreurSyntactique &e) {
		std::cerr << e.quoi();
		return nullptr;
	}

	for (const auto &pair : assembleuse.donnees_menus()) {
		m_menus.insere(pair);
	}

	return assembleuse.menu();
}

QMenu *GestionnaireInterface::compile_menu_entrerogeable(
		DonneesInterface &donnees,
		const char *texte_entree)
{
	AssembleurDisposition assembleuse(
				donnees.manipulable,
				donnees.repondant_bouton,
				donnees.conteneur);

	auto tampon = lng::tampon_source(texte_entree);
	auto decoupeuse = Decoupeuse(tampon);

	try {
		decoupeuse.decoupe();

		auto analyseuse = AnalyseuseDisposition(tampon, decoupeuse.morceaux());
		analyseuse.installe_assembleur(&assembleuse);
		analyseuse.lance_analyse(std::cerr);
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
	auto menu_entrerogeable = new MenuEntrerogeable("");

	for (auto &action : assembleuse.menu()->actions()) {
		menu_entrerogeable->addAction(action);
	}

	/* À FAIRE : déduplique les menus. */
	for (const auto &pair : assembleuse.donnees_menus()) {
		m_menus_entrerogeables.insere(pair);
	}

	return menu_entrerogeable;
}

QBoxLayout *GestionnaireInterface::compile_entreface(DonneesInterface &donnees, const char *texte_entree, int temps)
{
	if (donnees.manipulable == nullptr) {
		return nullptr;
	}

	AssembleurDisposition assembleuse(
				donnees.manipulable,
				donnees.repondant_bouton,
				donnees.conteneur,
				temps);

	auto tampon = lng::tampon_source(texte_entree);
	auto decoupeuse = Decoupeuse(tampon);

	try {
		decoupeuse.decoupe();

		auto analyseuse = AnalyseuseDisposition(tampon, decoupeuse.morceaux());
		analyseuse.installe_assembleur(&assembleuse);
		analyseuse.lance_analyse(std::cerr);
	}
	catch (const ErreurFrappe &e) {
		std::cerr << e.quoi();
		return nullptr;
	}
	catch (const ErreurSyntactique &e) {
		std::cerr << e.quoi();
		return nullptr;
	}

	auto disposition = assembleuse.disposition();
	disposition->addStretch();

	auto nom = assembleuse.nom_disposition();

	m_dispositions.insere({nom, disposition});

	return disposition;
}

void GestionnaireInterface::initialise_entreface(Manipulable *manipulable, const char *texte_entree)
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

	auto tampon = lng::tampon_source(texte_entree);
	auto decoupeuse = Decoupeuse(tampon);

	try {
		decoupeuse.decoupe();

		auto analyseuse = AnalyseuseDisposition(tampon, decoupeuse.morceaux());
		analyseuse.installe_assembleur(&assembleuse);
		analyseuse.lance_analyse(std::cerr);
	}
	catch (const ErreurFrappe &e) {
		std::cerr << e.quoi();
	}
	catch (const ErreurSyntactique &e) {
		std::cerr << e.quoi();
	}
}

QMenu *GestionnaireInterface::pointeur_menu(const dls::chaine &nom)
{
	auto iter = m_menus.trouve(nom);

	if (iter == m_menus.fin()) {
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

	auto tampon = lng::tampon_source(texte_entree);
	auto decoupeuse = Decoupeuse(tampon);

	try {
		decoupeuse.decoupe();

		auto analyseuse = AnalyseuseDisposition(tampon, decoupeuse.morceaux());
		analyseuse.installe_assembleur(&assembleur);
		analyseuse.lance_analyse(std::cerr);
	}
	catch (const ErreurFrappe &e) {
		std::cerr << e.quoi();
		return nullptr;
	}
	catch (const ErreurSyntactique &e) {
		std::cerr << e.quoi();
		return nullptr;
	}

	m_barres_outils.pousse(assembleur.barre_outils());

	return assembleur.barre_outils();
}

bool GestionnaireInterface::montre_dialogue(DonneesInterface &donnees, const char *texte_entree)
{
	auto disposition = this->compile_entreface(donnees, texte_entree);

	if (disposition == nullptr) {
		return false;
	}

	auto dialogue = Dialogue(disposition, this->m_parent_dialogue);
	dialogue.show();

	return dialogue.exec() == QDialog::Accepted;
}

/* ************************************************************************** */

static GestionnaireInterface __gestionnaire;

QBoxLayout *compile_entreface(
		DonneesInterface &donnees,
		const char *texte_entree,
		int temps)
{
	return __gestionnaire.compile_entreface(donnees, texte_entree, temps);
}

QBoxLayout *compile_entreface(
		DonneesInterface &donnees,
		const std::experimental::filesystem::path &chemin_texte,
		int temps)
{
	if (donnees.manipulable == nullptr) {
		return nullptr;
	}

	const auto texte_entree = dls::contenu_fichier(chemin_texte.c_str());
	return __gestionnaire.compile_entreface(donnees, texte_entree.c_str(), temps);
}

QMenu *compile_menu(DonneesInterface &donnees, const char *texte_entree)
{
	return __gestionnaire.compile_menu(donnees, texte_entree);
}

QMenu *compile_menu_entrerogeable(DonneesInterface &donnees, const char *texte_entree)
{
	return __gestionnaire.compile_menu_entrerogeable(donnees, texte_entree);
}

void compile_feuille_logique(const char *texte_entree)
{
	auto tampon = lng::tampon_source(texte_entree);
	auto decoupeuse = Decoupeuse(tampon);

	try {
		decoupeuse.decoupe();

		auto analyseuse = AnalyseuseLogique(nullptr, tampon, decoupeuse.morceaux());
		analyseuse.lance_analyse(std::cerr);
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

void initialise_entreface(Manipulable *manipulable, const char *texte_entree)
{
	return __gestionnaire.initialise_entreface(manipulable, texte_entree);
}

bool montre_dialogue(DonneesInterface &donnees, const char *texte_entree)
{
	return __gestionnaire.montre_dialogue(donnees, texte_entree);
}

}  /* namespace danjo */
