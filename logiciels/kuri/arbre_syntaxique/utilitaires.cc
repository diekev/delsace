/*
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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "utilitaires.hh"

#include "noeud_expression.hh"

#include "biblinternes/outils/assert.hh"
#include "biblinternes/outils/conditions.h"

#include "compilation/broyage.hh"
#include "compilation/compilatrice.hh"
#include "compilation/erreur.h"
#include "compilation/espace_de_travail.hh"
#include "compilation/typage.hh"

#include "parsage/identifiant.hh"
#include "parsage/modules.hh"
#include "parsage/outils_lexemes.hh"

#include "assembleuse.hh"
#include "noeud_code.hh"

/* ************************************************************************** */

static void aplatis_arbre(NoeudExpression *racine,
                          kuri::tableau<NoeudExpression *, int> &arbre_aplatis,
                          DrapeauxNoeud drapeau)
{
    if (racine == nullptr) {
        return;
    }

    switch (racine->genre) {
        case GenreNoeud::DECLARATION_BIBLIOTHEQUE:
        case GenreNoeud::DIRECTIVE_DEPENDANCE_BIBLIOTHEQUE:
        case GenreNoeud::DECLARATION_MODULE:
        {
            break;
        }
        case GenreNoeud::DIRECTIVE_AJOUTE_INIT:
        {
            auto ajoute_init = racine->comme_ajoute_init();
            aplatis_arbre(ajoute_init->expression, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(ajoute_init);
            break;
        }
        case GenreNoeud::DIRECTIVE_AJOUTE_FINI:
        {
            auto ajoute_fini = racine->comme_ajoute_fini();
            aplatis_arbre(ajoute_fini->expression, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(ajoute_fini);
            break;
        }
        case GenreNoeud::INSTRUCTION_COMPOSEE:
        {
            auto bloc = static_cast<NoeudBloc *>(racine);

            POUR (*bloc->expressions.verrou_lecture()) {
                aplatis_arbre(it, arbre_aplatis, drapeau);
            }

            // Il nous faut le bloc pour savoir quoi différer
            arbre_aplatis.ajoute(bloc);

            break;
        }
        case GenreNoeud::DECLARATION_ENTETE_FONCTION:
        case GenreNoeud::DECLARATION_CORPS_FONCTION:
        {
            /* L'aplatissement d'une fonction dans une fonction doit déjà avoir été fait */
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::DECLARATION_ENUM:
        {
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::DECLARATION_STRUCTURE:
        {
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::DECLARATION_VARIABLE:
        {
            auto expr = static_cast<NoeudDeclarationVariable *>(racine);

            // N'aplatis pas expr->valeur car ça ne sers à rien dans ce cas.
            // Évite également les déclaration de types polymorphiques, cela gène la validation car
            // la déclaration n'est dans aucun bloc.
            if (!expr->possede_drapeau(EST_DECLARATION_TYPE_OPAQUE) ||
                !expr->expression->possede_drapeau(DECLARATION_TYPE_POLYMORPHIQUE)) {
                aplatis_arbre(
                    expr->expression, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
                aplatis_arbre(expr->expression_type,
                              arbre_aplatis,
                              drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
            }

            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
        {
            auto expr = racine->comme_assignation_variable();
            expr->drapeaux |= drapeau;

            aplatis_arbre(expr->variable, arbre_aplatis, drapeau);
            aplatis_arbre(
                expr->expression, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::EXPRESSION_COMME:
        {
            auto expr = racine->comme_comme();
            expr->drapeaux |= drapeau;
            aplatis_arbre(expr->expression, arbre_aplatis, drapeau);
            aplatis_arbre(
                expr->expression_type, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
            arbre_aplatis.ajoute(expr);
            break;
        }
        case GenreNoeud::EXPRESSION_INDEXAGE:
        case GenreNoeud::EXPRESSION_PLAGE:
        case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
        {
            auto expr = static_cast<NoeudExpressionBinaire *>(racine);
            expr->drapeaux |= drapeau;

            aplatis_arbre(expr->operande_gauche, arbre_aplatis, drapeau);
            aplatis_arbre(expr->operande_droite, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::OPERATEUR_BINAIRE:
        {
            auto expr = static_cast<NoeudExpressionBinaire *>(racine);
            expr->drapeaux |= drapeau;

            if (est_assignation_composee(expr->lexeme->genre)) {
                drapeau |= DrapeauxNoeud::DROITE_ASSIGNATION;
            }

            aplatis_arbre(expr->operande_gauche, arbre_aplatis, drapeau);
            aplatis_arbre(expr->operande_droite, arbre_aplatis, drapeau);

            if (expr->lexeme->genre != GenreLexeme::VIRGULE) {
                arbre_aplatis.ajoute(expr);
            }

            break;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
        {
            auto expr = static_cast<NoeudExpressionMembre *>(racine);
            expr->drapeaux |= drapeau;

            aplatis_arbre(expr->accedee, arbre_aplatis, drapeau);
            expr->accedee->aide_generation_code = EST_NOEUD_ACCES;
            // n'ajoute pas le membre, car la validation sémantique le considérera
            // comme une référence déclaration, ce qui soit clashera avec une variable
            // du même nom, soit résultera en une erreur de compilation
            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
        case GenreNoeud::EXPRESSION_APPEL:
        {
            auto expr = static_cast<NoeudExpressionAppel *>(racine);
            expr->drapeaux |= drapeau;

            auto appelee = expr->expression;

            if (appelee->genre == GenreNoeud::EXPRESSION_REFERENCE_MEMBRE) {
                // pour les expresssions de références de membre, puisqu'elles peuvent être des
                // expressions d'appels avec syntaxe uniforme, nous n'aplatissons que la branche
                // de l'accédée, la branche de membre pouvant être une fonction, ferait échouer la
                // validation sémantique
                auto ref_membre = static_cast<NoeudExpressionMembre *>(appelee);
                aplatis_arbre(ref_membre->accedee, arbre_aplatis, drapeau);
            }
            else {
                aplatis_arbre(appelee, arbre_aplatis, drapeau);
            }

            POUR (expr->parametres) {
                if (it->est_assignation_variable()) {
                    // n'aplatis pas le nom du paramètre car cela clasherait avec une variable
                    // locale, ou résulterait en une erreur de compilation « variable inconnue »
                    auto expr_assing = it->comme_assignation_variable();
                    aplatis_arbre(expr_assing->expression,
                                  arbre_aplatis,
                                  drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
                }
                else {
                    aplatis_arbre(it, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
                }
            }

            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
        case GenreNoeud::EXPRESSION_INFO_DE:
        case GenreNoeud::EXPRESSION_MEMOIRE:
        case GenreNoeud::EXPRESSION_PARENTHESE:
        case GenreNoeud::OPERATEUR_UNAIRE:
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        case GenreNoeud::EXPANSION_VARIADIQUE:
        case GenreNoeud::EXPRESSION_TYPE_DE:
        case GenreNoeud::INSTRUCTION_EMPL:
        {
            auto expr = static_cast<NoeudExpressionUnaire *>(racine);
            expr->drapeaux |= drapeau;
            aplatis_arbre(expr->operande, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(expr);
            break;
        }
        case GenreNoeud::INSTRUCTION_ARRETE:
        {
            auto inst = racine->comme_arrete();
            inst->drapeaux |= drapeau;
            aplatis_arbre(inst->expression, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(inst);
            break;
        }
        case GenreNoeud::INSTRUCTION_CONTINUE:
        {
            auto inst = racine->comme_continue();
            inst->drapeaux |= drapeau;
            aplatis_arbre(inst->expression, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(inst);
            break;
        }
        case GenreNoeud::INSTRUCTION_REPRENDS:
        {
            auto inst = racine->comme_reprends();
            inst->drapeaux |= drapeau;
            aplatis_arbre(inst->expression, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(inst);
            break;
        }
        case GenreNoeud::INSTRUCTION_CHARGE:
        case GenreNoeud::INSTRUCTION_IMPORTE:
        {
            racine->drapeaux |= drapeau;
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::INSTRUCTION_RETOUR:
        {
            auto inst = racine->comme_retourne();
            inst->drapeaux |= drapeau;
            drapeau |= DrapeauxNoeud::DROITE_ASSIGNATION;
            aplatis_arbre(inst->expression, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(inst);
            break;
        }
        case GenreNoeud::INSTRUCTION_RETIENS:
        {
            auto inst = racine->comme_retiens();
            inst->drapeaux |= drapeau;
            drapeau |= DrapeauxNoeud::DROITE_ASSIGNATION;
            aplatis_arbre(inst->expression, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(inst);
            break;
        }
        case GenreNoeud::DIRECTIVE_CUISINE:
        {
            auto cuisine = racine->comme_cuisine();
            cuisine->drapeaux |= drapeau;

            drapeau |= DROITE_ASSIGNATION;
            drapeau |= POUR_CUISSON;

            aplatis_arbre(cuisine->expression, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(cuisine);
            break;
        }
        case GenreNoeud::DIRECTIVE_EXECUTE:
        {
            auto expr = static_cast<NoeudDirectiveExecute *>(racine);
            expr->drapeaux |= drapeau;

            if (expr->ident == ID::assert_ || expr->ident == ID::test) {
                drapeau |= DROITE_ASSIGNATION;
            }

            aplatis_arbre(expr->expression, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(expr);
            break;
        }
        case GenreNoeud::EXPRESSION_INIT_DE:
        {
            auto init_de = racine->comme_init_de();
            aplatis_arbre(init_de->expression, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
        case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
        case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
        case GenreNoeud::EXPRESSION_LITTERALE_NUL:
        case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
        case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
        {
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
        {
            // Ceci ne doit pas être dans l'arbre à ce niveau
            break;
        }
        case GenreNoeud::INSTRUCTION_BOUCLE:
        case GenreNoeud::INSTRUCTION_REPETE:
        case GenreNoeud::INSTRUCTION_TANTQUE:
        {
            auto expr = static_cast<NoeudBoucle *>(racine);

            aplatis_arbre(expr->condition,
                          arbre_aplatis,
                          DrapeauxNoeud::DROITE_ASSIGNATION | DrapeauxNoeud::DROITE_CONDITION);
            arbre_aplatis.ajoute(expr);
            aplatis_arbre(expr->bloc, arbre_aplatis, DrapeauxNoeud::AUCUN);

            break;
        }
        case GenreNoeud::INSTRUCTION_POUR:
        {
            auto expr = static_cast<NoeudPour *>(racine);

            // n'ajoute pas la variable, sa déclaration n'a pas de type
            aplatis_arbre(expr->expression, arbre_aplatis, DrapeauxNoeud::DROITE_ASSIGNATION);
            arbre_aplatis.ajoute(expr);

            aplatis_arbre(expr->bloc, arbre_aplatis, DrapeauxNoeud::AUCUN);
            aplatis_arbre(expr->bloc_sansarret, arbre_aplatis, DrapeauxNoeud::AUCUN);
            aplatis_arbre(expr->bloc_sinon, arbre_aplatis, DrapeauxNoeud::AUCUN);

            break;
        }
        case GenreNoeud::INSTRUCTION_DISCR:
        case GenreNoeud::INSTRUCTION_DISCR_ENUM:
        case GenreNoeud::INSTRUCTION_DISCR_UNION:
        {
            auto expr = static_cast<NoeudDiscr *>(racine);

            aplatis_arbre(
                expr->expression_discriminee, arbre_aplatis, DrapeauxNoeud::DROITE_ASSIGNATION);
            arbre_aplatis.ajoute(expr);

            POUR (expr->paires_discr) {
                aplatis_arbre(it->bloc, arbre_aplatis, DrapeauxNoeud::AUCUN);
            }

            aplatis_arbre(expr->bloc_sinon, arbre_aplatis, DrapeauxNoeud::AUCUN);

            break;
        }
        case GenreNoeud::EXPRESSION_PAIRE_DISCRIMINATION:
        {
            // Géré au dessus.
            break;
        }
        case GenreNoeud::INSTRUCTION_SAUFSI:
        case GenreNoeud::INSTRUCTION_SI:
        {
            auto expr = static_cast<NoeudSi *>(racine);

            /* préserve le drapeau au cas où nous serions à droite d'une expression */
            expr->drapeaux |= drapeau;

            aplatis_arbre(expr->condition,
                          arbre_aplatis,
                          DrapeauxNoeud::DROITE_ASSIGNATION | DrapeauxNoeud::DROITE_CONDITION);
            aplatis_arbre(expr->bloc_si_vrai, arbre_aplatis, DrapeauxNoeud::AUCUN);
            aplatis_arbre(expr->bloc_si_faux, arbre_aplatis, DrapeauxNoeud::AUCUN);

            /* mets l'instruction à la fin afin de pouvoir déterminer le type de
             * l'expression selon les blocs si nous sommes à droite d'une expression */
            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::INSTRUCTION_SI_STATIQUE:
        {
            auto inst = racine->comme_si_statique();
            aplatis_arbre(inst->condition, arbre_aplatis, DrapeauxNoeud::DROITE_ASSIGNATION);
            arbre_aplatis.ajoute(inst);
            aplatis_arbre(inst->bloc_si_vrai, arbre_aplatis, DrapeauxNoeud::AUCUN);
            arbre_aplatis.ajoute(inst);  // insère une deuxième fois pour pouvoir sauter le code du
                                         // bloc_si_faux si la condition évalue à « vrai »
            inst->index_bloc_si_faux = arbre_aplatis.taille() - 1;
            aplatis_arbre(inst->bloc_si_faux, arbre_aplatis, DrapeauxNoeud::AUCUN);
            inst->index_apres = arbre_aplatis.taille() - 1;
            break;
        }
        case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
        {
            auto expr = static_cast<NoeudPousseContexte *>(racine);

            aplatis_arbre(expr->expression, arbre_aplatis, DrapeauxNoeud::DROITE_ASSIGNATION);
            arbre_aplatis.ajoute(expr);
            aplatis_arbre(expr->bloc, arbre_aplatis, DrapeauxNoeud::AUCUN);

            break;
        }
        case GenreNoeud::INSTRUCTION_TENTE:
        {
            auto inst = static_cast<NoeudInstructionTente *>(racine);

            if (inst->expression_piegee) {
                drapeau |= DrapeauxNoeud::DROITE_ASSIGNATION;
            }

            aplatis_arbre(inst->expression_appelee, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(inst);
            aplatis_arbre(inst->bloc, arbre_aplatis, DrapeauxNoeud::AUCUN);

            break;
        }
        case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
        {
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::EXPRESSION_VIRGULE:
        {
            auto expr = racine->comme_virgule();

            POUR (expr->expressions) {
                aplatis_arbre(it, arbre_aplatis, drapeau);
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_DIFFERE:
        {
            auto inst = racine->comme_differe();
            aplatis_arbre(inst->expression, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(inst);
        }
    }
}

void aplatis_arbre(NoeudExpression *declaration)
{
    if (declaration->est_entete_fonction()) {
        auto entete = declaration->comme_entete_fonction();
        if (entete->arbre_aplatis.taille() == 0) {
            aplatis_arbre(entete->bloc_constantes, entete->arbre_aplatis, {});
            aplatis_arbre(entete->bloc_parametres, entete->arbre_aplatis, {});

            POUR (entete->params) {
                aplatis_arbre(it, entete->arbre_aplatis, {});
            }

            POUR (entete->params_sorties) {
                aplatis_arbre(it, entete->arbre_aplatis, {});
            }
        }
        return;
    }

    if (declaration->est_corps_fonction()) {
        auto corps = declaration->comme_corps_fonction();
        if (corps->arbre_aplatis.taille() == 0) {
            aplatis_arbre(corps->bloc, corps->arbre_aplatis, {});
        }
        return;
    }

    if (declaration->est_structure()) {
        auto structure = declaration->comme_structure();
        if (structure->arbre_aplatis.taille() == 0) {
            POUR (structure->params_polymorphiques) {
                aplatis_arbre(it, structure->arbre_aplatis_params, {});
            }

            aplatis_arbre(structure->bloc, structure->arbre_aplatis, {});
        }
        return;
    }

    if (declaration->est_execute()) {
        auto execute = declaration->comme_execute();
        if (execute->arbre_aplatis.taille() == 0) {
            aplatis_arbre(execute, execute->arbre_aplatis, {});
        }
        return;
    }

    if (declaration->est_declaration_variable()) {
        auto declaration_variable = declaration->comme_declaration_variable();
        if (declaration_variable->arbre_aplatis.taille() == 0) {
            aplatis_arbre(declaration_variable, declaration_variable->arbre_aplatis, {});
        }
        return;
    }

    if (declaration->est_ajoute_fini()) {
        auto ajoute_fini = declaration->comme_ajoute_fini();
        if (ajoute_fini->arbre_aplatis.taille() == 0) {
            aplatis_arbre(ajoute_fini, ajoute_fini->arbre_aplatis, {});
        }
        return;
    }

    if (declaration->est_ajoute_init()) {
        auto ajoute_init = declaration->comme_ajoute_init();
        if (ajoute_init->arbre_aplatis.taille() == 0) {
            aplatis_arbre(ajoute_init, ajoute_init->arbre_aplatis, {});
        }
        return;
    }
}

struct Simplificatrice {
    EspaceDeTravail *espace;
    AssembleuseArbre *assem;
    Typeuse &typeuse;

    NoeudDeclarationEnteteFonction *fonction_courante = nullptr;

    Simplificatrice(EspaceDeTravail *e, AssembleuseArbre *a, Typeuse &t)
        : espace(e), assem(a), typeuse(t)
    {
    }

    void simplifie(NoeudExpression *noeud);

  private:
    void simplifie_boucle_pour(NoeudPour *inst);
    void simplifie_comparaison_chainee(NoeudExpressionBinaire *comp);
    void simplifie_coroutine(NoeudDeclarationEnteteFonction *corout);
    void simplifie_discr(NoeudDiscr *discr);
    template <int N>
    void simplifie_discr_impl(NoeudDiscr *discr);
    void simplifie_retiens(NoeudRetiens *retiens);
    void simplifie_retour(NoeudRetour *inst);

    NoeudExpression *simplifie_operateur_binaire(NoeudExpressionBinaire *expr_bin,
                                                 bool pour_operande);
    NoeudSi *cree_condition_boucle(NoeudExpression *inst, GenreNoeud genre_noeud);
    NoeudExpression *cree_expression_pour_op_chainee(
        kuri::tableau<NoeudExpressionBinaire> &comparaisons, const Lexeme *lexeme_op_logique);

    /* remplace la dernière expression d'un bloc par une assignation afin de pouvoir simplifier les
     * conditions à droite des assigations */
    void corrige_bloc_pour_assignation(NoeudExpression *expr, NoeudExpression *ref_temp);
};

void Simplificatrice::simplifie(NoeudExpression *noeud)
{
    if (!noeud) {
        return;
    }

    switch (noeud->genre) {
        case GenreNoeud::DECLARATION_BIBLIOTHEQUE:
        case GenreNoeud::DIRECTIVE_DEPENDANCE_BIBLIOTHEQUE:
        case GenreNoeud::DECLARATION_MODULE:
        case GenreNoeud::EXPRESSION_PAIRE_DISCRIMINATION:
        {
            break;
        }
        case GenreNoeud::DIRECTIVE_AJOUTE_FINI:
        {
            auto ajoute_fini = noeud->comme_ajoute_fini();
            simplifie(ajoute_fini->expression);
            break;
        }
        case GenreNoeud::DIRECTIVE_AJOUTE_INIT:
        {
            auto ajoute_init = noeud->comme_ajoute_init();
            simplifie(ajoute_init->expression);
            break;
        }
        case GenreNoeud::DECLARATION_ENTETE_FONCTION:
        {
            auto entete = noeud->comme_entete_fonction();

            if (entete->est_externe) {
                return;
            }

            if (entete->est_declaration_type) {
                entete->substitution = assem->cree_reference_type(entete->lexeme, entete->type);
                return;
            }

            if (entete->est_coroutine) {
                simplifie_coroutine(entete);
                return;
            }

            fonction_courante = entete;
            simplifie(entete->corps);
            return;
        }
        case GenreNoeud::DECLARATION_CORPS_FONCTION:
        {
            auto corps = noeud->comme_corps_fonction();
            simplifie(corps->bloc);
            return;
        }
        case GenreNoeud::INSTRUCTION_COMPOSEE:
        {
            auto bloc = noeud->comme_bloc();

            POUR (*bloc->expressions.verrou_ecriture()) {
                if (it->est_entete_fonction()) {
                    continue;
                }
                simplifie(it);
            }

            return;
        }
        case GenreNoeud::OPERATEUR_BINAIRE:
        {
            auto expr_bin = noeud->comme_expression_binaire();

            if (expr_bin->type->est_type_de_donnees()) {
                noeud->substitution = assem->cree_reference_type(expr_bin->lexeme, expr_bin->type);
                return;
            }

            simplifie(expr_bin->operande_gauche);
            simplifie(expr_bin->operande_droite);

            if (expr_bin->op && expr_bin->op->est_arithmetique_pointeur) {
                auto comme_type = [&](NoeudExpression *expr_ptr, Type *type) {
                    auto comme = assem->cree_comme(expr_ptr->lexeme);
                    comme->type = type;
                    comme->expression = expr_ptr;
                    comme->transformation = {TypeTransformation::POINTEUR_VERS_ENTIER, type};
                    return comme;
                };

                auto type1 = expr_bin->operande_gauche->type;
                auto type2 = expr_bin->operande_droite->type;

                // ptr - ptr => (ptr comme z64 - ptr comme z64) / taille_de(type_pointe)
                if (type1->est_pointeur() && type2->est_pointeur()) {
                    auto const &type_z64 = typeuse[TypeBase::Z64];
                    auto type_pointe = type2->comme_pointeur()->type_pointe;
                    auto soustraction = assem->cree_expression_binaire(
                        expr_bin->lexeme,
                        type_z64->operateur_sst,
                        comme_type(expr_bin->operande_gauche, type_z64),
                        comme_type(expr_bin->operande_droite, type_z64));
                    auto taille_de = assem->cree_litterale_entier(
                        expr_bin->lexeme, type_z64, std::max(type_pointe->taille_octet, 1u));
                    auto div = assem->cree_expression_binaire(
                        expr_bin->lexeme, type_z64->operateur_div, soustraction, taille_de);
                    expr_bin->substitution = div;
                }
                else {
                    Type *type_entier = Type::nul();
                    Type *type_pointeur = Type::nul();

                    NoeudExpression *expr_entier = nullptr;
                    NoeudExpression *expr_pointeur = nullptr;

                    // ent + ptr => (ptr comme type_entier + ent * taille_de(type_pointe)) comme
                    // type_ptr
                    if (est_type_entier(type1)) {
                        type_entier = type1;
                        type_pointeur = type2;
                        expr_entier = expr_bin->operande_gauche;
                        expr_pointeur = expr_bin->operande_droite;
                    }
                    // ptr - ent => (ptr comme type_entier - ent * taille_de(type_pointe)) comme
                    // type_ptr ptr + ent => (ptr comme type_entier + ent * taille_de(type_pointe))
                    // comme type_ptr
                    else if (est_type_entier(type2)) {
                        type_entier = type2;
                        type_pointeur = type1;
                        expr_entier = expr_bin->operande_droite;
                        expr_pointeur = expr_bin->operande_gauche;
                    }

                    auto type_pointe = type_pointeur->comme_pointeur()->type_pointe;

                    auto taille_de = assem->cree_litterale_entier(
                        expr_entier->lexeme, type_entier, std::max(type_pointe->taille_octet, 1u));
                    auto mul = assem->cree_expression_binaire(
                        expr_entier->lexeme, type_entier->operateur_mul, expr_entier, taille_de);

                    OperateurBinaire *op_arithm = nullptr;

                    if (expr_bin->lexeme->genre == GenreLexeme::MOINS ||
                        expr_bin->lexeme->genre == GenreLexeme::MOINS_EGAL) {
                        op_arithm = type_entier->operateur_sst;
                    }
                    else if (expr_bin->lexeme->genre == GenreLexeme::PLUS ||
                             expr_bin->lexeme->genre == GenreLexeme::PLUS_EGAL) {
                        op_arithm = type_entier->operateur_ajt;
                    }

                    auto arithm = assem->cree_expression_binaire(
                        expr_bin->lexeme, op_arithm, comme_type(expr_pointeur, type_entier), mul);

                    auto comme_pointeur = assem->cree_comme(expr_bin->lexeme);
                    comme_pointeur->type = type_pointeur;
                    comme_pointeur->expression = arithm;
                    comme_pointeur->transformation = {TypeTransformation::ENTIER_VERS_POINTEUR,
                                                      type_pointeur};

                    expr_bin->substitution = comme_pointeur;
                }

                if (expr_bin->possede_drapeau(EST_ASSIGNATION_COMPOSEE)) {
                    expr_bin->substitution = assem->cree_assignation_variable(
                        expr_bin->lexeme, expr_bin->operande_gauche, expr_bin->substitution);
                }

                return;
            }

            if (expr_bin->possede_drapeau(EST_ASSIGNATION_COMPOSEE)) {
                noeud->substitution = assem->cree_assignation_variable(
                    expr_bin->lexeme,
                    expr_bin->operande_gauche,
                    simplifie_operateur_binaire(expr_bin, true));
                return;
            }

            if (dls::outils::est_element(
                    noeud->lexeme->genre, GenreLexeme::BARRE_BARRE, GenreLexeme::ESP_ESP)) {
                // À FAIRE : simplifie les accès à des énum_drapeaux dans les expressions || ou &&,
                // il faudra également modifier la RI pour prendre en compte la substitution
                return;
            }

            noeud->substitution = simplifie_operateur_binaire(expr_bin, false);
            return;
        }
        case GenreNoeud::OPERATEUR_UNAIRE:
        {
            auto expr_un = noeud->comme_expression_unaire();

            if (expr_un->type->est_type_de_donnees()) {
                expr_un->substitution = assem->cree_reference_type(expr_un->lexeme, expr_un->type);
                return;
            }

            simplifie(expr_un->operande);

            /* op peut être nul pour les opérateurs ! et * */
            if (expr_un->op && !expr_un->op->est_basique) {
                auto appel = assem->cree_appel(
                    expr_un->lexeme, expr_un->op->decl, expr_un->op->type_resultat);
                appel->parametres_resolus.ajoute(expr_un->operande);
                expr_un->substitution = appel;
                return;
            }

            return;
        }
        case GenreNoeud::EXPRESSION_TYPE_DE:
        {
            /* change simplement le genre du noeud car le type de l'expression est le type de sa
             * sous expression */
            noeud->genre = GenreNoeud::EXPRESSION_REFERENCE_TYPE;
            return;
        }
        case GenreNoeud::DIRECTIVE_CUISINE:
        {
            auto cuisine = noeud->comme_cuisine();
            auto expr = cuisine->expression->comme_appel();
            cuisine->substitution = assem->cree_reference_declaration(
                expr->lexeme, expr->expression->comme_entete_fonction());
            return;
        }
        case GenreNoeud::INSTRUCTION_SI_STATIQUE:
        {
            auto inst = noeud->comme_si_statique();

            if (inst->condition_est_vraie) {
                simplifie(inst->bloc_si_vrai);
                inst->substitution = inst->bloc_si_vrai;
                return;
            }

            simplifie(inst->bloc_si_faux);
            inst->substitution = inst->bloc_si_faux;
            return;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
        {
            auto expr_ref = noeud->comme_reference_declaration();
            auto decl_ref = expr_ref->declaration_referee;

            if (decl_ref->drapeaux & EST_CONSTANTE) {
                auto decl_const = decl_ref->comme_declaration_variable();

                if (decl_ref->possede_drapeau(EST_DECLARATION_TYPE_OPAQUE)) {
                    expr_ref->substitution = assem->cree_reference_type(
                        expr_ref->lexeme, typeuse.type_type_de_donnees(decl_ref->type));
                    return;
                }

                if (decl_ref->type->est_type_de_donnees()) {
                    expr_ref->substitution = assem->cree_reference_type(
                        expr_ref->lexeme, typeuse.type_type_de_donnees(decl_ref->type));
                    return;
                }

                if (decl_ref->type->est_reel()) {
                    expr_ref->substitution = assem->cree_litterale_reel(
                        expr_ref->lexeme, decl_ref->type, decl_const->valeur_expression.reelle());
                    return;
                }

                if (decl_ref->type->est_bool()) {
                    expr_ref->substitution = assem->cree_litterale_bool(
                        expr_ref->lexeme,
                        decl_ref->type,
                        decl_const->valeur_expression.booleenne());
                    return;
                }

                if (est_type_entier(decl_ref->type) || decl_ref->type->est_entier_constant()) {
                    expr_ref->substitution = assem->cree_litterale_entier(
                        expr_ref->lexeme,
                        decl_ref->type,
                        static_cast<unsigned long>(decl_const->valeur_expression.entiere()));
                    return;
                }

                if (decl_ref->type->est_chaine()) {
                    expr_ref->substitution = decl_const->expression;
                    return;
                }

                return;
            }

            if (dls::outils::est_element(decl_ref->genre,
                                         GenreNoeud::DECLARATION_ENUM,
                                         GenreNoeud::DECLARATION_STRUCTURE)) {
                expr_ref->substitution = assem->cree_reference_type(
                    expr_ref->lexeme, typeuse.type_type_de_donnees(decl_ref->type));
                return;
            }

            return;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
        {
            auto ref_membre = noeud->comme_reference_membre();
            auto accede = ref_membre->accedee;
            auto type_accede = accede->type;

            if (ref_membre->possede_drapeau(ACCES_EST_ENUM_DRAPEAU)) {
                /* Devraient être simplifié là où ils sont utilisés. */
                if (!ref_membre->possede_drapeau(DROITE_CONDITION)) {
                    return;
                }

                // a.DRAPEAU => (a & DRAPEAU) != 0
                auto type_enum = static_cast<TypeEnum *>(ref_membre->type);
                auto valeur_enum = type_enum->membres[ref_membre->index_membre].valeur;

                auto valeur_lit_enum = assem->cree_litterale_entier(
                    noeud->lexeme, type_enum, static_cast<unsigned>(valeur_enum));
                auto op = type_enum->operateur_etb;
                auto et = assem->cree_expression_binaire(
                    noeud->lexeme, op, accede, valeur_lit_enum);

                auto zero = assem->cree_litterale_entier(noeud->lexeme, type_enum, 0);
                op = type_enum->operateur_dif;
                auto dif = assem->cree_expression_binaire(noeud->lexeme, op, et, zero);

                ref_membre->substitution = dif;
                return;
            }

            if (accede->est_reference_declaration()) {
                if (accede->comme_reference_declaration()
                        ->declaration_referee->est_declaration_module()) {
                    ref_membre->substitution = accede;
                    simplifie(accede);
                    return;
                }
            }

            while (type_accede->est_pointeur() || type_accede->est_reference()) {
                type_accede = type_dereference_pour(type_accede);
            }

            if (type_accede->est_opaque()) {
                type_accede = type_accede->comme_opaque()->type_opacifie;
            }

            if (type_accede->est_tableau_fixe()) {
                auto taille = type_accede->comme_tableau_fixe()->taille;
                noeud->substitution = assem->cree_litterale_entier(
                    noeud->lexeme, noeud->type, static_cast<unsigned long>(taille));
                return;
            }

            if (type_accede->est_enum() || type_accede->est_erreur()) {
                auto type_enum = static_cast<TypeEnum *>(type_accede);
                auto valeur_enum = type_enum->membres[ref_membre->index_membre].valeur;
                noeud->substitution = assem->cree_litterale_entier(
                    noeud->lexeme, type_enum, static_cast<unsigned>(valeur_enum));
                return;
            }

            if (type_accede->est_type_de_donnees() && noeud->genre_valeur == GenreValeur::DROITE) {
                noeud->substitution = assem->cree_reference_type(
                    noeud->lexeme, typeuse.type_type_de_donnees(noeud->type));
                return;
            }

            auto type_compose = static_cast<TypeCompose *>(type_accede);
            auto &membre = type_compose->membres[ref_membre->index_membre];

            if (membre.drapeaux == TypeCompose::Membre::EST_CONSTANT) {
                simplifie(membre.expression_valeur_defaut);
                noeud->substitution = membre.expression_valeur_defaut;
                return;
            }

            /* pour les appels de fonctions */
            simplifie(ref_membre->accedee);

            return;
        }
        case GenreNoeud::EXPRESSION_COMME:
        {
            auto inst = noeud->comme_comme();
            auto expr = inst->expression;

            if (expr->type == inst->type) {
                simplifie(expr);
                noeud->substitution = expr;
                return;
            }

            if (expr->type->est_entier_constant() &&
                inst->transformation.type == TypeTransformation::ENTIER_VERS_POINTEUR) {
                expr->type = typeuse[TypeBase::Z64];
                return;
            }

            simplifie(inst->expression);
            return;
        }
        case GenreNoeud::INSTRUCTION_POUR:
        {
            simplifie_boucle_pour(noeud->comme_pour());
            return;
        }
        case GenreNoeud::INSTRUCTION_BOUCLE:
        {
            auto boucle = noeud->comme_boucle();
            simplifie(boucle->bloc);
            return;
        }
        case GenreNoeud::INSTRUCTION_REPETE:
        {
            /*

              boucle {
                corps:
                    ...

                si condition {
                    arrête (implicite)
                }
              }

             */
            auto boucle = noeud->comme_repete();
            simplifie(boucle->condition);
            simplifie(boucle->bloc);

            auto nouvelle_boucle = assem->cree_boucle(noeud->lexeme);
            nouvelle_boucle->bloc_parent = boucle->bloc_parent;

            auto condition = cree_condition_boucle(nouvelle_boucle,
                                                   GenreNoeud::INSTRUCTION_SAUFSI);
            condition->condition = boucle->condition;

            auto nouveau_bloc = assem->cree_bloc_seul(nullptr, boucle->bloc_parent);
            nouveau_bloc->expressions->ajoute(boucle->bloc);
            nouveau_bloc->expressions->ajoute(condition);

            nouvelle_boucle->bloc = nouveau_bloc;
            boucle->substitution = nouvelle_boucle;
            return;
        }
        case GenreNoeud::INSTRUCTION_TANTQUE:
        {
            /*

              boucle {
                si condition {
                    arrête (implicite)
                }

                corps:
                    ...

              }

             */
            auto boucle = noeud->comme_tantque();
            simplifie(boucle->condition);
            simplifie(boucle->bloc);

            auto nouvelle_boucle = assem->cree_boucle(noeud->lexeme);
            nouvelle_boucle->bloc_parent = boucle->bloc_parent;

            auto condition = cree_condition_boucle(nouvelle_boucle,
                                                   GenreNoeud::INSTRUCTION_SAUFSI);
            condition->condition = boucle->condition;

            auto nouveau_bloc = assem->cree_bloc_seul(nullptr, boucle->bloc_parent);
            nouveau_bloc->expressions->ajoute(condition);
            nouveau_bloc->expressions->ajoute(boucle->bloc);

            nouvelle_boucle->bloc = nouveau_bloc;
            boucle->substitution = nouvelle_boucle;
            return;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
        {
            auto tableau = noeud->comme_construction_tableau();
            simplifie(tableau->expression);
            return;
        }
        case GenreNoeud::EXPRESSION_VIRGULE:
        {
            auto virgule = noeud->comme_virgule();

            POUR (virgule->expressions) {
                simplifie(it);
            }

            return;
        }
        case GenreNoeud::INSTRUCTION_RETIENS:
        {
            simplifie_retiens(noeud->comme_retiens());
            return;
        }
        case GenreNoeud::INSTRUCTION_DISCR:
        case GenreNoeud::INSTRUCTION_DISCR_ENUM:
        case GenreNoeud::INSTRUCTION_DISCR_UNION:
        {
            auto discr = noeud->comme_discr();
            simplifie_discr(discr);
            return;
        }
        case GenreNoeud::EXPRESSION_PARENTHESE:
        {
            auto parenthese = noeud->comme_parenthese();
            simplifie(parenthese->expression);
            parenthese->substitution = parenthese->expression;
            return;
        }
        case GenreNoeud::INSTRUCTION_TENTE:
        {
            auto tente = noeud->comme_tente();
            simplifie(tente->expression_appelee);
            if (tente->bloc) {
                simplifie(tente->bloc);
            }
            return;
        }
        case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
        {
            auto tableau = noeud->comme_args_variadiques();

            POUR (tableau->expressions) {
                simplifie(it);
            }

            return;
        }
        case GenreNoeud::INSTRUCTION_RETOUR:
        {
            auto retour = noeud->comme_retourne();
            simplifie_retour(retour);
            return;
        }
        case GenreNoeud::INSTRUCTION_SAUFSI:
        case GenreNoeud::INSTRUCTION_SI:
        {
            auto si = static_cast<NoeudSi *>(noeud);
            simplifie(si->condition);
            simplifie(si->bloc_si_vrai);
            simplifie(si->bloc_si_faux);

            if (si->possede_drapeau(DrapeauxNoeud::DROITE_ASSIGNATION)) {
                /*

                  x := si y { z } sinon { w }

                  {
                    decl := XXX;
                    si y { decl = z; } sinon { decl = w; }
                    decl; // nous avons une référence simple car la RI empilera sa valeur qui
                  pourra être dépilée et utilisée pour l'assignation
                  }

                 */

                auto bloc = assem->cree_bloc_seul(si->lexeme, si->bloc_parent);

                auto decl_temp = assem->cree_declaration_variable(
                    si->lexeme, si->type, nullptr, nullptr);
                auto ref_temp = assem->cree_reference_declaration(si->lexeme, decl_temp);

                auto nouveau_si = assem->cree_si(si->lexeme, si->genre);
                nouveau_si->condition = si->condition;
                nouveau_si->bloc_si_vrai = si->bloc_si_vrai;
                nouveau_si->bloc_si_faux = si->bloc_si_faux;

                corrige_bloc_pour_assignation(nouveau_si->bloc_si_vrai, ref_temp);
                corrige_bloc_pour_assignation(nouveau_si->bloc_si_faux, ref_temp);

                bloc->membres->ajoute(decl_temp);
                bloc->expressions->ajoute(decl_temp);
                bloc->expressions->ajoute(nouveau_si);
                bloc->expressions->ajoute(ref_temp);

                si->substitution = bloc;
            }

            return;
        }
        case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
        {
            simplifie_comparaison_chainee(noeud->comme_comparaison_chainee());
            return;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
        {
            auto appel = noeud->comme_construction_structure();

            POUR (appel->parametres_resolus) {
                simplifie(it);
            }

            return;
        }
        case GenreNoeud::EXPRESSION_APPEL:
        {
            auto appel = noeud->comme_appel();

            if (appel->aide_generation_code == CONSTRUIT_OPAQUE) {
                simplifie(appel->parametres_resolus[0]);

                auto comme = assem->cree_comme(appel->lexeme);
                comme->type = appel->type;
                comme->expression = appel->parametres_resolus[0];
                comme->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                         appel->type};
                comme->drapeaux |= TRANSTYPAGE_IMPLICITE;

                appel->substitution = comme;
                return;
            }

            if (appel->aide_generation_code == MONOMORPHE_TYPE_OPAQUE) {
                appel->substitution = assem->cree_reference_type(appel->lexeme, appel->type);
            }

            if (appel->noeud_fonction_appelee) {
                appel->expression->substitution = assem->cree_reference_declaration(
                    appel->lexeme, appel->noeud_fonction_appelee->comme_entete_fonction());
            }
            else {
                simplifie(appel->expression);
            }

            POUR (appel->parametres_resolus) {
                simplifie(it);
            }

            return;
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
        {
            auto assignation = noeud->comme_assignation_variable();

            simplifie(assignation->variable);

            POUR (assignation->donnees_exprs.plage()) {
                auto expression_fut_simplifiee = false;

                for (auto var : it.variables.plage()) {
                    if (var->possede_drapeau(ACCES_EST_ENUM_DRAPEAU)) {
                        auto ref_membre = var->comme_reference_membre();
                        auto ref_var = ref_membre->accedee;

                        // À FAIRE : référence
                        auto nouvelle_ref = assem->cree_reference_declaration(
                            ref_var->lexeme,
                            ref_var->comme_reference_declaration()->declaration_referee);
                        var->substitution = nouvelle_ref;

                        auto type_enum = static_cast<TypeEnum *>(ref_membre->type);
                        auto valeur_enum = type_enum->membres[ref_membre->index_membre].valeur;

                        if (it.expression->comme_litterale_bool()->valeur) {
                            // a.DRAPEAU = vrai -> a = a | DRAPEAU
                            auto valeur_lit_enum = assem->cree_litterale_entier(
                                noeud->lexeme, type_enum, static_cast<unsigned>(valeur_enum));
                            auto op = type_enum->operateur_oub;
                            auto ou = assem->cree_expression_binaire(
                                noeud->lexeme, op, nouvelle_ref, valeur_lit_enum);
                            it.expression->substitution = ou;
                        }
                        else {
                            // a.DRAPEAU = faux -> a = a & ~DRAPEAU
                            auto valeur_lit_enum = assem->cree_litterale_entier(
                                noeud->lexeme, type_enum, ~static_cast<unsigned>(valeur_enum));
                            auto op = type_enum->operateur_etb;
                            auto et = assem->cree_expression_binaire(
                                noeud->lexeme, op, nouvelle_ref, valeur_lit_enum);
                            it.expression->substitution = et;
                        }

                        expression_fut_simplifiee = true;
                    }
                }

                if (!expression_fut_simplifiee) {
                    simplifie(it.expression);
                }
            }

            return;
        }
        case GenreNoeud::DECLARATION_VARIABLE:
        {
            auto declaration = noeud->comme_declaration_variable();

            POUR (declaration->donnees_decl.plage()) {
                simplifie(it.expression);
            }

            return;
        }
        case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
        {
            auto pousse_contexte = noeud->comme_pousse_contexte();
            simplifie(pousse_contexte->bloc);
            return;
        }
        case GenreNoeud::EXPRESSION_INDEXAGE:
        {
            auto indexage = noeud->comme_indexage();
            simplifie(indexage->operande_gauche);
            simplifie(indexage->operande_droite);

            if (indexage->op) {
                indexage->substitution = simplifie_operateur_binaire(indexage, true);
            }

            return;
        }
        case GenreNoeud::DECLARATION_STRUCTURE:
        {
            auto structure = noeud->comme_structure();
            auto type = static_cast<TypeCompose *>(structure->type);

            POUR (type->membres) {
                simplifie(it.expression_valeur_defaut);
            }

            return;
        }
        case GenreNoeud::INSTRUCTION_DIFFERE:
        {
            auto inst = noeud->comme_differe();
            simplifie(inst->expression);
            return;
        }
        case GenreNoeud::EXPRESSION_PLAGE:
        {
            auto plage = noeud->comme_plage();
            simplifie(plage->debut);
            simplifie(plage->fin);
            return;
        }
        case GenreNoeud::DIRECTIVE_EXECUTE:
        case GenreNoeud::DECLARATION_ENUM:
        case GenreNoeud::EXPANSION_VARIADIQUE:
        case GenreNoeud::EXPRESSION_INFO_DE:
        case GenreNoeud::EXPRESSION_INIT_DE:
        case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
        case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
        case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
        case GenreNoeud::EXPRESSION_LITTERALE_NUL:
        case GenreNoeud::EXPRESSION_MEMOIRE:
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
        case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
        case GenreNoeud::INSTRUCTION_EMPL:
        case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
        case GenreNoeud::INSTRUCTION_CHARGE:
        case GenreNoeud::INSTRUCTION_IMPORTE:
        case GenreNoeud::INSTRUCTION_ARRETE:
        case GenreNoeud::INSTRUCTION_CONTINUE:
        case GenreNoeud::INSTRUCTION_REPRENDS:
        /* NOTE : taille_de doit persister jusque dans la RI. */
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        {
            return;
        }
    }

    return;
}

static OperateurBinaire const *operateur_pour_lexeme(GenreLexeme lexeme_op, Type *type, bool plage)
{
    /* NOTE : les opérateurs sont l'inverse de ce qu'indique les lexèmes car la condition est
     * inversée. */
    if (lexeme_op == GenreLexeme::INFERIEUR) {
        if (plage) {
            return type->operateur_sup;
        }

        return type->operateur_seg;
    }

    if (lexeme_op == GenreLexeme::SUPERIEUR) {
        if (plage) {
            return type->operateur_inf;
        }

        return type->operateur_ieg;
    }

    return nullptr;
}

void Simplificatrice::simplifie_boucle_pour(NoeudPour *inst)
{
    simplifie(inst->expression);
    simplifie(inst->bloc);
    simplifie(inst->bloc_sansarret);
    simplifie(inst->bloc_sinon);

    auto feuilles = inst->variable->comme_virgule();

    auto var = feuilles->expressions[0];
    auto idx = NoeudExpression::nul();
    auto expression_iteree = inst->expression;
    auto bloc = inst->bloc;
    auto bloc_sans_arret = inst->bloc_sansarret;
    auto bloc_sinon = inst->bloc_sinon;

    auto ident_index_it = ID::index_it;
    auto type_index_it = typeuse[TypeBase::Z64];
    if (feuilles->expressions.taille() == 2) {
        idx = feuilles->expressions[1];
        ident_index_it = idx->ident;
        type_index_it = idx->type;
    }

    auto boucle = assem->cree_boucle(inst->lexeme);
    boucle->ident = var->ident;
    boucle->bloc_parent = inst->bloc_parent;
    boucle->bloc = assem->cree_bloc_seul(inst->lexeme, boucle->bloc_parent);
    boucle->bloc_sansarret = bloc_sans_arret;
    boucle->bloc_sinon = bloc_sinon;

    auto it = var->comme_declaration_variable();

    auto zero = assem->cree_litterale_entier(var->lexeme, type_index_it, 0);
    auto index_it = idx ? idx->comme_declaration_variable() :
                          assem->cree_declaration_variable(
                              var->lexeme, type_index_it, ident_index_it, zero);

    auto ref_it = it->valeur->comme_reference_declaration();
    auto ref_index = index_it->valeur->comme_reference_declaration();

    auto bloc_pre = assem->cree_bloc_seul(nullptr, boucle->bloc_parent);
    bloc_pre->membres->ajoute(it);
    bloc_pre->membres->ajoute(index_it);

    bloc_pre->expressions->ajoute(it);
    bloc_pre->expressions->ajoute(index_it);

    auto bloc_inc = assem->cree_bloc_seul(nullptr, boucle->bloc_parent);

    auto condition = cree_condition_boucle(boucle, GenreNoeud::INSTRUCTION_SI);

    boucle->bloc_pre = bloc_pre;
    boucle->bloc_inc = bloc_inc;

    auto const inverse_boucle = inst->lexeme_op == GenreLexeme::SUPERIEUR;

    auto type_itere = expression_iteree->type->est_opaque() ?
                          expression_iteree->type->comme_opaque()->type_opacifie :
                          expression_iteree->type;
    expression_iteree->type = type_itere;

    /* boucle */

    switch (inst->aide_generation_code) {
        case GENERE_BOUCLE_PLAGE:
        case GENERE_BOUCLE_PLAGE_INDEX:
        case GENERE_BOUCLE_PLAGE_IMPLICITE:
        case GENERE_BOUCLE_PLAGE_IMPLICITE_INDEX:
        {
            /*

              pour 0 ... 10 {
                corps
              }

              it := 0
              index_it := 0

              boucle {
                si it > 10 {
                    arrête
                }

                corps:
                  ...

                incrémente:
                    it = it + 1
                    index_it = index_it + 1
              }

             */

            NoeudExpression *expr_debut = nullptr;
            NoeudExpression *expr_fin = nullptr;

            if (inst->aide_generation_code == GENERE_BOUCLE_PLAGE_IMPLICITE ||
                inst->aide_generation_code == GENERE_BOUCLE_PLAGE_IMPLICITE_INDEX) {
                // 0 ... expr - 1
                expr_debut = assem->cree_litterale_entier(
                    expression_iteree->lexeme, expression_iteree->type, 0);

                auto valeur_un = assem->cree_litterale_entier(
                    expression_iteree->lexeme, expression_iteree->type, 1);
                expr_fin = assem->cree_expression_binaire(expression_iteree->lexeme,
                                                          expression_iteree->type->operateur_sst,
                                                          expression_iteree,
                                                          valeur_un);
            }
            else {
                auto expr_plage = expression_iteree->comme_plage();
                expr_debut = expr_plage->debut;
                expr_fin = expr_plage->fin;
            }

            /* condition */

            if (inverse_boucle) {
                std::swap(expr_debut, expr_fin);
            }

            auto init_it = assem->cree_assignation_variable(ref_it->lexeme, ref_it, expr_debut);
            bloc_pre->expressions->ajoute(init_it);

            auto op_comp = operateur_pour_lexeme(inst->lexeme_op, ref_it->type, true);
            condition->condition = assem->cree_expression_binaire(
                inst->lexeme, op_comp, ref_it, expr_fin);
            boucle->bloc->expressions->ajoute(condition);

            /* corps */
            boucle->bloc->expressions->ajoute(bloc);

            /* suivant */
            if (inverse_boucle) {
                auto inc_it = assem->cree_decrementation(ref_it->lexeme, ref_it);
                bloc_inc->expressions->ajoute(inc_it);
            }
            else {
                auto inc_it = assem->cree_incrementation(ref_it->lexeme, ref_it);
                bloc_inc->expressions->ajoute(inc_it);
            }

            auto inc_it = assem->cree_incrementation(ref_index->lexeme, ref_index);
            bloc_inc->expressions->ajoute(inc_it);

            break;
        }
        case GENERE_BOUCLE_TABLEAU:
        case GENERE_BOUCLE_TABLEAU_INDEX:
        {
            /*

              pour chn {
                corps
              }

              index_it := 0
              boucle {
                si index_it >= chn.taille {
                    arrête
                }

                it := chn.pointeur[index_it]

                corps:
                  ...

                incrémente:
                    index_it = index_it + 1
              }

             */

            /* condition */
            auto expr_taille = NoeudExpression::nul();

            if (type_itere->est_tableau_fixe()) {
                auto taille_tableau = type_itere->comme_tableau_fixe()->taille;
                expr_taille = assem->cree_litterale_entier(
                    inst->lexeme,
                    typeuse[TypeBase::Z64],
                    static_cast<unsigned long>(taille_tableau));
            }
            else {
                expr_taille = assem->cree_reference_membre(
                    inst->lexeme, expression_iteree, typeuse[TypeBase::Z64], 1);
            }

            auto type_z64 = typeuse[TypeBase::Z64];
            condition->condition = assem->cree_expression_binaire(
                inst->lexeme, type_z64->operateur_seg, ref_index, expr_taille);

            auto expr_pointeur = NoeudExpression::nul();

            auto type_compose = type_itere->comme_compose();

            if (type_itere->est_tableau_fixe()) {
                auto indexage = assem->cree_indexage(inst->lexeme, expression_iteree, zero, true);

                static const Lexeme lexeme_adresse = {",", {}, GenreLexeme::FOIS_UNAIRE, 0, 0, 0};
                auto prise_adresse = assem->cree_expression_unaire(&lexeme_adresse);
                prise_adresse->operande = indexage;
                prise_adresse->type = typeuse.type_pointeur_pour(indexage->type);

                expr_pointeur = prise_adresse;
            }
            else {
                expr_pointeur = assem->cree_reference_membre(
                    inst->lexeme, expression_iteree, type_compose->membres[0].type, 0);
            }

            NoeudExpression *expr_index = ref_index;

            if (inverse_boucle) {
                expr_index = assem->cree_expression_binaire(
                    inst->lexeme, ref_index->type->operateur_sst, expr_taille, ref_index);
                expr_index = assem->cree_expression_binaire(
                    inst->lexeme,
                    ref_index->type->operateur_sst,
                    expr_index,
                    assem->cree_litterale_entier(ref_index->lexeme, ref_index->type, 1));
            }

            auto indexage = assem->cree_indexage(inst->lexeme, expr_pointeur, expr_index, true);
            NoeudExpression *expression_assignee = indexage;

            if (inst->prend_reference || inst->prend_pointeur) {
                auto noeud_comme = assem->cree_comme(it->lexeme);
                noeud_comme->type = it->type;
                noeud_comme->expression = indexage;
                noeud_comme->transformation = TypeTransformation::PREND_REFERENCE;
                noeud_comme->drapeaux |= TRANSTYPAGE_IMPLICITE;

                expression_assignee = noeud_comme;
            }

            auto assign_it = assem->cree_assignation_variable(
                inst->lexeme, ref_it, expression_assignee);

            boucle->bloc->expressions->ajoute(condition);
            boucle->bloc->expressions->ajoute(assign_it);

            /* corps */
            boucle->bloc->expressions->ajoute(bloc);

            /* incrémente */
            auto inc_it = assem->cree_incrementation(ref_index->lexeme, ref_index);
            bloc_inc->expressions->ajoute(inc_it);
            break;
        }
        case GENERE_BOUCLE_COROUTINE:
        case GENERE_BOUCLE_COROUTINE_INDEX:
        {
            /* À FAIRE(ri) : coroutine */
#if 0
			auto expr_appel = static_cast<NoeudExpressionAppel *>(enfant2);
			auto decl_fonc = static_cast<NoeudDeclarationCorpsFonction const *>(expr_appel->noeud_fonction_appelee);
			auto nom_etat = "__etat" + dls::vers_chaine(enfant2);
			auto nom_type_coro = "__etat_coro" + decl_fonc->nom_broye;

			constructrice << nom_type_coro << " " << nom_etat << " = {\n";
			constructrice << ".mutex_boucle = PTHREAD_MUTEX_INITIALIZER,\n";
			constructrice << ".mutex_coro = PTHREAD_MUTEX_INITIALIZER,\n";
			constructrice << ".cond_coro = PTHREAD_COND_INITIALIZER,\n";
			constructrice << ".cond_boucle = PTHREAD_COND_INITIALIZER,\n";
			constructrice << ".contexte = contexte,\n";
			constructrice << ".__termine_coro = 0\n";
			constructrice << "};\n";

			/* intialise les arguments de la fonction. */
			POUR (expr_appel->params) {
				genere_code_C(it, constructrice, compilatrice, false);
			}

			auto iter_enf = expr_appel->params.begin();

			POUR (decl_fonc->params) {
				auto nom_broye = broye_nom_simple(it->ident);
				constructrice << nom_etat << '.' << nom_broye << " = ";
				constructrice << (*iter_enf)->chaine_calculee();
				constructrice << ";\n";
				++iter_enf;
			}

			constructrice << "pthread_t fil_coro;\n";
			constructrice << "pthread_create(&fil_coro, NULL, " << decl_fonc->nom_broye << ", &" << nom_etat << ");\n";
			constructrice << "pthread_mutex_lock(&" << nom_etat << ".mutex_boucle);\n";
			constructrice << "pthread_cond_wait(&" << nom_etat << ".cond_boucle, &" << nom_etat << ".mutex_boucle);\n";
			constructrice << "pthread_mutex_unlock(&" << nom_etat << ".mutex_boucle);\n";

			/* À FAIRE : utilisation du type */
			auto nombre_vars_ret = decl_fonc->type_fonc->types_sorties.taille;

			auto feuilles = dls::tablet<NoeudExpression *, 10>{};
			rassemble_feuilles(enfant1, feuilles);

			auto idx = static_cast<NoeudExpression *>(nullptr);
			auto nom_idx = kuri::chaine{};

			if (b->aide_generation_code == GENERE_BOUCLE_COROUTINE_INDEX) {
				idx = feuilles.back();
				nom_idx = "__idx" + dls::vers_chaine(b);
				constructrice << "int " << nom_idx << " = 0;";
			}

			constructrice << "while (" << nom_etat << ".__termine_coro == 0) {\n";
			constructrice << "pthread_mutex_lock(&" << nom_etat << ".mutex_boucle);\n";

			for (auto i = 0l; i < nombre_vars_ret; ++i) {
				auto f = feuilles[i];
				auto nom_var_broye = broye_chaine(f);
				constructrice.declare_variable(type, nom_var_broye, "");
				constructrice << nom_var_broye << " = "
							   << nom_etat << '.' << decl_fonc->noms_retours[i]
							   << ";\n";
			}

			constructrice << "pthread_mutex_lock(&" << nom_etat << ".mutex_coro);\n";
			constructrice << "pthread_cond_signal(&" << nom_etat << ".cond_coro);\n";
			constructrice << "pthread_mutex_unlock(&" << nom_etat << ".mutex_coro);\n";
			constructrice << "pthread_cond_wait(&" << nom_etat << ".cond_boucle, &" << nom_etat << ".mutex_boucle);\n";
			constructrice << "pthread_mutex_unlock(&" << nom_etat << ".mutex_boucle);\n";

			if (idx) {
				constructrice << "int " << broye_chaine(idx) << " = " << nom_idx << ";\n";
				constructrice << nom_idx << " += 1;";
			}
#endif
            break;
        }
    }

    inst->substitution = boucle;
}

static void rassemble_operations_chainees(NoeudExpression *racine,
                                          kuri::tableau<NoeudExpressionBinaire> &comparaisons)
{
    auto expr_bin = static_cast<NoeudExpressionBinaire *>(racine);

    if (est_operateur_comparaison(expr_bin->operande_gauche->lexeme->genre)) {
        rassemble_operations_chainees(expr_bin->operande_gauche, comparaisons);

        auto expr_operande = static_cast<NoeudExpressionBinaire *>(expr_bin->operande_gauche);

        auto comparaison = NoeudExpressionBinaire{};
        comparaison.lexeme = expr_bin->lexeme;
        comparaison.operande_gauche = expr_operande->operande_droite;
        comparaison.operande_droite = expr_bin->operande_droite;
        comparaison.op = expr_bin->op;
        comparaison.permute_operandes = expr_bin->permute_operandes;

        comparaisons.ajoute(comparaison);
    }
    else {
        auto comparaison = NoeudExpressionBinaire{};
        comparaison.lexeme = expr_bin->lexeme;
        comparaison.operande_gauche = expr_bin->operande_gauche;
        comparaison.operande_droite = expr_bin->operande_droite;
        comparaison.op = expr_bin->op;
        comparaison.permute_operandes = expr_bin->permute_operandes;

        comparaisons.ajoute(comparaison);
    }
}

NoeudExpression *Simplificatrice::cree_expression_pour_op_chainee(
    kuri::tableau<NoeudExpressionBinaire> &comparaisons, Lexeme const *lexeme_op_logique)
{
    dls::pile<NoeudExpression *> exprs;

    for (auto i = comparaisons.taille() - 1; i >= 0; --i) {
        auto &it = comparaisons[i];
        simplifie(it.operande_gauche);
        simplifie(it.operande_droite);
        exprs.empile(simplifie_operateur_binaire(&it, true));
    }

    if (exprs.taille() == 1) {
        return exprs.depile();
    }

    auto resultat = NoeudExpression::nul();

    while (true) {
        auto a = exprs.depile();
        auto b = exprs.depile();

        auto et = assem->cree_expression_binaire(lexeme_op_logique);
        et->operande_gauche = a;
        et->operande_droite = b;

        if (exprs.est_vide()) {
            resultat = et;
            break;
        }

        exprs.empile(et);
    }

    return resultat;
}

void Simplificatrice::corrige_bloc_pour_assignation(NoeudExpression *expr,
                                                    NoeudExpression *ref_temp)
{
    if (expr->est_bloc()) {
        auto bloc = expr->comme_bloc();

        auto di = bloc->expressions->derniere();
        di = assem->cree_assignation_variable(di->lexeme, ref_temp, di);
        bloc->expressions->supprime_dernier();
        bloc->expressions->ajoute(di);
    }
    else if (expr->est_si()) {
        auto si = expr->comme_si();
        corrige_bloc_pour_assignation(si->bloc_si_vrai, ref_temp);
        corrige_bloc_pour_assignation(si->bloc_si_faux, ref_temp);
    }
    else if (expr->est_saufsi()) {
        auto si = expr->comme_saufsi();
        corrige_bloc_pour_assignation(si->bloc_si_vrai, ref_temp);
        corrige_bloc_pour_assignation(si->bloc_si_faux, ref_temp);
    }
    else {
        assert_rappel(false, [&]() {
            erreur::imprime_site(*espace, expr);
            std::cerr << "Expression invalide pour la simplification de l'assignation implicite "
                         "d'un bloc si !\n";
        });
    }
}

void Simplificatrice::simplifie_comparaison_chainee(NoeudExpressionBinaire *comp)
{
    auto comparaisons = kuri::tableau<NoeudExpressionBinaire>();
    rassemble_operations_chainees(comp, comparaisons);

    /*
      a <= b <= c

      a <= b && b <= c && c <= d

      &&
        &&
          a <= b
          b <= c
        c <= d
     */

    static const Lexeme lexeme_et = {",", {}, GenreLexeme::ESP_ESP, 0, 0, 0};
    comp->substitution = cree_expression_pour_op_chainee(comparaisons, &lexeme_et);
}

/* Les retours sont simplifiés sous forme d'assignations des valeurs de retours,
 * et d'un chargement pour les retours simples. */
void Simplificatrice::simplifie_retour(NoeudRetour *inst)
{
    // crée une assignation pour chaque sortie
    auto type_fonction = fonction_courante->type->comme_fonction();
    auto type_sortie = type_fonction->type_sortie;

    if (type_sortie->est_rien()) {
        return;
    }

    POUR (inst->donnees_exprs.plage()) {
        simplifie(it.expression);

        /* Les variables sont les déclarations des paramètres, donc crée des références. */
        for (auto &var : it.variables.plage()) {
            var = assem->cree_reference_declaration(var->lexeme,
                                                    var->comme_declaration_variable());
        }
    }

    auto assignation = assem->cree_assignation_variable(inst->lexeme);
    assignation->expression = inst->expression;
    assignation->donnees_exprs = std::move(inst->donnees_exprs);

    auto retour = assem->cree_retourne(inst->lexeme);

    if (type_sortie->est_rien()) {
        retour->expression = nullptr;
    }
    else {
        retour->expression = assem->cree_reference_declaration(
            fonction_courante->param_sortie->lexeme, fonction_courante->param_sortie);
    }

    auto bloc = assem->cree_bloc_seul(inst->lexeme, inst->bloc_parent);
    bloc->expressions->ajoute(assignation);
    bloc->expressions->ajoute(retour);
    retour->bloc_parent = bloc;

    inst->substitution = bloc;
    return;
}

NoeudExpression *Simplificatrice::simplifie_operateur_binaire(NoeudExpressionBinaire *expr_bin,
                                                              bool pour_operande)
{
    if (expr_bin->op->est_basique) {
        if (!pour_operande) {
            return nullptr;
        }

        /* Crée une nouvelle expression binaire afin d'éviter les dépassements de piles car
         * sinon la substitution serait toujours réévaluée lors de l'évaluation de l'expression
         * d'assignation. */
        return assem->cree_expression_binaire(
            expr_bin->lexeme, expr_bin->op, expr_bin->operande_gauche, expr_bin->operande_droite);
    }

    auto appel = assem->cree_appel(
        expr_bin->lexeme, expr_bin->op->decl, expr_bin->op->type_resultat);

    if (expr_bin->permute_operandes) {
        appel->parametres_resolus.ajoute(expr_bin->operande_droite);
        appel->parametres_resolus.ajoute(expr_bin->operande_gauche);
    }
    else {
        appel->parametres_resolus.ajoute(expr_bin->operande_gauche);
        appel->parametres_resolus.ajoute(expr_bin->operande_droite);
    }

    return appel;
}

void Simplificatrice::simplifie_coroutine(NoeudDeclarationEnteteFonction *corout)
{
#if 0
	compilatrice.commence_fonction(decl);

	/* Crée fonction */
	auto nom_fonction = decl->nom_broye;
	auto nom_type_coro = "__etat_coro" + nom_fonction;

	/* Déclare la structure d'état de la coroutine. */
	constructrice << "typedef struct " << nom_type_coro << " {\n";
	constructrice << "pthread_mutex_t mutex_boucle;\n";
	constructrice << "pthread_cond_t cond_boucle;\n";
	constructrice << "pthread_mutex_t mutex_coro;\n";
	constructrice << "pthread_cond_t cond_coro;\n";
	constructrice << "bool __termine_coro;\n";
	constructrice << "ContexteProgramme contexte;\n";

	auto idx_ret = 0l;
	POUR (decl->type_fonc->types_sorties) {
		auto &nom_ret = decl->noms_retours[idx_ret++];
		constructrice.declare_variable(it, nom_ret, "");
	}

	POUR (decl->params) {
		auto nom_broye = broye_nom_simple(it->ident);
		constructrice.declare_variable(it->type, nom_broye, "");
	}

	constructrice << " } " << nom_type_coro << ";\n";

	/* Déclare la fonction. */
	constructrice << "static void *" << nom_fonction << "(\nvoid *data)\n";
	constructrice << "{\n";
	constructrice << nom_type_coro << " *__etat = (" << nom_type_coro << " *) data;\n";
	constructrice << "ContexteProgramme contexte = __etat->contexte;\n";

	/* déclare les paramètres. */
	POUR (decl->params) {
		auto nom_broye = broye_nom_simple(it->ident);
		constructrice.declare_variable(it->type, nom_broye, "__etat->" + nom_broye);
	}

	/* Crée code pour le bloc. */
	genere_code_C(decl->bloc, constructrice, compilatrice, false);

	if (b->aide_generation_code == REQUIERS_CODE_EXTRA_RETOUR) {
		genere_code_extra_pre_retour(decl->bloc, compilatrice, constructrice);
	}

	constructrice << "}\n";

	compilatrice.termine_fonction();
	noeud->drapeaux |= RI_FUT_GENEREE;
#else
    static_cast<void>(corout);
#endif
    return;
}

void Simplificatrice::simplifie_retiens(NoeudRetiens *retiens)
{
#if 0
	auto inst = static_cast<NoeudExpressionUnaire *>(noeud);
	auto df = compilatrice.donnees_fonction;
	auto enfant = inst->expr;

	constructrice << "pthread_mutex_lock(&__etat->mutex_coro);\n";

	auto feuilles = dls::tablet<NoeudExpression *, 10>{};
	rassemble_feuilles(enfant, feuilles);

	for (auto i = 0l; i < feuilles.taille(); ++i) {
		auto f = feuilles[i];

		genere_code_C(f, constructrice, compilatrice, true);

		constructrice << "__etat->" << df->noms_retours[i] << " = ";
		constructrice << f->chaine_calculee();
		constructrice << ";\n";
	}

	constructrice << "pthread_mutex_lock(&__etat->mutex_boucle);\n";
	constructrice << "pthread_cond_signal(&__etat->cond_boucle);\n";
	constructrice << "pthread_mutex_unlock(&__etat->mutex_boucle);\n";
	constructrice << "pthread_cond_wait(&__etat->cond_coro, &__etat->mutex_coro);\n";
	constructrice << "pthread_mutex_unlock(&__etat->mutex_coro);\n";
#else
    static_cast<void>(retiens);
#endif
    return;
}

static auto trouve_index_membre(TypeCompose *type_compose, IdentifiantCode *nom_membre)
{
    auto idx_membre = 0u;

    POUR (type_compose->membres) {
        if (it.nom == nom_membre) {
            break;
        }

        idx_membre += 1;
    }

    return idx_membre;
}

static auto trouve_index_membre(TypeCompose *type_compose, Type *type_membre)
{
    auto idx_membre = 0u;

    POUR (type_compose->membres) {
        if (it.type == type_membre) {
            break;
        }

        idx_membre += 1;
    }

    return idx_membre;
}

static int valeur_enum(TypeEnum *type_enum, IdentifiantCode *ident)
{
    auto decl_enum = type_enum->decl;
    auto index_membre = 0;

    POUR (*decl_enum->bloc->membres.verrou_lecture()) {
        if (it->ident == ident) {
            break;
        }

        index_membre += 1;
    }

    return type_enum->membres[index_membre].valeur;
}

enum {
    DISCR_UNION,
    DISCR_UNION_ANONYME,
    DISCR_DEFAUT,
    DISCR_ENUM,
};

template <int N>
void Simplificatrice::simplifie_discr_impl(NoeudDiscr *discr)
{
    /*

      discr x {
        a { ... }
        b, c { ... }
        sinon { ... }
      }

      si x == a {
        ...
      }
      sinon si x == b || x == c {
        ...
      }
      sinon {
        ...
      }

     */

    static const Lexeme lexeme_ou = {",", {}, GenreLexeme::BARRE_BARRE, 0, 0, 0};

    auto si = assem->cree_si(discr->lexeme, GenreNoeud::INSTRUCTION_SI);
    auto si_courant = si;

    discr->substitution = si;

    auto expression = NoeudExpression::nul();
    if (N == DISCR_UNION || N == DISCR_UNION_ANONYME) {
        /* nous utilisons directement un accès de membre... il faudra proprement gérer les unions
         */
        expression = assem->cree_reference_membre(discr->expression_discriminee->lexeme,
                                                  discr->expression_discriminee,
                                                  typeuse[TypeBase::Z32],
                                                  1);
    }
    else {
        expression = discr->expression_discriminee;
    }

    for (auto i = 0; i < discr->paires_discr.taille(); ++i) {
        auto &it = discr->paires_discr[i];
        auto virgule = it->expression->comme_virgule();

        // crée les comparaisons
        kuri::tableau<NoeudExpressionBinaire> comparaisons;

        for (auto expr : virgule->expressions) {
            auto comparaison = NoeudExpressionBinaire();
            comparaison.lexeme = discr->lexeme;
            comparaison.op = discr->op;
            comparaison.operande_gauche = expression;

            if (N == DISCR_ENUM) {
                auto valeur = valeur_enum(static_cast<TypeEnum *>(expression->type), expr->ident);
                auto constante = assem->cree_litterale_entier(
                    expr->lexeme, expression->type, static_cast<unsigned long>(valeur));
                comparaison.operande_droite = constante;
            }
            else if (N == DISCR_UNION) {
                auto const type_union = discr->expression_discriminee->type->comme_union();
                auto index = trouve_index_membre(type_union, expr->ident);
                auto constante = assem->cree_litterale_entier(
                    expr->lexeme, expression->type, static_cast<unsigned long>(index + 1));
                comparaison.operande_droite = constante;
            }
            else if (N == DISCR_UNION_ANONYME) {
                auto const type_union = discr->expression_discriminee->type->comme_union();
                auto index = trouve_index_membre(type_union, expr->type);
                auto constante = assem->cree_litterale_entier(
                    expr->lexeme, expression->type, static_cast<unsigned long>(index + 1));
                comparaison.operande_droite = constante;
            }
            else {
                /* cette expression est simplifiée via cree_expression_pour_op_chainee */
                comparaison.operande_droite = expr;
            }

            comparaisons.ajoute(comparaison);
        }

        si_courant->condition = cree_expression_pour_op_chainee(comparaisons, &lexeme_ou);

        // À FAIRE(union) : création d'une variable si nous avons une union
        simplifie(it->bloc);
        si_courant->bloc_si_vrai = it->bloc;

        if (i != (discr->paires_discr.taille() - 1)) {
            si = assem->cree_si(discr->lexeme, GenreNoeud::INSTRUCTION_SI);
            si_courant->bloc_si_faux = si;
            si_courant = si;
        }
    }

    simplifie(discr->bloc_sinon);
    si_courant->bloc_si_faux = discr->bloc_sinon;

#if 0
	/* génération du code pour l'expression contre laquelle nous testons */
	if (noeud->genre == GenreNoeud::INSTRUCTION_DISCR_ENUM) {
  valeur_f = valeur_enum(static_cast<TypeEnum *>(expression->type), f->ident);
	}
	else if (noeud->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
		auto type_union = noeud->expr->type->comme_union();
		if (type_union->est_anonyme) {
			unsigned idx_membre = 0;

			POUR (type_union->membres) {
				if (it.type == f->type) {
					break;
				}

				idx_membre += 1;
			}

			valeur_f = cree_z32(idx_membre + 1);

			/* ajout du membre au bloc */
			auto valeur = cree_acces_membre(noeud, ptr_structure, 0);
			table_locales[f->ident] = valeur;
		}
		else {
			auto idx_membre = trouve_index_membre(decl_struct, f->ident);
			valeur_f = cree_z32(idx_membre + 1);

			/* ajout du membre au bloc */
			auto valeur = cree_acces_membre(noeud, ptr_structure, 0);
			table_locales[f->ident] = valeur;
		}
	}
	else {
		genere_ri_pour_expression_droite(f, nullptr);
		valeur_f = depile_valeur();
	}

#endif
}

void Simplificatrice::simplifie_discr(NoeudDiscr *discr)
{
    if (discr->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
        auto const type_union = discr->expression_discriminee->type->comme_union();

        if (type_union->est_anonyme) {
            simplifie_discr_impl<DISCR_UNION_ANONYME>(discr);
        }
        else {
            simplifie_discr_impl<DISCR_UNION>(discr);
        }
    }
    else if (discr->genre == GenreNoeud::INSTRUCTION_DISCR_ENUM) {
        simplifie_discr_impl<DISCR_ENUM>(discr);
    }
    else {
        simplifie_discr_impl<DISCR_DEFAUT>(discr);
    }
}

NoeudSi *Simplificatrice::cree_condition_boucle(NoeudExpression *inst, GenreNoeud genre_noeud)
{
    static const Lexeme lexeme_arrete = {",", {}, GenreLexeme::ARRETE, 0, 0, 0};

    /* condition d'arrêt de la boucle */
    auto condition = assem->cree_si(inst->lexeme, genre_noeud);
    auto bloc_si_vrai = assem->cree_bloc_seul(inst->lexeme, inst->bloc_parent);

    auto arrete = assem->cree_arrete(&lexeme_arrete);
    arrete->drapeaux |= EST_IMPLICITE;
    arrete->boucle_controlee = inst;
    arrete->bloc_parent = bloc_si_vrai;

    bloc_si_vrai->expressions->ajoute(arrete);
    condition->bloc_si_vrai = bloc_si_vrai;

    return condition;
}

void simplifie_arbre(EspaceDeTravail *espace,
                     AssembleuseArbre *assem,
                     Typeuse &typeuse,
                     NoeudExpression *arbre)
{
    auto simplificatrice = Simplificatrice(espace, assem, typeuse);
    simplificatrice.simplifie(arbre);
}

#if 0
bool expression_est_constante(NoeudExpression *expression)
{
	switch (expression->genre) {
		default:
		{
			return false;
		}
		case GenreNoeud::EXPRESSION_LITTERALE_NUL:
		case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
		case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
		case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
		case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
		case GenreNoeud::EXPRESSION_TYPE_DE:
		case GenreNoeud::EXPRESSION_TAILLE_DE:
		case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
		{
			return true;
		}
		case GenreNoeud::EXPRESSION_PARENTHESE:
		case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
		case GenreNoeud::OPERATEUR_UNAIRE:
		{
			auto op = static_cast<NoeudExpressionUnaire *>(expression);
			return expression_est_constante(op->expr);
		}
		case GenreNoeud::OPERATEUR_BINAIRE:
		{
			auto op = static_cast<NoeudExpressionBinaire *>(expression);

			if (!expression_est_constante(op->operande_gauche)) {
				return false;
			}

			if (!expression_est_constante(op->operande_droite)) {
				return false;
			}

			return true;
		}
		case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
		case GenreNoeud::EXPRESSION_APPEL:
		{
			auto appel = static_cast<NoeudExpressionAppel *>(expression);

			POUR (appel->exprs) {
				if (!expression_est_constante(it)) {
					return false;
				}
			}

			return true;
		}
		case GenreNoeud::EXPRESSION_VIRGULE:
		{
			auto op = static_cast<NoeudExpressionVirgule *>(expression);

			POUR (op->expressions) {
				if (!expression_est_constante(it)) {
					return false;
				}
			}

			return true;
		}
	}
}
#endif

// -----------------------------------------------------------------------------
// Implémentation des méthodes supplémentaires de l'arbre syntaxique

kuri::chaine const &NoeudDeclarationEnteteFonction::nom_broye(EspaceDeTravail *espace)
{
    if (nom_broye_ != "") {
        return nom_broye_;
    }

    if (ident != ID::principale && !possede_drapeau(EST_EXTERNE | FORCE_SANSBROYAGE)) {
        auto fichier = espace->compilatrice().fichier(lexeme->fichier);

        if (est_metaprogramme) {
            nom_broye_ = enchaine("metaprogramme", this);
        }
        else if (est_initialisation_type) {
            auto type_param = params[0]->type->comme_pointeur()->type_pointe;
            // Ajout du pointeur du type pour différencier les types monomorphisés.
            nom_broye_ = enchaine("initialise_", type_param);
        }
        else {
            nom_broye_ = broye_nom_fonction(this, fichier->module->nom()->nom);
        }
    }
    else {
        nom_broye_ = lexeme->chaine;
    }

    return nom_broye_;
}

// -----------------------------------------------------------------------------
// Implémentation des fonctions supplémentaires de la ConvertisseuseNoeudCode

InfoType *ConvertisseuseNoeudCode::cree_info_type_pour(Type *type)
{
    auto cree_info_type_entier = [this](uint taille_en_octet, bool est_signe) {
        auto info_type = allocatrice_infos_types.infos_types_entiers.ajoute_element();
        info_type->genre = GenreInfoType::ENTIER;
        info_type->taille_en_octet = taille_en_octet;
        info_type->est_signe = est_signe;

        return info_type;
    };

    // À FAIRE : il est possible que les types ne soient pas encore validé quand nous générons des
    // messages pour les entêtes de fonctions
    if (type == nullptr) {
        return nullptr;
    }

    if (type->info_type != nullptr) {
        return type->info_type;
    }

    switch (type->genre) {
        case GenreType::POLYMORPHIQUE:
        case GenreType::TUPLE:
        {
            return nullptr;
        }
        case GenreType::OCTET:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::OCTET;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreType::BOOL:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::BOOLEEN;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreType::CHAINE:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::CHAINE;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreType::EINI:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::EINI;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreType::TABLEAU_DYNAMIQUE:
        {
            auto type_tableau = type->comme_tableau_dynamique();

            auto info_type = allocatrice_infos_types.infos_types_tableaux.ajoute_element();
            info_type->genre = GenreInfoType::TABLEAU;
            info_type->taille_en_octet = type->taille_octet;
            info_type->est_tableau_fixe = false;
            info_type->taille_fixe = 0;
            info_type->type_pointe = cree_info_type_pour(type_tableau->type_pointe);

            type->info_type = info_type;
            break;
        }
        case GenreType::VARIADIQUE:
        {
            auto type_variadique = type->comme_variadique();

            auto info_type = allocatrice_infos_types.infos_types_tableaux.ajoute_element();
            info_type->genre = GenreInfoType::TABLEAU;
            info_type->taille_en_octet = type->taille_octet;
            info_type->est_tableau_fixe = false;
            info_type->taille_fixe = 0;

            // type nul pour les types variadiques des fonctions externes (p.e. printf(const char
            // *, ...))
            if (type_variadique->type_pointe) {
                info_type->type_pointe = cree_info_type_pour(type_variadique->type_pointe);
            }

            type->info_type = info_type;
            break;
        }
        case GenreType::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_tableau_fixe();

            auto info_type = allocatrice_infos_types.infos_types_tableaux.ajoute_element();
            info_type->genre = GenreInfoType::TABLEAU;
            info_type->taille_en_octet = type->taille_octet;
            info_type->est_tableau_fixe = true;
            info_type->taille_fixe = type_tableau->taille;
            info_type->type_pointe = cree_info_type_pour(type_tableau->type_pointe);

            type->info_type = info_type;
            break;
        }
        case GenreType::ENTIER_CONSTANT:
        {
            type->info_type = cree_info_type_entier(4, true);
            break;
        }
        case GenreType::ENTIER_NATUREL:
        {
            type->info_type = cree_info_type_entier(type->taille_octet, false);
            break;
        }
        case GenreType::ENTIER_RELATIF:
        {
            type->info_type = cree_info_type_entier(type->taille_octet, true);
            break;
        }
        case GenreType::REEL:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::REEL;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreType::RIEN:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::RIEN;
            info_type->taille_en_octet = 0;

            type->info_type = info_type;
            break;
        }
        case GenreType::TYPE_DE_DONNEES:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::TYPE_DE_DONNEES;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreType::POINTEUR:
        case GenreType::REFERENCE:
        {
            auto info_type = allocatrice_infos_types.infos_types_pointeurs.ajoute_element();
            info_type->genre = GenreInfoType::POINTEUR;
            info_type->type_pointe = cree_info_type_pour(type_dereference_pour(type));
            info_type->taille_en_octet = type->taille_octet;
            info_type->est_reference = type->genre == GenreType::REFERENCE;

            type->info_type = info_type;
            break;
        }
        case GenreType::STRUCTURE:
        {
            auto type_struct = type->comme_structure();

            auto info_type = allocatrice_infos_types.infos_types_structures.ajoute_element();
            type->info_type = info_type;

            info_type->genre = GenreInfoType::STRUCTURE;
            info_type->taille_en_octet = type->taille_octet;

            if (type_struct->nom) {
                info_type->nom = type_struct->nom->nom;
            }
            else {
                info_type->nom = "anonyme";
            }

            info_type->membres.reserve(type_struct->membres.taille());

            POUR (type_struct->membres) {
                auto info_type_membre =
                    allocatrice_infos_types.infos_types_membres_structures.ajoute_element();
                info_type_membre->info = cree_info_type_pour(it.type);
                info_type_membre->decalage = it.decalage;
                info_type_membre->nom = it.nom->nom;
                info_type_membre->drapeaux = it.drapeaux;

                info_type->membres.ajoute(info_type_membre);
            }

            info_type->structs_employees.reserve(type_struct->types_employes.taille());
            POUR (type_struct->types_employes) {
                auto info_struct_employe = cree_info_type_pour(it);
                info_type->structs_employees.ajoute(
                    static_cast<InfoTypeStructure *>(info_struct_employe));
            }

            break;
        }
        case GenreType::UNION:
        {
            auto type_union = type->comme_union();

            auto info_type = allocatrice_infos_types.infos_types_unions.ajoute_element();
            info_type->genre = GenreInfoType::UNION;
            info_type->est_sure = !type_union->est_nonsure;
            info_type->type_le_plus_grand = cree_info_type_pour(type_union->type_le_plus_grand);
            info_type->decalage_index = type_union->decalage_index;
            info_type->taille_en_octet = type_union->taille_octet;

            if (type_union->nom) {
                info_type->nom = type_union->nom->nom;
            }
            else {
                info_type->nom = "anonyme";
            }

            info_type->membres.reserve(type_union->membres.taille());

            POUR (type_union->membres) {
                auto info_type_membre =
                    allocatrice_infos_types.infos_types_membres_structures.ajoute_element();
                info_type_membre->info = cree_info_type_pour(it.type);
                info_type_membre->decalage = it.decalage;
                info_type_membre->nom = it.nom->nom;
                info_type_membre->drapeaux = it.drapeaux;

                info_type->membres.ajoute(info_type_membre);
            }

            type->info_type = info_type;
            break;
        }
        case GenreType::ENUM:
        case GenreType::ERREUR:
        {
            auto type_enum = static_cast<TypeEnum *>(type);

            auto info_type = allocatrice_infos_types.infos_types_enums.ajoute_element();
            info_type->genre = GenreInfoType::ENUM;
            info_type->nom = type_enum->nom->nom;
            info_type->est_drapeau = type_enum->est_drapeau;
            info_type->taille_en_octet = type_enum->taille_octet;

            info_type->noms.reserve(type_enum->membres.taille());
            info_type->valeurs.reserve(type_enum->membres.taille());

            POUR (type_enum->membres) {
                if (it.drapeaux == TypeCompose::Membre::EST_IMPLICITE) {
                    continue;
                }

                info_type->noms.ajoute(it.nom->nom);
                info_type->valeurs.ajoute(it.valeur);
            }

            type->info_type = info_type;
            break;
        }
        case GenreType::FONCTION:
        {
            auto type_fonction = type->comme_fonction();

            auto info_type = allocatrice_infos_types.infos_types_fonctions.ajoute_element();
            info_type->genre = GenreInfoType::FONCTION;
            info_type->est_coroutine = false;
            info_type->taille_en_octet = type->taille_octet;

            info_type->types_entrees.reserve(type_fonction->types_entrees.taille());

            POUR (type_fonction->types_entrees) {
                info_type->types_entrees.ajoute(cree_info_type_pour(it));
            }

            auto type_sortie = type_fonction->type_sortie;

            if (type_sortie->est_tuple()) {
                auto tuple = type_sortie->comme_tuple();
                info_type->types_sorties.reserve(tuple->membres.taille());

                POUR (tuple->membres) {
                    info_type->types_sorties.ajoute(cree_info_type_pour(it.type));
                }
            }
            else {
                info_type->types_sorties.reserve(1);
                info_type->types_sorties.ajoute(cree_info_type_pour(type_sortie));
            }

            type->info_type = info_type;
            break;
        }
        case GenreType::OPAQUE:
        {
            auto type_opaque = type->comme_opaque();

            auto info_type = allocatrice_infos_types.infos_types_opaques.ajoute_element();
            info_type->nom = type_opaque->ident->nom;
            info_type->type_opacifie = cree_info_type_pour(type_opaque->type_opacifie);

            type->info_type = info_type;
            break;
        }
    }

    return type->info_type;
}

Type *ConvertisseuseNoeudCode::convertis_info_type(Typeuse &typeuse, InfoType *type)
{
    switch (type->genre) {
        case GenreInfoType::EINI:
        {
            return typeuse[TypeBase::EINI];
        }
        case GenreInfoType::REEL:
        {
            if (type->taille_en_octet == 2) {
                return typeuse[TypeBase::R16];
            }

            if (type->taille_en_octet == 4) {
                return typeuse[TypeBase::R32];
            }

            if (type->taille_en_octet == 8) {
                return typeuse[TypeBase::R64];
            }

            return nullptr;
        }
        case GenreInfoType::ENTIER:
        {
            const auto info_type_entier = static_cast<const InfoTypeEntier *>(type);

            if (info_type_entier->est_signe) {
                if (type->taille_en_octet == 1) {
                    return typeuse[TypeBase::Z8];
                }

                if (type->taille_en_octet == 2) {
                    return typeuse[TypeBase::Z16];
                }

                if (type->taille_en_octet == 4) {
                    return typeuse[TypeBase::Z32];
                }

                if (type->taille_en_octet == 8) {
                    return typeuse[TypeBase::Z64];
                }

                return nullptr;
            }

            if (type->taille_en_octet == 1) {
                return typeuse[TypeBase::N8];
            }

            if (type->taille_en_octet == 2) {
                return typeuse[TypeBase::N16];
            }

            if (type->taille_en_octet == 4) {
                return typeuse[TypeBase::N32];
            }

            if (type->taille_en_octet == 8) {
                return typeuse[TypeBase::N64];
            }

            return nullptr;
        }
        case GenreInfoType::OCTET:
        {
            return typeuse[TypeBase::OCTET];
        }
        case GenreInfoType::BOOLEEN:
        {
            return typeuse[TypeBase::BOOL];
        }
        case GenreInfoType::CHAINE:
        {
            return typeuse[TypeBase::CHAINE];
        }
        case GenreInfoType::RIEN:
        {
            return typeuse[TypeBase::RIEN];
        }
        case GenreInfoType::POINTEUR:
        {
            const auto info_type_pointeur = static_cast<const InfoTypePointeur *>(type);

            auto type_pointe = convertis_info_type(typeuse, info_type_pointeur->type_pointe);

            if (info_type_pointeur->est_reference) {
                return typeuse.type_reference_pour(type_pointe);
            }

            return typeuse.type_pointeur_pour(type_pointe);
        }
        case GenreInfoType::TABLEAU:
        {
            const auto info_type_tableau = static_cast<const InfoTypeTableau *>(type);

            auto type_pointe = convertis_info_type(typeuse, info_type_tableau->type_pointe);

            if (info_type_tableau->est_tableau_fixe) {
                return typeuse.type_tableau_fixe(type_pointe, info_type_tableau->taille_fixe);
            }

            return typeuse.type_tableau_dynamique(type_pointe);
        }
        case GenreInfoType::TYPE_DE_DONNEES:
        {
            // À FAIRE : préserve l'information de type connu
            return typeuse.type_type_de_donnees(nullptr);
        }
        case GenreInfoType::FONCTION:
        {
            // À FAIRE
            return nullptr;
        }
        case GenreInfoType::STRUCTURE:
        {
            // À FAIRE
            return nullptr;
        }
        case GenreInfoType::ENUM:
        {
            // À FAIRE
            return nullptr;
        }
        case GenreInfoType::UNION:
        {
            // À FAIRE
            return nullptr;
        }
        case GenreInfoType::OPAQUE:
        {
            // À FAIRE
            return nullptr;
        }
    }

    return nullptr;
}

// -----------------------------------------------------------------------------
// Implémentation des fonctions supplémentaires de l'AssembleuseArbre

NoeudExpressionBinaire *AssembleuseArbre::cree_expression_binaire(const Lexeme *lexeme,
                                                                  const OperateurBinaire *op,
                                                                  NoeudExpression *expr1,
                                                                  NoeudExpression *expr2)
{
    assert(op);
    auto op_bin = cree_expression_binaire(lexeme);
    op_bin->operande_gauche = expr1;
    op_bin->operande_droite = expr2;
    op_bin->op = op;
    op_bin->type = op->type_resultat;
    return op_bin;
}

NoeudExpressionReference *AssembleuseArbre::cree_reference_declaration(const Lexeme *lexeme,
                                                                       NoeudDeclaration *decl)
{
    auto ref = cree_reference_declaration(lexeme);
    ref->declaration_referee = decl;
    ref->type = decl->type;
    ref->ident = decl->ident;
    return ref;
}

NoeudSi *AssembleuseArbre::cree_si(const Lexeme *lexeme, GenreNoeud genre_noeud)
{
    if (genre_noeud == GenreNoeud::INSTRUCTION_SI) {
        return static_cast<NoeudSi *>(cree_noeud<GenreNoeud::INSTRUCTION_SI>(lexeme));
    }

    return static_cast<NoeudSi *>(cree_noeud<GenreNoeud::INSTRUCTION_SAUFSI>(lexeme));
}

NoeudBloc *AssembleuseArbre::cree_bloc_seul(const Lexeme *lexeme, NoeudBloc *bloc_parent)
{
    auto bloc = cree_noeud<GenreNoeud::INSTRUCTION_COMPOSEE>(lexeme)->comme_bloc();
    bloc->bloc_parent = bloc_parent;
    return bloc;
}

NoeudAssignation *AssembleuseArbre::cree_assignation_variable(const Lexeme *lexeme,
                                                              NoeudExpression *assignee,
                                                              NoeudExpression *expression)
{
    auto assignation = cree_noeud<GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE>(lexeme)
                           ->comme_assignation_variable();

    auto donnees = DonneesAssignations();
    donnees.expression = expression;
    donnees.variables.ajoute(assignee);
    donnees.transformations.ajoute({});

    assignation->donnees_exprs.ajoute(donnees);

    return assignation;
}

NoeudDeclarationVariable *AssembleuseArbre::cree_declaration_variable(const Lexeme *lexeme,
                                                                      Type *type,
                                                                      IdentifiantCode *ident,
                                                                      NoeudExpression *expression)
{
    auto ref = cree_reference_declaration(lexeme);
    ref->ident = ident;
    ref->type = type;
    return cree_declaration_variable(ref, expression);
}

NoeudDeclarationVariable *AssembleuseArbre::cree_declaration_variable(
    NoeudExpressionReference *ref, NoeudExpression *expression)
{
    auto declaration = cree_declaration_variable(ref->lexeme);
    declaration->ident = ref->ident;
    declaration->type = ref->type;
    declaration->valeur = ref;
    declaration->expression = expression;

    ref->declaration_referee = declaration;

    auto donnees = DonneesAssignations();
    donnees.expression = expression;
    donnees.variables.ajoute(ref);
    donnees.transformations.ajoute({});

    declaration->donnees_decl.ajoute(donnees);

    return declaration;
}

NoeudDeclarationVariable *AssembleuseArbre::cree_declaration_variable(
    NoeudExpressionReference *ref)
{
    auto decl = cree_declaration_variable(ref->lexeme);
    decl->valeur = ref;
    decl->ident = ref->ident;
    ref->declaration_referee = decl;
    return decl;
}

NoeudExpressionMembre *AssembleuseArbre::cree_reference_membre(const Lexeme *lexeme,
                                                               NoeudExpression *accede,
                                                               Type *type,
                                                               int index)
{
    auto acces = cree_reference_membre(lexeme);
    acces->accedee = accede;
    acces->type = type;
    acces->index_membre = index;
    return acces;
}

NoeudExpressionBinaire *AssembleuseArbre::cree_indexage(const Lexeme *lexeme,
                                                        NoeudExpression *expr1,
                                                        NoeudExpression *expr2,
                                                        bool ignore_verification)
{
    auto indexage = cree_noeud<GenreNoeud::EXPRESSION_INDEXAGE>(lexeme)->comme_indexage();
    indexage->operande_gauche = expr1;
    indexage->operande_droite = expr2;
    indexage->type = type_dereference_pour(expr1->type);
    if (ignore_verification) {
        indexage->aide_generation_code = IGNORE_VERIFICATION;
    }
    return indexage;
}

NoeudExpressionAppel *AssembleuseArbre::cree_appel(const Lexeme *lexeme,
                                                   NoeudExpression *appelee,
                                                   Type *type)
{
    auto appel = cree_appel(lexeme);
    appel->noeud_fonction_appelee = appelee;
    appel->type = type;

    if (appelee->est_entete_fonction()) {
        appel->expression = cree_reference_declaration(lexeme, appelee->comme_entete_fonction());
    }
    else {
        appel->expression = appelee;
    }

    return appel;
}

NoeudExpressionAppel *AssembleuseArbre::cree_construction_structure(const Lexeme *lexeme,
                                                                    TypeCompose *type)
{
    auto structure = cree_appel(lexeme);
    structure->genre = GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE;
    structure->parametres_resolus.reserve(type->membres.taille());

    if (type->est_structure()) {
        structure->expression = type->comme_structure()->decl;
        structure->noeud_fonction_appelee = type->comme_structure()->decl;
    }
    else if (type->est_union()) {
        structure->expression = type->comme_union()->decl;
        structure->noeud_fonction_appelee = type->comme_union()->decl;
    }

    structure->type = type;
    return structure;
}

NoeudExpressionLitteraleEntier *AssembleuseArbre::cree_litterale_entier(Lexeme const *lexeme,
                                                                        Type *type,
                                                                        unsigned long valeur)
{
    auto lit = cree_litterale_entier(lexeme);
    lit->type = type;
    lit->valeur = valeur;
    return lit;
}

NoeudExpressionLitteraleBool *AssembleuseArbre::cree_litterale_bool(Lexeme const *lexeme,
                                                                    Type *type,
                                                                    bool valeur)
{
    auto lit = cree_litterale_bool(lexeme);
    lit->type = type;
    lit->valeur = valeur;
    return lit;
}

NoeudExpressionLitteraleReel *AssembleuseArbre::cree_litterale_reel(Lexeme const *lexeme,
                                                                    Type *type,
                                                                    double valeur)
{
    auto lit = cree_litterale_reel(lexeme);
    lit->type = type;
    lit->valeur = valeur;
    return lit;
}

NoeudExpression *AssembleuseArbre::cree_reference_type(Lexeme const *lexeme, Type *type)
{
    auto ref_type = cree_reference_type(lexeme);
    ref_type->type = type;
    return ref_type;
}

NoeudAssignation *AssembleuseArbre::cree_incrementation(const Lexeme *lexeme,
                                                        NoeudExpression *valeur)
{
    auto type = valeur->type;

    auto inc = cree_expression_binaire(lexeme);
    inc->op = type->operateur_ajt;
    assert(inc->op);
    inc->operande_gauche = valeur;
    inc->type = type;

    if (est_type_entier(type)) {
        inc->operande_droite = cree_litterale_entier(valeur->lexeme, type, 1);
    }
    else if (type->est_reel()) {
        // À FAIRE(r16)
        inc->operande_droite = cree_litterale_reel(valeur->lexeme, type, 1.0);
    }

    return cree_assignation_variable(valeur->lexeme, valeur, inc);
}

NoeudAssignation *AssembleuseArbre::cree_decrementation(const Lexeme *lexeme,
                                                        NoeudExpression *valeur)
{
    auto type = valeur->type;

    auto inc = cree_expression_binaire(lexeme);
    inc->op = type->operateur_sst;
    assert(inc->op);
    inc->operande_gauche = valeur;
    inc->type = type;

    if (est_type_entier(type)) {
        inc->operande_droite = cree_litterale_entier(valeur->lexeme, type, 1);
    }
    else if (type->est_reel()) {
        // À FAIRE(r16)
        inc->operande_droite = cree_litterale_reel(valeur->lexeme, type, 1.0);
    }

    return cree_assignation_variable(valeur->lexeme, valeur, inc);
}

static const char *ordre_fonction(NoeudDeclarationEnteteFonction const *entete)
{
    if (entete->est_operateur) {
        return "l'opérateur";
    }

    if (entete->est_coroutine) {
        return "la coroutine";
    }

    return "la fonction";
}

void imprime_details_fonction(EspaceDeTravail *espace,
                              NoeudDeclarationEnteteFonction const *entete,
                              std::ostream &os)
{
    os << "Détail pour " << ordre_fonction(entete) << " " << entete->lexeme->chaine << " :\n";
    os << "-- Type                    : " << chaine_type(entete->type) << '\n';
    os << "-- Est polymorphique       : " << std::boolalpha << entete->est_polymorphe << '\n';
    os << "-- Est #corps_texte        : " << std::boolalpha << entete->corps->est_corps_texte
       << '\n';
    os << "-- Entête fut validée      : " << std::boolalpha
       << entete->possede_drapeau(DECLARATION_FUT_VALIDEE) << '\n';
    os << "-- Corps fut validé        : " << std::boolalpha
       << entete->corps->possede_drapeau(DECLARATION_FUT_VALIDEE) << '\n';
    os << "-- Est monomorphisation    : " << std::boolalpha << entete->est_monomorphisation
       << '\n';
    os << "-- Est initialisation type : " << std::boolalpha << entete->est_initialisation_type
       << '\n';
    if (entete->est_monomorphisation) {
        os << "-- Paramètres de monomorphisation :\n";
        POUR ((*entete->bloc_constantes->membres.verrou_lecture())) {
            os << "     " << it->ident->nom << " : " << chaine_type(it->type) << '\n';
        }
    }
    if (espace) {
        os << "-- Site de définition :\n";
        erreur::imprime_site(*espace, entete);
    }
}

/* Fonctions d'initialisation des types. */

static Lexeme lexeme_sentinel = {};

static NoeudDeclarationEnteteFonction *cree_entete_pour_initialisation_type(
    Type *type, Compilatrice &compilatrice, AssembleuseArbre *assembleuse, Typeuse &typeuse)
{
    if (!type->fonction_init) {
        auto type_param = typeuse.type_pointeur_pour(type);
        if (type->est_union() && !type->comme_union()->est_nonsure) {
            auto type_normalise = normalise_type(typeuse, type);
            type_param = typeuse.type_pointeur_pour(type_normalise, false, false);
        }

        auto types_entrees = dls::tablet<Type *, 6>();
        types_entrees.ajoute(type_param);

        auto type_fonction = typeuse.type_fonction(types_entrees, typeuse[TypeBase::RIEN], false);

        static Lexeme lexeme_entete = {};
        auto entete = assembleuse->cree_entete_fonction(&lexeme_entete);
        entete->est_initialisation_type = true;

        entete->bloc_constantes = assembleuse->cree_bloc_seul(&lexeme_sentinel, nullptr);
        entete->bloc_parametres = assembleuse->cree_bloc_seul(&lexeme_sentinel,
                                                              entete->bloc_constantes);

        /* Paramètre d'entrée. */
        {
            static Lexeme lexeme_decl = {};
            auto decl_param = assembleuse->cree_declaration_variable(
                &lexeme_decl, type_param, ID::pointeur, nullptr);

            decl_param->type = type_param;
            decl_param->drapeaux |= DECLARATION_FUT_VALIDEE;

            entete->params.ajoute(decl_param);
        }

        /* Paramètre de sortie. */
        {
            static const Lexeme lexeme_rien = {"rien", {}, GenreLexeme::RIEN, 0, 0, 0};
            auto type_declare = assembleuse->cree_reference_type(&lexeme_rien);

            auto ident = compilatrice.table_identifiants->identifiant_pour_chaine("__ret0");

            auto ref = assembleuse->cree_reference_declaration(&lexeme_rien);
            ref->ident = ident;
            ref->type = typeuse[TypeBase::RIEN];

            auto decl = assembleuse->cree_declaration_variable(ref);
            decl->expression_type = type_declare;
            decl->type = ref->type;

            entete->params_sorties.ajoute(decl);
            entete->param_sortie = entete->params_sorties[0]->comme_declaration_variable();
        }

        entete->type = type_fonction;
        entete->drapeaux |= (FORCE_ENLIGNE | DECLARATION_FUT_VALIDEE | FORCE_NULCTX |
                             FORCE_SANSTRACE);

        type->fonction_init = entete;
    }

    return type->fonction_init;
}

static void cree_assignation(AssembleuseArbre *assembleuse,
                             NoeudExpression *variable,
                             NoeudExpression *expression)
{
    assert(variable->type);
    assert(expression->type);
    auto bloc = assembleuse->bloc_courant();
    auto assignation = assembleuse->cree_assignation_variable(
        &lexeme_sentinel, variable, expression);
    assignation->type = expression->type;
    bloc->expressions->ajoute(assignation);
}

static void cree_initialisation_defaut_pour_type(Type *type,
                                                 Compilatrice &compilatrice,
                                                 AssembleuseArbre *assembleuse,
                                                 NoeudExpression *ref_param,
                                                 NoeudExpression *expr_valeur_defaut,
                                                 Typeuse &typeuse)
{
    switch (type->genre) {
        case GenreType::RIEN:
        case GenreType::POLYMORPHIQUE:
        {
            break;
        }
        case GenreType::EINI:
        case GenreType::CHAINE:
        case GenreType::STRUCTURE:
        case GenreType::TABLEAU_DYNAMIQUE:
        case GenreType::VARIADIQUE:
        case GenreType::UNION:
        {
            static Lexeme lexeme_op = {};
            lexeme_op.genre = GenreLexeme::FOIS_UNAIRE;
            auto prise_adresse = assembleuse->cree_expression_unaire(&lexeme_op);
            prise_adresse->operande = ref_param;
            prise_adresse->type = typeuse.type_pointeur_pour(type);
            auto fonction = cree_entete_pour_initialisation_type(
                type, compilatrice, assembleuse, typeuse);
            auto appel = assembleuse->cree_appel(
                &lexeme_sentinel, fonction, typeuse[TypeBase::RIEN]);
            appel->parametres_resolus.ajoute(prise_adresse);
            appel->drapeaux |= FORCE_NULCTX;
            assembleuse->bloc_courant()->expressions->ajoute(appel);
            break;
        }
        case GenreType::BOOL:
        {
            static Lexeme litteral_bool = {};
            litteral_bool.genre = GenreLexeme::FAUX;
            auto valeur_defaut = expr_valeur_defaut;
            if (!valeur_defaut) {
                valeur_defaut = assembleuse->cree_litterale_bool(&litteral_bool);
                valeur_defaut->type = type;
            }
            cree_assignation(assembleuse, ref_param, valeur_defaut);
            break;
        }
        case GenreType::OCTET:
        case GenreType::ENTIER_CONSTANT:
        case GenreType::ENTIER_NATUREL:
        case GenreType::ENTIER_RELATIF:
        case GenreType::TYPE_DE_DONNEES:
        case GenreType::ENUM:
        case GenreType::ERREUR:
        {
            static Lexeme litteral = {};
            auto valeur_defaut = expr_valeur_defaut;
            if (!valeur_defaut) {
                valeur_defaut = assembleuse->cree_litterale_entier(&litteral, type, 0);
            }
            cree_assignation(assembleuse, ref_param, valeur_defaut);
            break;
        }
        case GenreType::REEL:
        {
            static Lexeme litteral = {};
            auto valeur_defaut = expr_valeur_defaut;
            if (!valeur_defaut) {
                valeur_defaut = assembleuse->cree_litterale_reel(&litteral, type, 0);
            }
            cree_assignation(assembleuse, ref_param, valeur_defaut);
            break;
        }
        case GenreType::REFERENCE:
        {
            break;
        }
        case GenreType::POINTEUR:
        case GenreType::FONCTION:
        {
            static Lexeme litteral = {};
            auto valeur_defaut = expr_valeur_defaut;
            if (!valeur_defaut) {
                valeur_defaut = assembleuse->cree_litterale_nul(&litteral);
            }
            valeur_defaut->type = ref_param->type;
            cree_assignation(assembleuse, ref_param, valeur_defaut);
            break;
        }
        case GenreType::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_tableau_fixe();
            auto type_pointe = type_tableau->type_pointe;

            auto type_pointeur_type_pointe = typeuse.type_pointeur_pour(type_pointe, false, false);

            /* Toutes les variables doivent être initialisées (ou nous devons nous assurer que tous
             * les types possibles créés par la compilation ont une fonction d'initalisation). */
            auto init_it = assembleuse->cree_litterale_nul(&lexeme_sentinel);
            init_it->type = type_pointeur_type_pointe;

            auto decl_it = assembleuse->cree_declaration_variable(
                &lexeme_sentinel, type_pointeur_type_pointe, ID::it, init_it);
            auto ref_it = assembleuse->cree_reference_declaration(&lexeme_sentinel, decl_it);

            assembleuse->bloc_courant()->membres->ajoute(decl_it);

            auto variable = assembleuse->cree_virgule(&lexeme_sentinel);
            variable->expressions.ajoute(decl_it);

            // il nous faut créer une boucle sur le tableau.
            // pour * tableau { initialise_type(it); }
            static Lexeme lexeme = {};
            auto pour = assembleuse->cree_pour(&lexeme);
            pour->prend_pointeur = true;
            pour->expression = ref_param;
            pour->bloc = assembleuse->cree_bloc(&lexeme);
            pour->aide_generation_code = GENERE_BOUCLE_TABLEAU;
            pour->variable = variable;

            auto fonction = cree_entete_pour_initialisation_type(
                type_pointe, compilatrice, assembleuse, typeuse);
            auto appel = assembleuse->cree_appel(
                &lexeme_sentinel, fonction, typeuse[TypeBase::RIEN]);
            appel->drapeaux |= FORCE_NULCTX;
            appel->parametres_resolus.ajoute(ref_it);

            pour->bloc->expressions->ajoute(appel);

            assembleuse->bloc_courant()->expressions->ajoute(pour);
            break;
        }
        case GenreType::OPAQUE:
        {
            auto opaque = type->comme_opaque();
            auto type_opacifie = opaque->type_opacifie;

            // Transtype vers le type opacifié, et crée l'initialisation pour le type opacifié.
            static Lexeme lexeme_op = {};
            lexeme_op.genre = GenreLexeme::FOIS_UNAIRE;
            auto prise_adresse = assembleuse->cree_expression_unaire(&lexeme_op);
            prise_adresse->operande = ref_param;
            prise_adresse->type = typeuse.type_pointeur_pour(type);

            auto comme = assembleuse->cree_comme(&lexeme_sentinel);
            comme->expression = prise_adresse;
            comme->type = typeuse.type_pointeur_pour(type_opacifie);
            comme->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, comme->type};

            auto fonc_init = cree_entete_pour_initialisation_type(
                type_opacifie, compilatrice, assembleuse, typeuse);
            auto appel = assembleuse->cree_appel(
                &lexeme_sentinel, fonc_init, typeuse[TypeBase::RIEN]);
            appel->drapeaux |= FORCE_NULCTX;
            appel->parametres_resolus.ajoute(comme);
            assembleuse->bloc_courant()->expressions->ajoute(appel);
            break;
        }
        case GenreType::TUPLE:
        {
            // Les tuples ne sont que pour représenter les sorties des fonctions, ils ne devraient
            // pas avoir d'initialisation.
            break;
        }
    }
}

void cree_noeud_initialisation_type(EspaceDeTravail *espace,
                                    Type *type,
                                    AssembleuseArbre *assembleuse)
{
    auto &typeuse = espace->compilatrice().typeuse;
    auto entete = cree_entete_pour_initialisation_type(
        type, espace->compilatrice(), assembleuse, typeuse);

    auto corps = entete->corps;
    corps->aide_generation_code = REQUIERS_CODE_EXTRA_RETOUR;

    corps->bloc = assembleuse->cree_bloc_seul(&lexeme_sentinel, entete->bloc_parametres);

    assembleuse->bloc_courant(corps->bloc);

    static Lexeme lexeme_decl = {};
    auto decl_param = entete->params[0]->comme_declaration_variable();
    auto ref_param = assembleuse->cree_reference_declaration(&lexeme_decl, decl_param);

    switch (type->genre) {
        case GenreType::RIEN:
        case GenreType::POLYMORPHIQUE:
        {
            break;
        }
        case GenreType::BOOL:
        case GenreType::OCTET:
        case GenreType::ENTIER_CONSTANT:
        case GenreType::ENTIER_NATUREL:
        case GenreType::ENTIER_RELATIF:
        case GenreType::TYPE_DE_DONNEES:
        case GenreType::REEL:
        case GenreType::REFERENCE:
        case GenreType::POINTEUR:
        case GenreType::FONCTION:
        case GenreType::TABLEAU_FIXE:
        case GenreType::ENUM:
        case GenreType::ERREUR:
        case GenreType::OPAQUE:
        {
            auto deref = assembleuse->cree_memoire(&lexeme_sentinel);
            deref->expression = ref_param;
            deref->type = type;
            cree_initialisation_defaut_pour_type(
                type, espace->compilatrice(), assembleuse, deref, nullptr, typeuse);
            break;
        }
        case GenreType::EINI:
        case GenreType::CHAINE:
        case GenreType::STRUCTURE:
        case GenreType::TABLEAU_DYNAMIQUE:
        case GenreType::VARIADIQUE:
        {
            static Lexeme lexeme = {};
            auto type_compose = static_cast<TypeCompose *>(type);

            if (type_compose->est_structure()) {
                auto decl = type_compose->comme_structure()->decl;
                if (decl && decl->est_polymorphe) {
                    espace->rapporte_erreur_sans_site(
                        "Erreur interne : création d'une fonction d'initialisation pour un type "
                        "polymorphique !");
                }
            }

            auto index_membre = 0;
            POUR (type_compose->membres) {
                if ((it.drapeaux & TypeCompose::Membre::EST_CONSTANT) == 0) {
                    auto ref_membre = assembleuse->cree_reference_membre(
                        &lexeme, ref_param, it.type, index_membre);
                    cree_initialisation_defaut_pour_type(it.type,
                                                         espace->compilatrice(),
                                                         assembleuse,
                                                         ref_membre,
                                                         it.expression_valeur_defaut,
                                                         typeuse);
                }
                index_membre += 1;
            }

            break;
        }
        case GenreType::UNION:
        {
            auto type_union = type->comme_union();
            auto index_membre = 0;
            // À FAIRE(union) : test proprement cette logique
            POUR (type_union->membres) {
                if (it.type != type_union->type_le_plus_grand) {
                    index_membre += 1;
                    continue;
                }

                if (type_union->est_nonsure) {
                    /* Stocke directement dans le paramètre. */
                    auto type_le_plus_grand = type_union->type_le_plus_grand;
                    auto transtype = assembleuse->cree_comme(&lexeme_sentinel);
                    transtype->expression = ref_param;
                    transtype->transformation = TransformationType{
                        TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                        typeuse.type_pointeur_pour(type_le_plus_grand)};
                    transtype->type = transtype->transformation.type_cible;

                    auto deref = assembleuse->cree_memoire(&lexeme_sentinel);
                    deref->expression = transtype;
                    deref->type = type_le_plus_grand;

                    cree_initialisation_defaut_pour_type(it.type,
                                                         espace->compilatrice(),
                                                         assembleuse,
                                                         deref,
                                                         it.expression_valeur_defaut,
                                                         typeuse);
                }
                else {
                    auto ref_membre = assembleuse->cree_reference_membre(&lexeme_sentinel);
                    ref_membre->accedee = ref_param;
                    ref_membre->index_membre = 0;
                    ref_membre->type = it.type;
                    ref_membre->aide_generation_code = IGNORE_VERIFICATION;
                    cree_initialisation_defaut_pour_type(it.type,
                                                         espace->compilatrice(),
                                                         assembleuse,
                                                         ref_membre,
                                                         it.expression_valeur_defaut,
                                                         typeuse);

                    ref_membre = assembleuse->cree_reference_membre(&lexeme_sentinel);
                    ref_membre->accedee = ref_param;
                    ref_membre->index_membre = 1;
                    ref_membre->type = typeuse[TypeBase::Z32];
                    ref_membre->aide_generation_code = IGNORE_VERIFICATION;
                    cree_initialisation_defaut_pour_type(typeuse[TypeBase::Z32],
                                                         espace->compilatrice(),
                                                         assembleuse,
                                                         ref_membre,
                                                         nullptr,
                                                         typeuse);
                }

                break;
            }

            break;
        }
        case GenreType::TUPLE:
        {
            // Les tuples ne sont que pour représenter les sorties des fonctions, ils ne devraient
            // pas avoir d'initialisation.
            break;
        }
    }

    simplifie_arbre(espace, assembleuse, typeuse, entete);

    type->drapeaux |= INITIALISATION_TYPE_FUT_CREEE;
    corps->drapeaux |= DECLARATION_FUT_VALIDEE;
}
