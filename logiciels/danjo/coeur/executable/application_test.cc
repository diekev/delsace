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

#include "application_test.h"

#include <iostream>

#include "biblinternes/outils/fichier.hh"

#include <QApplication>
#include <QMenuBar>
#include <QWidget>
#include <QBoxLayout>

/* ************************************************************************** */

WidgetTest::WidgetTest(QWidget *parent)
	: danjo::ConteneurControles(parent)
{}

void WidgetTest::ajourne_manipulable()
{
}

void WidgetTest::obtiens_liste(const dls::chaine &attache, dls::tableau<dls::chaine> &chaines)
{
	std::cerr << "Obtention de la liste pour l'attache : " << attache << '\n';

	chaines.pousse("image");
}

/* ************************************************************************** */

void RepondantBoutonTest::repond_clique(const dls::chaine &valeur, const dls::chaine &metadonnee)
{
	std::cerr << "Répondant au clique pour " << valeur
			  << ", avec métadonnée " << metadonnee << '\n';

	if (valeur == "redimension_image") {
		danjo::Manipulable manipulable;

		auto donnees = danjo::DonneesInterface{};
		donnees.manipulable = &manipulable;
		donnees.repondant_bouton = this;
		donnees.conteneur = nullptr;

		auto texte_entree = dls::contenu_fichier("exemples/redimension_image.jo");
		auto ok = danjo::montre_dialogue(donnees, texte_entree.c_str());

		if (ok) {
			std::cerr << "Dialogue accepté !\n";
			std::cerr << "Largeur : " << manipulable.evalue_entier("largeur") << '\n';
			std::cerr << "Hauteur : " << manipulable.evalue_entier("hauteur") << '\n';
		}
		else {
			std::cerr << "Dialogue rejeté !\n";
		}
	}
}

bool RepondantBoutonTest::evalue_predicat(const dls::chaine &valeur, const dls::chaine &metadonnee)
{
	std::cerr << "Évalue prédicat pour " << valeur
			  << ", avec métadonnée " << metadonnee << '\n';

	return true;
}

/* ************************************************************************** */

FenetreTest::FenetreTest()
	: QMainWindow()
{
	auto widget_test = new WidgetTest;

	danjo::Propriete prop;
	prop.type = danjo::TypePropriete::ENTIER;
	prop.valeur = 9;

	m_manipulable.ajoute_propriete_extra("prop_entier", prop);

	prop.type = danjo::TypePropriete::DECIMAL;
	prop.valeur = 16.2f;

	m_manipulable.ajoute_propriete_extra("prop_decimal", prop);

	auto donnees = danjo::DonneesInterface{};
	donnees.manipulable = &m_manipulable;
	donnees.repondant_bouton = &m_repondant;
	donnees.conteneur = widget_test;

	auto texte_entree = dls::contenu_fichier("exemples/disposition_test.jo");

	auto disposition = danjo::compile_entreface(donnees, texte_entree.c_str());
	widget_test->setLayout(disposition);

	setCentralWidget(widget_test);

	auto script_menu = dls::contenu_fichier("exemples/menu_fichier.jo");
	auto menu = danjo::compile_menu(donnees, script_menu.c_str());

	menuBar()->addMenu(menu);

	script_menu = dls::contenu_fichier("exemples/menu_edition.jo");
	menu = danjo::compile_menu(donnees, script_menu.c_str());

	menuBar()->addMenu(menu);
}

/* ************************************************************************** */

int main(int argc, char *argv[])
{
#if 1
	QApplication app(argc, argv);

	FenetreTest fenetre;
	fenetre.show();

	return app.exec();
#else
//	auto texte_logique = dls::contenu_fichier("exemples/redimension_image.dan");
//	danjo::compile_feuille_logique(texte_logique.c_str());

	danjo::Manipulable manipulable;
	danjo::initialise_entreface("exemples/redimension_image.dan", &manipulable);
#endif
}
