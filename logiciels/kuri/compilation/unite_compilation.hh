/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <iosfwd>
#include <optional>

#include "structures/chaine.hh"
#include "structures/tableau.hh"

#include "attente.hh"

struct ArbreAplatis;
struct EspaceDeTravail;
struct Fichier;
struct MetaProgramme;
struct NoeudDeclarationCorpsFonction;
struct NoeudDeclarationEnteteFonction;
struct NoeudExpression;
struct Programme;

#define ENUMERE_RAISON_D_ETRE(O)                                                                  \
    O(AUCUNE, aucune_raison, "aucune raison")                                                     \
    O(CHARGEMENT_FICHIER, chargement_fichier, "chargement fichier")                               \
    O(LEXAGE_FICHIER, lexage_fichier, "lexage fichier")                                           \
    O(PARSAGE_FICHIER, parsage_fichier, "parsage fichier")                                        \
    O(CREATION_FONCTION_INIT_TYPE, creation_fonction_init_type, "création fonction init type")    \
    O(TYPAGE, typage, "typage")                                                                   \
    O(CONVERSION_NOEUD_CODE, conversion_noeud_code, "conversion noeud code")                      \
    O(ENVOIE_MESSAGE, envoie_message, "envoie message")                                           \
    O(GENERATION_RI, generation_ri, "génération RI")                                              \
    O(GENERATION_RI_PRINCIPALE_MP, generation_ri_principale_mp, "génération RI principale mp")    \
    O(EXECUTION, execution, "exécution")                                                          \
    O(LIAISON_PROGRAMME, liaison_programme, "liaison programme")                                  \
    O(GENERATION_CODE_MACHINE, generation_code_machine, "génération code machine")

enum class RaisonDEtre : unsigned char {
#define ENUMERE_RAISON_D_ETRE_EX(Genre, nom, chaine) Genre,
    ENUMERE_RAISON_D_ETRE(ENUMERE_RAISON_D_ETRE_EX)
#undef ENUMERE_RAISON_D_ETRE_EX
};

#define NOMBRE_DE_RAISON_D_ETRE (int(RaisonDEtre::GENERATION_CODE_MACHINE) + 1)

const char *chaine_raison_d_etre(RaisonDEtre raison_d_etre);
std::ostream &operator<<(std::ostream &os, RaisonDEtre raison_d_etre);

#define ENUMERE_ETAT_UNITE_COMPILATION(O)                                                         \
    O(EN_COURS_DE_COMPILATION)                                                                    \
    O(EN_ATTENTE)                                                                                 \
    O(ANNULÉE_CAR_REMPLACÉE)                                                                      \
    O(ANNULÉE_CAR_MESSAGERIE_FERMÉE)                                                              \
    O(ANNULÉE_CAR_ESPACE_POSSÈDE_ERREUR)                                                          \
    O(DONNÉE_À_ORDONNANCEUSE)                                                                     \
    O(EN_COURS_DE_TRAITEMENT_PAR_TACHERONNE)                                                      \
    O(COMPILATION_TERMINÉE)

#define ENUMERE_ETAT_ATTENTE(O)                                                                   \
    O(ATTENTES_RÉSOLUES)                                                                          \
    O(ATTENTES_BLOQUÉES)                                                                          \
    O(ATTENTES_NON_RÉSOLUES)                                                                      \
    O(UN_SYMBOLE_EST_ATTENDU)

#undef ENREGISTRE_HISTORIQUE

struct ÉtatFileUnitésChargementFile;

struct UniteCompilation {
    enum class État : uint8_t {
#define ENUMERE_ETAT_UNITE_COMPILATION_EX(Genre) Genre,
        ENUMERE_ETAT_UNITE_COMPILATION(ENUMERE_ETAT_UNITE_COMPILATION_EX)
#undef ENUMERE_ETAT_UNITE_COMPILATION_EX
    };

    enum class ÉtatAttentes : uint8_t {
#define ENUMERE_ETAT_ATTENTE_EX(Genre) Genre,
        ENUMERE_ETAT_ATTENTE(ENUMERE_ETAT_ATTENTE_EX)
#undef ENUMERE_ETAT_ATTENTE_EX
    };

  private:
    État état = État::EN_COURS_DE_COMPILATION;
    RaisonDEtre m_raison_d_être = RaisonDEtre::AUCUNE;

  public:
    /* Le nombre de cycles d'attentes, à savoir le nombre de fois où nous avons vérifié que
     * l'attente est résolue. */
    mutable int cycle = 0;
    /* L'id de la phase de compilation pour lequel nous comptons les cycles d'attentes. */
    mutable int id_phase_cycle = 0;

    /* Données pour l'historique de compilation de cette unité. */
    struct Historique {
        /* L'état lors de cet enregistrement. */
        État état{};
        /* La raison d'être lors de cet enregistrement. */
        RaisonDEtre raison{};
        /* La fonction qui a ajouté cet enregistrement. */
        const char *fonction{};
    };

    /* Pour l'état de chargement des fichiers. */
    UniteCompilation *suivante = nullptr;
    UniteCompilation *précédente = nullptr;
    ÉtatFileUnitésChargementFile *enfilée_dans = nullptr;

  private:
    kuri::tableau<Attente, int> m_attentes{};

#ifdef ENREGISTRE_HISTORIQUE
    kuri::tableau<Historique, int> m_historique{};
#endif

    /* Les attentes sur symbole sont spéciales : puisque les symboles sont ajoutés aux blocs au fur
     * et à mesure de la validation des déclarations correspondantes, nous devons toujours retenter
     * de compiler l'unité jusqu'à ce qu'à atteindre le nombre maximum de cycles. Pour ce faire,
     * nous nous rappelons de l'attente sur symbole précédente et augmentons le cycle uniquement si
     * nous avons toujours la même attente avant de retenter la compilation. */
    NoeudExpression *m_attente_sur_symbole_précédente = nullptr;

  public:
    EspaceDeTravail *espace = nullptr;
    union {
        Fichier *fichier = nullptr;
        NoeudExpression *noeud;
        MetaProgramme *metaprogramme;
        Programme *programme;
        Message *message;
        Type *type;
    };

    /* Mis en place par les sémanticiennes. Uniquement valide lors de la
     * validation sémantique, pour les unités de compilation dont la raison
     * d'être est pour le typage. */
    ArbreAplatis *arbre_aplatis = nullptr;

    explicit UniteCompilation(EspaceDeTravail *esp) : espace(esp)
    {
    }

    EMPECHE_COPIE(UniteCompilation);

    void ajoute_attente(Attente attente);

  private:
    void marque_prête(bool préserve_cycle);

  public:
    bool est_prête() const
    {
        return m_attentes.est_vide();
    }

    void définis_état(État nouvelle_état)
    {
        état = nouvelle_état;
#ifdef ENREGISTRE_HISTORIQUE
        m_historique.ajoute({état, m_raison_d_être, __func__});
#endif
    }

    État donne_état() const
    {
        return état;
    }

    bool fut_annulée() const
    {
        return état == État::ANNULÉE_CAR_MESSAGERIE_FERMÉE ||
               état == État::ANNULÉE_CAR_REMPLACÉE ||
               état == État::ANNULÉE_CAR_ESPACE_POSSÈDE_ERREUR;
    }

    void mute_raison_d_être(RaisonDEtre nouvelle_raison)
    {
        m_raison_d_être = nouvelle_raison;
#ifdef ENREGISTRE_HISTORIQUE
        m_historique.ajoute({état, m_raison_d_être, __func__});
#endif
    }

    RaisonDEtre donne_raison_d_être() const
    {
        return m_raison_d_être;
    }

    kuri::tableau_statique<Historique> donne_historique() const
    {
#ifdef ENREGISTRE_HISTORIQUE
        return m_historique;
#else
        return kuri::tableau_statique<Historique>(nullptr, 0);
#endif
    }

    kuri::tableau_statique<Attente> donne_attentes() const
    {
        return m_attentes;
    }

    inline Attente *attend_sur_message(Message const *message_)
    {
        POUR (m_attentes) {
            if (it.est<AttenteSurMessage>() && it.message().message == message_) {
                return &it;
            }
        }
        return nullptr;
    }

    inline void supprime_attentes_sur_messages()
    {
        POUR (m_attentes) {
            if (it.est<AttenteSurMessage>()) {
                it = {};
            }
        }
    }

    inline Attente *attend_sur_noeud_code(NoeudExpression *noeud_)
    {
        POUR (m_attentes) {
            if (it.est<AttenteSurNoeudCode>() && it.noeud_code().noeud == noeud_) {
                return &it;
            }
        }
        return nullptr;
    }

    inline bool attend_sur_déclaration(NoeudDeclaration *decl)
    {
        POUR (m_attentes) {
            if (it.est<AttenteSurDeclaration>() && it.declaration() == decl) {
                return false;
            }
        }
        return true;
    }

#define DEFINIS_DISCRIMINATION(Genre, nom, chaine)                                                \
    inline bool est_pour_##nom() const                                                            \
    {                                                                                             \
        return m_raison_d_être == RaisonDEtre::Genre;                                             \
    }

    ENUMERE_RAISON_D_ETRE(DEFINIS_DISCRIMINATION)

#undef DEFINIS_DISCRIMINATION

    bool est_bloquée() const;

    void rapporte_erreur() const;

    ÉtatAttentes détermine_état_attentes();

    kuri::chaine chaine_attentes_récursives() const;

  private:
    bool est_attente_sur_symbole_précédent(Attente attente) const;
    void marque_prête_pour_attente_sur_symbole();

    /* Retourne la première Attente qui semble ne pas pouvoir être résolue, ou nul si elles sont
     * toutes résolvables. */
    Attente const *première_attente_bloquée() const;

    Attente const *première_attente_bloquée_ou_non() const;
};

const char *chaine_état_unité_compilation(UniteCompilation::État état);
std::ostream &operator<<(std::ostream &os, UniteCompilation::État état);

/* ------------------------------------------------------------------------- */
/** \name Fonctions auxilliaires pour le débogage.
 * \{ */

void imprime_historique_unité(std::ostream &os, const UniteCompilation *unité);

void imprime_attentes_unité(std::ostream &os, const UniteCompilation *unité);

/** Imprime Unité.état ainsi que l'historique et les attentes de l'unité. */
void imprime_état_unité(std::ostream &os, const UniteCompilation *unité);

void imprime_noeud_index_courant_unité(std::ostream &os, const UniteCompilation *unité);

/** \} */
