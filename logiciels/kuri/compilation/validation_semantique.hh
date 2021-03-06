﻿/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/file_fixe.hh"

#include "arbre_syntaxique/noeud_expression.hh"
#include "graphe_dependance.hh"
#include "validation_expression_appel.hh"

struct Compilatrice;
struct Lexeme;
struct MetaProgramme;
struct Tacheronne;
struct TypeCompose;
struct TypeEnum;
struct TypeTableauFixe;
struct UniteCompilation;

namespace erreur {
enum class Genre : int;
}

enum class CodeRetourValidation : int {
    OK,
    Erreur,
};

/* Structure utilisée pour récupérer la mémoire entre plusieurs validations de déclaration,
 * mais également éviter de construire les différentes structures de données y utilisées;
 * ces constructions se voyant dans les profils d'exécution, notamment pour les
 * DonneesAssignations. */
struct ContexteValidationDeclaration {
    struct DeclarationEtReference {
        NoeudExpression *ref_decl = nullptr;
        NoeudDeclarationVariable *decl = nullptr;
    };

    /* Les variables déclarées, entre les virgules, si quelqu'une. */
    dls::tablet<NoeudExpression *, 6> feuilles_variables{};

    /* Les noeuds de déclarations des variables et les références pointant vers ceux-ci. */
    dls::tablet<DeclarationEtReference, 6> decls_et_refs{};

    /* Les expressions pour les initialisations, entre les virgules, si quelqu'une. */
    dls::tablet<NoeudExpression *, 6> feuilles_expressions{};

    /* Les variables à assigner, chaque expression le nombre de variables nécessaires pour recevoir
     * le résultat de son évaluation. */
    file_fixe<NoeudExpression *, 6> variables{};

    /* Les données finales pour les assignations, faisant correspondre les expressions aux
     * variables. */
    dls::tablet<DonneesAssignations, 6> donnees_assignations{};

    /* Données temporaires pour la constructions des donnees_assignations. */
    DonneesAssignations donnees_temp{};
};

struct ContexteValidationCode {
    Compilatrice &m_compilatrice;
    Tacheronne &m_tacheronne;
    NoeudDeclarationEnteteFonction *fonction_courante = nullptr;
    Type *union_ou_structure_courante = nullptr;

    /* Les données des dépendances d'un noeud syntaxique. */
    DonneesDependance donnees_dependance{};

    UniteCompilation *unite = nullptr;
    EspaceDeTravail *espace = nullptr;

    double temps_chargement = 0.0;

    ContexteValidationCode(Compilatrice &compilatrice,
                           Tacheronne &tacheronne,
                           UniteCompilation &unite);

    COPIE_CONSTRUCT(ContexteValidationCode);

    void commence_fonction(NoeudDeclarationEnteteFonction *fonction);

    void termine_fonction();

    CodeRetourValidation valide_semantique_noeud(NoeudExpression *);
    CodeRetourValidation valide_acces_membre(NoeudExpressionMembre *expression_membre);

    CodeRetourValidation valide_type_fonction(NoeudDeclarationEnteteFonction *);
    CodeRetourValidation valide_fonction(NoeudDeclarationCorpsFonction *);
    CodeRetourValidation valide_operateur(NoeudDeclarationCorpsFonction *);

    template <int N>
    CodeRetourValidation valide_enum_impl(NoeudEnum *decl, TypeEnum *type_enum);
    CodeRetourValidation valide_enum(NoeudEnum *);

    CodeRetourValidation valide_structure(NoeudStruct *);
    CodeRetourValidation valide_declaration_variable(NoeudDeclarationVariable *decl);
    CodeRetourValidation valide_assignation(NoeudAssignation *inst);
    CodeRetourValidation valide_arbre_aplatis(
        NoeudExpression *declaration, kuri::tableau<NoeudExpression *, int> &arbre_aplatis);
    CodeRetourValidation valide_expression_retour(NoeudRetour *inst_retour);
    CodeRetourValidation valide_cuisine(NoeudDirectiveCuisine *directive);
    CodeRetourValidation valide_reference_declaration(NoeudExpressionReference *expr,
                                                      NoeudBloc *bloc_recherche);

    template <typename TypeControleBoucle>
    CodeRetourValidation valide_controle_boucle(TypeControleBoucle *inst);

    CodeRetourValidation resoud_type_final(NoeudExpression *expression_type, Type *&type_final);

    void rapporte_erreur(const char *message, NoeudExpression *noeud);
    void rapporte_erreur(const char *message, NoeudExpression *noeud, erreur::Genre genre);
    void rapporte_erreur_redefinition_symbole(NoeudExpression *decl, NoeudDeclaration *decl_prec);
    void rapporte_erreur_redefinition_fonction(NoeudDeclarationEnteteFonction *decl,
                                               NoeudDeclaration *decl_prec);
    void rapporte_erreur_type_arguments(NoeudExpression *type_arg, NoeudExpression *type_enf);
    void rapporte_erreur_assignation_type_differents(const Type *type_gauche,
                                                     const Type *type_droite,
                                                     NoeudExpression *noeud);
    void rapporte_erreur_type_operation(const Type *type_gauche,
                                        const Type *type_droite,
                                        NoeudExpression *noeud);
    void rapporte_erreur_acces_hors_limites(NoeudExpression *b,
                                            TypeTableauFixe *type_tableau,
                                            long index_acces);
    void rapporte_erreur_membre_inconnu(NoeudExpression *acces,
                                        NoeudExpression *structure,
                                        NoeudExpression *membre,
                                        TypeCompose *type);
    void rapporte_erreur_valeur_manquante_discr(
        NoeudExpression *expression,
        const dls::ensemble<kuri::chaine_statique> &valeurs_manquantes);
    void rapporte_erreur_fonction_nulctx(NoeudExpression const *appl_fonc,
                                         NoeudExpression const *decl_fonc,
                                         NoeudExpression const *decl_appel);

    CodeRetourValidation transtype_si_necessaire(NoeudExpression *&expression, Type *type_cible);
    void transtype_si_necessaire(NoeudExpression *&expression,
                                 TransformationType const &transformation);

    MetaProgramme *cree_metaprogramme_corps_texte(NoeudBloc *bloc_corps_texte,
                                                  NoeudBloc *bloc_parent,
                                                  const Lexeme *lexeme);
};
