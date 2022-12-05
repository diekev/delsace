/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "biblinternes/outils/definitions.h"

#include "structures/chaine_statique.hh"
#include "structures/tableau.hh"
#include "structures/tablet.hh"

#include "parsage/identifiant.hh"

#include "monomorpheuse.hh"
#include "monomorphisations.hh"
#include "transformation_type.hh"
#include "validation_semantique.hh"  // pour ResultatValidation

struct Compilatrice;
struct ContexteValidationCode;
struct EspaceDeTravail;
struct IdentifiantCode;
struct ItemMonomorphisation;
struct NoeudDeclaration;
struct NoeudExpression;
struct NoeudExpressionAppel;
struct Type;

enum class CodeRetourValidation : int;

struct IdentifiantEtExpression {
    IdentifiantCode *ident;
    NoeudExpression *expr_ident;
    NoeudExpression *expr;
};

enum {
    AUCUNE_RAISON,

    // À FAIRE : ceci n'est que pour attendre sur un type lors de la vérification de la
    // compatibilité entre les types des arguments et ceux des expressions reçues
    ERREUR_DEPENDANCE,

    EXPRESSION_MANQUANTE_POUR_UNION,
    MANQUE_NOM_APRES_VARIADIC,
    ARGUMENTS_MANQUANTS,
    MECOMPTAGE_ARGS,
    MENOMMAGE_ARG,
    METYPAGE_ARG,
    NOMMAGE_ARG_POINTEUR_FONCTION,
    RENOMMAGE_ARG,
    TROP_D_EXPRESSION_POUR_UNION,
    TYPE_N_EST_PAS_FONCTION,
    EXPANSION_VARIADIQUE_FONCTION_EXTERNE,
    MULTIPLE_EXPANSIONS_VARIADIQUES,
    EXPANSION_VARIADIQUE_APRES_ARGUMENTS_VARIADIQUES,
    ARGUMENTS_VARIADIQEUS_APRES_EXPANSION_VARIAQUES,
    MONOMORPHISATION,
};

enum {
    NOTE_INVALIDE,
    CANDIDATE_EST_APPEL_FONCTION,
    CANDIDATE_EST_CUISSON_FONCTION,
    CANDIDATE_EST_APPEL_POINTEUR,
    CANDIDATE_EST_INITIALISATION_STRUCTURE,
    CANDIDATE_EST_TYPE_POLYMORPHIQUE,
    CANDIDATE_EST_APPEL_INIT_DE,
    CANDIDATE_EST_INITIALISATION_OPAQUE,
    CANDIDATE_EST_MONOMORPHISATION_OPAQUE,
};

struct ErreurAppariement {
    kuri::chaine_statique nom_arg{};

    Attente attente{};

    /* Ce que nous avons à gauche */
    int note = NOTE_INVALIDE;

    int raison = AUCUNE_RAISON;

    /* Le type de l'élément à gauche de l'expression (pour les structures et les pointeurs de
     * fonctions) */
    Type *type = nullptr;

    kuri::tablet<IdentifiantCode *, 10> arguments_manquants_{};
    NoeudExpression const *site_erreur = nullptr;
    NoeudDeclaration *noeud_decl = nullptr;

    ErreurMonomorphisation erreur_monomorphisation = {};

    struct NombreArguments {
        long int nombre_obtenu = 0;
        long int nombre_requis = 0;
    } nombre_arguments{};

    static ErreurAppariement mecomptage_arguments(NoeudExpression const *site,
                                                  long nombre_requis,
                                                  long nombre_obtenu)
    {
        auto erreur = cree_erreur(MECOMPTAGE_ARGS, site);
        erreur.nombre_arguments.nombre_obtenu = nombre_obtenu;
        erreur.nombre_arguments.nombre_requis = nombre_requis;
        return erreur;
    }

    struct TypeArgument {
        Type *type_attendu = nullptr;
        Type *type_obtenu = nullptr;
    } type_arguments{};

    static ErreurAppariement metypage_argument(NoeudExpression const *site,
                                               Type *type_attendu,
                                               Type *type_obtenu)
    {
        auto erreur = cree_erreur(METYPAGE_ARG, site);
        erreur.type_arguments.type_attendu = type_attendu;
        erreur.type_arguments.type_obtenu = type_obtenu;
        return erreur;
    }

    static ErreurAppariement monomorphisation(NoeudExpression *site,
                                              ErreurMonomorphisation erreur_monomorphisation)
    {
        auto erreur = cree_erreur(MONOMORPHISATION, site);
        erreur.erreur_monomorphisation = erreur_monomorphisation;
        return erreur;
    }

    static ErreurAppariement type_non_fonction(NoeudExpression const *site, Type *type)
    {
        auto erreur = cree_erreur(TYPE_N_EST_PAS_FONCTION, site);
        erreur.type = type;
        return erreur;
    }

    static ErreurAppariement menommage_arguments(NoeudExpression const *site,
                                                 IdentifiantCode *ident)
    {
        auto erreur = cree_erreur(MENOMMAGE_ARG, site);
        erreur.nom_arg = ident->nom;
        return erreur;
    }

    static ErreurAppariement renommage_argument(NoeudExpression const *site,
                                                IdentifiantCode *ident)
    {
        auto erreur = cree_erreur(RENOMMAGE_ARG, site);
        erreur.nom_arg = ident->nom;
        return erreur;
    }

    static ErreurAppariement dependance_non_satisfaite(NoeudExpression const *site,
                                                       Attente attente)
    {
        auto erreur = cree_erreur(ERREUR_DEPENDANCE, site);
        erreur.attente = attente;
        return erreur;
    }

#define CREATION_ERREUR(nom_enum, nom_fonction)                                                   \
    static ErreurAppariement nom_fonction(NoeudExpression const *site)                            \
    {                                                                                             \
        return cree_erreur(nom_enum, site);                                                       \
    }

    CREATION_ERREUR(EXPRESSION_MANQUANTE_POUR_UNION, expression_manquante_union);
    CREATION_ERREUR(MANQUE_NOM_APRES_VARIADIC, nom_manquant_apres_variadique);
    CREATION_ERREUR(ARGUMENTS_MANQUANTS, arguments_manquants);
    CREATION_ERREUR(NOMMAGE_ARG_POINTEUR_FONCTION, nommage_argument_pointeur_fonction);
    CREATION_ERREUR(TROP_D_EXPRESSION_POUR_UNION, expression_extra_pour_union);
    CREATION_ERREUR(EXPANSION_VARIADIQUE_FONCTION_EXTERNE, expansion_variadique_externe);
    CREATION_ERREUR(MULTIPLE_EXPANSIONS_VARIADIQUES, multiple_expansions_variadiques);
    CREATION_ERREUR(EXPANSION_VARIADIQUE_APRES_ARGUMENTS_VARIADIQUES,
                    expansion_variadique_post_argument);
    CREATION_ERREUR(ARGUMENTS_VARIADIQEUS_APRES_EXPANSION_VARIAQUES,
                    argument_post_expansion_variadique);

#undef CRETION_ERREUR

  private:
    static ErreurAppariement cree_erreur(int raison, NoeudExpression const *site)
    {
        ErreurAppariement erreur;
        erreur.raison = raison;
        erreur.site_erreur = site;
        return erreur;
    }
};

struct CandidateAppariement {
    double poids_args = 0.0;
    int note = NOTE_INVALIDE;

    REMBOURRE(4);
    /* Le type de l'élément à gauche de l'expression (pour les structures et les pointeurs de
     * fonctions) */
    Type *type = nullptr;

    /* les expressions remises dans l'ordre selon les noms. */
    kuri::tablet<NoeudExpression *, 10> exprs{};

    NoeudExpression *noeud_decl = nullptr;

    kuri::tableau<TransformationType, int> transformations{};
    kuri::tableau<ItemMonomorphisation, int> items_monomorphisation{};

    static CandidateAppariement *nul()
    {
        return nullptr;
    }

    static CandidateAppariement appel_fonction(
        double poids,
        NoeudExpression *noeud_decl,
        Type *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations)
    {
        return cree_candidate(CANDIDATE_EST_APPEL_FONCTION,
                              poids,
                              noeud_decl,
                              type,
                              std::move(exprs),
                              std::move(transformations));
    }

    static CandidateAppariement appel_fonction(
        double poids,
        NoeudExpression *noeud_decl,
        Type *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations,
        kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
    {
        auto candidate = cree_candidate(CANDIDATE_EST_APPEL_FONCTION,
                                        poids,
                                        noeud_decl,
                                        type,
                                        std::move(exprs),
                                        std::move(transformations));
        candidate.items_monomorphisation = std::move(items_monomorphisation);
        return candidate;
    }

    static CandidateAppariement cuisson_fonction(
        double poids,
        NoeudExpression *noeud_decl,
        Type *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations,
        kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
    {
        auto candidate = cree_candidate(CANDIDATE_EST_CUISSON_FONCTION,
                                        poids,
                                        noeud_decl,
                                        type,
                                        std::move(exprs),
                                        std::move(transformations));
        candidate.items_monomorphisation = std::move(items_monomorphisation);
        return candidate;
    }

    static CandidateAppariement appel_pointeur(
        double poids,
        NoeudExpression *noeud_decl,
        Type *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations)
    {
        return cree_candidate(CANDIDATE_EST_APPEL_POINTEUR,
                              poids,
                              noeud_decl,
                              type,
                              std::move(exprs),
                              std::move(transformations));
    }

    static CandidateAppariement initialisation_structure(
        double poids,
        NoeudExpression *noeud_decl,
        Type *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations)
    {
        return cree_candidate(CANDIDATE_EST_INITIALISATION_STRUCTURE,
                              poids,
                              noeud_decl,
                              type,
                              std::move(exprs),
                              std::move(transformations));
    }

    static CandidateAppariement initialisation_structure(
        double poids,
        NoeudExpression *noeud_decl,
        Type *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations,
        kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
    {
        auto candidate = cree_candidate(CANDIDATE_EST_INITIALISATION_STRUCTURE,
                                        poids,
                                        noeud_decl,
                                        type,
                                        std::move(exprs),
                                        std::move(transformations));
        candidate.items_monomorphisation = std::move(items_monomorphisation);
        return candidate;
    }

    static CandidateAppariement type_polymorphique(
        double poids,
        Type *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations)
    {
        return cree_candidate(CANDIDATE_EST_TYPE_POLYMORPHIQUE,
                              poids,
                              nullptr,
                              type,
                              std::move(exprs),
                              std::move(transformations));
    }

    static CandidateAppariement type_polymorphique(
        double poids,
        Type *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations,
        kuri::tableau<ItemMonomorphisation, int> &&items_monomorphisation)
    {
        auto candidate = cree_candidate(CANDIDATE_EST_TYPE_POLYMORPHIQUE,
                                        poids,
                                        nullptr,
                                        type,
                                        std::move(exprs),
                                        std::move(transformations));
        candidate.items_monomorphisation = std::move(items_monomorphisation);
        return candidate;
    }

    static CandidateAppariement appel_init_de(
        double poids,
        Type *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations)
    {
        return cree_candidate(CANDIDATE_EST_APPEL_INIT_DE,
                              poids,
                              nullptr,
                              type,
                              std::move(exprs),
                              std::move(transformations));
    }

    static CandidateAppariement initialisation_opaque(
        double poids,
        NoeudExpression *noeud_decl,
        Type *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations)
    {
        return cree_candidate(CANDIDATE_EST_INITIALISATION_OPAQUE,
                              poids,
                              noeud_decl,
                              type,
                              std::move(exprs),
                              std::move(transformations));
    }

    static CandidateAppariement monomorphisation_opaque(
        double poids,
        NoeudExpression *noeud_decl,
        Type *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations)
    {
        return cree_candidate(CANDIDATE_EST_MONOMORPHISATION_OPAQUE,
                              poids,
                              noeud_decl,
                              type,
                              std::move(exprs),
                              std::move(transformations));
    }

  private:
    static CandidateAppariement cree_candidate(
        int note,
        double poids,
        NoeudExpression *noeud_decl,
        Type *type,
        kuri::tablet<NoeudExpression *, 10> &&exprs,
        kuri::tableau<TransformationType, int> &&transformations)
    {
        CandidateAppariement candidate;
        candidate.note = note;
        candidate.poids_args = poids;
        candidate.type = type;
        candidate.exprs = std::move(exprs);
        candidate.transformations = std::move(transformations);
        candidate.noeud_decl = noeud_decl;
        return candidate;
    }
};

ResultatValidation valide_appel_fonction(Compilatrice &compilatrice,
                                         EspaceDeTravail &espace,
                                         ContexteValidationCode &contexte_validation,
                                         NoeudExpressionAppel *expr);
