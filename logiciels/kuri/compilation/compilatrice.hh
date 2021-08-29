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

#include "biblinternes/moultfilage/synchrone.hh"

#include "parsage/gerante_chaine.hh"
#include "parsage/identifiant.hh"
#include "parsage/modules.hh"

#include "gestionnaire_code.hh"
#include "messagere.hh"
#include "structures.hh"
#include "tacheronne.hh"

struct ContexteLexage;
struct EspaceDeTravail;
struct OptionsDeCompilation;
struct Statistiques;

struct Compilatrice {
    dls::outils::Synchrone<TableIdentifiant> table_identifiants{};

    dls::outils::Synchrone<OrdonnanceuseTache> ordonnanceuse;

    dls::outils::Synchrone<GeranteChaine> gerante_chaine{};

    dls::outils::Synchrone<Messagere> messagere{};

    dls::outils::Synchrone<GestionnaireCode> gestionnaire_code{};

    /* Option pour pouvoir désactivé l'import implicite de Kuri dans les tests unitaires notamment.
     */
    bool importe_kuri = true;
    bool m_possede_erreur = false;
    erreur::Genre m_code_erreur{};
    bool active_tests = false;

    template <typename T>
    using tableau_synchrone = dls::outils::Synchrone<kuri::tableau<T, int>>;

    tableau_synchrone<kuri::chaine> chaines_ajoutees_a_la_compilation{};

    tableau_synchrone<EspaceDeTravail *> espaces_de_travail{};
    EspaceDeTravail *espace_de_travail_defaut = nullptr;

    kuri::chaine racine_kuri{};

    dls::outils::Synchrone<SystemeModule> sys_module{};

    /* ********************************************************************** */

    Compilatrice();

    /* ********************************************************************** */

    /* Désactive la copie, car il ne peut y avoir qu'un seul contexte par
     * compilation. */
    Compilatrice(const Compilatrice &) = delete;
    Compilatrice &operator=(const Compilatrice &) = delete;

    ~Compilatrice();

    /* ********************************************************************** */

    /**
     * Charge le module dont le nom est spécifié.
     *
     * Le nom doit être celui d'un fichier s'appelant '<nom>.kuri' et se trouvant
     * dans le dossier du module racine.
     *
     * Les fonctions contenues dans le module auront leurs noms préfixés par le nom
     * du module, sauf pour le module racine.
     *
     * Le std::ostream est un flux de sortie où sera imprimé le nom du module ouvert
     * pour tenir compte de la progression de la compilation. Si un nom de module ne
     * pointe pas vers un fichier Kuri, ou si le fichier ne peut être ouvert, une
     * exception est lancée.
     *
     * Les Lexeme doivent être celles du nom du module et sont utilisées
     * pour les erreurs lancées.
     *
     * Le paramètre est_racine ne doit être vrai que pour le module racine.
     */
    Module *importe_module(EspaceDeTravail *espace,
                           kuri::chaine const &nom,
                           NoeudExpression const *site);

    /* ********************************************************************** */

    void ajoute_fichier_a_la_compilation(EspaceDeTravail *espace,
                                         kuri::chaine const &chemin,
                                         Module *module,
                                         NoeudExpression const *site);

    /* ********************************************************************** */

    EspaceDeTravail *demarre_un_espace_de_travail(OptionsDeCompilation const &options,
                                                  kuri::chaine const &nom);

    /* ********************************************************************** */

    ContexteLexage contexte_lexage();

    long memoire_utilisee() const;

    void rassemble_statistiques(Statistiques &stats) const;

    void rapporte_erreur(EspaceDeTravail const *espace,
                         kuri::chaine_statique message,
                         erreur::Genre genre);

    bool possede_erreur() const
    {
        return m_possede_erreur;
    }

    bool possede_erreur(EspaceDeTravail const *espace) const;

    erreur::Genre code_erreur() const
    {
        return m_code_erreur;
    }

  public:
    OptionsDeCompilation *options_compilation();
    void ajourne_options_compilation(OptionsDeCompilation *options);
    void ajoute_chaine_compilation(EspaceDeTravail *espace, kuri::chaine_statique c);
    void ajoute_chaine_au_module(EspaceDeTravail *espace, Module *module, kuri::chaine_statique c);
    void ajoute_fichier_compilation(EspaceDeTravail *espace, kuri::chaine_statique c);
    const Message *attend_message();
    EspaceDeTravail *espace_defaut_compilation();
    kuri::tableau<kuri::Lexeme> lexe_fichier(kuri::chaine_statique chemin_donne,
                                             const NoeudExpression *site);
};

int fonction_test_variadique_externe(int sentinel, ...);
