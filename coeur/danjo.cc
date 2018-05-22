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

#include <QMenu>
#include <QToolBar>

#include "interne/action.h"
#include "interne/analyseuse_disposition.h"
#include "interne/analyseuse_logique.h"
#include "interne/assembleuse_disposition.h"
#include "interne/decoupeuse.h"

#include "erreur.h"
#include "manipulable.h"

namespace danjo {

std::string contenu_fichier(const std::experimental::filesystem::path &chemin)
{
	if (!std::experimental::filesystem::exists(chemin)) {
		return "";
	}

	std::ifstream entree;
	entree.open(chemin.c_str());

	std::string contenu((std::istreambuf_iterator<char>(entree)),
						(std::istreambuf_iterator<char>()));

	return contenu;
}

QBoxLayout *compile_interface(DonneesInterface &donnnes, const char *texte_entree)
{
	if (donnnes.manipulable == nullptr) {
		return nullptr;
	}

	AssembleurDisposition assembleur(
				donnnes.manipulable,
				donnnes.repondant_bouton,
				donnnes.conteneur);

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

	donnnes.manipulable->initialise();

	return assembleur.disposition();
}

QBoxLayout *compile_interface(
		DonneesInterface &donnnes,
		const std::experimental::filesystem::path &chemin_texte)
{
	if (donnnes.manipulable == nullptr) {
		return nullptr;
	}

	const auto texte_entree = contenu_fichier(chemin_texte.c_str());
	return compile_interface(donnnes, texte_entree.c_str());
}

QMenu *compile_menu(DonneesInterface &donnnes, const char *texte_entree)
{
	AssembleurDisposition assembleur(
				donnnes.manipulable,
				donnnes.repondant_bouton,
				donnnes.conteneur);

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

	return assembleur.menu();
}

GestionnaireInterface::~GestionnaireInterface()
{
	for (const auto &donnees : m_menus) {
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

QMenu *GestionnaireInterface::compile_menu(DonneesInterface &donnnes, const char *texte_entree)
{
	AssembleurDisposition assembleur(
				donnnes.manipulable,
				donnnes.repondant_bouton,
				donnnes.conteneur);

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

QBoxLayout *GestionnaireInterface::compile_interface(DonneesInterface &donnnes, const char *texte_entree)
{
	if (donnnes.manipulable == nullptr) {
		return nullptr;
	}

	AssembleurDisposition assembleur(
				donnnes.manipulable,
				donnnes.repondant_bouton,
				donnnes.conteneur);

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

	auto dispostion = assembleur.disposition();
	auto nom = assembleur.nom_disposition();

	m_dispositions.insert({nom, dispostion});

	donnnes.manipulable->initialise();

	return dispostion;
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

QToolBar *GestionnaireInterface::compile_barre_outils(DonneesInterface &donnnes, const char *texte_entree)
{
	AssembleurDisposition assembleur(
				donnnes.manipulable,
				donnnes.repondant_bouton,
				donnnes.conteneur);

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

void compile_feuille_logique(const char *texte_entree)
{
	AnalyseuseLogique analyseuse;
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

void genere_fichier_dan(const std::experimental::filesystem::path &chemin)
{
	try {
		auto texte = contenu_fichier(chemin);

		Decoupeuse decoupeuse(texte.c_str());
		decoupeuse.decoupe();

		auto debut = decoupeuse.morceaux().begin();
		auto fin = decoupeuse.morceaux().end();

		if (debut->identifiant != IDENTIFIANT_DISPOSITION) {
			return;
		}

		auto chemin_dan = chemin;
		chemin_dan.replace_extension("dan");

		std::ofstream fichier;
		fichier.open(chemin_dan.c_str());

		std::ostream &os = fichier;

		++debut;

		auto nom_disposition = debut->contenu;

		os << "feuille \"" << nom_disposition << "\" {\n";
		os << "\tinterface {\n";

		auto nom_propriete = std::string("");
		auto valeur_propriete = std::string("");

		while (debut++ != fin) {
			if (!est_identifiant_controle(debut->identifiant)) {
				continue;
			}

			if (debut->identifiant == IDENTIFIANT_ETIQUETTE) {
				continue;
			}

			auto identifiant = debut->identifiant;

			nom_propriete = "";
			valeur_propriete = "";

			while (debut->identifiant != IDENTIFIANT_PARENTHESE_FERMANTE) {
				if (debut->identifiant == IDENTIFIANT_VALEUR) {
					++debut;
					++debut;

					valeur_propriete = debut->contenu;
				}
				else if (debut->identifiant == IDENTIFIANT_ATTACHE) {
					++debut;
					++debut;

					nom_propriete = debut->contenu;
				}

				++debut;
			}

			os << "\t\t" << nom_propriete << ":";

			switch (identifiant) {
				case IDENTIFIANT_COULEUR:
					os << "couleur(" << valeur_propriete << ");\n";
					break;
				case IDENTIFIANT_VECTEUR:
					os << "vecteur(" << valeur_propriete << ");\n";
					break;
				case IDENTIFIANT_FICHIER_ENTREE:
				case IDENTIFIANT_FICHIER_SORTIE:
				case IDENTIFIANT_CHAINE:
				case IDENTIFIANT_LISTE:
					os << "\"" << valeur_propriete << "\";\n";
					break;
				default:
					os << valeur_propriete << ";\n";
					break;
			}
		}

		os << "\t}\n";
		os << "}";
	}
	catch (const ErreurFrappe &e) {
		std::cerr << e.quoi();
	}
}

}  /* namespace danjo */
