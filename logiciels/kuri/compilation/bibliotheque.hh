/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "biblinternes/structures/tableau_page.hh"
#include "biblinternes/systeme_fichier/shared_library.h"

#include "parsage/identifiant.hh"

#include "structures/chaine.hh"
#include "structures/chemin_systeme.hh"
#include "structures/ensemble.hh"
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

enum class ÉtatRechercheSymbole : unsigned char {
    NON_RECHERCHÉ,
    TROUVÉ,
    INTROUVÉ,
};

enum class RaisonRechercheSymbole : unsigned char {
    /* Le symbole doit être chargé pour être utilisée dans la machine virtuelle. */
    EXÉCUTION_MÉTAPROGRAMME,
    /* Le symbole doit être chargé pour la liaison du programme final. */
    LIAISON_PROGRAMME_FINAL,
};

struct Symbole {
    using type_adresse_fonction = void (*)();
    using type_adresse_objet = void *;

    Bibliothèque *bibliothèque = nullptr;
    kuri::chaine nom = "";
    ÉtatRechercheSymbole état_recherche = ÉtatRechercheSymbole::NON_RECHERCHÉ;

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

enum class ÉtatRechercheBibliothèque : unsigned char {
    NON_RECHERCHÉE,
    TROUVÉE,
    INTROUVÉE,
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

    NUM_TYPES_BIBLIOTHÈQUE,
};

enum {
    POUR_PRODUCTION,
    POUR_PROFILAGE,
    POUR_DÉBOGAGE,
    POUR_DÉBOGAGE_ASAN,

    NUM_TYPES_INFORMATION_BIBLIOTHÈQUE,
};

struct Bibliothèque {
    /* l'identifiant qui sera utilisé après les directives #externe, défini via ident ::
     * #bibliothèque "nom" */
    IdentifiantCode *ident = nullptr;

    kuri::chaine_statique nom = "";

    NoeudExpression *site = nullptr;
    dls::systeme_fichier::shared_library bib{};

    ÉtatRechercheBibliothèque état_recherche = ÉtatRechercheBibliothèque::NON_RECHERCHÉE;

    kuri::chemin_systeme chemins_de_base[NUM_TYPES_PLATEFORME] = {};
    kuri::chemin_systeme chemins[NUM_TYPES_PLATEFORME][NUM_TYPES_BIBLIOTHÈQUE]
                                [NUM_TYPES_INFORMATION_BIBLIOTHÈQUE] = {};

    kuri::chaine noms[NUM_TYPES_INFORMATION_BIBLIOTHÈQUE] = {};

    kuri::tableau_compresse<Bibliothèque *, int> dépendances{};
    kuri::tableau_compresse<Bibliothèque *, int> prépendances{};
    tableau_page<Symbole> symboles{};

    Symbole *crée_symbole(kuri::chaine_statique nom_symbole, TypeSymbole type);

    bool charge(EspaceDeTravail *espace);

    int64_t mémoire_utilisée() const;

    void ajoute_dépendance(Bibliothèque *dépendance);

    kuri::chaine_statique chemin_de_base(OptionsDeCompilation const &options) const;
    kuri::chaine_statique chemin_statique(OptionsDeCompilation const &options) const;
    kuri::chaine_statique chemin_dynamique(OptionsDeCompilation const &options) const;
    kuri::chaine_statique nom_pour_liaison(OptionsDeCompilation const &options) const;

    bool peut_lier_statiquement() const
    {
        return chemins[PLATEFORME_64_BIT][STATIQUE][POUR_PRODUCTION] != kuri::chemin_systeme("") &&
               nom != "c";
    }
};

/* ------------------------------------------------------------------------- */
/** \name BibliothèquesUtilisées
 * Représentation des bibliothèques utilisées par un programme. Outre le
 * stockage des bibliothèques utilisées, cette structure sers également à
 * déterminer si une bibliothèque peut être liée statiquement.
 * \{ */

struct BibliothèquesUtilisées {
  private:
    kuri::tableau<Bibliothèque *> m_bibliothèques{};
    kuri::ensemble<Bibliothèque *> m_ensemble{};

  public:
    BibliothèquesUtilisées();

    BibliothèquesUtilisées(kuri::ensemble<Bibliothèque *> const &ensemble);

    kuri::tableau_statique<Bibliothèque *> donne_tableau() const;

    int64_t mémoire_utilisée() const;

    void efface();

    bool peut_lier_statiquement(Bibliothèque const *bibliothèque) const;
};

/** \} */

struct GestionnaireBibliothèques {
    Compilatrice &compilatrice;
    tableau_page<Bibliothèque> bibliothèques{};

    GestionnaireBibliothèques(Compilatrice &compilatrice_);

    /**
     * Charge les bibliothèques requises pour l'exécution des métaprogrammes.
     * Retourne vrai si les bibliothèques ont pû être chargés.
     */
    static bool initialise_bibliothèques_pour_exécution(Compilatrice &compilatrice);

    Bibliothèque *trouve_bibliothèque(IdentifiantCode *ident);

    Bibliothèque *trouve_ou_crée_bibliothèque(EspaceDeTravail &espace, IdentifiantCode *ident);

    Bibliothèque *crée_bibliothèque(EspaceDeTravail &espace, NoeudExpression *site);

    Bibliothèque *crée_bibliothèque(EspaceDeTravail &espace,
                                    NoeudExpression *site,
                                    IdentifiantCode *ident,
                                    kuri::chaine_statique nom);

    int64_t mémoire_utilisée() const;

    void rassemble_statistiques(Statistiques &stats) const;

  private:
    void résoud_chemins_bibliothèque(EspaceDeTravail &espace,
                                     NoeudExpression *site,
                                     Bibliothèque *bibliotheque);
};

void *notre_malloc(size_t n);

void *notre_realloc(void *ptr, size_t taille);

void notre_free(void *ptr);
