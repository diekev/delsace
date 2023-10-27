/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

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
#include "canonicalisation.hh"
#include "noeud_code.hh"

/* ------------------------------------------------------------------------- */
/** \name DrapeauxNoeud
 * \{ */

std::ostream &operator<<(std::ostream &os, DrapeauxNoeud const drapeaux)
{
    os << static_cast<uint32_t>(drapeaux);
    return os;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DrapeauxNoeudFonction
 * \{ */

std::ostream &operator<<(std::ostream &os, DrapeauxNoeudFonction const drapeaux)
{
    os << static_cast<uint32_t>(drapeaux);
    return os;
}

/** \} */

/* ************************************************************************** */

static void aplatis_arbre(NoeudExpression *racine,
                          kuri::tableau<NoeudExpression *, int> &arbre_aplatis,
                          DrapeauxNoeud drapeau);

/* Fonction pour aplatir l'arbre d'une entête de fonction. Pour les déclarations de types
 * fonctions (fonc()(rien)) l'arbre aplatis est l'arbre du noeud parent (structure, fonction,
 * etc.), et non celui de l'entête. */
static void aplatis_entête_fonction(NoeudDeclarationEnteteFonction *entête,
                                    kuri::tableau<NoeudExpression *, int> &arbre_aplatis)
{
    /* aplatis_arbre pour les bloc n'aplatis que les expressions. */
    POUR (*entête->bloc_constantes->membres.verrou_lecture()) {
        if (!it->possede_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
            continue;
        }
        aplatis_arbre(it, arbre_aplatis, {});
    }

    POUR (entête->params) {
        if (it->possede_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
            continue;
        }
        aplatis_arbre(it, arbre_aplatis, {});
    }

    POUR (entête->params_sorties) {
        aplatis_arbre(it, arbre_aplatis, {});
    }
}

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
        case GenreNoeud::DIRECTIVE_INTROSPECTION:
        case GenreNoeud::DIRECTIVE_CORPS_BOUCLE:
        {
            arbre_aplatis.ajoute(racine);
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
        case GenreNoeud::DIRECTIVE_PRE_EXECUTABLE:
        {
            auto pre_executable = racine->comme_pre_executable();
            aplatis_arbre(pre_executable->expression, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(pre_executable);
            break;
        }
        case GenreNoeud::INSTRUCTION_COMPOSEE:
        {
            auto bloc = static_cast<NoeudBloc *>(racine);

            auto const &expressions = bloc->expressions.verrou_lecture();

            /* Supprime ce drapeau si nous l'avons hérité, il ne doit pas être utilisé pour des
             * instructions si/saufsi qui ne sont pas des enfants de l'instruction si/saufsi
             * parent. */
            drapeau &= ~DrapeauxNoeud::EXPRESSION_BLOC_SI;

            auto dernière_expression = expressions->taille() ? expressions->dernière() :
                                                               NoeudExpression::nul();
            POUR (*expressions) {
                auto drapeaux = it == dernière_expression ? drapeau : DrapeauxNoeud::AUCUN;
                aplatis_arbre(it, arbre_aplatis, drapeaux);
            }

            // Il nous faut le bloc pour savoir quoi différer
            arbre_aplatis.ajoute(bloc);

            break;
        }
        case GenreNoeud::DECLARATION_ENTETE_FONCTION:
        {
            auto entête = racine->comme_entete_fonction();
            if (entête->est_declaration_type) {
                /* Inclus l'arbre du type dans le nôtre. */
                aplatis_entête_fonction(entête, arbre_aplatis);
            }

            /* L'aplatissement d'une fonction dans une fonction doit déjà avoir été fait. */
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::DECLARATION_OPERATEUR_POUR:
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
        case GenreNoeud::DECLARATION_OPAQUE:
        {
            auto opaque = racine->comme_type_opaque();
            /* Évite les déclarations de types polymorphiques car cela gène la validation puisque
             * la déclaration n'est dans aucun bloc. */
            if (!opaque->expression_type->possede_drapeau(
                    DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE)) {
                aplatis_arbre(opaque->expression_type, arbre_aplatis, drapeau);
            }
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::DECLARATION_VARIABLE:
        {
            auto expr = static_cast<NoeudDeclarationVariable *>(racine);

            // N'aplatis pas expr->valeur car ça ne sers à rien dans ce cas.
            aplatis_arbre(
                expr->expression, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
            aplatis_arbre(
                expr->expression_type, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);

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
                aplatis_arbre(ref_membre->accedee,
                              arbre_aplatis,
                              drapeau | DrapeauxNoeud::GAUCHE_EXPRESSION_APPEL);
            }
            else {
                aplatis_arbre(
                    appelee, arbre_aplatis, drapeau | DrapeauxNoeud::GAUCHE_EXPRESSION_APPEL);
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

            drapeau |= DrapeauxNoeud::DROITE_ASSIGNATION;
            drapeau |= DrapeauxNoeud::POUR_CUISSON;

            aplatis_arbre(cuisine->expression, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(cuisine);
            break;
        }
        case GenreNoeud::DIRECTIVE_EXECUTE:
        {
            auto expr = static_cast<NoeudDirectiveExecute *>(racine);
            expr->drapeaux |= drapeau;

            if (expr->ident == ID::assert_ || expr->ident == ID::test) {
                drapeau |= DrapeauxNoeud::DROITE_ASSIGNATION;
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
            racine->drapeaux |= drapeau;
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

            POUR (expr->paires_discr) {
                for (auto expression : it->expression->comme_virgule()->expressions) {
                    if (!expression->est_appel()) {
                        aplatis_arbre(expression,
                                      arbre_aplatis,
                                      DrapeauxNoeud::EXPRESSION_TEST_DISCRIMINATION);
                        continue;
                    }

                    /* Les expressions d'appel sont des expressions d'extraction des valeurs, nous
                     * ne voulons pas valider sémantiquement les « paramètres » car ils ne
                     * références pas des variables mais en crée (les valider résulterait alors en
                     * une erreur de compilation). */
                    auto appel = expression->comme_appel();
                    aplatis_arbre(appel->expression,
                                  arbre_aplatis,
                                  DrapeauxNoeud::EXPRESSION_TEST_DISCRIMINATION);
                }
            }

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

            /* Seul l'expression racine, directement après l'assignation, doit être marquée comme
             * tel. */
            if ((drapeau & DrapeauxNoeud::EXPRESSION_BLOC_SI) ==
                DrapeauxNoeud::EXPRESSION_BLOC_SI) {
                expr->drapeaux &= ~DrapeauxNoeud::DROITE_ASSIGNATION;
            }

            aplatis_arbre(expr->condition,
                          arbre_aplatis,
                          DrapeauxNoeud::DROITE_ASSIGNATION | DrapeauxNoeud::DROITE_CONDITION);
            aplatis_arbre(
                expr->bloc_si_vrai, arbre_aplatis, drapeau | DrapeauxNoeud::EXPRESSION_BLOC_SI);
            aplatis_arbre(
                expr->bloc_si_faux, arbre_aplatis, drapeau | DrapeauxNoeud::EXPRESSION_BLOC_SI);

            /* mets l'instruction à la fin afin de pouvoir déterminer le type de
             * l'expression selon les blocs si nous sommes à droite d'une expression */
            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::INSTRUCTION_SI_STATIQUE:
        case GenreNoeud::INSTRUCTION_SAUFSI_STATIQUE:
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
            aplatis_entête_fonction(entete, entete->arbre_aplatis);
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

    if (declaration->est_type_structure()) {
        auto structure = declaration->comme_type_structure();

        if (structure->bloc_constantes && structure->arbre_aplatis_params.taille() == 0) {
            POUR (*structure->bloc_constantes->membres.verrou_lecture()) {
                aplatis_arbre(it, structure->arbre_aplatis_params, {});
            }
        }

        if (structure->arbre_aplatis.taille() == 0) {
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

    if (declaration->est_type_opaque()) {
        auto opaque = declaration->comme_type_opaque();
        if (opaque->arbre_aplatis.taille() == 0) {
            aplatis_arbre(opaque, opaque->arbre_aplatis, {});
        }
        return;
    }
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

kuri::chaine_statique NoeudDeclarationEnteteFonction::nom_broye(EspaceDeTravail *espace,
                                                                Broyeuse &broyeuse)
{
    if (nom_broye_ != "") {
        return nom_broye_;
    }

    if (ident != ID::principale && !possede_drapeau(DrapeauxNoeudFonction::EST_EXTERNE |
                                                    DrapeauxNoeudFonction::FORCE_SANSBROYAGE)) {
        auto fichier = espace->compilatrice().fichier(lexeme->fichier);
        nom_broye_ = broyeuse.broye_nom_fonction(this, fichier->module->nom());
    }
    else {
        nom_broye_ = lexeme->chaine;
    }

    return nom_broye_;
}

Type *NoeudDeclarationEnteteFonction::type_initialisé() const
{
    assert(possede_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE));
    return params[0]->type->comme_type_pointeur()->type_pointe;
}

int NoeudBloc::nombre_de_membres() const
{
    return membres->taille();
}

void NoeudBloc::reserve_membres(int nombre)
{
    membres->reserve(nombre);
}

static constexpr auto TAILLE_MAX_TABLEAU_MEMBRES = 16;

template <typename T>
using PointeurTableauVerrouille = typename kuri::tableau_synchrone<T>::PointeurVerrouille;

using TableMembres = kuri::table_hachage<IdentifiantCode const *, NoeudDeclaration *>;

static void ajoute_membre(TableMembres &table_membres, NoeudDeclaration *decl)
{
    /* Nous devons faire en sorte que seul le premier membre du nom est ajouté, afin que l'ensemble
     * de surcharge lui soit réservé, et que c'est ce membre qui est retourné. */
    if (table_membres.possede(decl->ident)) {
        return;
    }
    table_membres.insère(decl->ident, decl);
}

static void init_table_hachage_membres(PointeurTableauVerrouille<NoeudDeclaration *> &membres,
                                       TableMembres &table_membres)
{
    if (table_membres.taille() != 0) {
        return;
    }

    POUR (*membres) {
        ajoute_membre(table_membres, it);
    }
}

void NoeudBloc::ajoute_membre(NoeudDeclaration *decl)
{
    if (decl->est_declaration_symbole()) {
        auto decl_existante = declaration_pour_ident(decl->ident);
        if (decl_existante && decl_existante->est_declaration_symbole()) {
            auto entete_existante = decl_existante->comme_declaration_symbole();
            entete_existante->ensemble_de_surchages->ajoute(decl->comme_declaration_symbole());
        }
    }

    auto membres_ = membres.verrou_ecriture();
    if (membres_->taille() >= TAILLE_MAX_TABLEAU_MEMBRES) {
        init_table_hachage_membres(membres_, table_membres);
        ::ajoute_membre(table_membres, decl);
    }

    membres_->ajoute(decl);
    membres_sont_sales = true;
}

void NoeudBloc::ajoute_membre_au_debut(NoeudDeclaration *decl)
{
    auto membres_ = membres.verrou_ecriture();
    if (membres_->taille() >= TAILLE_MAX_TABLEAU_MEMBRES) {
        init_table_hachage_membres(membres_, table_membres);
        ::ajoute_membre(table_membres, decl);
    }

    membres_->pousse_front(decl);
    membres_sont_sales = true;
}

void NoeudBloc::fusionne_membres(NoeudBloc *de)
{
    if (!de) {
        /* Permet de passer un bloc nul. */
        return;
    }

    POUR ((*de->membres.verrou_lecture())) {
        ajoute_membre(it);
    }
}

NoeudDeclaration *NoeudBloc::membre_pour_index(int index) const
{
    return membres->a(index);
}

NoeudDeclaration *NoeudBloc::declaration_pour_ident(IdentifiantCode const *ident_recherche) const
{
    auto membres_ = membres.verrou_lecture();
    nombre_recherches += 1;

    if (table_membres.taille() != 0) {
        return table_membres.valeur_ou(ident_recherche, nullptr);
    }

    POUR (*membres_) {
        if (it->ident == ident_recherche) {
            return it;
        }
    }
    return nullptr;
}

NoeudDeclaration *NoeudBloc::declaration_avec_meme_ident_que(NoeudExpression const *expr) const
{
    auto membres_ = membres.verrou_lecture();
    nombre_recherches += 1;

    if (table_membres.taille() != 0) {
        auto resultat = table_membres.valeur_ou(expr->ident, nullptr);
        if (resultat != expr) {
            return resultat;
        }
        return nullptr;
    }

    POUR (*membres_) {
        if (it != expr && it->ident == expr->ident) {
            return it;
        }
    }
    return nullptr;
}

void NoeudBloc::ajoute_expression(NoeudExpression *expr)
{
    expressions->ajoute(expr);
    expressions_sont_sales = true;
}

// -----------------------------------------------------------------------------
// Implémentation des fonctions supplémentaires de la ConvertisseuseNoeudCode

static void copie_annotations(kuri::tableau<Annotation, int> const &source,
                              kuri::tableau<const Annotation *> &dest)
{
    dest.reserve(source.taille());
    for (auto &annotation : source) {
        dest.ajoute(&annotation);
    }
}

InfoType *ConvertisseuseNoeudCode::cree_info_type_pour(Type *type)
{
    auto cree_info_type_entier = [this](uint32_t taille_en_octet, bool est_signe) {
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
            auto type_tableau = type->comme_type_tableau_dynamique();

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
            auto type_variadique = type->comme_type_variadique();

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
            auto type_tableau = type->comme_type_tableau_fixe();

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
            info_type->est_reference = type->est_type_reference();

            type->info_type = info_type;
            break;
        }
        case GenreType::STRUCTURE:
        {
            auto type_struct = type->comme_type_structure();

            auto info_type = allocatrice_infos_types.infos_types_structures.ajoute_element();
            type->info_type = info_type;

            info_type->genre = GenreInfoType::STRUCTURE;
            info_type->taille_en_octet = type->taille_octet;
            info_type->nom = donne_nom_hierarchique(type_struct);

            info_type->membres.reserve(type_struct->membres.taille());

            POUR (type_struct->membres) {
                if (it.nom == ID::chaine_vide) {
                    continue;
                }

                if (it.possède_drapeau(MembreTypeComposé::PROVIENT_D_UN_EMPOI)) {
                    continue;
                }

                auto info_type_membre =
                    allocatrice_infos_types.infos_types_membres_structures.ajoute_element();
                info_type_membre->info = cree_info_type_pour(it.type);
                info_type_membre->decalage = it.decalage;
                info_type_membre->nom = it.nom->nom;
                info_type_membre->drapeaux = it.drapeaux;

                if (it.decl) {
                    copie_annotations(it.decl->annotations, info_type_membre->annotations);
                }

                info_type->membres.ajoute(info_type_membre);
            }

            if (type_struct->decl) {
                auto decl_struct = type_struct->decl;
                copie_annotations(decl_struct->annotations, info_type->annotations);
            }

            info_type->structs_employees.reserve(type_struct->types_employés.taille());
            POUR (type_struct->types_employés) {
                auto info_struct_employe = cree_info_type_pour(it->type);
                info_type->structs_employees.ajoute(
                    static_cast<InfoTypeStructure *>(info_struct_employe));
            }

            break;
        }
        case GenreType::UNION:
        {
            auto type_union = type->comme_type_union();

            auto info_type = allocatrice_infos_types.infos_types_unions.ajoute_element();
            info_type->genre = GenreInfoType::UNION;
            info_type->est_sure = !type_union->est_nonsure;
            info_type->type_le_plus_grand = cree_info_type_pour(type_union->type_le_plus_grand);
            info_type->decalage_index = type_union->decalage_index;
            info_type->taille_en_octet = type_union->taille_octet;
            info_type->nom = donne_nom_hierarchique(type_union);

            info_type->membres.reserve(type_union->membres.taille());

            POUR (type_union->membres) {
                auto info_type_membre =
                    allocatrice_infos_types.infos_types_membres_structures.ajoute_element();
                info_type_membre->info = cree_info_type_pour(it.type);
                info_type_membre->decalage = it.decalage;
                info_type_membre->nom = it.nom->nom;
                info_type_membre->drapeaux = it.drapeaux;

                if (it.decl) {
                    copie_annotations(it.decl->annotations, info_type_membre->annotations);
                }

                info_type->membres.ajoute(info_type_membre);
            }

            if (type_union->decl) {
                auto decl_struct = type_union->decl;
                copie_annotations(decl_struct->annotations, info_type->annotations);
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
            info_type->nom = donne_nom_hierarchique(type_enum);
            info_type->est_drapeau = type_enum->est_drapeau;
            info_type->taille_en_octet = type_enum->taille_octet;
            info_type->type_sous_jacent = static_cast<InfoTypeEntier *>(
                cree_info_type_pour(type_enum->type_sous_jacent));

            info_type->noms.reserve(type_enum->membres.taille());
            info_type->valeurs.reserve(type_enum->membres.taille());

            POUR (type_enum->membres) {
                if (it.drapeaux == MembreTypeComposé::EST_IMPLICITE) {
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
            auto type_fonction = type->comme_type_fonction();

            auto info_type = allocatrice_infos_types.infos_types_fonctions.ajoute_element();
            info_type->genre = GenreInfoType::FONCTION;
            info_type->est_coroutine = false;
            info_type->taille_en_octet = type->taille_octet;

            info_type->types_entrees.reserve(type_fonction->types_entrees.taille());

            POUR (type_fonction->types_entrees) {
                info_type->types_entrees.ajoute(cree_info_type_pour(it));
            }

            auto type_sortie = type_fonction->type_sortie;

            if (type_sortie->est_type_tuple()) {
                auto tuple = type_sortie->comme_type_tuple();
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
            auto type_opaque = type->comme_type_opaque();

            auto info_type = allocatrice_infos_types.infos_types_opaques.ajoute_element();
            info_type->genre = GenreInfoType::OPAQUE;
            info_type->nom = donne_nom_hierarchique(type_opaque);
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
            return TypeBase::EINI;
        }
        case GenreInfoType::REEL:
        {
            if (type->taille_en_octet == 2) {
                return TypeBase::R16;
            }

            if (type->taille_en_octet == 4) {
                return TypeBase::R32;
            }

            if (type->taille_en_octet == 8) {
                return TypeBase::R64;
            }

            return nullptr;
        }
        case GenreInfoType::ENTIER:
        {
            const auto info_type_entier = static_cast<const InfoTypeEntier *>(type);

            if (info_type_entier->est_signe) {
                if (type->taille_en_octet == 1) {
                    return TypeBase::Z8;
                }

                if (type->taille_en_octet == 2) {
                    return TypeBase::Z16;
                }

                if (type->taille_en_octet == 4) {
                    return TypeBase::Z32;
                }

                if (type->taille_en_octet == 8) {
                    return TypeBase::Z64;
                }

                return nullptr;
            }

            if (type->taille_en_octet == 1) {
                return TypeBase::N8;
            }

            if (type->taille_en_octet == 2) {
                return TypeBase::N16;
            }

            if (type->taille_en_octet == 4) {
                return TypeBase::N32;
            }

            if (type->taille_en_octet == 8) {
                return TypeBase::N64;
            }

            return nullptr;
        }
        case GenreInfoType::OCTET:
        {
            return TypeBase::OCTET;
        }
        case GenreInfoType::BOOLEEN:
        {
            return TypeBase::BOOL;
        }
        case GenreInfoType::CHAINE:
        {
            return TypeBase::CHAINE;
        }
        case GenreInfoType::RIEN:
        {
            return TypeBase::RIEN;
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
                                                                  const OpérateurBinaire *op,
                                                                  NoeudExpression *expr1,
                                                                  NoeudExpression *expr2)
{
    assert(op);
    auto op_bin = cree_expression_binaire(lexeme);
    op_bin->operande_gauche = expr1;
    op_bin->operande_droite = expr2;
    op_bin->op = op;
    op_bin->type = op->type_résultat;
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
    assignation->variable = assignee;
    assignation->expression = expression;

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

    if (type->est_type_structure()) {
        structure->expression = type->comme_type_structure()->decl;
        structure->noeud_fonction_appelee = type->comme_type_structure()->decl;
    }
    else if (type->est_type_union()) {
        structure->expression = type->comme_type_union()->decl;
        structure->noeud_fonction_appelee = type->comme_type_union()->decl;
    }

    structure->type = type;
    return structure;
}

NoeudExpressionLitteraleEntier *AssembleuseArbre::cree_litterale_entier(Lexeme const *lexeme,
                                                                        Type *type,
                                                                        uint64_t valeur)
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
    inc->op = type->table_opérateurs->opérateur_ajt;
    assert(inc->op);
    inc->operande_gauche = valeur;
    inc->type = type;

    if (est_type_entier(type)) {
        inc->operande_droite = cree_litterale_entier(valeur->lexeme, type, 1);
    }
    else if (type->est_type_reel()) {
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
    inc->op = type->table_opérateurs->opérateur_sst;
    assert(inc->op);
    inc->operande_gauche = valeur;
    inc->type = type;

    if (est_type_entier(type)) {
        inc->operande_droite = cree_litterale_entier(valeur->lexeme, type, 1);
    }
    else if (type->est_type_reel()) {
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
    os << "-- Est polymorphique       : " << std::boolalpha
       << entete->possede_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE) << '\n';
    os << "-- Est #corps_texte        : " << std::boolalpha << entete->corps->est_corps_texte
       << '\n';
    os << "-- Entête fut validée      : " << std::boolalpha
       << entete->possede_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE) << '\n';
    os << "-- Corps fut validé        : " << std::boolalpha
       << entete->corps->possede_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE) << '\n';
    os << "-- Est monomorphisation    : " << std::boolalpha
       << entete->possede_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION) << '\n';
    os << "-- Est initialisation type : " << std::boolalpha
       << entete->possede_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE) << '\n';
    if (entete->possede_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION)) {
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

kuri::chaine nom_humainement_lisible(NoeudExpression const *noeud)
{
    if (noeud->est_entete_fonction()) {
        auto entete = noeud->comme_entete_fonction();

        if (entete->possede_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE)) {
            return enchaine("init_de(", chaine_type(entete->type_initialisé()), ")");
        }

        if (entete->est_operateur) {
            return enchaine("opérateur ", entete->lexeme->chaine);
        }

        if (entete->ident) {
            return entete->ident->nom;
        }

        return "fonction anonyme";
    }

    if (noeud->ident) {
        return noeud->ident->nom;
    }

    return "anonyme";
}

/* Fonctions d'initialisation des types. */

static Lexeme lexeme_sentinel = {};

NoeudDeclarationEnteteFonction *cree_entete_pour_initialisation_type(Type *type,
                                                                     Compilatrice &compilatrice,
                                                                     AssembleuseArbre *assembleuse,
                                                                     Typeuse &typeuse)
{
    if (!type->fonction_init) {
        auto type_param = typeuse.type_pointeur_pour(type);
        if (type->est_type_union() && !type->comme_type_union()->est_nonsure) {
            type_param = typeuse.type_pointeur_pour(type, false, false);
        }

        auto types_entrees = kuri::tablet<Type *, 6>();
        types_entrees.ajoute(type_param);

        auto type_fonction = typeuse.type_fonction(types_entrees, TypeBase::RIEN, false);

        static Lexeme lexeme_entete = {};
        auto entete = assembleuse->cree_entete_fonction(&lexeme_entete);
        entete->drapeaux_fonction |= DrapeauxNoeudFonction::EST_INITIALISATION_TYPE;

        entete->bloc_constantes = assembleuse->cree_bloc_seul(&lexeme_sentinel, nullptr);
        entete->bloc_parametres = assembleuse->cree_bloc_seul(&lexeme_sentinel,
                                                              entete->bloc_constantes);

        /* Paramètre d'entrée. */
        {
            static Lexeme lexeme_decl = {};
            auto decl_param = assembleuse->cree_declaration_variable(
                &lexeme_decl, type_param, ID::pointeur, nullptr);

            decl_param->type = type_param;
            decl_param->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

            entete->params.ajoute(decl_param);
        }

        /* Paramètre de sortie. */
        {
            static const Lexeme lexeme_rien = {"rien", {}, GenreLexeme::RIEN, 0, 0, 0};
            auto type_declare = assembleuse->cree_reference_type(&lexeme_rien);

            auto ident = compilatrice.table_identifiants->identifiant_pour_chaine("__ret0");

            auto ref = assembleuse->cree_reference_declaration(&lexeme_rien);
            ref->ident = ident;
            ref->type = TypeBase::RIEN;

            auto decl = assembleuse->cree_declaration_variable(ref);
            decl->expression_type = type_declare;
            decl->type = ref->type;

            entete->params_sorties.ajoute(decl);
            entete->param_sortie = entete->params_sorties[0]->comme_declaration_variable();
        }

        entete->type = type_fonction;
        entete->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
        entete->drapeaux_fonction |= (DrapeauxNoeudFonction::FORCE_ENLIGNE |
                                      DrapeauxNoeudFonction::FORCE_SANSTRACE |
                                      DrapeauxNoeudFonction::FUT_GÉNÉRÉE_PAR_LA_COMPILATRICE);

        type->fonction_init = entete;

        if (type->est_type_union()) {
            /* Assigne également la fonction au type structure car c'est lui qui est utilisé lors
             * de la génération de RI. */
            auto type_structure = type->comme_type_union()->type_structure;
            if (type_structure) {
                type_structure->fonction_init = entete;
            }
        }
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
    bloc->ajoute_expression(assignation);
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
            if (expr_valeur_defaut) {
                cree_assignation(assembleuse, ref_param, expr_valeur_defaut);
                break;
            }

            static Lexeme lexeme_op = {};
            lexeme_op.genre = GenreLexeme::FOIS_UNAIRE;
            auto prise_adresse = assembleuse->cree_expression_unaire(&lexeme_op);
            prise_adresse->operande = ref_param;
            prise_adresse->type = typeuse.type_pointeur_pour(type);
            auto fonction = cree_entete_pour_initialisation_type(
                type, compilatrice, assembleuse, typeuse);
            auto appel = assembleuse->cree_appel(&lexeme_sentinel, fonction, TypeBase::RIEN);
            appel->parametres_resolus.ajoute(prise_adresse);
            assembleuse->bloc_courant()->ajoute_expression(appel);
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
            auto type_tableau = type->comme_type_tableau_fixe();
            auto type_pointe = type_tableau->type_pointe;

            auto type_pointeur_type_pointe = typeuse.type_pointeur_pour(type_pointe, false, false);

            /* NOTE: pour les tableaux fixes, puisque le déréférencement de pointeur est compliqué
             * avec les indexages, nous passons par une variable locale temporaire et copierons la
             * variable initialisée dans la mémoire pointée par le paramètre. */
            auto valeur_resultat = assembleuse->cree_declaration_variable(
                &lexeme_sentinel,
                type_tableau,
                ID::resultat,
                assembleuse->cree_non_initialisation(&lexeme_sentinel));
            assembleuse->bloc_courant()->ajoute_membre(valeur_resultat);
            assembleuse->bloc_courant()->ajoute_expression(valeur_resultat);
            auto ref_resultat = assembleuse->cree_reference_declaration(&lexeme_sentinel,
                                                                        valeur_resultat);

            /* Toutes les variables doivent être initialisées (ou nous devons nous assurer que tous
             * les types possibles créés par la compilation ont une fonction d'initalisation). */
            auto init_it = assembleuse->cree_litterale_nul(&lexeme_sentinel);
            init_it->type = type_pointeur_type_pointe;

            auto decl_it = assembleuse->cree_declaration_variable(
                &lexeme_sentinel, type_pointeur_type_pointe, ID::it, init_it);
            auto ref_it = assembleuse->cree_reference_declaration(&lexeme_sentinel, decl_it);

            assembleuse->bloc_courant()->ajoute_membre(decl_it);

            auto variable = assembleuse->cree_virgule(&lexeme_sentinel);
            variable->expressions.ajoute(decl_it);

            // il nous faut créer une boucle sur le tableau.
            // pour * tableau { initialise_type(it); }
            static Lexeme lexeme = {};
            auto pour = assembleuse->cree_pour(&lexeme);
            pour->prend_pointeur = true;
            pour->expression = ref_resultat;
            pour->bloc = assembleuse->cree_bloc(&lexeme);
            pour->aide_generation_code = GENERE_BOUCLE_TABLEAU;
            pour->variable = variable;
            pour->decl_it = decl_it;
            pour->decl_index_it = assembleuse->cree_declaration_variable(
                &lexeme_sentinel, TypeBase::Z64, ID::index_it, nullptr);

            auto fonction = cree_entete_pour_initialisation_type(
                type_pointe, compilatrice, assembleuse, typeuse);
            auto appel = assembleuse->cree_appel(&lexeme_sentinel, fonction, TypeBase::RIEN);
            appel->parametres_resolus.ajoute(ref_it);

            pour->bloc->ajoute_expression(appel);

            assembleuse->bloc_courant()->ajoute_expression(pour);

            auto assignation_resultat = assembleuse->cree_assignation_variable(
                &lexeme_sentinel, ref_param, ref_resultat);
            assembleuse->bloc_courant()->ajoute_expression(assignation_resultat);
            break;
        }
        case GenreType::OPAQUE:
        {
            auto opaque = type->comme_type_opaque();
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
            auto appel = assembleuse->cree_appel(&lexeme_sentinel, fonc_init, TypeBase::RIEN);
            appel->parametres_resolus.ajoute(comme);
            assembleuse->bloc_courant()->ajoute_expression(appel);
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

/* Assigne la fonction d'initialisation de type au type énum en se basant sur son type de données.
 */
static void assigne_fonction_init_enum(Typeuse &typeuse, TypeEnum *type)
{
#define ASSIGNE_SI(ident_maj, ident_min)                                                          \
    if (type_données == TypeBase::ident_maj) {                                                    \
        assigne_fonction_init(type, typeuse.init_type_##ident_min);                               \
        return;                                                                                   \
    }

    auto type_données = type->type_sous_jacent;

    ASSIGNE_SI(N8, n8);
    ASSIGNE_SI(N16, n16);
    ASSIGNE_SI(N32, n32);
    ASSIGNE_SI(N64, n64);

    ASSIGNE_SI(Z8, z8);
    ASSIGNE_SI(Z16, z16);
    ASSIGNE_SI(Z32, z32);
    ASSIGNE_SI(Z64, z64);

#undef ASSIGNE_SI
}

/* Sauvegarde dans la typeuse la fonction d'intialisation du type si celle-ci est à sauvegarder. */
static void sauvegarde_fonction_init(Typeuse &typeuse,
                                     Type *type,
                                     NoeudDeclarationEnteteFonction *entete)
{
#define ASSIGNE_SI(ident_maj, ident_min)                                                          \
    if (type == TypeBase::ident_maj) {                                                            \
        typeuse.init_type_##ident_min = entete;                                                   \
        return;                                                                                   \
    }

    ASSIGNE_SI(N8, n8);
    ASSIGNE_SI(N16, n16);
    ASSIGNE_SI(N32, n32);
    ASSIGNE_SI(N64, n64);
    ASSIGNE_SI(Z8, z8);
    ASSIGNE_SI(Z16, z16);
    ASSIGNE_SI(Z32, z32);
    ASSIGNE_SI(Z64, z64);
    ASSIGNE_SI(PTR_RIEN, pointeur);

#undef ASSIGNE_SI
}

void cree_noeud_initialisation_type(EspaceDeTravail *espace,
                                    Type *type,
                                    AssembleuseArbre *assembleuse)
{
    auto &typeuse = espace->compilatrice().typeuse;

    if (type->est_type_enum()) {
        assigne_fonction_init_enum(typeuse, type->comme_type_enum());
        return;
    }

    /* Qu'init_type_pointeur soit nul n'est possible qu'au début de la compilation lors de la
     * création des tâches préliminaire à la compilation. Si nous avons un pointeur ici, c'est un
     * pointeur faisant partie des TypeBase, pour les autres, l'assignation de la fonction se fait
     * lors de la création du type pointeur. */
    if (type->est_type_pointeur() && typeuse.init_type_pointeur) {
        assigne_fonction_init(type, typeuse.init_type_pointeur);
        return;
    }

    auto entete = cree_entete_pour_initialisation_type(
        type, espace->compilatrice(), assembleuse, typeuse);

    sauvegarde_fonction_init(typeuse, type, entete);

    auto corps = entete->corps;
    corps->aide_generation_code = REQUIERS_CODE_EXTRA_RETOUR;

    corps->bloc = assembleuse->cree_bloc_seul(&lexeme_sentinel, entete->bloc_parametres);

    assert(assembleuse->bloc_courant() == nullptr);
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
        {
            auto deref = assembleuse->cree_memoire(&lexeme_sentinel);
            deref->expression = ref_param;
            deref->type = type;
            cree_initialisation_defaut_pour_type(
                type, espace->compilatrice(), assembleuse, deref, nullptr, typeuse);
            break;
        }
        case GenreType::OPAQUE:
        {
            auto type_opacifie = type->comme_type_opaque()->type_opacifie;
            auto type_pointeur_opacifie = typeuse.type_pointeur_pour(type_opacifie);

            auto comme_type_opacifie = assembleuse->cree_comme(&lexeme_sentinel);
            comme_type_opacifie->expression = ref_param;
            comme_type_opacifie->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                                   type_pointeur_opacifie};
            comme_type_opacifie->type = type_pointeur_opacifie;

            auto deref = assembleuse->cree_memoire(&lexeme_sentinel);
            deref->expression = comme_type_opacifie;
            deref->type = type_opacifie;

            cree_initialisation_defaut_pour_type(
                type_opacifie, espace->compilatrice(), assembleuse, deref, nullptr, typeuse);
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

            if (type_compose->est_type_structure()) {
                auto decl = type_compose->comme_type_structure()->decl;
                if (decl && decl->est_polymorphe) {
                    espace->rapporte_erreur_sans_site(
                        "Erreur interne : création d'une fonction d'initialisation pour un type "
                        "polymorphique !");
                }
            }

            POUR_INDEX (type_compose->membres) {
                if (it.ne_doit_pas_être_dans_code_machine() &&
                    !it.expression_initialisation_est_spéciale()) {
                    continue;
                }

                if (it.expression_valeur_defaut &&
                    it.expression_valeur_defaut->est_non_initialisation()) {
                    continue;
                }

                auto ref_membre = assembleuse->cree_reference_membre(
                    &lexeme, ref_param, it.type, index_it);
                cree_initialisation_defaut_pour_type(it.type,
                                                     espace->compilatrice(),
                                                     assembleuse,
                                                     ref_membre,
                                                     it.expression_valeur_defaut,
                                                     typeuse);
            }

            break;
        }
        case GenreType::UNION:
        {
            auto const type_union = type->comme_type_union();

            MembreTypeComposé membre;
            if (type_union->type_le_plus_grand) {
                auto const info_membre = donne_membre_pour_type(type_union,
                                                                type_union->type_le_plus_grand);
                assert(info_membre.has_value());
                membre = info_membre->membre;
            }
            else if (type_union->membres.taille()) {
                /* Si l'union ne contient que des membres de type « rien », utilise le premier
                 * membre. */
                membre = type_union->membres[0];
            }
            else {
                break;
            }

            // À FAIRE(union) : test proprement cette logique
            if (type_union->est_nonsure) {
                /* Stocke directement dans le paramètre. */
                auto transtype = assembleuse->cree_comme(&lexeme_sentinel);
                transtype->expression = ref_param;
                transtype->transformation = TransformationType{
                    TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                    typeuse.type_pointeur_pour(membre.type)};
                transtype->type = const_cast<Type *>(transtype->transformation.type_cible);

                auto deref = assembleuse->cree_memoire(&lexeme_sentinel);
                deref->expression = transtype;
                deref->type = membre.type;

                cree_initialisation_defaut_pour_type(membre.type,
                                                     espace->compilatrice(),
                                                     assembleuse,
                                                     deref,
                                                     membre.expression_valeur_defaut,
                                                     typeuse);
            }
            else {
                /* Transtype l'argument vers le type de la structure.
                 *
                 * Nous ne pouvons pas utiliser une expression de référence de membre d'union ici
                 * car la RI se base sur le fait qu'une telle expression se fait sur le type
                 * d'union et convertira vers le type structure dans ce cas.
                 *
                 * Nous ne pouvons pas utiliser une expression de référence de membre de structure
                 * en utilisant le type union comme type d'accès, car sinon l'accès se ferait sur
                 * les membres de l'union alors que nous voulons que l'accès se fasse sur les
                 * membres de la structure de l'union (membre le plus grand + index).
                 */
                auto type_pointeur_type_structure = typeuse.type_pointeur_pour(
                    type_union->type_structure, false, false);

                auto param_comme_structure = assembleuse->cree_comme(&lexeme_sentinel);
                param_comme_structure->type = type_pointeur_type_structure;
                param_comme_structure->expression = ref_param;
                param_comme_structure->transformation = TransformationType{
                    TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_pointeur_type_structure};

                if (membre.type->est_type_rien()) {
                    /* Seul l'index doit être initialisé. (Support union ne contenant que « rien »
                     * comme types des membres). */
                    auto ref_membre = assembleuse->cree_reference_membre(&lexeme_sentinel);
                    ref_membre->accedee = param_comme_structure;
                    ref_membre->index_membre = 0;
                    ref_membre->type = TypeBase::Z32;
                    ref_membre->aide_generation_code = IGNORE_VERIFICATION;
                    cree_initialisation_defaut_pour_type(TypeBase::Z32,
                                                         espace->compilatrice(),
                                                         assembleuse,
                                                         ref_membre,
                                                         nullptr,
                                                         typeuse);
                    break;
                }

                auto ref_membre = assembleuse->cree_reference_membre(&lexeme_sentinel);
                ref_membre->accedee = param_comme_structure;
                ref_membre->index_membre = 0;
                ref_membre->type = membre.type;
                ref_membre->aide_generation_code = IGNORE_VERIFICATION;
                cree_initialisation_defaut_pour_type(membre.type,
                                                     espace->compilatrice(),
                                                     assembleuse,
                                                     ref_membre,
                                                     membre.expression_valeur_defaut,
                                                     typeuse);

                ref_membre = assembleuse->cree_reference_membre(&lexeme_sentinel);
                ref_membre->accedee = param_comme_structure;
                ref_membre->index_membre = 1;
                ref_membre->type = TypeBase::Z32;
                ref_membre->aide_generation_code = IGNORE_VERIFICATION;
                cree_initialisation_defaut_pour_type(TypeBase::Z32,
                                                     espace->compilatrice(),
                                                     assembleuse,
                                                     ref_membre,
                                                     nullptr,
                                                     typeuse);
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

    assembleuse->depile_bloc();
    simplifie_arbre(espace, assembleuse, typeuse, entete);
    assigne_fonction_init(type, entete);
    corps->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
}

bool possede_annotation(const NoeudDeclarationVariable *decl, kuri::chaine_statique annotation)
{
    POUR (decl->annotations) {
        if (it.nom == annotation) {
            return true;
        }
    }

    return false;
}

bool est_déclaration_polymorphique(NoeudDeclaration const *decl)
{
    if (decl->est_entete_fonction()) {
        auto const entete = decl->comme_entete_fonction();
        return entete->possede_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE);
    }

    if (decl->est_type_structure()) {
        auto const structure = decl->comme_type_structure();
        return structure->est_polymorphe;
    }

    if (decl->est_type_opaque()) {
        auto const opaque = decl->comme_type_opaque();
        return opaque->expression_type->possede_drapeau(
            DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE);
    }

    return false;
}

#if 0
static bool les_invariants_de_la_fonction_sont_respectés(
    NoeudDeclarationEnteteFonction const *fonction)
{
    if (!fonction) {
        return true;
    }

    if (fonction->bloc_constantes->appartiens_à_fonction != fonction) {
        return false;
    }
    if (fonction->bloc_parametres->appartiens_à_fonction != fonction) {
        return false;
    }
    if (fonction->bloc_parametres->bloc_parent != fonction->bloc_constantes) {
        return false;
    }
    if (fonction->corps->bloc->appartiens_à_fonction != fonction) {
        return false;
    }
    if (fonction->corps->bloc->bloc_parent != fonction->bloc_parametres) {
        return false;
    }

    return true;
}
#endif
