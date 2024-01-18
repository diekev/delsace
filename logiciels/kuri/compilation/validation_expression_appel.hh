/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "structures/chaine_statique.hh"
#include "structures/tableau.hh"
#include "structures/tablet.hh"

#include "parsage/identifiant.hh"

#include "utilitaires/macros.hh"

#include "monomorpheuse.hh"
#include "monomorphisations.hh"
#include "transformation_type.hh"
#include "validation_semantique.hh"  // pour RésultatValidation

struct Compilatrice;
struct Sémanticienne;
struct EspaceDeTravail;
struct IdentifiantCode;
struct ItemMonomorphisation;
struct NoeudDeclaration;
struct NoeudExpression;
struct NoeudExpressionAppel;
struct NoeudDeclarationType;
using Type = NoeudDeclarationType;

struct IdentifiantEtExpression {
    IdentifiantCode *ident;
    NoeudExpression *expr_ident;
    NoeudExpression *expr;
};

enum {
    AUCUNE_RAISON,

    EXPRESSION_MANQUANTE_POUR_UNION,
    MANQUE_NOM_APRÈS_VARIADIC,
    ARGUMENTS_MANQUANTS,
    MÉCOMPTAGE_ARGS,
    MÉNOMMAGE_ARG,
    MÉTYPAGE_ARG,
    NOMMAGE_ARG_POINTEUR_FONCTION,
    RENOMMAGE_ARG,
    TROP_D_EXPRESSION_POUR_UNION,
    TYPE_N_EST_PAS_FONCTION,
    EXPANSION_VARIADIQUE_FONCTION_EXTERNE,
    MULTIPLE_EXPANSIONS_VARIADIQUES,
    EXPANSION_VARIADIQUE_APRÈS_ARGUMENTS_VARIADIQUES,
    ARGUMENTS_VARIADIQEUS_APRÈS_EXPANSION_VARIAQUES,
    MONOMORPHISATION,
};

enum {
    NOTE_INVALIDE,
    CANDIDATE_EST_APPEL_FONCTION,
    CANDIDATE_EST_CUISSON_FONCTION,
    CANDIDATE_EST_APPEL_POINTEUR,
    CANDIDATE_EST_INITIALISATION_STRUCTURE,
    CANDIDATE_EST_MONOMORPHISATION_STRUCTURE,
    CANDIDATE_EST_TYPE_POLYMORPHIQUE,
    CANDIDATE_EST_APPEL_INIT_DE,
    CANDIDATE_EST_INITIALISATION_OPAQUE,
    CANDIDATE_EST_INITIALISATION_OPAQUE_DEPUIS_STRUCTURE,
    CANDIDATE_EST_MONOMORPHISATION_OPAQUE,
};

struct ErreurAppariement {
    kuri::chaine_statique nom_arg{};

    /* Ce que nous avons à gauche */
    int note = NOTE_INVALIDE;

    int raison = AUCUNE_RAISON;

    /* Le type de l'élément à gauche de l'expression (pour les structures et les pointeurs de
     * fonctions) */
    Type const *type = nullptr;

    kuri::tablet<IdentifiantCode *, 10> arguments_manquants_{};
    NoeudExpression const *site_erreur = nullptr;
    NoeudDeclaration const *noeud_decl = nullptr;

    ErreurMonomorphisation erreur_monomorphisation = {};

    struct NombreArguments {
        int64_t nombre_obtenu = 0;
        int64_t nombre_requis = 0;
    } nombre_arguments{};

    static ErreurAppariement mécomptage_arguments(NoeudExpression const *site,
                                                  int64_t nombre_requis,
                                                  int64_t nombre_obtenu);

    struct TypeArgument {
        Type *type_attendu = nullptr;
        Type *type_obtenu = nullptr;
    } type_arguments{};

    static ErreurAppariement métypage_argument(NoeudExpression const *site,
                                               Type *type_attendu,
                                               Type *type_obtenu);

    static ErreurAppariement monomorphisation(NoeudExpression const *site,
                                              ErreurMonomorphisation erreur_monomorphisation);

    static ErreurAppariement type_non_fonction(NoeudExpression const *site, Type *type);

    static ErreurAppariement ménommage_arguments(NoeudExpression const *site,
                                                 IdentifiantCode *ident);

    static ErreurAppariement renommage_argument(NoeudExpression const *site,
                                                IdentifiantCode *ident);

#define CREATION_ERREUR(nom_enum, nom_fonction)                                                   \
    static ErreurAppariement nom_fonction(NoeudExpression const *site)                            \
    {                                                                                             \
        return crée_erreur(nom_enum, site);                                                       \
    }

    CREATION_ERREUR(EXPRESSION_MANQUANTE_POUR_UNION, expression_manquante_union);
    CREATION_ERREUR(MANQUE_NOM_APRÈS_VARIADIC, nom_manquant_apres_variadique);
    CREATION_ERREUR(ARGUMENTS_MANQUANTS, arguments_manquants);
    CREATION_ERREUR(NOMMAGE_ARG_POINTEUR_FONCTION, nommage_argument_pointeur_fonction);
    CREATION_ERREUR(TROP_D_EXPRESSION_POUR_UNION, expression_extra_pour_union);
    CREATION_ERREUR(EXPANSION_VARIADIQUE_FONCTION_EXTERNE, expansion_variadique_externe);
    CREATION_ERREUR(MULTIPLE_EXPANSIONS_VARIADIQUES, multiple_expansions_variadiques);
    CREATION_ERREUR(EXPANSION_VARIADIQUE_APRÈS_ARGUMENTS_VARIADIQUES,
                    expansion_variadique_post_argument);
    CREATION_ERREUR(ARGUMENTS_VARIADIQEUS_APRÈS_EXPANSION_VARIAQUES,
                    argument_post_expansion_variadique);

#undef CREATION_ERREUR

  private:
    static ErreurAppariement crée_erreur(int raison, NoeudExpression const *site);
};

struct CandidateAppariement {
    double poids_args = 0.0;
    int note = NOTE_INVALIDE;

    REMBOURRE(4);
    /* Le type de l'élément à gauche de l'expression (pour les structures et les pointeurs de
     * fonctions) */
    Type const *type = nullptr;

    /* les expressions remises dans l'ordre selon les noms. */
    kuri::tablet<NoeudExpression *, 10> exprs{};

    NoeudExpression const *noeud_decl = nullptr;

    kuri::tableau<TransformationType, int> transformations{};
    kuri::tableau<ItemMonomorphisation, int> items_monomorphisation{};

    static CandidateAppariement *nul()
    {
        return nullptr;
    }

    static CandidateAppariement appel_fonction(
        double poids,
        NoeudExpression const *noeud_decl,
        Type const *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations);

    static CandidateAppariement appel_fonction(
        double poids,
        NoeudExpression const *noeud_decl,
        Type const *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations,
        kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation);

    static CandidateAppariement cuisson_fonction(
        double poids,
        NoeudExpression const *noeud_decl,
        Type const *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations,
        kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation);

    static CandidateAppariement appel_pointeur(
        double poids,
        NoeudExpression const *noeud_decl,
        Type const *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations);

    static CandidateAppariement initialisation_structure(
        double poids,
        NoeudExpression const *noeud_decl,
        Type const *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations);

    static CandidateAppariement monomorphisation_structure(
        double poids,
        NoeudExpression const *noeud_decl,
        Type const *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations,
        kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation);

    static CandidateAppariement type_polymorphique(
        double poids,
        Type *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations);

    static CandidateAppariement type_polymorphique(
        double poids,
        Type const *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations,
        kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation);

    static CandidateAppariement appel_init_de(
        double poids,
        Type const *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations);

    static CandidateAppariement initialisation_opaque(
        double poids,
        Type const *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations);

    static CandidateAppariement monomorphisation_opaque(
        double poids,
        Type const *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations);

  private:
    static CandidateAppariement crée_candidate(
        int note,
        double poids,
        NoeudExpression const *noeud_decl,
        Type const *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations);
};

using RésultatAppariement = std::variant<ErreurAppariement, CandidateAppariement, Attente>;

static constexpr auto TAILLE_CANDIDATES_DEFAUT = 10;

struct CandidateExpressionAppel {
    int quoi = 0;
    NoeudExpression *decl = nullptr;
};

using ListeCandidatesExpressionAppel =
    kuri::tablet<CandidateExpressionAppel, TAILLE_CANDIDATES_DEFAUT>;

/* ------------------------------------------------------------------------- */
/** \name État de la résolution d'une expression d'appel.
 * \{ */

/**
 * Cette structure stocke l'état de la résolution d'une expression d'appel afin
 * de pouvoir reprendre la résolution de l'expression après qu'une attente fut
 * émise pour ne pas refaire du travail redondant.
 */
struct EtatResolutionAppel {
    enum class État {
        RÉSOLUTION_NON_COMMENCÉE,
        ARGUMENTS_RASSEMBLÉS,
        LISTE_CANDIDATES_CRÉÉE,
        APPARIEMENT_CANDIDATES_FAIT,
        CANDIDATE_SÉLECTIONNÉE,
        TERMINÉ,
    };

    État état = État::RÉSOLUTION_NON_COMMENCÉE;

    /* Les arguments de l'expression d'appel mis sous forme de paires
     * ident-expression. Les identifiants code peuvent être nuls si les
     * arguments ne sont pas nommés. Ceci est utilisé pour simplifier
     * l'accès à ces données ; pour ne pas avoir à toujours les parser. */
    kuri::tableau<IdentifiantEtExpression> args{};

    /* La liste de chaque déclaration pouvant être choisi pour déterminer
     * ce qui est appelé. */
    ListeCandidatesExpressionAppel liste_candidates{};

    /* Les #ResultatAppariements pour chaque déclaration candidate à l'élection
     * de l'expression appelée. */
    kuri::tablet<RésultatAppariement, 10> résultats{};

    /* Les candidates choisis lors de l'appariement. */
    kuri::tablet<CandidateAppariement, 10> candidates{};
    /* Erreurs survenues durant l'élection. */
    kuri::tablet<ErreurAppariement, 10> erreurs{};

    CandidateAppariement *candidate_finale = nullptr;

    void réinitialise()
    {
        état = État::RÉSOLUTION_NON_COMMENCÉE;
        args.efface();
        résultats.efface();
        liste_candidates.efface();
        candidates.efface();
        erreurs.efface();
        candidate_finale = nullptr;
    }
};

/** \} */

RésultatValidation valide_appel_fonction(Compilatrice &compilatrice,
                                         EspaceDeTravail &espace,
                                         Sémanticienne &contexte_validation,
                                         NoeudExpressionAppel *expr);
