/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "biblinternes/structures/tableau_page.hh"
#include "biblinternes/systeme_fichier/shared_library.h"

#include "parsage/identifiant.hh"

#include "structures/chaine.hh"
#include "structures/chemin_systeme.hh"
#include "structures/tableau_compresse.hh"

struct Bibliothèque;
struct Compilatrice;
struct EspaceDeTravail;
struct NoeudExpression;
struct OptionsDeCompilation;
struct Statistiques;

enum class TypeSymbole : uint8_t {
    INCONNU,
    /* Le symbole est une globale. */
    VARIABLE_GLOBALE,
    /* Le symbole est une fonction. */
    FONCTION,
};

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
    using type_adresse_fonction = void (*)();
    using type_adresse_objet = void *;

    Bibliothèque *bibliotheque = nullptr;
    kuri::chaine nom = "";
    EtatRechercheSymbole etat_recherche = EtatRechercheSymbole::NON_RECHERCHE;

  private:
    TypeSymbole type{};

    union type_adresse {
        type_adresse_fonction fonction = nullptr;
        type_adresse_objet objet;
    };

    /* L'adresse pour la liaison du programme final. */
    type_adresse adresse_liaison{};
    /* L'adresse pour l'exécution d'un métaprogramme. Elle peut-être différente de #adresse_liaison
     * si nous le remplaçons par l'une de nos fonctions. */
    type_adresse adresse_exécution{};

  public:
    Symbole(TypeSymbole type_) : type(type_)
    {
    }

    EMPECHE_COPIE(Symbole);

    bool charge(EspaceDeTravail *espace,
                NoeudExpression const *site,
                RaisonRechercheSymbole raison);

    /* Renseigne une adresse spécifique à utilisée pour l'exécution de métaprogrammes. */
    void définis_adresse_pour_exécution(type_adresse_fonction adresse);
    void définis_adresse_pour_exécution(type_adresse_objet adresse);

    /* Retourne l'adresse à utilisée pour l'exécution de métaprogramme : soit l'adresse chargée
     * depuis la bibliothèque, soit l'adresse mise en place par #adresse_pour_execution(adresse) si
     * elle existe.
     * La fonction a utilisée dépend du type du symbole. Retourne nul si aucune adresse existe pour
     * le type. */
    type_adresse_fonction donne_adresse_fonction_pour_exécution();
    type_adresse_objet donne_adresse_objet_pour_exécution();
};

enum class EtatRechercheBibliothèque : unsigned char {
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

struct Bibliothèque {
    /* l'identifiant qui sera utilisé après les directives #externe, défini via ident ::
     * #bibliothèque "nom" */
    IdentifiantCode *ident = nullptr;

    kuri::chaine_statique nom = "";

    NoeudExpression *site = nullptr;
    dls::systeme_fichier::shared_library bib{};

    EtatRechercheBibliothèque etat_recherche = EtatRechercheBibliothèque::NON_RECHERCHEE;

    kuri::chemin_systeme chemins_de_base[NUM_TYPES_PLATEFORME] = {};
    kuri::chemin_systeme chemins[NUM_TYPES_PLATEFORME][NUM_TYPES_BIBLIOTHEQUE]
                                [NUM_TYPES_INFORMATION_BIBLIOTHEQUE] = {};

    kuri::chaine noms[NUM_TYPES_INFORMATION_BIBLIOTHEQUE] = {};

    kuri::tableau_compresse<Bibliothèque *, int> dependances{};
    tableau_page<Symbole> symboles{};

    Symbole *crée_symbole(kuri::chaine_statique nom_symbole, TypeSymbole type);

    bool charge(EspaceDeTravail *espace);

    int64_t memoire_utilisee() const;

    kuri::chaine_statique chemin_de_base(OptionsDeCompilation const &options) const;
    kuri::chaine_statique chemin_statique(OptionsDeCompilation const &options) const;
    kuri::chaine_statique chemin_dynamique(OptionsDeCompilation const &options) const;
    kuri::chaine_statique nom_pour_liaison(OptionsDeCompilation const &options) const;

    bool peut_lier_statiquement() const
    {
        return chemins[STATIQUE][PLATEFORME_64_BIT][POUR_PRODUCTION] != "" && nom != "c";
    }
};

struct GestionnaireBibliothèques {
    Compilatrice &compilatrice;
    tableau_page<Bibliothèque> bibliotheques{};

    GestionnaireBibliothèques(Compilatrice &compilatrice_);

    /**
     * Charge les bibliothèques requises pour l'exécution des métaprogrammes.
     * Retourne vrai si les bibliothèques ont pû être chargés.
     */
    static bool initialise_bibliotheques_pour_execution(Compilatrice &compilatrice);

    Bibliothèque *trouve_bibliothèque(IdentifiantCode *ident);

    Bibliothèque *trouve_ou_crée_bibliothèque(EspaceDeTravail &espace, IdentifiantCode *ident);

    Bibliothèque *crée_bibliothèque(EspaceDeTravail &espace, NoeudExpression *site);

    Bibliothèque *crée_bibliothèque(EspaceDeTravail &espace,
                                    NoeudExpression *site,
                                    IdentifiantCode *ident,
                                    kuri::chaine_statique nom);

    int64_t memoire_utilisee() const;

    void rassemble_statistiques(Statistiques &stats) const;

  private:
    void resoud_chemins_bibliothèque(EspaceDeTravail &espace,
                                     NoeudExpression *site,
                                     Bibliothèque *bibliotheque);
};

void *notre_malloc(size_t n);

void *notre_realloc(void *ptr, size_t taille);

void notre_free(void *ptr);
