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

#include "biblinternes/structures/chaine.hh"

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/pile.hh"
#include "biblinternes/structures/tableau.hh"

#include "../danjo.h"

#include "controles_proprietes/donnees_controle.h"

#include "morceaux.h"

class QBoxLayout;
class QGridLayout;
class QHBoxLayout;
class QLayout;
class QMenu;
class QTabWidget;
class QToolBar;
class QVBoxLayout;

namespace danjo {

class Action;
class Bouton;
class ControlePropriete;

/* ------------------------------------------------------------------------- */
/** \name Maçonnes
 * \{ */

class MaçonneDispositionLigne;
class MaçonneDispositionColonne;
class MaçonneDispositionGrille;

struct ContexteMaçonnage {
    ConteneurControles *conteneur = nullptr;
    Manipulable *manipulable = nullptr;
};

class MaçonneDisposition {
    ContexteMaçonnage *m_ctx = nullptr;
    MaçonneDisposition *m_parent = nullptr;

  protected:
    MaçonneDisposition(ContexteMaçonnage *ctx, MaçonneDisposition *parent);

    ControlePropriete *crée_controle_propriété(DonneesControle const &données_controle,
                                               BasePropriete *prop);

    ControlePropriete *crée_étiquette(std::string_view nom);

    ControlePropriete *crée_étiquette_activable(std::string_view nom, BasePropriete *prop);

    MaçonneDispositionLigne *crée_maçonne_ligne();

    MaçonneDispositionColonne *crée_maçonne_colonne();

    MaçonneDispositionGrille *crée_maçonne_grille();
};

class MaçonneDispositionLigne : public MaçonneDisposition {
    QHBoxLayout *m_layout = nullptr;

  public:
    MaçonneDispositionLigne(ContexteMaçonnage *ctx, MaçonneDisposition *parent);

    MaçonneDispositionLigne *débute_ligne();

    MaçonneDispositionColonne *débute_colonne();

    MaçonneDispositionGrille *débute_grille();

    void ajoute_controle(DonneesControle const &données_controle, BasePropriete *prop);

    void ajoute_etiquette(std::string_view nom);
    void ajoute_étiquette_activable(std::string_view nom, BasePropriete *prop);

    void ajoute_espaceur(int taille);

    QLayout *donne_layout();
};

class MaçonneDispositionColonne : public MaçonneDisposition {
    QVBoxLayout *m_layout = nullptr;

  public:
    MaçonneDispositionColonne(ContexteMaçonnage *ctx, MaçonneDisposition *parent);

    MaçonneDispositionLigne *débute_ligne();

    MaçonneDispositionColonne *débute_colonne();

    MaçonneDispositionGrille *débute_grille();

    void ajoute_controle(DonneesControle const &données_controle, BasePropriete *prop);

    void ajoute_etiquette(std::string_view nom);
    void ajoute_étiquette_activable(std::string_view nom, BasePropriete *prop);

    void ajoute_espaceur(int taille);

    QLayout *donne_layout();
};

class MaçonneDispositionGrille : public MaçonneDisposition {
    QGridLayout *m_layout = nullptr;

  public:
    MaçonneDispositionGrille(ContexteMaçonnage *ctx, MaçonneDisposition *parent);

    MaçonneDispositionLigne *débute_ligne(int ligne,
                                          int colonne,
                                          int empan_ligne,
                                          int empan_colonne);

    MaçonneDispositionColonne *débute_colonne(int ligne,
                                              int colonne,
                                              int empan_ligne,
                                              int empan_colonne);

    MaçonneDispositionGrille *débute_grille(int ligne,
                                            int colonne,
                                            int empan_ligne,
                                            int empan_colonne);

    void ajoute_controle(DonneesControle const &données_controle,
                         BasePropriete *prop,
                         int ligne,
                         int colonne,
                         int empan_ligne,
                         int empan_colonne);

    void ajoute_etiquette(
        std::string_view nom, int ligne, int colonne, int empan_ligne, int empan_colonne);

    void ajoute_étiquette_activable(std::string_view nom,
                                    BasePropriete *prop,
                                    int ligne,
                                    int colonne,
                                    int empan_ligne,
                                    int empan_colonne);

    void ajoute_espaceur(int taille, int ligne, int colonne, int empan_ligne, int empan_colonne);

    QLayout *donne_layout();
};

/** \} */

/**
 * La classe AssembleurDisposition s'occupe de créer l'entreface en fonction de
 * ce que l'analyseur lui dit.
 *
 * L'assembleur met en place les connections entre signaux et slots des
 * contrôles et de leur conteneur.
 */
class AssembleurDisposition {
    dls::pile<QBoxLayout *> m_pile_dispositions{};
    dls::pile<QMenu *> m_pile_menus{};

    dls::tableau<std::pair<dls::chaine, QMenu *>> m_donnees_menus{};

    Action *m_derniere_action = nullptr;
    Bouton *m_dernier_bouton = nullptr;
    QMenu *m_menu_racine = nullptr;

    DonneesInterface m_donnees{};

    QTabWidget *m_dernier_dossier = nullptr;

    QToolBar *m_barre_outils = nullptr;

    dls::chaine m_nom = "";

    int m_temps = 0;
    bool m_initialisation_seule = false;

    id_morceau m_identifiant_donnees_controle = id_morceau::INCONNU;
    DonneesControle m_donnees_controle{};

  public:
    dls::dico_desordonne<dls::chaine, ControlePropriete *> controles{};

    /**
     * Construit une instance d'un assembleur avec les paramètres spécifiés.
     *
     * Le manipulable est l'objet qui contient les propriétés exposées dans
     * l'entreface.
     *
     * Le repondant_bouton est l'objet qui doit répondre des cliques de bouton.
     *
     * Le conteneur est l'objet qui soit détient le manipulable, soit répond
     * aux changements des contrôles exposés dans l'entreface.
     */
    explicit AssembleurDisposition(const DonneesInterface &donnees,
                                   int temps = 0,
                                   bool initialisation_seule = false);

    AssembleurDisposition(AssembleurDisposition const &) = default;
    AssembleurDisposition &operator=(AssembleurDisposition const &) = default;

    /**
     * Ajoute une disposition (ligne ou colonne) à la pile de disposition.
     */
    void ajoute_disposition(id_morceau identifiant);

    void ajoute_étiquette(dls::chaine texte);

    void ajoute_controle_pour_propriété(const DonneesControle &donnees, BasePropriete *prop);

    /**
     * Ajoute un contrôle à la disposition se trouvant au sommet de la pile.
     */
    void ajoute_controle(id_morceau identifiant);

    /**
     * Ajoute une item à un contrôle de type liste déroulante. Le dernier
     * contrôle ajouté via ajoute_controle est considéré comme étant une liste
     * déroulante.
     */
    void ajoute_item_liste(const dls::chaine &nom, const dls::chaine &valeur);

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
    void sors_disposition();

    /**
     * Ajoute une propriété au dernier contrôle ajouté.
     */
    void propriete_controle(id_morceau identifiant, const dls::chaine &valeur);

    /**
     * Ajoute une propriété au dernier bouton ajouté.
     */
    void propriete_bouton(id_morceau indentifiant, const dls::chaine &valeur);

    /**
     * Retourne la disposition se trouvant au sommet de la pile de dispositions.
     * Si aucune disposition n'existe dans la pile, retourne nullptr.
     */
    QBoxLayout *disposition();

    QMenu *menu();

    void ajoute_menu(const dls::chaine &nom);

    void sort_menu();

    void ajoute_action();

    void propriete_action(id_morceau identifiant, const dls::chaine &valeur);

    void ajoute_separateur();

    /**
     * Ajoute un dossier à la disposition se trouvant au sommet de la pile.
     */
    void ajoute_dossier();

    /**
     * Achève la création du dernier dossier ajouté.
     */
    void finalise_dossier();

    /**
     * Ajoute un onglet au dernier dossier ajouté. La disposition de l'onglet
     * est mise sur la pile de dispositions.
     */
    void ajoute_onglet(const dls::chaine &nom);

    /**
     * Achève la création du dernier onglet ajouté. La disposition de l'onglet
     * est enlevé de la pile de dispositions.
     */
    void finalise_onglet();

    /**
     * Établie le nom de la disposition étant créée.
     */
    void nom_disposition(const dls::chaine &chaine);

    /**
     * Retourne le nom de la disposition créée.
     */
    dls::chaine nom_disposition() const;

    /**
     * Retourne un vecteur contenant des paires composées de noms des menus et
     * de ceux-ci.
     */
    const dls::tableau<std::pair<dls::chaine, QMenu *>> &donnees_menus() const;

    /**
     * Créé une nouvelle barre à outils qui remplace toute barre à outils
     * précédemment créée.
     */
    void ajoute_barre_outils();

    /**
     * Retourne un pointeur vers la barre à outils créée.
     */
    QToolBar *barre_outils() const;

    /**
     * Créé les controles pour les propriétés extras.
     */
    void cree_controles_proprietes_extra();
};

} /* namespace danjo */
