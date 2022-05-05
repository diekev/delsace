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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/tableau_page.hh"
#include "biblinternes/systeme_fichier/shared_library.h"

#include "parsage/identifiant.hh"

#include "structures/chaine.hh"
#include "structures/tableau_compresse.hh"

struct Bibliotheque;
struct Compilatrice;
struct EspaceDeTravail;
struct NoeudExpression;
struct OptionsDeCompilation;
struct Statistiques;

enum class EtatRechercheSymbole : unsigned char {
    NON_RECHERCHE,
    TROUVE,
    INTROUVE,
    SURECRIS,
};

struct Symbole {
    using type_fonction = void (*)();

    Bibliotheque *bibliotheque = nullptr;
    kuri::chaine nom = "";
    EtatRechercheSymbole etat_recherche = EtatRechercheSymbole::NON_RECHERCHE;

    type_fonction ptr_fonction = nullptr;

    bool charge(EspaceDeTravail *espace, NoeudExpression const *site);

    void surecris_pointeur(type_fonction pointeur)
    {
        ptr_fonction = pointeur;
        /* puisque nous remplaçons certaines fonctions de la bibliothèque C pour les
         * métaprogrammes, il nous faut se souvenir de ceci afin que lorsque nous
         * aurons un lieur, le lieur charge le bon symbole de la bibliothèque */
        etat_recherche = EtatRechercheSymbole::SURECRIS;
    }
};

enum class EtatRechercheBibliotheque : unsigned char {
    NON_RECHERCHEE,
    TROUVEE,
    INTROUVEE,
};

enum {
    PLATEFORME_32_BIT,
    PLATEFORME_64_BIT,

    NUM_TYPES_PLATEFORME,
};

enum {
    /* Bibliothèque statique (*.a sur Linux). */
    STATIQUE,
    /* Bibliothèque dynamique (*.so sur Linux). */
    DYNAMIQUE,

    NUM_TYPES_BIBLIOTHEQUE,
};

enum {
    POUR_PRODUCTION,
    POUR_PROFILAGE,
    POUR_DEBOGAGE,
    POUR_DEBOGAGE_ASAN,

    NUM_TYPES_INFORMATION_BIBLIOTHEQUE,
};

struct Bibliotheque {
    /* l'identifiant qui sera utilisé après les directives #externe, défini via ident ::
     * #bibliothèque "nom" */
    IdentifiantCode *ident = nullptr;

    kuri::chaine_statique nom = "";

    NoeudExpression *site = nullptr;
    dls::systeme_fichier::shared_library bib{};

    EtatRechercheBibliotheque etat_recherche = EtatRechercheBibliotheque::NON_RECHERCHEE;

    kuri::chaine chemins_de_base[NUM_TYPES_PLATEFORME];
    kuri::chaine chemins[NUM_TYPES_PLATEFORME][NUM_TYPES_BIBLIOTHEQUE]
                        [NUM_TYPES_INFORMATION_BIBLIOTHEQUE] = {};

    kuri::tableau_compresse<Bibliotheque *, int> dependances{};
    tableau_page<Symbole> symboles{};

    Symbole *cree_symbole(kuri::chaine_statique nom_symbole);

    bool charge(EspaceDeTravail *espace);

    long memoire_utilisee() const;

    kuri::chaine_statique chemin_de_base(OptionsDeCompilation const &options) const;
    kuri::chaine_statique chemin_statique(OptionsDeCompilation const &options) const;
    kuri::chaine_statique chemin_dynamique(OptionsDeCompilation const &options) const;

    bool peut_lier_statiquement() const
    {
        return chemins[STATIQUE][PLATEFORME_64_BIT][POUR_PRODUCTION] != "" && nom != "c";
    }
};

struct GestionnaireBibliotheques {
    Compilatrice &compilatrice;
    tableau_page<Bibliotheque> bibliotheques{};

    GestionnaireBibliotheques(Compilatrice &compilatrice_) : compilatrice(compilatrice_)
    {
    }

    Bibliotheque *trouve_bibliotheque(IdentifiantCode *ident);

    Bibliotheque *trouve_ou_cree_bibliotheque(EspaceDeTravail &espace, IdentifiantCode *ident);

    Bibliotheque *cree_bibliotheque(EspaceDeTravail &espace, NoeudExpression *site);

    Bibliotheque *cree_bibliotheque(EspaceDeTravail &espace,
                                    NoeudExpression *site,
                                    IdentifiantCode *ident,
                                    kuri::chaine_statique nom);

    long memoire_utilisee() const;

    void rassemble_statistiques(Statistiques &stats) const;

  private:
    void resoud_chemins_bibliotheque(EspaceDeTravail &espace,
                                     NoeudExpression *site,
                                     Bibliotheque *bibliotheque);
};
