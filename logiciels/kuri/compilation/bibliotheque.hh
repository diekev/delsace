/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

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
};

enum class RaisonRechercheSymbole : unsigned char {
    /* Le symbole doit être chargé pour être utilisée dans la machine virtuelle. */
    EXECUTION_METAPROGRAMME,
    /* Le symbole doit être chargé pour la liaison du programme final. */
    LIAISON_PROGRAMME_FINAL,
};

struct Symbole {
    using type_fonction = void (*)();

    Bibliotheque *bibliotheque = nullptr;
    kuri::chaine nom = "";
    EtatRechercheSymbole etat_recherche = EtatRechercheSymbole::NON_RECHERCHE;

    /* L'adresse pour la liaison du programme final. */
    type_fonction adresse_liaison = nullptr;
    /* L'adresse pour l'exécution d'un métaprogramme. Elle peut-être différente de #adresse_liaison
     * si nous le remplaçons par l'une de nos fonctions. */
    type_fonction adresse_execution = nullptr;

    bool charge(EspaceDeTravail *espace,
                NoeudExpression const *site,
                RaisonRechercheSymbole raison);

    /* Renseigne une adresse spécifique à utilisée pour l'exécution de métaprogrammes. */
    void adresse_pour_execution(type_fonction pointeur);

    /* Retourne l'adresse à utilisée pour l'exécution de métaprogramme : soit l'adresse chargée
     * depuis la bibliothèque, soit l'adresse mise en place par #adresse_pour_execution(adresse) si
     * elle existe. */
    type_fonction adresse_pour_execution();
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

    kuri::chaine chemins_de_base[NUM_TYPES_PLATEFORME] = {};
    kuri::chaine chemins[NUM_TYPES_PLATEFORME][NUM_TYPES_BIBLIOTHEQUE]
                        [NUM_TYPES_INFORMATION_BIBLIOTHEQUE] = {};

    kuri::chaine noms[NUM_TYPES_INFORMATION_BIBLIOTHEQUE] = {};

    kuri::tableau_compresse<Bibliotheque *, int> dependances{};
    tableau_page<Symbole> symboles{};

    Symbole *cree_symbole(kuri::chaine_statique nom_symbole);

    bool charge(EspaceDeTravail *espace);

    long memoire_utilisee() const;

    kuri::chaine_statique chemin_de_base(OptionsDeCompilation const &options) const;
    kuri::chaine_statique chemin_statique(OptionsDeCompilation const &options) const;
    kuri::chaine_statique chemin_dynamique(OptionsDeCompilation const &options) const;
    kuri::chaine_statique nom_pour_liaison(OptionsDeCompilation const &options) const;

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
