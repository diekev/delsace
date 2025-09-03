/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "arbre_syntaxique/prodeclaration.hh"

#include "structures/ensemble.hh"
#include "structures/rassembleuse.hh"
#include "structures/tableau.hh"

#include <optional>

#include "message.hh"  // pour PhaseCompilation

struct AtomeGlobale;
struct AtomeFonction;
struct AtomeConstanteDonnéesConstantes;
struct Bibliothèque;
struct BibliothèquesUtilisées;
struct CompilatriceRI;
struct Coulisse;
struct Enchaineuse;
struct EspaceDeTravail;
struct Fichier;
struct MetaProgramme;
struct Statistiques;
using Type = NoeudDéclarationType;

namespace kuri {
struct chaine;
struct chemin_systeme;
}  // namespace kuri

/* ------------------------------------------------------------------------- */
/** \name ÉtatCompilation.
 * \{ */

/* Machine à état pour la PhaseCompilation. */
class ÉtatCompilation {
    PhaseCompilation m_phase_courante{};

  public:
    void avance_état()
    {
        if (m_phase_courante == PhaseCompilation::COMPILATION_TERMINÉE) {
            return;
        }

        const auto indice_phase_courante = static_cast<int>(m_phase_courante);
        const auto indice_phase_suivante = indice_phase_courante + 1;
        m_phase_courante = static_cast<PhaseCompilation>(indice_phase_suivante);
    }

    void essaie_d_aller_à(PhaseCompilation cible)
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

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Diagnostique état compilation.
 * \{ */

struct DiagnostiqueÉtatCompilation {
    bool tous_les_fichiers_sont_chargés = false;
    bool tous_les_fichiers_sont_lexés = false;
    bool tous_les_fichiers_sont_parsés = false;
    bool toutes_les_déclarations_à_typer_le_sont = false;
    bool toutes_les_ri_sont_generees = false;

    Type *type_à_valider = nullptr;
    NoeudDéclaration *déclaration_à_valider = nullptr;

    Type *ri_type_à_générer = nullptr;
    Type *fonction_initialisation_type_à_créer = nullptr;
    NoeudDéclaration *ri_déclaration_à_générer = nullptr;
};

void imprime_diagnostique(DiagnostiqueÉtatCompilation const &diagnostique,
                          Enchaineuse &enchaineuse);

bool operator==(DiagnostiqueÉtatCompilation const &diag1,
                DiagnostiqueÉtatCompilation const &diag2);

bool operator!=(DiagnostiqueÉtatCompilation const &diag1,
                DiagnostiqueÉtatCompilation const &diag2);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Programme
 * \{ */

enum RaisonAjoutType {
    DÉPENDANCE_DIRECTE,
    DÉPENDACE_INDIRECTE,
};

/* Représentation d'un programme. Ceci peut être le programme final tel que généré par la
 * compilation ou bien un métaprogramme. Il contient toutes les globales et tous les types utilisés
 * par les fonctions qui le composent. */
struct Programme {
  protected:
    kuri::rassembleuse<NoeudDéclarationEntêteFonction *> m_fonctions{};

    /* Fonctions dont les dépendances sont potentiellement manquantes. */
    kuri::ensemble<NoeudDéclaration *> m_dépendances_manquantes{};

    kuri::rassembleuse<NoeudDéclarationVariable *> m_globales{};

    kuri::rassembleuse<Type *> m_types{};
    kuri::rassembleuse<Type *> m_init_de{};
    kuri::rassembleuse<Type *> m_info_de{};

    /* Tous les fichiers utilisés dans le programme. */
    kuri::rassembleuse<Fichier *> m_fichiers{};
    mutable bool m_fichiers_sont_sales = true;

    EspaceDeTravail *m_espace = nullptr;

    /* Non-nul si le programme est celui d'un métaprogramme. */
    MetaProgramme *m_pour_métaprogramme = nullptr;

    // la coulisse à utiliser pour générer le code du programme
    Coulisse *m_coulisse = nullptr;

    ÉtatCompilation m_état_compilation{};

    /* Pour le débogage, stockage du dernier diagnostique de compilation afin de ne pas retourner
     * toujours le même et éviter ainsi de polluer les impressions de débogages. */
    std::optional<DiagnostiqueÉtatCompilation> m_dernier_diagnostique{};

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

    mutable bool m_éléments_sont_sales[NOMBRE_ELEMENTS][NOMBRE_RAISONS];

  public:
    /* Création. */

    static Programme *crée(EspaceDeTravail *espace);

    static Programme *crée_pour_espace(EspaceDeTravail *espace);

    static Programme *crée_pour_metaprogramme(EspaceDeTravail *espace,
                                              MetaProgramme *metaprogramme);

    ~Programme();

    void change_d_espace(EspaceDeTravail *espace_)
    {
        m_espace = espace_;
    }

    /* Modifications. */

    void ajoute_fonction(NoeudDéclarationEntêteFonction *fonction);

    void ajoute_globale(NoeudDéclarationVariable *globale);

    void ajoute_type(Type *type, RaisonAjoutType raison, NoeudExpression *noeud);
    void ajoute_init_de(Type *type);
    void ajoute_info_de(Type *type);

    bool possède(NoeudDéclarationEntêteFonction *fonction) const
    {
        return m_fonctions.possède(fonction);
    }

    bool possède(NoeudDéclarationVariable *globale) const
    {
        return m_globales.possède(globale);
    }

    bool possède(Type *type) const
    {
        return m_types.possède(type);
    }

    kuri::tableau_statique<NoeudDéclarationEntêteFonction *> fonctions() const
    {
        return m_fonctions.donne_éléments();
    }

    kuri::tableau_statique<NoeudDéclarationVariable *> globales() const
    {
        return m_globales.donne_éléments();
    }

    kuri::tableau_statique<Type *> types() const
    {
        return m_types.donne_éléments();
    }

    /* Retourne vrai si toutes les fonctions, toutes les globales, et tous les types utilisés par
     * le programme ont eu leurs types validés. */
    bool typages_terminés(DiagnostiqueÉtatCompilation &diagnostique) const;

    /* Retourne vrai si toutes les fonctions, toutes les globales, et tous les types utilisés par
     * le programme ont eu leurs RI générées. */
    bool ri_générées(DiagnostiqueÉtatCompilation &diagnostique) const;

    bool ri_générées() const;

    MetaProgramme *pour_métaprogramme() const
    {
        return m_pour_métaprogramme;
    }

    void ajoute_racine(NoeudDéclarationEntêteFonction *racine);

    Coulisse *coulisse() const
    {
        return m_coulisse;
    }

    EspaceDeTravail *espace() const
    {
        return m_espace;
    }

    DiagnostiqueÉtatCompilation diagnostique_compilation() const;

    ÉtatCompilation ajourne_état_compilation();

    void change_de_phase(PhaseCompilation phase);

    int64_t mémoire_utilisée() const;

    void rassemble_statistiques(Statistiques &stats);

    kuri::ensemble<Module *> modules_utilisés() const;

    void ajourne_pour_nouvelles_options_espace();

    kuri::ensemble<NoeudDéclaration *> &dépendances_manquantes();

    /* Imprime le diagnostique de compilation dans le flux de sortie spécifié.
     * Ceci n'imprimera le diagnostique que lors du premier appel sauf si :
     * - ignore_doublon est faux, ou
     * - le diagnostique est différent du dernier.
     */
    [[nodiscard]] kuri::chaine imprime_diagnostique(bool ignore_doublon = true);

    kuri::chemin_systeme donne_chemin_pour_fichier_ri() const;

  private:
    void verifie_état_compilation_fichier(DiagnostiqueÉtatCompilation &diagnostique) const;

    void ajoute_fichier(Fichier *fichier);
};

enum {
    IMPRIME_TOUT = 0,
    IMPRIME_TYPES = 1,
    IMPRIME_FONCTIONS = 2,
    IMPRIME_GLOBALES = 4,
};

void imprime_contenu_programme(Programme const &programme, uint32_t quoi, std::ostream &os);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Représentation intermédiaire Programme.
 * \{ */

struct DonnéesTableauxConstants {
    AtomeGlobale *globale = nullptr;
    AtomeConstanteDonnéesConstantes const *tableau = nullptr;
    /* Décalage en octet du premier élément au sein des données constantes. Ceci pointe à
     * l'adresse suivant le rembourrage potentielle. */
    int64_t décalage_dans_données_constantes = 0;
    /* Rembourrage requis pour s'assurer que les données sont alignées à une adresse correcte.
     * Les coulisses sont responsables d'ajouter ce rembourrage aux données constantes _avant_
     * les données de ce tableau. */
    int64_t rembourrage = 0;
};

struct DonnéesConstantes {
    kuri::tableau<DonnéesTableauxConstants> tableaux_constants{};
    int64_t taille_données_tableaux_constants = 0;
    uint32_t alignement_désiré = 0;
};

/* La représentation intermédiaire des fonctions et globles contenues dans un Programme, ainsi que
 * tous les types utilisées. */
struct ProgrammeRepreInter {
    /* Pour les partitions des globales. */
    enum {
        GLOBALES_EXTERNES,
        GLOBALES_INTERNES,
        GLOBALES_CONSTANTES,
        GLOBALES_MUTABLES,

        /* Pour les métaprogrammes. */
        GLOBALES_INFO_TYPES,
        GLOBALES_NON_INFO_TYPES,

        NOMBRE_PARTITIONS_GLOBALES,
    };

    /* Pour les partitions des fonctions. */
    enum {
        FONCTIONS_EXTERNES,
        FONCTIONS_INTERNES,
        FONCTIONS_ENLIGNÉES,
        FONCTIONS_HORSLIGNÉES,

        NOMBRE_PARTITIONS_FONCTIONS,
    };

  private:
    friend struct ConstructriceProgrammeFormeRI;

    kuri::tableau<AtomeGlobale *> globales{};
    std::pair<int, int> partitions_globales[NOMBRE_PARTITIONS_GLOBALES];

    kuri::tableau<AtomeFonction *> fonctions{};
    std::pair<int, int> partitions_fonctions[NOMBRE_PARTITIONS_FONCTIONS];

    kuri::tableau<Type *> types{};

  private:
    mutable DonnéesConstantes m_données_constantes{};
    mutable bool m_données_constantes_construites = false;

    void ajoute_globale(AtomeGlobale *globale);

    void définis_partition(kuri::tableau_statique<AtomeGlobale *> partition, int quoi)
    {
        auto décalage = int(partition.begin() - globales.begin());
        partitions_globales[quoi] = {décalage, int(partition.taille())};
    }

    void définis_partition(kuri::tableau_statique<AtomeFonction *> partition, int quoi)
    {
        auto décalage = int(partition.begin() - fonctions.begin());
        partitions_fonctions[quoi] = {décalage, int(partition.taille())};
    }

  public:
    kuri::tableau_statique<AtomeGlobale *> donne_globales() const;
    kuri::tableau_statique<AtomeGlobale *> donne_globales_internes() const;
    kuri::tableau_statique<AtomeGlobale *> donne_globales_info_types() const;
    kuri::tableau_statique<AtomeGlobale *> donne_globales_non_info_types() const;

    kuri::tableau_statique<AtomeFonction *> donne_fonctions() const;
    kuri::tableau_statique<AtomeFonction *> donne_fonctions_enlignées() const;
    kuri::tableau_statique<AtomeFonction *> donne_fonctions_horslignées() const;

    kuri::tableau_statique<Type *> donne_types() const;

    std::optional<DonnéesConstantes const *> donne_données_constantes() const;

    BibliothèquesUtilisées donne_bibliothèques_utilisées() const;

    int64_t mémoire_utilisée() const;
};

void imprime_contenu_programme(const ProgrammeRepreInter &programme,
                               uint32_t quoi,
                               std::ostream &os);

std::optional<ProgrammeRepreInter> représentation_intermédiaire_programme(
    EspaceDeTravail &espace, CompilatriceRI &compilatrice_ri, Programme const &programme);

void imprime_ri_programme(ProgrammeRepreInter const &programme, std::ostream &os);

/** \} */
