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

#include "conteneur_controles.h"
#include "kangao.h"
#include "manipulable.h"
#include "repondant_bouton.h"

#include <iostream>
#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QWidget>
#include <QBoxLayout>

struct DonneesSorties {
	QLayout *disposition;
	QMenu *menu;
};

class WidgetTest : public ConteneurControles {
public:
	explicit WidgetTest(QWidget *parent = nullptr)
		: ConteneurControles(parent)
	{}

private Q_SLOTS:
	void ajourne_manipulable() override
	{
	}
};

class RepondantBoutonTest : public RepondantBouton {
public:
	void repond_clique(const std::string &valeur) override
	{
		std::cerr << "Répondant au clique pour " << valeur << '\n';
	}
};

class FenetreTest : public QMainWindow {
	Manipulable m_manipulable;
	RepondantBoutonTest m_repondant;

public:
	FenetreTest()
		: QMainWindow()
	{

		auto texte_entree = "disposition \"1disposition_début\" {\n"
							"    étiquette(valeur=\"Disposition\")\n"
							"    colonne {\n"
							"        ligne {\n"
							"            étiquette(valeur=\"Taille image\")\n"
							"            entier(valeur=\"6\"; min=\"0\"; max=\"200\"; infobulle=\"La taille de l'image sur l'axe des X.\"; attache=\"taille_x\")\n"
							"        }\n"
							"        ligne {\n"
							"            étiquette(valeur=\"Taille image Y\")\n"
							"            décimal(valeur=\"6\"; min=\"0\"; max=\"200\"; infobulle=\"La taille de l'image sur l'axe des X.\"; attache=\"taille_y\")\n"
							"        }\n"
							"        liste(valeur=\"exr\"; items=[{ nom=\"JPEG\", valeur=\"jpeg\"}, { nom=\"EXR\", valeur=\"exr\"}, { nom=\"PNG\", valeur=\"png\"}]; attache=\"liste\")\n"
							"        chaine(valeur=\"\"; attache=\"chaine\")\n"
							"        fichier_entrée(valeur=\"\"; attache=\"fichier_in\")\n"
							"        fichier_sortie(valeur=\"\"; attache=\"fichier_ex\")\n"
							"        vecteur(valeur=\"\"; attache=\"vecteur\")\n"
							"    }\n"
							"	dossier {\n"
							"       onglet \"Onglet Test\" {"
							"           étiquette(valeur=\"Youpi !\")\n"
							"       }\n"
							"       onglet \"Onglet Test 2\" {"
							"           étiquette(valeur=\"Youpi 2 !\")\n"
							"    ligne {\n"
							"        couleur(valeur=\"\"; attache=\"couleur\")\n"
							"        case(valeur=\"vrai\"; attache=\"case\")\n"
							"    }\n"
							"       }\n"
							"   }\n"
							"    ligne {\n"
							"        étiquette(valeur=\"Ligne 2\")\n"
							"        bouton(valeur=\"Bouton test\"; attache=\"attache_test\")\n"
							"        bouton(valeur=\"Bouton test 2\"; attache=\"attache_test2\")\n"
							"    }\n"
							"}";

		m_manipulable.ajoute_propriete("taille_x", TypePropriete::ENTIER);
		m_manipulable.ajoute_propriete("taille_y", TypePropriete::DECIMAL);
		m_manipulable.ajoute_propriete("liste", TypePropriete::ENUM);
		m_manipulable.ajoute_propriete("chaine", TypePropriete::CHAINE_CARACTERE);
		m_manipulable.ajoute_propriete("fichier_in", TypePropriete::FICHIER_ENTREE);
		m_manipulable.ajoute_propriete("fichier_ex", TypePropriete::FICHIER_SORTIE);
		m_manipulable.ajoute_propriete("couleur", TypePropriete::COULEUR);
		m_manipulable.ajoute_propriete("vecteur", TypePropriete::VECTEUR);
		m_manipulable.ajoute_propriete("case", TypePropriete::BOOL);

		auto widget_test = new WidgetTest;

		auto donnees = DonneesInterface();
		donnees.manipulable = &m_manipulable;
		donnees.repondant_bouton = &m_repondant;
		donnees.conteneur = widget_test;

		auto disposition = compile_interface(donnees, texte_entree);

		widget_test->setLayout(disposition);

		setCentralWidget(widget_test);

		auto script_menu = "menu \"Fichier\" {\n"
						   "	action(valeur=\"Nouveau fichier ou projet...\"; attache=\"nouveau_fichier\")\n"
						   "	action(valeur=\"Ouvrir un fichier ou projet...\"; attache=\"ouvrir_fichier\")\n"
						   "	menu \"Fichiers récents\" {\n"
						   "	}\n"
						   "	séparateur\n"
						   "	action(valeur=\"Sauvegarder...\"; attache=\"sauvegarder\")\n"
						   "	action(valeur=\"Sauvegarder sous...\"; attache=\"sauvegarder_sous\")\n"
						   "	séparateur\n"
						   "	action(valeur=\"Quitter...\"; attache=\"quitter\")\n"
						   "}";

		auto menu = compile_menu(donnees, script_menu);

		menuBar()->addMenu(menu);

		script_menu = "menu \"Édition\" {\n"
					  "	action(valeur=\"Annuler\"; attache=\"annuler\")\n"
					  "	action(valeur=\"Refaire\"; attache=\"refaire\")\n"
					  "	séparateur\n"
					  "	action(valeur=\"Coller depuis l'historique\"; attache=\"sauvegarder\")\n"
					  "	action(valeur=\"Couper\"; attache=\"couper\")\n"
					  "	action(valeur=\"Copier\"; attache=\"copier\")\n"
					  "	action(valeur=\"Coller\"; attache=\"coller\")\n"
					  "	séparateur\n"
					  "	action(valeur=\"Tout sélectionner\"; attache=\"tout_sélectionner\")\n"
					  "	séparateur\n"
					  "	menu \"Avancé\" {\n"
					  "   action(valeur=\"Autoindente la sélection\"; attache=\"auto_indente\")\n"
					  "   action(valeur=\"Autoindente la sélection\"; attache=\"auto_indente\")\n"
					  "}\n"
					  "	séparateur\n"
					  "	menu \"Trouver/Remplacer\" {}\n"
					  "	action(valeur=\"Aller à la ligne...\"; attache=\"aller_ligne\")\n"
					  "	action(valeur=\"Sélectionner encodage...\"; attache=\"encodage\")\n"
					  "}";

		menu = compile_menu(donnees, script_menu);

		menuBar()->addMenu(menu);
	}
};

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	FenetreTest fenetre;
	fenetre.show();

	return app.exec();
}
