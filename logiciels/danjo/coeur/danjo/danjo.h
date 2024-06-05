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

#include <filesystem>

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/tableau.hh"

#include "repondant_bouton.h"

class QBoxLayout;
class QMenu;
class QToolBar;
class QWidget;

namespace danjo {

class ControlePropriete;
class ConteneurControles;
class Manipulable;

struct DonneesInterface {
    Manipulable *manipulable = nullptr;
    RepondantBouton *repondant_bouton = nullptr;
    ConteneurControles *conteneur = nullptr;
    QWidget *parent_barre_outils = nullptr;
    QWidget *parent_menu = nullptr;
};

struct DonneesAction {
    dls::chaine nom{};
    dls::chaine attache{};
    dls::chaine metadonnee{};
    RepondantBouton *repondant_bouton = nullptr;

    DonneesAction() = default;
    DonneesAction(DonneesAction const &) = default;
    DonneesAction &operator=(DonneesAction const &) = default;
};

class GestionnaireInterface {
    dls::dico_desordonne<dls::chaine, QMenu *> m_menus{};
    dls::dico_desordonne<dls::chaine, QBoxLayout *> m_dispositions{};
    dls::tableau<QToolBar *> m_barres_outils{};

    QWidget *m_parent_dialogue = nullptr;

    /* cache des controles pour ajourner l'interface */
    dls::dico_desordonne<dls::chaine, ControlePropriete *> m_controles{};

  public:
    ~GestionnaireInterface();

    void parent_dialogue(QWidget *p);

    void ajourne_menu(const dls::chaine &nom);

    void recree_menu(const dls::chaine &nom, const dls::tableau<DonneesAction> &donnees_actions);

    void ajourne_disposition(const dls::chaine &nom, int temps = 0);

    QMenu *compile_menu_texte(DonneesInterface &donnees, dls::chaine const &texte);

    QMenu *compile_menu_fichier(DonneesInterface &donnees, dls::chaine const &fichier);

    QBoxLayout *compile_entreface_texte(DonneesInterface &donnees,
                                        dls::chaine const &texte,
                                        int temps = 0);

    QBoxLayout *compile_entreface_fichier(DonneesInterface &donnees,
                                          dls::chaine const &texte,
                                          int temps = 0);

    void ajourne_entreface(Manipulable *manipulable);

    void initialise_entreface_texte(Manipulable *manipulable, dls::chaine const &texte);

    void initialise_entreface_fichier(Manipulable *manipulable, dls::chaine const &fichier);

    QMenu *pointeur_menu(const dls::chaine &nom);

    QToolBar *compile_barre_outils_texte(DonneesInterface &donnees, dls::chaine const &texte);

    QToolBar *compile_barre_outils_fichier(DonneesInterface &donnees, dls::chaine const &fichier);

    bool montre_dialogue_texte(DonneesInterface &donnees, dls::chaine const &texte);

    bool montre_dialogue_fichier(DonneesInterface &donnees, dls::chaine const &fichier);

    void ajoute_controle(dls::chaine identifiant, ControlePropriete *controle);

    void ajourne_controles();
};

QMenu *compile_menu(DonneesInterface &donnees, const char *texte_entree);

/**
 * Compile le script d'entreface contenu dans texte_entree, et retourne un
 * pointeur vers le QBoxLayout ainsi créé.
 */
QBoxLayout *compile_entreface(DonneesInterface &donnees, const char *texte_entree, int temps = 0);

/**
 * Compile le script d'entreface contenu dans le fichier dont le chemin est
 * spécifié, et retourne un pointeur vers le QBoxLayout ainsi créé.
 */
QBoxLayout *compile_entreface(DonneesInterface &donnees,
                              const std::filesystem::path &chemin_texte,
                              int temps = 0);

void compile_feuille_logique(const char *texte_entree);

/**
 * Initialise les propriétés d'un manipulable depuis un fichier .jo.
 */
void initialise_entreface(Manipulable *manipulable, const char *texte_entree);

bool montre_dialogue(DonneesInterface &donnees, const char *texte_entree);

} /* namespace danjo */
