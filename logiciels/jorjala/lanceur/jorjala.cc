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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QApplication>
#include <QFile>
#include <QTimer>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "biblinternes/structures/chaine.hh"

#include "entreface/fenetre_principale.h"
#include "entreface/gestion_entreface.hh"

#include "coeur/jorjala.hh"

#include <iostream>

/*

  À FAIRE :

  EditriceLigneTemps :
    m_jorjala.ajourne_pour_nouveau_temps

  VueEditeurNoeud :
    genere_menu_noeuds_cycles

  Visionneuse2D
    m_jorjala.usine_operatrices
    m_jorjala.lcc->fonctions.table.taille
    m_jorjala.bdd.graphe_composites

  EditriceVue3D
    jorjala.type_manipulation_3d
    jorjala.manipulatrice_3d
    m_jorjala.bdd

  VisionneurScene
    cree_contexte_evaluation
    m_jorjala.bdd.rendu

  EditriceProprietes
    noeud->type

*/

int main(int argc, char *argv[])
{
    std::optional<JJL::Jorjala> jorjala_initialisé = initialise_jorjala();
    if (!jorjala_initialisé.has_value()) {
        return 1;
    }

    JJL::Jorjala jorjala = jorjala_initialisé.value();

    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("delsace");
    QCoreApplication::setApplicationName("jorjala");

    QFile file("styles/main.qss");

    if (file.open(QFile::ReadOnly)) {
        QString style_sheet = QLatin1String(file.readAll());
        qApp->setStyleSheet(style_sheet);
    }

    FenetrePrincipale w(jorjala);
    w.setWindowTitle(QCoreApplication::applicationName());
    w.showMaximized();

    if (argc == 2) {
        appele_commande(jorjala, "ouvrir_fichier", argv[1]);
    }

    auto résultat = a.exec();

    issitialise_jorjala(jorjala);

    return résultat;
}
