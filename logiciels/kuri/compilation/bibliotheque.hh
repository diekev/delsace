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
struct EspaceDeTravail;
struct NoeudExpression;
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

struct Bibliotheque {
    /* l'identifiant qui sera utilisé après les directives #externe, défini via ident ::
     * #bibliothèque "nom" */
    IdentifiantCode *ident = nullptr;

    kuri::chaine_statique nom = "";

    NoeudExpression *site = nullptr;
    dls::systeme_fichier::shared_library bib{};

    EtatRechercheBibliotheque etat_recherche = EtatRechercheBibliotheque::NON_RECHERCHEE;

    /* Le chemin pour une bibliothèque statique (*.a sur Linux). */
    kuri::chaine chemin_statique = "";

    /* Le chemin pour la version dynamique (*.so sur Linux). */
    kuri::chaine chemin_dynamique = "";

    kuri::tableau_compresse<Bibliotheque *, int> dependances{};
    tableau_page<Symbole> symboles{};

    Symbole *cree_symbole(kuri::chaine_statique nom_symbole);

    bool charge(EspaceDeTravail *espace);

    long memoire_utilisee() const;

    bool peut_lier_statiquement() const
    {
        return chemin_statique != "" && nom != "c";
    }
};

struct GestionnaireBibliotheques {
    EspaceDeTravail &espace;
    tableau_page<Bibliotheque> bibliotheques{};

    GestionnaireBibliotheques(EspaceDeTravail &espace_) : espace(espace_)
    {
    }

    Bibliotheque *trouve_bibliotheque(IdentifiantCode *ident);

    Bibliotheque *trouve_ou_cree_bibliotheque(IdentifiantCode *ident);

    Bibliotheque *cree_bibliotheque(NoeudExpression *site);

    Bibliotheque *cree_bibliotheque(NoeudExpression *site,
                                    IdentifiantCode *ident,
                                    kuri::chaine_statique nom);

    long memoire_utilisee() const;

    void rassemble_statistiques(Statistiques &stats) const;

  private:
    void resoud_chemins_bibliotheque(NoeudExpression *site, Bibliotheque *bibliotheque);
};
