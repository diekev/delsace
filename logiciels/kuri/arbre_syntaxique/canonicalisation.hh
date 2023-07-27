/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "structures/tableau.hh"

enum class GenreNoeud : uint8_t;

struct AssembleuseArbre;
struct EspaceDeTravail;
struct Lexeme;
struct NoeudBloc;
struct NoeudDeclarationEnteteFonction;
struct NoeudDiscr;
struct NoeudExpression;
struct NoeudExpressionAppel;
struct NoeudExpressionBinaire;
struct NoeudExpressionConstructionStructure;
struct NoeudPour;
struct NoeudRetiens;
struct NoeudRetour;
struct NoeudSi;
struct Typeuse;

/* ------------------------------------------------------------------------- */
/** \name Canonicalisation des arbres syntaxiques.
 *
 * Nous transformation les arbres syntaxiques en des formes plus simples à gérer pour la
 * génération de RI. La forme canonique de chaque noeud, si applicable, se trouvera dans
 * NoeudExpression.substitution.
 * \{ */

struct Simplificatrice {
    EspaceDeTravail *espace;
    AssembleuseArbre *assem;
    Typeuse &typeuse;

    NoeudDeclarationEnteteFonction *fonction_courante = nullptr;

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

    Simplificatrice(EspaceDeTravail *e, AssembleuseArbre *a, Typeuse &t)
        : espace(e), assem(a), typeuse(t)
    {
    }

    void simplifie(NoeudExpression *noeud);

  private:
    void simplifie_boucle_pour(NoeudPour *inst);
    void simplifie_boucle_pour_opérateur(NoeudPour *inst);
    void simplifie_comparaison_chainee(NoeudExpressionBinaire *comp);
    void simplifie_coroutine(NoeudDeclarationEnteteFonction *corout);
    void simplifie_discr(NoeudDiscr *discr);
    template <int N>
    void simplifie_discr_impl(NoeudDiscr *discr);
    void simplifie_retiens(NoeudRetiens *retiens);
    void simplifie_retour(NoeudRetour *inst);
    void simplifie_construction_structure(NoeudExpressionConstructionStructure *construction);
    void simplifie_construction_union(NoeudExpressionConstructionStructure *construction);
    void simplifie_construction_structure_position_code_source(
        NoeudExpressionConstructionStructure *construction);
    void simplifie_construction_structure_impl(NoeudExpressionConstructionStructure *construction);

    NoeudExpression *simplifie_assignation_enum_drapeau(NoeudExpression *var,
                                                        NoeudExpression *expression);

    NoeudExpression *simplifie_operateur_binaire(NoeudExpressionBinaire *expr_bin,
                                                 bool pour_operande);
    NoeudSi *cree_condition_boucle(NoeudExpression *inst, GenreNoeud genre_noeud);
    NoeudExpression *cree_expression_pour_op_chainee(
        kuri::tableau<NoeudExpressionBinaire> &comparaisons, const Lexeme *lexeme_op_logique);

    /* remplace la dernière expression d'un bloc par une assignation afin de pouvoir simplifier les
     * conditions à droite des assigations */
    void corrige_bloc_pour_assignation(NoeudExpression *expr, NoeudExpression *ref_temp);

    void cree_retourne_union_via_rien(NoeudDeclarationEnteteFonction *entete,
                                      NoeudBloc *bloc_d_insertion,
                                      const Lexeme *lexeme_reference);

    NoeudExpressionAppel *crée_appel_fonction_init(Lexeme const *lexeme,
                                                   NoeudExpression *expression_à_initialiser);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Point d'entrée pour la canonicalisation.
 * \{ */

void simplifie_arbre(EspaceDeTravail *espace,
                     AssembleuseArbre *assem,
                     Typeuse &typeuse,
                     NoeudExpression *arbre);

/** \} */
