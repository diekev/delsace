/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "arbre_syntaxique/prodeclaration.hh"

#include "structures/pile.hh"
#include "structures/pile_de_tableaux.hh"
#include "structures/tableau.hh"

enum class GenreNoeud : uint8_t;

struct AssembleuseArbre;
struct Contexte;
struct EspaceDeTravail;
struct IdentifiantCode;
struct Lexème;
struct Typeuse;

using TypeStructure = NoeudStruct;

/* ------------------------------------------------------------------------- */
/** \name Canonicalisation des arbres syntaxiques.
 *
 * Nous transformation les arbres syntaxiques en des formes plus simples à gérer pour la
 * génération de RI. La forme canonique de chaque noeud, si applicable, se trouvera dans
 * NoeudExpression.substitution.
 * \{ */

struct Simplificatrice {
  private:
    Contexte *m_contexte;
    EspaceDeTravail *espace;
    AssembleuseArbre *assem;
    Typeuse &typeuse;

    NoeudDéclarationEntêteFonction *fonction_courante = nullptr;

    /* Tiens trace des appels de fonction. Quand nous visitons les paramètres de d'appel d'une
     * fonction, si l'un des paramètres est la construction d'une instance de PositionCodeSource,
     * nous devons utiliser le site d'appel et non le site de l'argument car celui-ci pourrait être
     * le site de la déclaration de l'argument de la fonction.
     *
     * Par exemple :
     *
     * une_fonction :: fonc (position := PositionCodeSource()) ...
     *
     * une_autre_fonction :: fonc ()
     * {
     *     // nous devons prendre le site de l'appel, sinon les informations de PositionCodeSource
     *     // dériveraient du site de déclaration de l'argument.
     *     une_fonction()
     * }
     */
    NoeudExpressionAppel *m_site_pour_position_code_source = nullptr;

    bool m_dans_fonction = false;

    /* Données pour les substitions à faire dans le corps des boucles-pour provenant
     * d'opérateurs-pour. */
    struct SubstitutionBouclePourOpérée {
        /* Le paramètre de la fonction. */
        NoeudExpression *param = nullptr;
        /* Substitution pour la référence au paramètre de l'opérateur. */
        NoeudExpression *référence_paramètre = nullptr;
        /* Subsitution pour la directive #corps_boucle. */
        NoeudBloc *corps_boucle = nullptr;
    };

    kuri::pile<SubstitutionBouclePourOpérée> m_substitutions_boucles_pour{};

    /* Nouvelles expressions pour chaque bloc. */
    kuri::pile_de_tableaux<NoeudExpression *> m_expressions_blocs{};

    /* Compteur pour créer des noms de variable temporaire. */
    int m_nombre_variables = 0;

  public:
    explicit Simplificatrice(Contexte *contexte);

    EMPECHE_COPIE(Simplificatrice);

    NoeudExpression *simplifie(NoeudExpression *noeud);

  private:
    void ajoute_expression(NoeudExpression *expression)
    {
        m_expressions_blocs.ajoute_au_tableau_courant(expression);
    }

    NoeudDéclarationVariable *crée_déclaration_variable(Lexème const *lexème,
                                                        NoeudDéclarationType *type,
                                                        NoeudExpression *expression);

    NoeudComme *crée_comme_type_cible(Lexème const *lexème,
                                      NoeudExpression *expression,
                                      NoeudDéclarationType *type);

    IdentifiantCode *donne_identifiant_pour_variable();

    NoeudExpression *simplifie_boucle_pour(NoeudPour *inst);
    NoeudExpression *simplifie_boucle_pour_opérateur(NoeudPour *inst);
    NoeudExpression *simplifie_comparaison_chainée(NoeudExpressionBinaire *comp);
    NoeudExpression *simplifie_instruction_si(NoeudSi *inst_si);
    NoeudExpression *simplifie_discr(NoeudDiscr *discr);
    template <int N>
    NoeudExpression *simplifie_discr_impl(NoeudDiscr *discr);
    NoeudExpression *simplifie_retour(NoeudInstructionRetour *inst);
    NoeudExpression *simplifie_retour(NoeudInstructionRetourMultiple *inst);
    NoeudExpression *simplifie_construction_structure(
        NoeudExpressionConstructionStructure *construction);
    NoeudExpression *simplifie_construction_union(
        NoeudExpressionConstructionStructure *construction);
    NoeudExpression *simplifie_construction_structure_position_code_source(
        NoeudExpressionConstructionStructure *construction);
    NoeudExpression *simplifie_construction_structure_impl(
        NoeudExpressionConstructionStructure *construction);
    NoeudExpressionRéférence *génère_simplification_construction_structure(
        NoeudExpressionAppel *construction, TypeStructure *type_struct);
    NoeudExpression *simplifie_construction_opaque_depuis_structure(NoeudExpressionAppel *appel);
    NoeudExpression *simplifie_référence_rubrique(NoeudExpressionRubrique *ref_rubrique);

    NoeudExpression *simplifie_assignation_énum_drapeau(NoeudExpression *var,
                                                        NoeudExpression *expression);

    NoeudExpression *simplifie_opérateur_binaire(NoeudExpressionBinaire *expr_bin,
                                                 bool pour_operande);
    NoeudExpression *simplifie_arithmétique_pointeur(NoeudExpressionBinaire *expr_bin);
    NoeudSi *crée_condition_boucle(NoeudExpression *inst, GenreNoeud genre_noeud);
    NoeudExpression *crée_expression_pour_op_chainée(
        kuri::tableau<NoeudExpressionBinaire> &comparaisons, const Lexème *lexeme_op_logique);

    /* remplace la dernière expression d'un bloc par une assignation afin de pouvoir simplifier les
     * conditions à droite des assigations */
    void corrige_bloc_pour_assignation(NoeudExpression *expr, NoeudExpression *ref_temp);

    NoeudExpression *crée_retourne_union_via_rien(NoeudDéclarationEntêteFonction *entête,
                                                  NoeudBloc *bloc_d_insertion,
                                                  const Lexème *lexeme_reference);

    NoeudExpressionAppel *crée_appel_fonction_init(Lexème const *lexeme,
                                                   NoeudExpression *expression_à_initialiser);

    NoeudExpression *simplifie_expression_logique(NoeudExpressionLogique *logique);
    NoeudExpression *simplifie_assignation_logique(NoeudExpressionAssignationLogique *logique);
    NoeudExpression *simplifie_expression_pour_expression_logique(NoeudExpression *expression);

    NoeudExpression *simplifie_tente(NoeudInstructionTente *inst);

    NoeudExpression *développe_macro(NoeudDéclarationEntêteFonction *macro);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Point d'entrée pour la canonicalisation.
 * \{ */

void simplifie_arbre(Contexte *contexte, NoeudExpression *arbre);

/** \} */
