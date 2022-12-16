/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "structures/ensemble.hh"
#include "structures/tableau.hh"

#include "message.hh"  // pour PhaseCompilation

struct AtomeGlobale;
struct AtomeFonction;
struct Coulisse;
struct EspaceDeTravail;
struct Fichier;
struct MetaProgramme;
struct NoeudDeclaration;
struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationVariable;
struct Statistiques;
struct Type;

/* Machine à état pour la PhaseCompilation. */
class EtatCompilation {
    PhaseCompilation m_phase_courante{};

  public:
    void avance_etat()
    {
        if (m_phase_courante == PhaseCompilation::COMPILATION_TERMINEE) {
            return;
        }

        const auto index_phase_courante = static_cast<int>(m_phase_courante);
        const auto index_phase_suivante = index_phase_courante + 1;
        m_phase_courante = static_cast<PhaseCompilation>(index_phase_suivante);
    }

    void essaie_d_aller_a(PhaseCompilation cible)
    {
        if (static_cast<int>(cible) != static_cast<int>(m_phase_courante) + 1) {
            return;
        }

        m_phase_courante = cible;
    }

    void recule_vers(PhaseCompilation cible)
    {
        m_phase_courante = cible;
    }

    PhaseCompilation phase_courante() const
    {
        return m_phase_courante;
    }
};

struct DiagnostiqueEtatCompilation {
    bool tous_les_fichiers_sont_charges = false;
    bool tous_les_fichiers_sont_lexes = false;
    bool tous_les_fichiers_sont_parses = false;
    bool toutes_les_declarations_a_typer_le_sont = false;
    bool toutes_les_ri_sont_generees = false;

    Type *type_a_valider = nullptr;
    NoeudDeclaration *declaration_a_valider = nullptr;

    Type *ri_type_a_generer = nullptr;
    Type *fonction_initialisation_type_a_creer = nullptr;
    NoeudDeclaration *ri_declaration_a_generer = nullptr;
};

void imprime_diagnostique(DiagnostiqueEtatCompilation const &diagnositic);

/* Représentation d'un programme. Ceci peut être le programme final tel que généré par la
 * compilation ou bien un métaprogramme. Il contient toutes les globales et tous les types utilisés
 * par les fonctions qui le composent. */
struct Programme {
  protected:
    kuri::tableau<NoeudDeclarationEnteteFonction *> m_fonctions{};
    kuri::ensemble<NoeudDeclarationEnteteFonction *> m_fonctions_utilisees{};

    /* Fonctions dont les dépendances sont potentiellement manquantes. */
    kuri::ensemble<NoeudDeclaration *> m_dépendances_manquantes{};

    kuri::tableau<NoeudDeclarationVariable *> m_globales{};
    kuri::ensemble<NoeudDeclarationVariable *> m_globales_utilisees{};

    kuri::tableau<Type *> m_types{};
    kuri::ensemble<Type *> m_types_utilises{};

    /* Tous les fichiers utilisés dans le programme. */
    kuri::tableau<Fichier *> m_fichiers{};
    kuri::ensemble<Fichier *> m_fichiers_utilises{};
    mutable bool m_fichiers_sont_sales = true;

    EspaceDeTravail *m_espace = nullptr;

    /* Non-nul si le programme est celui d'un métaprogramme. */
    MetaProgramme *m_pour_metaprogramme = nullptr;

    // la coulisse à utiliser pour générer le code du programme
    Coulisse *m_coulisse = nullptr;

    EtatCompilation m_etat_compilation{};

    enum {
        TYPES,
        FONCTIONS,
        GLOBALES,

        NOMBRE_ELEMENTS,
    };

    enum {
        POUR_TYPAGE,
        POUR_RI,

        NOMBRE_RAISONS,
    };

    mutable bool elements_sont_sales[NOMBRE_ELEMENTS][NOMBRE_RAISONS];

  public:
    /* Création. */

    static Programme *cree(EspaceDeTravail *espace);

    static Programme *cree_pour_espace(EspaceDeTravail *espace);

    static Programme *cree_pour_metaprogramme(EspaceDeTravail *espace,
                                              MetaProgramme *metaprogramme);

    void change_d_espace(EspaceDeTravail *espace_)
    {
        m_espace = espace_;
    }

    /* Modifications. */

    void ajoute_fonction(NoeudDeclarationEnteteFonction *fonction);

    void ajoute_globale(NoeudDeclarationVariable *globale);

    void ajoute_type(Type *type);

    bool possede(NoeudDeclarationEnteteFonction *fonction) const
    {
        return m_fonctions_utilisees.possede(fonction);
    }

    bool possede(NoeudDeclarationVariable *globale) const
    {
        return m_globales_utilisees.possede(globale);
    }

    bool possede(Type *type) const
    {
        return m_types_utilises.possede(type);
    }

    kuri::tableau<NoeudDeclarationEnteteFonction *> const &fonctions() const
    {
        return m_fonctions;
    }

    kuri::tableau<NoeudDeclarationVariable *> const &globales() const
    {
        return m_globales;
    }

    kuri::tableau<Type *> const &types() const
    {
        return m_types;
    }

    /* Retourne vrai si toutes les fonctions, toutes les globales, et tous les types utilisés par
     * le programme ont eu leurs types validés. */
    bool typages_termines(DiagnostiqueEtatCompilation &diagnositique) const;

    /* Retourne vrai si toutes les fonctions, toutes les globales, et tous les types utilisés par
     * le programme ont eu leurs RI générées. */
    bool ri_generees(DiagnostiqueEtatCompilation &diagnositique) const;

    bool ri_generees() const;

    MetaProgramme *pour_metaprogramme() const
    {
        return m_pour_metaprogramme;
    }

    void ajoute_racine(NoeudDeclarationEnteteFonction *racine);

    Coulisse *coulisse() const
    {
        return m_coulisse;
    }

    EspaceDeTravail *espace() const
    {
        return m_espace;
    }

    DiagnostiqueEtatCompilation diagnositique_compilation() const;

    EtatCompilation ajourne_etat_compilation();

    void change_de_phase(PhaseCompilation phase);

    long memoire_utilisee() const;

    void rassemble_statistiques(Statistiques &stats);

    kuri::ensemble<Module *> modules_utilises() const;

    void ajourne_pour_nouvelles_options_espace();

    kuri::ensemble<NoeudDeclaration *> &dépendances_manquantes();

  private:
    void verifie_etat_compilation_fichier(DiagnostiqueEtatCompilation &diagnostique) const;

    void ajoute_fichier(Fichier *fichier);
};

enum {
    IMPRIME_TOUT = 0,
    IMPRIME_TYPES = 1,
    IMPRIME_FONCTIONS = 2,
    IMPRIME_GLOBALES = 4,
};

void imprime_contenu_programme(Programme const &programme, unsigned int quoi, std::ostream &os);

/* La représentation intermédiaire des fonctions et globles contenues dans un Programme, ainsi que
 * tous les types utilisées. */
struct ProgrammeRepreInter {
    kuri::tableau<AtomeGlobale *> globales{};
    kuri::tableau<AtomeFonction *> fonctions{};
    kuri::tableau<Type *> types{};

    void ajoute_fonction(AtomeFonction *fonction);
    void ajourne_globales_pour_fonction(AtomeFonction *fonction);
};

void imprime_contenu_programme(const ProgrammeRepreInter &programme,
                               unsigned int quoi,
                               std::ostream &os);

ProgrammeRepreInter representation_intermediaire_programme(Programme const &programme);
