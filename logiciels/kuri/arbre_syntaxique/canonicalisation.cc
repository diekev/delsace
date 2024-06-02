/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "canonicalisation.hh"

#include <iostream>

#include "compilation/compilatrice.hh"
#include "compilation/espace_de_travail.hh"
#include "compilation/typage.hh"
#include "utilitaires/log.hh"

#include "parsage/outils_lexemes.hh"

#include "assembleuse.hh"
#include "cas_genre_noeud.hh"
#include "noeud_expression.hh"
#include "utilitaires.hh"

/* ------------------------------------------------------------------------- */
/** \name Utilitaires locaux.
 * \{ */

/* Noeud global pour les expressions de non-initialisation, utilisé afin d'économiser un peu de
 * mémoire. */
static NoeudExpressionNonIntialisation non_initialisation{};

static NoeudExpression *crée_référence_pour_membre_employé(AssembleuseArbre *assem,
                                                           Lexème const *lexeme,
                                                           NoeudExpression *expression_accédée,
                                                           TypeCompose *type_composé,
                                                           MembreTypeComposé const &membre);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Canonicalisation.
 * \{ */

NoeudExpression *Simplificatrice::simplifie(NoeudExpression *noeud)
{
    if (!noeud) {
        return nullptr;
    }

    switch (noeud->genre) {
        case GenreNoeud::COMMENTAIRE:
        case GenreNoeud::DÉCLARATION_BIBLIOTHÈQUE:
        case GenreNoeud::DIRECTIVE_DÉPENDANCE_BIBLIOTHÈQUE:
        case GenreNoeud::DÉCLARATION_MODULE:
        case GenreNoeud::EXPRESSION_PAIRE_DISCRIMINATION:
        case GenreNoeud::DIRECTIVE_PRÉ_EXÉCUTABLE:
        case GenreNoeud::DÉCLARATION_CONSTANTE:
        case GenreNoeud::DIRECTIVE_FONCTION:
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
        case GenreNoeud::DÉCLARATION_ENTÊTE_FONCTION:
        {
            auto entête = noeud->comme_entête_fonction();

            if (entête->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE)) {
                return entête;
            }

            if (entête->est_coroutine) {
                simplifie_coroutine(entête);
                return entête;
            }

            fonction_courante = entête;
            simplifie(entête->corps);
            return entête;
        }
        case GenreNoeud::DÉCLARATION_OPÉRATEUR_POUR:
        {
            auto opérateur_pour = noeud->comme_opérateur_pour();
            fonction_courante = opérateur_pour;
            simplifie(opérateur_pour->corps);
            return opérateur_pour;
        }
        case GenreNoeud::DÉCLARATION_CORPS_FONCTION:
        {
            auto corps = noeud->comme_corps_fonction();

            auto fut_dans_fonction = m_dans_fonction;
            m_dans_fonction = true;
            simplifie(corps->bloc);
            m_dans_fonction = fut_dans_fonction;

            if (corps->aide_génération_code == REQUIERS_CODE_EXTRA_RETOUR) {
                auto retourne = assem->crée_retourne(corps->lexème, nullptr);
                retourne->bloc_parent = corps->bloc;
                corps->bloc->ajoute_expression(retourne);
            }
            else if (corps->aide_génération_code == REQUIERS_RETOUR_UNION_VIA_RIEN) {
                crée_retourne_union_via_rien(corps->entête, corps->bloc, corps->lexème);
            }

            return corps;
        }
        case GenreNoeud::INSTRUCTION_COMPOSÉE:
        {
            auto bloc = noeud->comme_bloc();

            assem->bloc_courant(bloc);
            m_expressions_blocs.empile_tableau();

            POUR (*bloc->expressions.verrou_ecriture()) {
                if (it->est_entête_fonction()) {
                    continue;
                }
                auto expression = simplifie(it);
                if (expression) {
                    ajoute_expression(expression);
                }
                /* Certaines expressions n'ont pas de substitution (par exemple les expressions
                 * finales des blocs des expressions-si). Nous devons les préserver. */
                else if (!it->est_pousse_contexte() && !it->est_tente()) {
                    ajoute_expression(it);
                }
            }

            auto nouvelles_expressions = m_expressions_blocs.donne_tableau_courant();
            bloc->expressions->remplace_données_par(nouvelles_expressions);

            m_expressions_blocs.dépile_tableau();
            assem->dépile_bloc();

            return bloc;
        }
        case GenreNoeud::OPÉRATEUR_BINAIRE:
        {
            auto expr_bin = noeud->comme_expression_binaire();

            if (expr_bin->type->est_type_type_de_données()) {
                noeud->substitution = assem->crée_référence_type(expr_bin->lexème, expr_bin->type);
                return noeud->substitution;
            }

            simplifie(expr_bin->opérande_gauche);
            simplifie(expr_bin->opérande_droite);

            if (expr_bin->op && expr_bin->op->est_arithmétique_pointeur) {
                return simplifie_arithmétique_pointeur(expr_bin);
            }

            if (expr_bin->possède_drapeau(DrapeauxNoeud::EST_ASSIGNATION_COMPOSEE)) {
                noeud->substitution = assem->crée_assignation_variable(
                    expr_bin->lexème,
                    expr_bin->opérande_gauche,
                    simplifie_opérateur_binaire(expr_bin, true));
                return noeud->substitution;
            }

            noeud->substitution = simplifie_opérateur_binaire(expr_bin, false);
            return noeud->substitution;
        }
        case GenreNoeud::EXPRESSION_LOGIQUE:
        {
            auto logique = noeud->comme_expression_logique();
            return simplifie_expression_logique(logique);
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_LOGIQUE:
        {
            auto logique = noeud->comme_assignation_logique();
            return simplifie_assignation_logique(logique);
        }
        case GenreNoeud::OPÉRATEUR_UNAIRE:
        {
            auto expr_un = noeud->comme_expression_unaire();

            if (expr_un->op && expr_un->op->genre == OpérateurUnaire::Genre::Négation) {
                if (expr_un->opérande->est_littérale_entier()) {
                    auto littérale = expr_un->opérande->comme_littérale_entier();
                    auto valeur_entière = int64_t(littérale->valeur);
                    valeur_entière = -valeur_entière;
                    littérale->valeur = uint64_t(valeur_entière);
                    expr_un->substitution = littérale;
                    return littérale;
                }
                if (expr_un->opérande->est_littérale_réel()) {
                    auto littérale = expr_un->opérande->comme_littérale_réel();
                    littérale->valeur = -littérale->valeur;
                    expr_un->substitution = littérale;
                    return littérale;
                }
            }

            simplifie(expr_un->opérande);

            /* op peut être nul pour les opérateurs ! et * */
            if (expr_un->op && !expr_un->op->est_basique) {
                auto appel = assem->crée_appel(
                    expr_un->lexème, expr_un->op->déclaration, expr_un->op->type_résultat);
                appel->paramètres_résolus.ajoute(expr_un->opérande);
                expr_un->substitution = appel;
                return appel;
            }

            return expr_un;
        }
        case GenreNoeud::EXPRESSION_PRISE_ADRESSE:
        {
            auto prise_adresse = noeud->comme_prise_adresse();
            if (prise_adresse->type->est_type_type_de_données()) {
                prise_adresse->substitution = assem->crée_référence_type(prise_adresse->lexème,
                                                                         prise_adresse->type);
                return prise_adresse->substitution;
            }
            simplifie(prise_adresse->opérande);
            return prise_adresse;
        }
        case GenreNoeud::EXPRESSION_PRISE_RÉFÉRENCE:
        {
            auto prise_référence = noeud->comme_prise_référence();
            if (prise_référence->type->est_type_type_de_données()) {
                prise_référence->substitution = assem->crée_référence_type(prise_référence->lexème,
                                                                           prise_référence->type);
                return prise_référence->substitution;
            }
            simplifie(prise_référence->opérande);
            return prise_référence;
        }
        case GenreNoeud::EXPRESSION_NÉGATION_LOGIQUE:
        {
            auto négation = noeud->comme_négation_logique();
            simplifie(négation->opérande);
            return noeud;
        }
        case GenreNoeud::EXPRESSION_TYPE_DE:
        {
            /* change simplement le genre du noeud car le type de l'expression est le type de sa
             * sous expression */
            noeud->genre = GenreNoeud::EXPRESSION_RÉFÉRENCE_TYPE;
            return noeud;
        }
        case GenreNoeud::DIRECTIVE_CUISINE:
        {
            auto cuisine = noeud->comme_cuisine();
            auto expr = cuisine->expression->comme_appel();
            cuisine->substitution = assem->crée_référence_déclaration(
                expr->lexème, expr->expression->comme_entête_fonction());
            return cuisine->substitution;
        }
        case GenreNoeud::INSTRUCTION_SI_STATIQUE:
        {
            auto inst = noeud->comme_si_statique();

            if (inst->condition_est_vraie) {
                simplifie(inst->bloc_si_vrai);
                inst->substitution = inst->bloc_si_vrai;
                return inst->substitution;
            }

            simplifie(inst->bloc_si_faux);
            inst->substitution = inst->bloc_si_faux;
            return inst->substitution;
        }
        case GenreNoeud::INSTRUCTION_SAUFSI_STATIQUE:
        {
            auto inst = noeud->comme_saufsi_statique();

            if (!inst->condition_est_vraie) {
                simplifie(inst->bloc_si_vrai);
                inst->substitution = inst->bloc_si_vrai;
                return inst->substitution;
            }

            simplifie(inst->bloc_si_faux);
            inst->substitution = inst->bloc_si_faux;
            return inst->substitution;
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_DÉCLARATION:
        {
            auto référence = noeud->comme_référence_déclaration();
            auto déclaration = référence->déclaration_référée;

            if (déclaration->est_déclaration_constante()) {
                auto decl_const = déclaration->comme_déclaration_constante();

                if (déclaration->type->est_type_type_de_données()) {
                    référence->substitution = assem->crée_référence_type(
                        référence->lexème, typeuse.type_type_de_donnees(déclaration->type));
                    return référence->substitution;
                }

                auto type = déclaration->type;
                if (type->est_type_opaque()) {
                    type = type->comme_type_opaque()->type_opacifié;
                }

                if (type->est_type_réel()) {
                    référence->substitution = assem->crée_littérale_réel(
                        référence->lexème,
                        déclaration->type,
                        decl_const->valeur_expression.réelle());
                    return référence->substitution;
                }

                if (type->est_type_bool()) {
                    référence->substitution = assem->crée_littérale_bool(
                        référence->lexème,
                        déclaration->type,
                        decl_const->valeur_expression.booléenne());
                    return référence->substitution;
                }

                if (est_type_entier(type) || type->est_type_entier_constant() ||
                    type->est_type_énum() || type->est_type_erreur()) {
                    référence->substitution = assem->crée_littérale_entier(
                        référence->lexème,
                        déclaration->type,
                        static_cast<uint64_t>(decl_const->valeur_expression.entière()));
                    return référence->substitution;
                }

                /* À FAIRE : test que les opaques fonctionnent ici. */
                if (déclaration->type->est_type_chaine()) {
                    référence->substitution = decl_const->expression;
                    return référence->substitution;
                }

                if (déclaration->type->est_type_tableau_fixe()) {
                    référence->substitution = decl_const->expression;
                    return référence->substitution;
                }

                assert(false);
                return référence;
            }

            if (déclaration->est_déclaration_type()) {
                référence->substitution = assem->crée_référence_type(référence->lexème,
                                                                     déclaration->type);
                return référence->substitution;
            }

            if (déclaration->est_déclaration_variable()) {
                auto declaration_variable = déclaration->comme_déclaration_variable();
                if (declaration_variable->déclaration_vient_d_un_emploi) {
                    /* Transforme en un accès de membre. */
                    auto ref_decl_var = assem->crée_référence_déclaration(
                        référence->lexème, declaration_variable->déclaration_vient_d_un_emploi);

                    auto type = ref_decl_var->type;
                    while (type->est_type_pointeur() || type->est_type_référence()) {
                        type = type_déréférencé_pour(type);
                    }

                    auto type_composé = type->comme_type_composé();
                    auto membre =
                        type_composé->membres[declaration_variable->index_membre_employé];

                    if (membre.possède_drapeau(MembreTypeComposé::PROVIENT_D_UN_EMPOI)) {
                        auto accès_membre = crée_référence_pour_membre_employé(
                            assem, référence->lexème, ref_decl_var, type_composé, membre);
                        référence->substitution = accès_membre;
                    }
                    else {
                        auto accès_membre = assem->crée_référence_membre(
                            référence->lexème,
                            ref_decl_var,
                            référence->type,
                            declaration_variable->index_membre_employé);
                        référence->substitution = accès_membre;
                    }
                }
            }

            if (!m_substitutions_boucles_pour.est_vide()) {
                /* Si nous somme dans le corps d'une boucle-pour personnalisée, substitue le
                 * paramètre de l'opérateur par la variable. */
                auto &données_substitution = m_substitutions_boucles_pour.haut();
                if (référence->déclaration_référée == données_substitution.param) {
                    référence->substitution = données_substitution.référence_paramètre;
                }
            }

            return noeud;
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_MEMBRE:
        {
            return simplifie_référence_membre(noeud->comme_référence_membre());
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_MEMBRE_UNION:
        {
            auto ref_membre_union = noeud->comme_référence_membre_union();
            simplifie(ref_membre_union->accédée);
            return ref_membre_union;
        }
        case GenreNoeud::EXPRESSION_COMME:
        {
            auto inst = noeud->comme_comme();
            auto expr = inst->expression;

            if (expr->type == inst->type) {
                simplifie(expr);
                noeud->substitution = expr;
                return expr;
            }

            if (expr->type->est_type_entier_constant() &&
                inst->transformation.type == TypeTransformation::ENTIER_VERS_POINTEUR) {
                expr->type = TypeBase::Z64;
                return inst;
            }

            simplifie(inst->expression);
            return inst;
        }
        case GenreNoeud::INSTRUCTION_POUR:
        {
            return simplifie_boucle_pour(noeud->comme_pour());
        }
        case GenreNoeud::INSTRUCTION_BOUCLE:
        {
            auto boucle = noeud->comme_boucle();
            simplifie(boucle->bloc);
            return boucle;
        }
        case GenreNoeud::INSTRUCTION_RÉPÈTE:
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
            auto boucle = noeud->comme_répète();
            simplifie(boucle->condition);
            simplifie(boucle->bloc);

            auto nouvelle_boucle = assem->crée_boucle(noeud->lexème, nullptr);
            nouvelle_boucle->bloc_parent = boucle->bloc_parent;

            auto condition = crée_condition_boucle(nouvelle_boucle,
                                                   GenreNoeud::INSTRUCTION_SAUFSI);
            condition->condition = boucle->condition;

            boucle->bloc->ajoute_expression(condition);

            nouvelle_boucle->bloc = boucle->bloc;
            boucle->substitution = nouvelle_boucle;
            return nouvelle_boucle;
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

            auto nouvelle_boucle = assem->crée_boucle(noeud->lexème, nullptr);
            nouvelle_boucle->bloc_parent = boucle->bloc_parent;

            auto condition = crée_condition_boucle(nouvelle_boucle,
                                                   GenreNoeud::INSTRUCTION_SAUFSI);
            condition->condition = boucle->condition;

            boucle->bloc->expressions->ajoute_au_début(condition);

            nouvelle_boucle->bloc = boucle->bloc;
            boucle->substitution = nouvelle_boucle;
            return nouvelle_boucle;
        }
        case GenreNoeud::DIRECTIVE_CORPS_BOUCLE:
        {
            assert(!m_substitutions_boucles_pour.est_vide());
            auto &données = m_substitutions_boucles_pour.haut();
            noeud->substitution = données.corps_boucle;
            /* Le nouveau bloc parent du bloc originel de la boucle doit être le bloc parent de
             * l'instruction qu'il remplace pour que les instructions « diffère » fonctionnent
             * proprement. */
            données.corps_boucle->bloc_parent = noeud->bloc_parent;
            return noeud;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
        {
            auto tableau = noeud->comme_construction_tableau();
            simplifie(tableau->expression);
            return tableau;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU_TYPÉ:
        {
            auto tableau = noeud->comme_construction_tableau_typé();
            simplifie(tableau->expression);
            return tableau;
        }
        case GenreNoeud::EXPRESSION_VIRGULE:
        {
            auto virgule = noeud->comme_virgule();

            POUR (virgule->expressions) {
                simplifie(it);
            }

            return virgule;
        }
        case GenreNoeud::INSTRUCTION_RETIENS:
        {
            return simplifie_retiens(noeud->comme_retiens());
        }
        case GenreNoeud::INSTRUCTION_DISCR:
        case GenreNoeud::INSTRUCTION_DISCR_ÉNUM:
        case GenreNoeud::INSTRUCTION_DISCR_UNION:
        {
            auto discr = noeud->comme_discr();
            return simplifie_discr(discr);
        }
        case GenreNoeud::EXPRESSION_PARENTHÈSE:
        {
            auto parenthèse = noeud->comme_parenthèse();
            simplifie(parenthèse->expression);
            parenthèse->substitution = parenthèse->expression;
            return parenthèse->substitution;
        }
        case GenreNoeud::INSTRUCTION_TENTE:
        {
            return simplifie_tente(noeud->comme_tente());
        }
        case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
        {
            auto tableau = noeud->comme_args_variadiques();

            POUR (tableau->expressions) {
                simplifie(it);
            }

            return tableau;
        }
        case GenreNoeud::INSTRUCTION_RETOUR:
        {
            auto retour = noeud->comme_retourne();
            return simplifie_retour(retour);
        }
        case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
        {
            auto retour = noeud->comme_retourne_multiple();
            return simplifie_retour(retour);
        }
        case GenreNoeud::INSTRUCTION_SAUFSI:
        case GenreNoeud::INSTRUCTION_SI:
        {
            return simplifie_instruction_si(noeud->comme_si());
        }
        case GenreNoeud::OPÉRATEUR_COMPARAISON_CHAINÉE:
        {
            return simplifie_comparaison_chainée(noeud->comme_comparaison_chainée());
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
        {
            return simplifie_construction_structure(noeud->comme_construction_structure());
        }
        case GenreNoeud::EXPRESSION_APPEL:
        {
            auto appel = noeud->comme_appel();

            auto ancien_site_pour_position_code_source = m_site_pour_position_code_source;
            m_site_pour_position_code_source = appel;

            if (appel->aide_génération_code == CONSTRUIT_OPAQUE) {
                simplifie(appel->paramètres_résolus[0]);
                auto comme = crée_comme_type_cible(
                    appel->lexème, appel->paramètres_résolus[0], appel->type);
                appel->substitution = comme;
                m_site_pour_position_code_source = ancien_site_pour_position_code_source;
                return comme;
            }

            if (appel->aide_génération_code == CONSTRUIT_OPAQUE_DEPUIS_STRUCTURE) {
                return simplifie_construction_opaque_depuis_structure(appel);
            }

            if (appel->aide_génération_code == MONOMORPHE_TYPE_OPAQUE) {
                appel->substitution = assem->crée_référence_type(appel->lexème, appel->type);
            }

            if (appel->noeud_fonction_appelée) {
                if (!appel->expression->est_référence_déclaration() ||
                    appel->expression->comme_référence_déclaration()->déclaration_référée !=
                        appel->noeud_fonction_appelée) {
                    appel->expression->substitution = assem->crée_référence_déclaration(
                        appel->lexème, appel->noeud_fonction_appelée->comme_déclaration_symbole());
                }
            }
            else {
                simplifie(appel->expression);
            }

            POUR (appel->paramètres_résolus) {
                simplifie(it);
            }

            m_site_pour_position_code_source = ancien_site_pour_position_code_source;
            return appel;
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
        {
            auto assignation = noeud->comme_assignation_variable();

            simplifie(assignation->assignée);

            if (assignation->assignée->possède_drapeau(DrapeauxNoeud::ACCES_EST_ENUM_DRAPEAU)) {
                /* NOTE : pour le moment nous ne pouvons déclarer de nouvelle variables ici
                 * pour les valeurs temporaires, et puisque nous ne pouvons pas utiliser
                 * l'expression dans sa substitution, nous modifions l'expression
                 * directement.
                 */
                assignation->expression = simplifie_assignation_énum_drapeau(
                    assignation->assignée, assignation->expression);
            }
            else {
                simplifie(assignation->expression);
            }

            return assignation;
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_MULTIPLE:
        {
            auto assignation = noeud->comme_assignation_multiple();

            simplifie(assignation->assignées);

            POUR (assignation->données_exprs.plage()) {
                auto expression_fut_simplifiee = false;

                for (auto var : it.variables.plage()) {
                    if (var->possède_drapeau(DrapeauxNoeud::ACCES_EST_ENUM_DRAPEAU)) {
                        /* NOTE : pour le moment nous ne pouvons déclarer de nouvelle variables ici
                         * pour les valeurs temporaires, et puisque nous ne pouvons pas utiliser
                         * l'expression dans sa substitution, nous modifions l'expression
                         * directement. Ceci est plus ou moins correcte, puisque donnees_expr n'est
                         * censé être que pour la génération de code.
                         */
                        it.expression = simplifie_assignation_énum_drapeau(var, it.expression);
                        expression_fut_simplifiee = true;
                    }
                }

                if (!expression_fut_simplifiee) {
                    simplifie(it.expression);
                }
            }

            return assignation;
        }
        case GenreNoeud::DÉCLARATION_VARIABLE:
        {
            auto déclaration = noeud->comme_déclaration_variable();
            simplifie(déclaration->expression);

            if (!fonction_courante && déclaration->expression &&
                !déclaration->expression->est_non_initialisation() &&
                m_expressions_blocs.taille_données() != 0) {
                auto bloc = assem->crée_bloc_seul(déclaration->lexème, déclaration->bloc_parent);
                auto expressions = m_expressions_blocs.donne_tableau_courant();
                POUR (expressions) {
                    bloc->expressions->ajoute(it);
                }

                /* À FAIRE : meilleure gestion des globales pour la génération de code. */
                if (déclaration->expression->substitution) {
                    bloc->expressions->ajoute(déclaration->expression->substitution);
                    déclaration->expression->substitution = bloc;
                }
                else {
                    bloc->expressions->ajoute(déclaration->expression);
                    déclaration->expression = bloc;
                }
            }

            return déclaration;
        }
        case GenreNoeud::DÉCLARATION_VARIABLE_MULTIPLE:
        {
            auto déclaration = noeud->comme_déclaration_variable_multiple();
            POUR (déclaration->données_decl.plage()) {
                simplifie(it.expression);
            }
            return déclaration;
        }
        case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
        {
            auto pousse_contexte = noeud->comme_pousse_contexte();
            simplifie(pousse_contexte->bloc);

            auto contexte_courant = espace->compilatrice().globale_contexte_programme;
            auto ref_contexte_courant = assem->crée_référence_déclaration(pousse_contexte->lexème,
                                                                          contexte_courant);

            // sauvegarde_contexte := __contexte_fil_principal
            auto sauvegarde_contexte = crée_déclaration_variable(
                pousse_contexte->lexème, contexte_courant->type, ref_contexte_courant);
            sauvegarde_contexte->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
            auto ref_sauvegarde_contexte = assem->crée_référence_déclaration(
                pousse_contexte->lexème, sauvegarde_contexte);
            ajoute_expression(sauvegarde_contexte);

            // __contexte_fil_principal = expr
            auto permute_contexte = assem->crée_assignation_variable(
                pousse_contexte->lexème, ref_contexte_courant, pousse_contexte->expression);
            ajoute_expression(permute_contexte);

            /* Il est possible qu'une instruction de retour se trouve dans le bloc, donc nous
             * devons différer la restauration du contexte :
             *
             * diffère __contexte_fil_principal = sauvegarde_contexte
             */
            auto expression_différée = assem->crée_assignation_variable(
                pousse_contexte->lexème, ref_contexte_courant, ref_sauvegarde_contexte);
            auto inst_diffère = assem->crée_diffère(pousse_contexte->lexème, expression_différée);
            pousse_contexte->bloc->expressions->ajoute_au_début(inst_diffère);

            ajoute_expression(pousse_contexte->bloc);

            return nullptr;
        }
        case GenreNoeud::EXPRESSION_INDEXAGE:
        {
            auto indexage = noeud->comme_indexage();
            simplifie(indexage->opérande_gauche);
            simplifie(indexage->opérande_droite);

            if (indexage->op) {
                indexage->substitution = simplifie_opérateur_binaire(indexage, true);
            }

            return indexage;
        }
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        {
            auto structure = noeud->comme_type_structure();

            POUR (structure->membres) {
                simplifie(it.expression_valeur_defaut);
            }

            return structure;
        }
        case GenreNoeud::INSTRUCTION_DIFFÈRE:
        {
            auto inst = noeud->comme_diffère();
            simplifie(inst->expression);
            return inst;
        }
        case GenreNoeud::EXPRESSION_PLAGE:
        {
            auto plage = noeud->comme_plage();
            simplifie(plage->début);
            simplifie(plage->fin);
            return plage;
        }
        case GenreNoeud::EXPANSION_VARIADIQUE:
        {
            auto expr = noeud->comme_expansion_variadique();
            if (expr->type->est_type_type_de_données()) {
                /* Nous avons un type variadique. */
                expr->substitution = assem->crée_référence_type(expr->lexème, expr->type);
            }
            else {
                simplifie(expr->expression);
            }
            return expr;
        }
        case GenreNoeud::INSTRUCTION_EMPL:
        {
            if (!m_dans_fonction) {
                /* Ne simplifie que les expressions empl dans le corps de fonctions. */
                return noeud;
            }

            auto expr_empl = noeud->comme_empl();
            simplifie(expr_empl->expression);
            /* empl n'est pas géré dans la RI. */
            expr_empl->substitution = expr_empl->expression;
            return expr_empl->substitution;
        }
        case GenreNoeud::EXPRESSION_MÉMOIRE:
        {
            auto expr_mémoire = noeud->comme_mémoire();
            simplifie(expr_mémoire->expression);
            return expr_mémoire;
        }
        case GenreNoeud::DIRECTIVE_INTROSPECTION:
        {
            if (noeud->ident == ID::chemin_de_ce_fichier) {
                auto &compilatrice = espace->compilatrice();
                auto littérale_chaine = assem->crée_littérale_chaine(noeud->lexème);
                littérale_chaine->drapeaux |=
                    DrapeauxNoeud::LEXÈME_EST_RÉUTILISÉ_POUR_SUBSTITUTION;
                auto fichier = compilatrice.fichier(noeud->lexème->fichier);
                littérale_chaine->valeur = compilatrice.gérante_chaine->ajoute_chaine(
                    fichier->chemin());
                littérale_chaine->type = TypeBase::CHAINE;
                noeud->substitution = littérale_chaine;
            }
            else if (noeud->ident == ID::chemin_de_ce_module) {
                auto &compilatrice = espace->compilatrice();
                auto littérale_chaine = assem->crée_littérale_chaine(noeud->lexème);
                littérale_chaine->drapeaux |=
                    DrapeauxNoeud::LEXÈME_EST_RÉUTILISÉ_POUR_SUBSTITUTION;
                auto fichier = compilatrice.fichier(noeud->lexème->fichier);
                littérale_chaine->valeur = compilatrice.gérante_chaine->ajoute_chaine(
                    fichier->module->chemin());
                littérale_chaine->type = TypeBase::CHAINE;
                noeud->substitution = littérale_chaine;
            }
            else if (noeud->ident == ID::nom_de_cette_fonction) {
                assert(fonction_courante);
                auto &compilatrice = espace->compilatrice();
                auto littérale_chaine = assem->crée_littérale_chaine(noeud->lexème);
                littérale_chaine->drapeaux |=
                    DrapeauxNoeud::LEXÈME_EST_RÉUTILISÉ_POUR_SUBSTITUTION;
                littérale_chaine->valeur = compilatrice.gérante_chaine->ajoute_chaine(
                    fonction_courante->ident->nom);
                littérale_chaine->type = TypeBase::CHAINE;
                noeud->substitution = littérale_chaine;
            }
            else if (noeud->ident == ID::type_de_cette_fonction ||
                     noeud->ident == ID::type_de_cette_structure) {
                noeud->substitution = assem->crée_référence_type(noeud->lexème, noeud->type);
            }

            return noeud->substitution;
        }
        case GenreNoeud::EXPRESSION_TYPE_TABLEAU_FIXE:
        case GenreNoeud::EXPRESSION_TYPE_TABLEAU_DYNAMIQUE:
        case GenreNoeud::EXPRESSION_TYPE_TRANCHE:
        case GenreNoeud::EXPRESSION_TYPE_FONCTION:
        {
            noeud->substitution = assem->crée_référence_type(noeud->lexème, noeud->type);
            return noeud->substitution;
        }
        case GenreNoeud::EXPRESSION_SÉLECTION:
        {
            auto sélection = noeud->comme_sélection();
            simplifie(sélection->condition);
            simplifie(sélection->si_vrai);
            simplifie(sélection->si_faux);
            return sélection;
        }
        case GenreNoeud::DIRECTIVE_EXÉCUTE:
        {
            auto exécute = noeud->comme_exécute();
            if (exécute->substitution) {
                simplifie(exécute->substitution);
            }
            return exécute->substitution;
        }
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ERREUR:
        case GenreNoeud::ENUM_DRAPEAU:
        CAS_POUR_NOEUDS_TYPES_FONDAMENTAUX:
        case GenreNoeud::DÉCLARATION_UNION:
        case GenreNoeud::DÉCLARATION_OPAQUE:
        case GenreNoeud::EXPRESSION_INFO_DE:
        case GenreNoeud::EXPRESSION_INIT_DE:
        case GenreNoeud::EXPRESSION_LITTÉRALE_BOOLÉEN:
        case GenreNoeud::EXPRESSION_LITTÉRALE_CARACTÈRE:
        case GenreNoeud::EXPRESSION_LITTÉRALE_CHAINE:
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_ENTIER:
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_RÉEL:
        case GenreNoeud::EXPRESSION_LITTÉRALE_NUL:
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_TYPE:
        case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
        case GenreNoeud::INSTRUCTION_CHARGE:
        case GenreNoeud::INSTRUCTION_IMPORTE:
        case GenreNoeud::INSTRUCTION_ARRÊTE:
        case GenreNoeud::INSTRUCTION_CONTINUE:
        case GenreNoeud::INSTRUCTION_REPRENDS:
        /* NOTE : taille_de doit persister jusque dans la RI. */
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        {
            return noeud;
        }
    }

    return noeud;
}

NoeudDéclarationVariable *Simplificatrice::crée_déclaration_variable(const Lexème *lexème,
                                                                     NoeudDéclarationType *type,
                                                                     NoeudExpression *expression)
{
    auto ident = donne_identifiant_pour_variable();
    return assem->crée_déclaration_variable(lexème, type, ident, expression);
}

NoeudComme *Simplificatrice::crée_comme_type_cible(const Lexème *lexème,
                                                   NoeudExpression *expression,
                                                   NoeudDéclarationType *type)
{
    auto résultat = assem->crée_comme(lexème, expression, nullptr);
    résultat->type = type;
    résultat->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;
    résultat->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type};
    return résultat;
}

IdentifiantCode *Simplificatrice::donne_identifiant_pour_variable()
{
    if (!fonction_courante) {
        return nullptr;
    }

    auto nom_base = kuri::chaine_statique("");
    if (fonction_courante->ident) {
        nom_base = fonction_courante->ident->nom;
    }
    else {
        nom_base = "fonction";
    }

    auto nom = enchaine("tmp_", nom_base, "_", m_nombre_variables);
    m_nombre_variables++;

    auto table_identifiant = espace->compilatrice().table_identifiants.verrou_ecriture();
    return table_identifiant->identifiant_pour_nouvelle_chaine(nom);
}

NoeudExpression *Simplificatrice::simplifie_boucle_pour(NoeudPour *inst)
{
    simplifie(inst->expression);
    simplifie(inst->bloc);
    simplifie(inst->bloc_sansarrêt);
    simplifie(inst->bloc_sinon);

    if (inst->aide_génération_code == BOUCLE_POUR_OPÉRATEUR) {
        return simplifie_boucle_pour_opérateur(inst);
    }

    auto it = inst->decl_it;
    auto index_it = inst->decl_index_it;
    auto expression_iteree = inst->expression;
    auto bloc_sans_arrêt = inst->bloc_sansarrêt;
    auto bloc_sinon = inst->bloc_sinon;

    auto boucle = assem->crée_boucle(inst->lexème, nullptr);
    boucle->ident = it->ident;
    boucle->bloc_parent = inst->bloc_parent;
    boucle->bloc = inst->bloc;
    boucle->bloc_sansarrêt = bloc_sans_arrêt;
    boucle->bloc_sinon = bloc_sinon;

    auto type_index_it = index_it->type;
    auto zero = assem->crée_littérale_entier(index_it->lexème, type_index_it, 0);

    auto ref_it = assem->crée_référence_déclaration(it->lexème, it);
    auto ref_index = assem->crée_référence_déclaration(it->lexème, index_it);

    /* Ajoute les déclarations de ces variables dans le bloc précédent. */
    ajoute_expression(it);
    ajoute_expression(index_it);

    auto bloc_inc = assem->crée_bloc_seul(nullptr, boucle->bloc_parent);

    auto condition = crée_condition_boucle(boucle, GenreNoeud::INSTRUCTION_SI);

    boucle->bloc_inc = bloc_inc;

    auto const inverse_boucle = inst->lexème_op == GenreLexème::SUPERIEUR;

    auto type_itéré = expression_iteree->type->est_type_opaque() ?
                          expression_iteree->type->comme_type_opaque()->type_opacifié :
                          expression_iteree->type;
    expression_iteree->type = type_itéré;

    /* boucle */

    switch (inst->aide_génération_code) {
        case GENERE_BOUCLE_PLAGE:
        case GENERE_BOUCLE_PLAGE_IMPLICITE:
        {
            /*

              pour 0 ... 10 {
                corps
              }

              it := 0
              index_it := 0
              itérations := (10 - 0) + 1

              boucle {
                si itérations == 0 {
                    arrête
                }

                corps:
                  ...

                incrémente:
                    it = it + 1
                    index_it = index_it + 1
                    itérations -= 1
              }

             */

            NoeudExpression *expr_debut = nullptr;
            NoeudExpression *expr_fin = nullptr;

            if (inst->aide_génération_code == GENERE_BOUCLE_PLAGE_IMPLICITE) {
                // 0 ... expr - 1
                expr_debut = assem->crée_littérale_entier(
                    expression_iteree->lexème, expression_iteree->type, 0);

                auto valeur_un = assem->crée_littérale_entier(
                    expression_iteree->lexème, expression_iteree->type, 1);
                expr_fin = assem->crée_expression_binaire(
                    expression_iteree->lexème,
                    expression_iteree->type->table_opérateurs->opérateur_sst,
                    expression_iteree,
                    valeur_un);
            }
            else {
                auto expr_plage = expression_iteree->comme_plage();
                expr_debut = expr_plage->début;
                expr_fin = expr_plage->fin;
            }

            /* Calcul le nombre d'itérations pour se prémunir des débordements pour les types
             * d'entiers naturels.
             * Nombre d'itérations = (fin - début) + 1
             */
            NoeudExpression *nombre_iterations = assem->crée_expression_binaire(
                expression_iteree->lexème,
                expression_iteree->type->table_opérateurs->opérateur_sst,
                expr_fin,
                expr_debut);

            auto valeur_un = assem->crée_littérale_entier(
                expression_iteree->lexème, expression_iteree->type, 1);
            nombre_iterations = assem->crée_expression_binaire(
                expression_iteree->lexème,
                expression_iteree->type->table_opérateurs->opérateur_ajt,
                nombre_iterations,
                valeur_un);

            /* condition */
            if (inverse_boucle) {
                std::swap(expr_debut, expr_fin);
            }

            /* Initialise la variable d'itération. */
            it->expression = expr_debut;

            auto op_comp = index_it->type->table_opérateurs->opérateur_seg;
            condition->condition = assem->crée_expression_binaire(
                inst->lexème, op_comp, ref_index, nombre_iterations);
            boucle->bloc->expressions->ajoute_au_début(condition);

            /* suivant */
            if (inverse_boucle) {
                auto inc_it = assem->crée_decrementation(ref_it->lexème, ref_it);
                bloc_inc->ajoute_expression(inc_it);
            }
            else {
                auto inc_it = assem->crée_incrementation(ref_it->lexème, ref_it);
                bloc_inc->ajoute_expression(inc_it);
            }

            auto inc_it = assem->crée_incrementation(ref_index->lexème, ref_index);
            bloc_inc->ajoute_expression(inc_it);

            break;
        }
        case GENERE_BOUCLE_TABLEAU:
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

            if (type_itéré->est_type_tableau_fixe()) {
                auto taille_tableau = type_itéré->comme_type_tableau_fixe()->taille;
                expr_taille = assem->crée_littérale_entier(
                    inst->lexème, TypeBase::Z64, static_cast<uint64_t>(taille_tableau));
            }
            else {
                expr_taille = assem->crée_référence_membre(
                    inst->lexème, expression_iteree, TypeBase::Z64, 1);
            }

            auto type_z64 = TypeBase::Z64;
            condition->condition = assem->crée_expression_binaire(
                inst->lexème, type_z64->table_opérateurs->opérateur_seg, ref_index, expr_taille);

            auto expr_pointeur = NoeudExpression::nul();

            auto type_compose = type_itéré->comme_type_composé();

            if (type_itéré->est_type_tableau_fixe()) {
                auto indexage = crée_indexage(inst->lexème, expression_iteree, zero);

                expr_pointeur = crée_prise_adresse(
                    assem, inst->lexème, indexage, typeuse.type_pointeur_pour(indexage->type));
            }
            else {
                expr_pointeur = assem->crée_référence_membre(
                    inst->lexème, expression_iteree, type_compose->membres[0].type, 0);
            }

            NoeudExpression *expr_index = ref_index;

            if (inverse_boucle) {
                expr_index = assem->crée_expression_binaire(
                    inst->lexème,
                    ref_index->type->table_opérateurs->opérateur_sst,
                    expr_taille,
                    ref_index);
                expr_index = assem->crée_expression_binaire(
                    inst->lexème,
                    ref_index->type->table_opérateurs->opérateur_sst,
                    expr_index,
                    assem->crée_littérale_entier(ref_index->lexème, ref_index->type, 1));
            }

            auto indexage = crée_indexage(inst->lexème, expr_pointeur, expr_index);
            NoeudExpression *expression_assignee = indexage;

            if (inst->prend_référence || inst->prend_pointeur) {
                auto noeud_comme = assem->crée_comme(it->lexème, indexage, nullptr);
                noeud_comme->type = it->type;
                noeud_comme->transformation = TransformationType(
                    TypeTransformation::PREND_REFERENCE);
                noeud_comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;

                expression_assignee = noeud_comme;
            }

            auto assign_it = assem->crée_assignation_variable(
                inst->lexème, ref_it, expression_assignee);

            /* Inverse l'ordre puisque nous les ajoutons au début. */
            boucle->bloc->expressions->ajoute_au_début(assign_it);
            boucle->bloc->expressions->ajoute_au_début(condition);

            /* incrémente */
            auto inc_it = assem->crée_incrementation(ref_index->lexème, ref_index);
            bloc_inc->ajoute_expression(inc_it);
            break;
        }
        case GENERE_BOUCLE_COROUTINE:
        {
            /* À FAIRE(ri) : coroutine */
#if 0
            auto expr_appel = static_cast<NoeudExpressionAppel *>(enfant2);
            auto decl_fonc = static_cast<NoeudDéclarationCorpsFonction const *>(expr_appel->noeud_fonction_appelée);
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
                génère_code_C(it, constructrice, compilatrice, false);
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

            auto feuilles = kuri::tablet<NoeudExpression *, 10>{};
            rassemble_feuilles(enfant1, feuilles);

            auto idx = NoeudExpression::nul();
            auto nom_idx = kuri::chaine{};

            if (b->aide_génération_code == GENERE_BOUCLE_COROUTINE_INDEX) {
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
        case BOUCLE_POUR_OPÉRATEUR:
        {
            assert(false);
            break;
        }
    }

    inst->substitution = boucle;
    return boucle;
}

NoeudExpression *Simplificatrice::simplifie_boucle_pour_opérateur(NoeudPour *inst)
{
    auto corps_opérateur_pour = inst->corps_opérateur_pour;

    auto bloc_substitution = assem->crée_bloc_seul(corps_opérateur_pour->bloc->lexème,
                                                   inst->bloc_parent);

    /* Crée une variable temporaire pour l'expression itérée. Si l'expression est par exemple un
     * appel, il sera toujours évalué, menant potentiellement à une boucle infinie. */
    auto temporaire = crée_déclaration_variable(
        inst->expression->lexème, inst->expression->type, inst->expression);
    temporaire->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
    auto ref_temporaire = assem->crée_référence_déclaration(temporaire->lexème, temporaire);
    bloc_substitution->ajoute_expression(temporaire);

    /* Ajoute les déclarations des variables d'itération dans le corps du bloc pour que la RI les
     * trouve avant de générer le code des références. */
    bloc_substitution->ajoute_expression(inst->decl_it);
    bloc_substitution->ajoute_expression(inst->decl_index_it);
    bloc_substitution->ajoute_expression(corps_opérateur_pour->bloc);

    /* Substitutions manuelles. */
    auto entête = corps_opérateur_pour->entête;
    auto param = entête->parametre_entree(0);

    SubstitutionBouclePourOpérée substitution_manuelle;
    substitution_manuelle.référence_paramètre = ref_temporaire;
    substitution_manuelle.param = param;
    substitution_manuelle.corps_boucle = inst->bloc;

    m_substitutions_boucles_pour.empile(substitution_manuelle);
    simplifie(inst->corps_opérateur_pour);
    m_substitutions_boucles_pour.depile();

    inst->substitution = bloc_substitution;
    return inst;
}

static void rassemble_opérations_chainées(NoeudExpression *racine,
                                          kuri::tableau<NoeudExpressionBinaire> &comparaisons)
{
    auto expr_bin = racine->comme_expression_binaire();

    if (est_opérateur_comparaison(expr_bin->opérande_gauche->lexème->genre)) {
        rassemble_opérations_chainées(expr_bin->opérande_gauche, comparaisons);

        auto expr_opérande = expr_bin->opérande_gauche->comme_expression_binaire();

        auto comparaison = NoeudExpressionBinaire{};
        comparaison.lexème = expr_bin->lexème;
        comparaison.opérande_gauche = expr_opérande->opérande_droite;
        comparaison.opérande_droite = expr_bin->opérande_droite;
        comparaison.op = expr_bin->op;
        comparaison.permute_opérandes = expr_bin->permute_opérandes;

        comparaisons.ajoute(comparaison);
    }
    else {
        auto comparaison = NoeudExpressionBinaire{};
        comparaison.lexème = expr_bin->lexème;
        comparaison.opérande_gauche = expr_bin->opérande_gauche;
        comparaison.opérande_droite = expr_bin->opérande_droite;
        comparaison.op = expr_bin->op;
        comparaison.permute_opérandes = expr_bin->permute_opérandes;

        comparaisons.ajoute(comparaison);
    }
}

NoeudExpression *Simplificatrice::crée_expression_pour_op_chainée(
    kuri::tableau<NoeudExpressionBinaire> &comparaisons, Lexème const *lexeme_op_logique)
{
    kuri::pile<NoeudExpression *> exprs;

    for (auto i = comparaisons.taille() - 1; i >= 0; --i) {
        auto &it = comparaisons[i];
        simplifie(it.opérande_gauche);
        simplifie(it.opérande_droite);
        exprs.empile(simplifie_opérateur_binaire(&it, true));
    }

    if (exprs.taille() == 1) {
        return exprs.depile();
    }

    auto résultat = NoeudExpression::nul();

    while (true) {
        auto a = exprs.depile();
        auto b = exprs.depile();

        auto et = assem->crée_expression_logique(lexeme_op_logique, a, b);

        if (exprs.est_vide()) {
            résultat = et;
            break;
        }

        exprs.empile(et);
    }

    return résultat;
}

NoeudExpression *Simplificatrice::cree_indexage(const Lexeme *lexeme,
                                                NoeudExpression *expr1,
                                                NoeudExpression *expr2)
{
    if (expr1->type->est_chaine()) {
        auto op = expr1->type->operateur_indexage;

        auto appel = assem->cree_appel(lexeme, op->decl, op->type_resultat);
        appel->parametres_resolus.ajoute(expr1);
        appel->parametres_resolus.ajoute(expr2);
        return appel;
    }

    return assem->cree_indexage(lexeme, expr1, expr2, true);
}

void Simplificatrice::corrige_bloc_pour_assignation(NoeudExpression *expr,
                                                    NoeudExpression *ref_temp)
{
    if (expr->est_bloc()) {
        auto bloc = expr->comme_bloc();

        auto di = bloc->expressions->dernier_élément();
        if (di->est_retourne() || di->est_retiens()) {
            return;
        }

        di = assem->crée_assignation_variable(di->lexème, ref_temp, di);
        bloc->expressions->supprime_dernier();
        bloc->ajoute_expression(di);
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
            dbg() << "Expression invalide pour la simplification de l'assignation implicite "
                     "d'un bloc si !\n"
                  << erreur::imprime_site(*espace, expr);
        });
    }
}

NoeudExpression *Simplificatrice::simplifie_comparaison_chainée(NoeudExpressionBinaire *comp)
{
    auto comparaisons = kuri::tableau<NoeudExpressionBinaire>();
    rassemble_opérations_chainées(comp, comparaisons);

    /*
      a <= b <= c

      a <= b && b <= c && c <= d

      &&
        &&
          a <= b
          b <= c
        c <= d
     */

    static const Lexème lexeme_et = {",", {}, GenreLexème::ESP_ESP, 0, 0, 0};
    comp->substitution = crée_expression_pour_op_chainée(comparaisons, &lexeme_et);
    return comp;
}

NoeudExpression *Simplificatrice::crée_retourne_union_via_rien(
    NoeudDéclarationEntêteFonction *entête,
    NoeudBloc *bloc_d_insertion,
    Lexème const *lexeme_reference)
{
    auto type_sortie = entête->type->comme_type_fonction()->type_sortie->comme_type_union();

    auto param_sortie = entête->param_sortie;
    auto ref_param_sortie = assem->crée_référence_déclaration(lexeme_reference, param_sortie);

    auto type_union = type_sortie;

    auto info_membre = donne_membre_pour_type(type_union, TypeBase::RIEN);
    assert(info_membre.has_value());
    auto index_membre = uint32_t(info_membre->index_membre);

    auto ref_membre = assem->crée_référence_membre(
        lexeme_reference, ref_param_sortie, TypeBase::Z32, 1);
    auto valeur_index = assem->crée_littérale_entier(
        lexeme_reference, TypeBase::Z32, index_membre + 1);

    auto assignation = assem->crée_assignation_variable(
        lexeme_reference, ref_membre, valeur_index);

    auto retourne = assem->crée_retourne(lexeme_reference, ref_param_sortie);
    retourne->bloc_parent = bloc_d_insertion;

    bloc_d_insertion->ajoute_expression(assignation);
    bloc_d_insertion->ajoute_expression(retourne);

    return retourne;
}

/* Les retours sont simplifiés sous forme d'un chargement pour les retours simples. */
NoeudExpression *Simplificatrice::simplifie_retour(NoeudInstructionRetour *inst)
{
    if (inst->aide_génération_code == RETOURNE_UNE_UNION_VIA_RIEN) {
        auto bloc = assem->crée_bloc_seul(inst->lexème, inst->bloc_parent);
        crée_retourne_union_via_rien(fonction_courante, bloc, inst->lexème);
        inst->substitution = bloc;
        return bloc;
    }

    /* Nous n'utilisons pas le type de la fonction_courante car elle peut être nulle dans le cas où
     * nous avons un #test. */
    auto type_sortie = inst->type;
    if (type_sortie->est_type_rien()) {
        return inst;
    }

    simplifie(inst->expression);
    return inst;
}

/* Les retours sont simplifiés sous forme d'assignations des valeurs de retours. */
NoeudExpression *Simplificatrice::simplifie_retour(NoeudInstructionRetourMultiple *inst)
{
    /* Nous n'utilisons pas le type de la fonction_courante car elle peut être nulle dans le cas où
     * nous avons un #test. */
    assert(!inst->type->est_type_rien());

    /* Crée une assignation pour chaque sortie. */
    POUR (inst->données_exprs.plage()) {
        simplifie(it.expression);

        /* Les variables sont les déclarations des paramètres, donc crée des références. */
        for (auto &var : it.variables.plage()) {
            var = assem->crée_référence_déclaration(var->lexème,
                                                    var->comme_déclaration_variable());
        }
    }

    auto assignation = assem->crée_assignation_multiple(inst->lexème, nullptr, inst->expression);
    assignation->données_exprs = std::move(inst->données_exprs);

    auto expression_retournée = assem->crée_référence_déclaration(
        fonction_courante->param_sortie->lexème, fonction_courante->param_sortie);
    auto retour = assem->crée_retourne(inst->lexème, expression_retournée);

    ajoute_expression(assignation);

    inst->substitution = retour;
    return retour;
}

NoeudExpression *Simplificatrice::simplifie_construction_structure(
    NoeudExpressionConstructionStructure *construction)
{
    POUR (construction->paramètres_résolus) {
        simplifie(it);
    }

    if (construction->type->est_type_union()) {
        return simplifie_construction_union(construction);
    }

    /* L'expression peut être nulle pour les structures anonymes crées par la compilatrice. */
    if (construction->expression && construction->expression->ident == ID::PositionCodeSource) {
        return simplifie_construction_structure_position_code_source(construction);
    }

    return simplifie_construction_structure_impl(construction);
}

NoeudExpressionAppel *Simplificatrice::crée_appel_fonction_init(
    Lexème const *lexeme, NoeudExpression *expression_à_initialiser)
{
    auto type_expression = expression_à_initialiser->type;
    auto fonction_init = crée_entête_pour_initialisation_type(type_expression, assem, typeuse);

    auto prise_adresse = crée_prise_adresse(
        assem, lexeme, expression_à_initialiser, typeuse.type_pointeur_pour(type_expression));
    auto appel = assem->crée_appel(lexeme, fonction_init, TypeBase::RIEN);
    appel->paramètres_résolus.ajoute(prise_adresse);

    return appel;
}

NoeudExpression *Simplificatrice::simplifie_expression_logique(NoeudExpressionLogique *logique)
{
    // À FAIRE : simplifie les accès à des énum_drapeaux dans les expressions || ou &&,
    // il faudra également modifier la RI pour prendre en compte la substitution
    if (logique->possède_drapeau(PositionCodeNoeud::DROITE_CONDITION)) {
        simplifie(logique->opérande_gauche);
        simplifie(logique->opérande_droite);
        return logique;
    }

    /* Simplifie comme GCC pour les assignations
     * a := b && c ->  x := b; si x == vrai { x = c; }; a := x;
     * a := b || c ->  x := b; si x == faux { x = c; }; a := x;
     */

    auto gauche = logique->opérande_gauche;
    auto droite = logique->opérande_droite;

    simplifie(gauche);
    gauche = simplifie_expression_pour_expression_logique(gauche);

    auto déclaration = crée_déclaration_variable(gauche->lexème, gauche->type, gauche);
    déclaration->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
    ajoute_expression(déclaration);

    /* Utilisation d'un lexème spécifique pour la RI, qui pour les conditions des expressions-si se
     * base sur le lexème... */
    static Lexème lexème_référence;
    lexème_référence.genre = GenreLexème::CHAINE_CARACTERE;

    auto référence = assem->crée_référence_déclaration(&lexème_référence, déclaration);

    auto bloc_parent = logique->bloc_parent;
    auto const lexème = logique->lexème;
    auto inst_si = (lexème->genre == GenreLexème::BARRE_BARRE) ?
                       assem->crée_saufsi(lexème, référence) :
                       assem->crée_si(lexème, référence);
    ajoute_expression(inst_si);

    auto bloc = assem->crée_bloc_seul(lexème, bloc_parent);
    inst_si->bloc_si_vrai = bloc;

    droite = simplifie_expression_pour_expression_logique(droite);
    auto assignation = assem->crée_assignation_variable(lexème, référence, droite);
    bloc->ajoute_expression(assignation);

    /* Simplifie le nouveau bloc, et non juste l'opérande droite, afin que les expressions générées
     * par la simplification de ladite opérande s'y retrouve. */
    simplifie(bloc);

    logique->substitution = référence;
    return logique->substitution;
}

NoeudExpression *Simplificatrice::simplifie_expression_pour_expression_logique(
    NoeudExpression *expression)
{
    auto type_condition = expression->type;
    if (type_condition->est_type_opaque()) {
        type_condition = type_condition->comme_type_opaque()->type_opacifié;
    }

    switch (type_condition->genre) {
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::ENTIER_RELATIF:
        case GenreNoeud::ENTIER_CONSTANT:
        {
            /* x -> x != 0 */
            auto zéro = assem->crée_littérale_entier(expression->lexème, type_condition, 0);
            auto op = type_condition->table_opérateurs->opérateur_dif;
            return assem->crée_expression_binaire(expression->lexème, op, expression, zéro);
        }
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::BOOL:
        {
            return expression;
        }
        case GenreNoeud::FONCTION:
        case GenreNoeud::POINTEUR:
        {
            /* x -> x != nul */
            auto zéro = assem->crée_littérale_nul(expression->lexème);
            zéro->type = type_condition;
            auto op = type_condition->table_opérateurs->opérateur_dif;
            return assem->crée_expression_binaire(expression->lexème, op, expression, zéro);
        }
        case GenreNoeud::EINI:
        {
            /* x -> x.pointeur != nul */
            auto ref_pointeur = assem->crée_référence_membre(
                expression->lexème, expression, TypeBase::PTR_RIEN, 0);
            auto zéro = assem->crée_littérale_nul(expression->lexème);
            zéro->type = TypeBase::PTR_RIEN;
            auto op = zéro->type->table_opérateurs->opérateur_dif;
            return assem->crée_expression_binaire(expression->lexème, op, ref_pointeur, zéro);
        }
        case GenreNoeud::CHAINE:
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        case GenreNoeud::TYPE_TRANCHE:
        {
            /* x -> x.taille != 0 */
            auto ref_taille = assem->crée_référence_membre(
                expression->lexème, expression, TypeBase::Z64, 1);
            auto zéro = assem->crée_littérale_entier(expression->lexème, TypeBase::Z64, 0);
            auto op = zéro->type->table_opérateurs->opérateur_dif;
            return assem->crée_expression_binaire(expression->lexème, op, ref_taille, zéro);
        }
        default:
        {
            assert_rappel(false, [&]() {
                dbg() << "Type non géré pour la génération d'une condition d'une branche : "
                      << chaine_type(type_condition);
            });
            break;
        }
    }

    return nullptr;
}

NoeudExpression *Simplificatrice::simplifie_tente(NoeudInstructionTente *inst)
{
    simplifie(inst->expression_appelée);
    if (inst->bloc) {
        simplifie(inst->bloc);
    }

    auto lexème = inst->lexème;
    auto expression_tentée = inst->expression_appelée;

    if (expression_tentée->type->est_type_erreur()) {
        /* tmp := expression_tentée */
        auto type_erreur = expression_tentée->type->comme_type_erreur();
        auto déclaration_erreur = crée_déclaration_variable(
            lexème, type_erreur, expression_tentée);
        déclaration_erreur->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
        ajoute_expression(déclaration_erreur);
        auto référence_erreur = assem->crée_référence_déclaration(lexème, déclaration_erreur);

        /* si tmp != 0 */
        auto zéro = assem->crée_littérale_entier(lexème, type_erreur, 0);
        auto op = type_erreur->table_opérateurs->opérateur_dif;
        assert(op);
        auto comparaison = assem->crée_expression_binaire(lexème, op, référence_erreur, zéro);

        auto branche = assem->crée_si(lexème, comparaison);
        ajoute_expression(branche);

        if (inst->expression_piégée == nullptr) {
            /* piége nonatteignable -> panique */
            auto bloc = assem->crée_bloc_seul(inst->lexème, inst->bloc_parent);
            branche->bloc_si_vrai = bloc;

            auto panique = espace->compilatrice().interface_kuri->decl_panique_erreur;
            assert(panique);

            auto appel = assem->crée_appel(inst->lexème, panique, TypeBase::RIEN);
            bloc->ajoute_expression(appel);
        }
        else {
            branche->bloc_si_vrai = inst->bloc;

            /* expression_piégée = tmp */
            auto déclaration_piège = inst->expression_piégée->comme_référence_déclaration()
                                         ->déclaration_référée->comme_déclaration_variable();
            déclaration_piège->expression = référence_erreur;
            inst->bloc->expressions->ajoute_au_début(déclaration_piège);
        }

        if (inst->possède_drapeau(PositionCodeNoeud::DROITE_ASSIGNATION)) {
            inst->substitution = référence_erreur;
        }
        return inst->substitution;
    }

    if (expression_tentée->type->est_type_union()) {
        auto type_union = expression_tentée->type->comme_type_union();
        auto index_membre_erreur = 0u;
        auto index_membre_variable = 1u;
        auto type_erreur = NoeudDéclarationType::nul();
        auto type_variable = NoeudDéclarationType::nul();

        if (type_union->membres[0].type->est_type_erreur()) {
            type_erreur = type_union->membres[0].type;
            type_variable = type_union->membres[1].type;
        }
        else {
            type_erreur = type_union->membres[1].type;
            type_variable = type_union->membres[0].type;
            index_membre_erreur = 1u;
            index_membre_variable = 0u;
        }

        /* tmp := expression_tentée */
        auto déclaration_erreur = crée_déclaration_variable(lexème, type_union, expression_tentée);
        déclaration_erreur->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
        ajoute_expression(déclaration_erreur);
        auto référence_erreur = assem->crée_référence_déclaration(lexème, déclaration_erreur);

        /* Variable qui détiendra la valeur non-erreur. Nous devons l'initialiser à zéro, car il se
         * peut que la fonction retourne une erreur nulle (0 comme type_erreur). */
        auto déclaration_variable = NoeudDéclarationVariable::nul();
        if (!type_variable->est_type_rien()) {
            déclaration_variable = crée_déclaration_variable(lexème, type_variable, nullptr);
            déclaration_variable->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
            ajoute_expression(déclaration_variable);
        }

        auto accès_membre_actif = assem->crée_référence_membre(
            lexème, référence_erreur, TypeBase::Z32, 1);

        /* si membre_actif == erreur */
        auto index = assem->crée_littérale_entier(lexème, type_erreur, index_membre_erreur + 1);
        auto op = TypeBase::Z32->table_opérateurs->opérateur_egt;
        assert(op);
        auto comparaison = assem->crée_expression_binaire(lexème, op, accès_membre_actif, index);

        auto branche = assem->crée_si(lexème, comparaison);
        ajoute_expression(branche);
        auto bloc_si_erreur = assem->crée_bloc_seul(inst->lexème, inst->bloc_parent);
        branche->bloc_si_vrai = bloc_si_erreur;

        auto extrait_erreur = assem->crée_comme(lexème, référence_erreur, nullptr);
        extrait_erreur->type = type_erreur;
        extrait_erreur->transformation = {TypeTransformation::EXTRAIT_UNION, type_erreur};
        extrait_erreur->transformation.index_membre = index_membre_erreur;
        extrait_erreur->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;

        auto déclaration_résultat = NoeudDéclarationVariable::nul();
        if (inst->expression_piégée == nullptr) {
            déclaration_résultat = crée_déclaration_variable(lexème, type_erreur, extrait_erreur);
            déclaration_résultat->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
            bloc_si_erreur->ajoute_expression(déclaration_résultat);
        }
        else {
            auto déclaration_piège = inst->expression_piégée->comme_référence_déclaration()
                                         ->déclaration_référée->comme_déclaration_variable();
            déclaration_résultat = déclaration_piège;
            déclaration_piège->expression = extrait_erreur;
            bloc_si_erreur->expressions->ajoute_au_début(déclaration_piège);
        }

        auto référence_résultat = assem->crée_référence_déclaration(lexème, déclaration_résultat);

        /* Même code que pour type_erreur plus haut. */
        auto zéro = assem->crée_littérale_entier(lexème, type_erreur, 0);
        op = type_erreur->table_opérateurs->opérateur_dif;
        assert(op);
        comparaison = assem->crée_expression_binaire(lexème, op, référence_résultat, zéro);

        auto branche_si_erreur = assem->crée_si(lexème, comparaison);
        bloc_si_erreur->ajoute_expression(branche_si_erreur);

        if (inst->expression_piégée == nullptr) {
            /* piége nonatteignable -> panique */
            auto bloc = assem->crée_bloc_seul(inst->lexème, bloc_si_erreur->bloc_parent);
            branche_si_erreur->bloc_si_vrai = bloc;

            auto panique = espace->compilatrice().interface_kuri->decl_panique_erreur;
            assert(panique);

            auto appel = assem->crée_appel(inst->lexème, panique, TypeBase::RIEN);
            bloc->ajoute_expression(appel);
        }
        else {
            branche_si_erreur->bloc_si_vrai = inst->bloc;
        }

        if (!type_variable->est_type_rien()) {
            auto bloc_si_pas_erreur = assem->crée_bloc_seul(inst->lexème, inst->bloc_parent);
            branche->bloc_si_faux = bloc_si_pas_erreur;

            auto extrait_variable = assem->crée_comme(lexème, référence_erreur, nullptr);
            extrait_variable->type = type_variable;
            extrait_variable->transformation = {TypeTransformation::EXTRAIT_UNION, type_variable};
            extrait_variable->transformation.index_membre = index_membre_variable;
            extrait_variable->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;

            auto référence_variable = assem->crée_référence_déclaration(lexème,
                                                                        déclaration_variable);
            auto init_variable = assem->crée_assignation_variable(
                lexème, référence_variable, extrait_variable);
            bloc_si_pas_erreur->ajoute_expression(init_variable);
            inst->substitution = référence_variable;
        }

        return inst->substitution;
    }

    return inst;
}

NoeudExpression *Simplificatrice::simplifie_assignation_logique(
    NoeudExpressionAssignationLogique *logique)
{
    auto gauche = logique->opérande_gauche;
    auto droite = logique->opérande_droite;

    simplifie(gauche);

    auto bloc_parent = logique->bloc_parent;
    auto const lexème = logique->lexème;
    auto inst_si = (lexème->genre == GenreLexème::BARRE_BARRE_EGAL) ?
                       assem->crée_saufsi(lexème, gauche) :
                       assem->crée_si(lexème, gauche);

    auto bloc = assem->crée_bloc_seul(lexème, bloc_parent);
    inst_si->bloc_si_vrai = bloc;

    auto assignation = assem->crée_assignation_variable(lexème, gauche, droite);
    bloc->ajoute_expression(assignation);

    /* Simplifie le nouveau bloc, et non juste l'opérande droite, afin que les expressions générées
     * par la simplification de ladite opérande s'y retrouve. */
    simplifie(bloc);

    logique->substitution = inst_si;
    return inst_si;
}

NoeudExpression *Simplificatrice::simplifie_construction_union(
    NoeudExpressionConstructionStructure *construction)
{
    auto const lexème = construction->lexème;
    auto type_union = construction->type->comme_type_union();

    if (construction->paramètres_résolus.est_vide()) {
        /* Initialise à zéro. */

        auto decl_position = crée_déclaration_variable(lexème, type_union, &non_initialisation);
        decl_position->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
        auto ref_position = assem->crée_référence_déclaration(decl_position->lexème,
                                                              decl_position);

        ajoute_expression(decl_position);

        auto appel = crée_appel_fonction_init(lexème, ref_position);
        ajoute_expression(appel);

        construction->substitution = ref_position;
        return ref_position;
    }

    auto index_membre = 0u;
    auto expression_initialisation = NoeudExpression::nul();

    POUR (construction->paramètres_résolus) {
        if (it != nullptr) {
            expression_initialisation = it;
            break;
        }

        index_membre += 1;
    }

    assert(expression_initialisation);

    /* Nous devons transtyper l'expression, la RI s'occupera d'initialiser le membre implicite en
     * cas d'union sûre. */
    auto comme = assem->crée_comme(lexème, expression_initialisation, nullptr);
    comme->type = type_union;
    comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;
    comme->transformation = {TypeTransformation::CONSTRUIT_UNION, type_union, index_membre};

    construction->substitution = comme;
    return comme;
}

NoeudExpression *Simplificatrice::simplifie_construction_structure_position_code_source(
    NoeudExpressionConstructionStructure *construction)
{
    auto const lexème = construction->lexème;
    const NoeudExpression *site = m_site_pour_position_code_source ?
                                      m_site_pour_position_code_source :
                                      construction;
    auto const lexème_site = site->lexème;

    auto &compilatrice = espace->compilatrice();

    /* Création des valeurs pour chaque membre. */

    /* PositionCodeSource.fichier
     * À FAIRE : sécurité, n'utilise pas le chemin, mais détermine une manière fiable et robuste
     * d'obtenir le fichier, utiliser simplement le nom n'est pas fiable (d'autres fichiers du même
     * nom dans le module). */
    auto const fichier = compilatrice.fichier(lexème_site->fichier);
    auto valeur_chemin_fichier = assem->crée_littérale_chaine(lexème);
    valeur_chemin_fichier->drapeaux |= DrapeauxNoeud::LEXÈME_EST_RÉUTILISÉ_POUR_SUBSTITUTION;
    valeur_chemin_fichier->valeur = compilatrice.gérante_chaine->ajoute_chaine(fichier->chemin());
    valeur_chemin_fichier->type = TypeBase::CHAINE;

    /* PositionCodeSource.fonction */
    auto nom_fonction = kuri::chaine_statique("");
    if (fonction_courante && fonction_courante->ident) {
        nom_fonction = fonction_courante->ident->nom;
    }

    auto valeur_nom_fonction = assem->crée_littérale_chaine(lexème);
    valeur_nom_fonction->drapeaux |= DrapeauxNoeud::LEXÈME_EST_RÉUTILISÉ_POUR_SUBSTITUTION;
    valeur_nom_fonction->valeur = compilatrice.gérante_chaine->ajoute_chaine(nom_fonction);
    valeur_nom_fonction->type = TypeBase::CHAINE;

    /* PositionCodeSource.ligne */
    auto pos = position_lexeme(*lexème_site);
    auto valeur_ligne = assem->crée_littérale_entier(
        lexème, TypeBase::Z32, static_cast<unsigned>(pos.numero_ligne));

    /* PositionCodeSource.colonne */
    auto valeur_colonne = assem->crée_littérale_entier(
        lexème, TypeBase::Z32, static_cast<unsigned>(pos.pos));

    /* Création d'une temporaire et assignation des membres. */

    auto const type_position_code_source = typeuse.type_position_code_source;
    auto decl_position = crée_déclaration_variable(
        lexème, type_position_code_source, &non_initialisation);
    decl_position->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
    auto ref_position = assem->crée_référence_déclaration(decl_position->lexème, decl_position);

    auto ref_membre_fichier = assem->crée_référence_membre(
        lexème, ref_position, TypeBase::CHAINE, 0);
    auto ref_membre_fonction = assem->crée_référence_membre(
        lexème, ref_position, TypeBase::CHAINE, 1);
    auto ref_membre_ligne = assem->crée_référence_membre(lexème, ref_position, TypeBase::Z32, 2);
    auto ref_membre_colonne = assem->crée_référence_membre(lexème, ref_position, TypeBase::Z32, 3);

    NoeudExpression *couples_ref_membre_expression[4][2] = {
        {ref_membre_fichier, valeur_chemin_fichier},
        {ref_membre_fonction, valeur_nom_fonction},
        {ref_membre_ligne, valeur_ligne},
        {ref_membre_colonne, valeur_colonne},
    };

    ajoute_expression(decl_position);

    for (auto couple : couples_ref_membre_expression) {
        auto assign = assem->crée_assignation_variable(lexème, couple[0], couple[1]);
        ajoute_expression(assign);
    }

    construction->substitution = ref_position;
    return ref_position;
}

NoeudExpressionRéférence *Simplificatrice::génère_simplification_construction_structure(
    NoeudExpressionAppel *construction, TypeStructure *type_struct)
{
    auto const lexème = construction->lexème;
    auto déclaration = crée_déclaration_variable(lexème, type_struct, &non_initialisation);
    déclaration->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
    auto référence = assem->crée_référence_déclaration(déclaration->lexème, déclaration);

    ajoute_expression(déclaration);

    POUR_INDEX (construction->paramètres_résolus) {
        const auto &membre = type_struct->membres[index_it];

        if ((membre.drapeaux & MembreTypeComposé::EST_CONSTANT) != 0) {
            continue;
        }

        if (membre.drapeaux & MembreTypeComposé::PROVIENT_D_UN_EMPOI) {
            if (it == nullptr || (!membre.expression_initialisation_est_spéciale() &&
                                  it == membre.expression_valeur_defaut)) {
                /* Le membre de base ayant ajouté ce membre est également initialisé, il est donc
                 * inutile ce membre s'il n'y a pas d'expression pour lui. */
                continue;
            }

            auto ref_membre = crée_référence_pour_membre_employé(
                assem, lexème, référence, type_struct, membre);
            auto assign = assem->crée_assignation_variable(lexème, ref_membre, it);
            ajoute_expression(assign);
            continue;
        }

        auto type_membre = membre.type;
        auto ref_membre = assem->crée_référence_membre(lexème, référence, type_membre, index_it);

        if (it != nullptr) {
            auto assign = assem->crée_assignation_variable(lexème, ref_membre, it);
            ajoute_expression(assign);
        }
        else {
            auto appel = crée_appel_fonction_init(lexème, ref_membre);
            ajoute_expression(appel);
        }
    }

    return référence;
}

NoeudExpression *Simplificatrice::simplifie_construction_structure_impl(
    NoeudExpressionConstructionStructure *construction)
{
    auto const type_struct = construction->type->comme_type_structure();
    auto ref_struct = génère_simplification_construction_structure(construction, type_struct);
    construction->substitution = ref_struct;
    return ref_struct;
}

NoeudExpression *Simplificatrice::simplifie_construction_opaque_depuis_structure(
    NoeudExpressionAppel *appel)
{
    auto const lexème = appel->lexème;
    auto type_opaque = appel->type->comme_type_opaque();
    auto type_struct =
        const_cast<Type *>(donne_type_opacifié_racine(type_opaque))->comme_type_structure();

    auto ref_struct = génère_simplification_construction_structure(appel, type_struct);

    /* ref_opaque := ref_struct comme TypeOpaque */
    auto comme = crée_comme_type_cible(appel->lexème, ref_struct, type_opaque);

    auto decl_opaque = crée_déclaration_variable(lexème, type_opaque, comme);
    decl_opaque->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
    auto ref_opaque = assem->crée_référence_déclaration(decl_opaque->lexème, decl_opaque);
    ajoute_expression(decl_opaque);

    appel->substitution = ref_opaque;
    return ref_opaque;
}

/**
 * Trouve le membre de \a type_composé ayant ajouté via `empl base: ...` la \a decl_employée et
 * retourne son #InformationMembreTypeCompose. */
static std::optional<InformationMembreTypeCompose> trouve_information_membre_ayant_ajouté_decl(
    TypeCompose *type_composé, NoeudDéclarationSymbole *decl_employée)
{
    auto info_membre = donne_membre_pour_type(type_composé, decl_employée->type);
    if (!info_membre) {
        return {};
    }

    if (!info_membre->membre.est_un_emploi()) {
        return {};
    }

    return info_membre;
}

/**
 * Trouve le membre de \a type_composé ayant pour nom le \a nom donné et
 * retourne son #InformationMembreTypeCompose. */
static std::optional<InformationMembreTypeCompose> trouve_information_membre_ajouté_par_emploi(
    TypeCompose *type_composé, IdentifiantCode *nom)
{
    return donne_membre_pour_nom(type_composé, nom);
}

/**
 * Construit la hiérarchie des structures employées par \a type_composé jusqu'à la structure
 * employée ayant ajoutée \a membre à \a type_composé.
 * Le résultat contiendra une #InformationMembreTypeCompose pour chaque membre employé de chaque
 * structure rencontrée + le membre de la structure à l'origine du membre ajouté par emploi.
 *
 * Par exemple, pour :
 *
 * Struct1 :: struct {
 *    x: z32
 * }
 *
 * Struct2 :: struct {
 *    empl base1: Struct1
 * }
 *
 * Struct3 :: struct {
 *    empl base2: Struct2
 * }
 *
 * Struct4 :: struct {
 *    empl base3: Struct3
 * }
 *
 * Retourne :
 * - base3
 * - base2
 * - base1
 * - x
 */
static kuri::tableau<InformationMembreTypeCompose, int> trouve_hiérarchie_emploi_membre(
    TypeCompose *type_composé, MembreTypeComposé const &membre)
{
    kuri::tableau<InformationMembreTypeCompose, int> hiérarchie;

    auto type_composé_courant = type_composé;
    auto membre_courant = membre;

    while (true) {
        auto decl_membre = membre_courant.decl->comme_déclaration_variable();
        auto decl_employée = decl_membre->déclaration_vient_d_un_emploi;
        auto info = trouve_information_membre_ayant_ajouté_decl(type_composé_courant,
                                                                decl_employée);
        assert(info.has_value());

        hiérarchie.ajoute(info.value());

        auto type_employé = info->membre.type->comme_type_composé();
        auto info_membre = trouve_information_membre_ajouté_par_emploi(type_employé,
                                                                       membre_courant.nom);
        assert(info_membre.has_value());

        if ((info_membre->membre.drapeaux & MembreTypeComposé::PROVIENT_D_UN_EMPOI) == 0) {
            /* Nous sommes au bout de la hiérarchie, ajoutons le membre, et arrêtons. */
            hiérarchie.ajoute(info_membre.value());
            break;
        }

        type_composé_courant = type_employé;
        membre_courant = info_membre->membre;
    }

    return hiérarchie;
}

static NoeudExpression *crée_référence_pour_membre_employé(AssembleuseArbre *assem,
                                                           Lexème const *lexeme,
                                                           NoeudExpression *expression_accédée,
                                                           TypeCompose *type_composé,
                                                           MembreTypeComposé const &membre)
{
    auto hiérarchie = trouve_hiérarchie_emploi_membre(type_composé, membre);
    auto ref_membre_courant = expression_accédée;

    POUR (hiérarchie) {
        auto accès_base = assem->crée_référence_membre(
            lexeme, ref_membre_courant, it.membre.type, it.index_membre);
        accès_base->ident = it.membre.nom;
        ref_membre_courant = accès_base;
    }

    assert(ref_membre_courant != expression_accédée);
    return ref_membre_courant;
}

NoeudExpression *Simplificatrice::simplifie_référence_membre(NoeudExpressionMembre *ref_membre)
{
    auto const lexème = ref_membre->lexème;
    auto accédée = ref_membre->accédée;

    if (ref_membre->possède_drapeau(DrapeauxNoeud::ACCES_EST_ENUM_DRAPEAU)) {
        simplifie(accédée);

        // a.DRAPEAU => (a & DRAPEAU) != 0
        auto type_enum = static_cast<TypeEnum *>(ref_membre->type);
        auto valeur_énum = type_enum->membres[ref_membre->index_membre].valeur;

        auto valeur_lit_enum = assem->crée_littérale_entier(
            lexème, type_enum, static_cast<unsigned>(valeur_énum));
        auto op = type_enum->table_opérateurs->opérateur_etb;
        auto et = assem->crée_expression_binaire(lexème, op, accédée, valeur_lit_enum);

        auto zero = assem->crée_littérale_entier(lexème, type_enum, 0);
        op = type_enum->table_opérateurs->opérateur_dif;
        auto dif = assem->crée_expression_binaire(lexème, op, et, zero);

        ref_membre->substitution = dif;
        return ref_membre;
    }

    if (accédée->est_référence_déclaration()) {
        if (accédée->comme_référence_déclaration()
                ->déclaration_référée->est_déclaration_module()) {
            ref_membre->substitution = assem->crée_référence_déclaration(
                lexème, ref_membre->déclaration_référée);
            simplifie(ref_membre->substitution);
            return ref_membre;
        }
    }

    auto type_accédé = donne_type_accédé_effectif(accédée->type);

    if (type_accédé->est_type_type_de_données()) {
        auto type_de_donnees = type_accédé->comme_type_type_de_données();
        if (type_de_donnees->type_connu != nullptr) {
            type_accédé = type_de_donnees->type_connu;
        }
    }

    if (type_accédé->est_type_tableau_fixe()) {
        auto taille = type_accédé->comme_type_tableau_fixe()->taille;
        ref_membre->substitution = assem->crée_littérale_entier(
            lexème, ref_membre->type, static_cast<uint64_t>(taille));
        return ref_membre;
    }

    if (type_accédé->est_type_énum() || type_accédé->est_type_erreur()) {
        auto type_enum = static_cast<TypeEnum *>(type_accédé);
        auto valeur_énum = type_enum->membres[ref_membre->index_membre].valeur;
        ref_membre->substitution = assem->crée_littérale_entier(
            lexème, type_enum, static_cast<unsigned>(valeur_énum));
        return ref_membre;
    }

    auto type_composé = type_accédé->comme_type_composé();
    auto &membre = type_composé->membres[ref_membre->index_membre];

    if (membre.drapeaux == MembreTypeComposé::EST_CONSTANT) {
        simplifie(membre.expression_valeur_defaut);
        ref_membre->substitution = membre.expression_valeur_defaut;
        if (ref_membre->substitution->type->est_type_entier_constant()) {
            ref_membre->substitution->type = membre.type;
        }
        return ref_membre;
    }

    if (membre.drapeaux & MembreTypeComposé::PROVIENT_D_UN_EMPOI) {
        /* Transforme x.y en x.base.y. */
        ref_membre->substitution = crée_référence_pour_membre_employé(
            assem, lexème, ref_membre->accédée, type_composé, membre);
    }

    /* Pour les appels de fonctions ou les accès après des parenthèse (p.e. (x comme *TypeBase).y).
     */
    simplifie(ref_membre->accédée);
    return ref_membre;
}

NoeudExpression *Simplificatrice::simplifie_assignation_énum_drapeau(NoeudExpression *var,
                                                                     NoeudExpression *expression)
{
    auto lexème = var->lexème;
    auto ref_membre = var->comme_référence_membre();

    // À FAIRE : référence
    // Nous prenons ref_membre->accédée directement car ce ne sera pas
    // simplifié, et qu'il faut prendre en compte les accés d'accés, les
    // expressions entre parenthèses, etc. Donc faire ceci est plus simple.
    auto nouvelle_ref = ref_membre->accédée;
    var->substitution = nouvelle_ref;

    /* Crée la conjonction d'un drapeau avec la variable (a | DRAPEAU) */
    auto crée_conjonction_drapeau =
        [&](NoeudExpression *ref_variable, TypeEnum *type_enum, unsigned valeur_énum) {
            auto valeur_lit_enum = assem->crée_littérale_entier(lexème, type_enum, valeur_énum);
            auto op = type_enum->table_opérateurs->opérateur_oub;
            return assem->crée_expression_binaire(var->lexème, op, ref_variable, valeur_lit_enum);
        };

    /* Crée la disjonction d'un drapeau avec la variable (a & ~DRAPEAU) */
    auto crée_disjonction_drapeau =
        [&](NoeudExpression *ref_variable, TypeEnum *type_enum, unsigned valeur_énum) {
            auto valeur_lit_enum = assem->crée_littérale_entier(
                lexème, type_enum, ~uint64_t(valeur_énum));
            auto op = type_enum->table_opérateurs->opérateur_etb;
            return assem->crée_expression_binaire(var->lexème, op, ref_variable, valeur_lit_enum);
        };

    auto type_énum = static_cast<TypeEnum *>(ref_membre->type);
    auto valeur_énum = type_énum->membres[ref_membre->index_membre].valeur;

    if (expression->est_littérale_bool()) {
        /* Nous avons une expression littérale, donc nous pouvons choisir la bonne instruction. */
        if (expression->comme_littérale_bool()->valeur) {
            // a.DRAPEAU = vrai -> a = a | DRAPEAU
            return crée_conjonction_drapeau(
                nouvelle_ref, type_énum, static_cast<unsigned>(valeur_énum));
        }
        // a.DRAPEAU = faux -> a = a & ~DRAPEAU
        return crée_disjonction_drapeau(
            nouvelle_ref, type_énum, static_cast<unsigned>(valeur_énum));
    }
    /* Transforme en une expression « ternaire » sans branche (similaire à a = b ? v1 : v2 en
     * C/C++) :
     * v1 = (a | DRAPEAU)
     * v2 = (a & ~DRAPEAU)
     */

    auto v1 = crée_conjonction_drapeau(
        nouvelle_ref, type_énum, static_cast<unsigned>(valeur_énum));
    auto v2 = crée_disjonction_drapeau(
        nouvelle_ref, type_énum, static_cast<unsigned>(valeur_énum));

    simplifie(expression);
    auto ref_b = expression->substitution ? expression->substitution : expression;

    auto sélection = assem->crée_sélection(var->lexème, ref_b, v1, v2);
    sélection->type = type_énum;
    return sélection;
}

NoeudExpression *Simplificatrice::simplifie_opérateur_binaire(NoeudExpressionBinaire *expr_bin,
                                                              bool pour_opérande)
{
    if (expr_bin->op->est_basique) {
        if (!pour_opérande) {
            return nullptr;
        }

        /* Crée une nouvelle expression binaire afin d'éviter les dépassements de piles car
         * sinon la substitution serait toujours réévaluée lors de l'évaluation de l'expression
         * d'assignation. */
        return assem->crée_expression_binaire(
            expr_bin->lexème, expr_bin->op, expr_bin->opérande_gauche, expr_bin->opérande_droite);
    }

    auto appel = assem->crée_appel(
        expr_bin->lexème, expr_bin->op->decl, expr_bin->op->type_résultat);

    if (expr_bin->permute_opérandes) {
        appel->paramètres_résolus.ajoute(expr_bin->opérande_droite);
        appel->paramètres_résolus.ajoute(expr_bin->opérande_gauche);
    }
    else {
        appel->paramètres_résolus.ajoute(expr_bin->opérande_gauche);
        appel->paramètres_résolus.ajoute(expr_bin->opérande_droite);
    }

    return appel;
}

NoeudExpression *Simplificatrice::simplifie_arithmétique_pointeur(NoeudExpressionBinaire *expr_bin)
{
    auto comme_type = [&](NoeudExpression *expr_ptr, Type *type) {
        auto comme = assem->crée_comme(expr_ptr->lexème, expr_ptr, nullptr);
        comme->type = type;
        comme->transformation = {TypeTransformation::POINTEUR_VERS_ENTIER, type};
        return comme;
    };

    auto type1 = expr_bin->opérande_gauche->type;
    auto type2 = expr_bin->opérande_droite->type;

    // ptr - ptr => (ptr comme z64 - ptr comme z64) / taille_de(type_pointé)
    if (type1->est_type_pointeur() && type2->est_type_pointeur()) {
        auto type_z64 = TypeBase::Z64;
        auto soustraction = assem->crée_expression_binaire(
            expr_bin->lexème,
            type_z64->table_opérateurs->opérateur_sst,
            comme_type(expr_bin->opérande_gauche, type_z64),
            comme_type(expr_bin->opérande_droite, type_z64));

        auto substitution = soustraction;

        auto type_pointé = type2->comme_type_pointeur()->type_pointé;
        if (type_pointé->taille_octet != 1) {
            auto taille_de = assem->crée_littérale_entier(
                expr_bin->lexème, type_z64, std::max(type_pointé->taille_octet, 1u));

            substitution = assem->crée_expression_binaire(
                expr_bin->lexème,
                type_z64->table_opérateurs->opérateur_div,
                soustraction,
                taille_de);
        }

        expr_bin->substitution = substitution;
    }
    else {
        Type *type_entier = Type::nul();
        Type *type_pointeur = Type::nul();

        NoeudExpression *expr_entier = nullptr;
        NoeudExpression *expr_pointeur = nullptr;

        // ent + ptr => (ptr comme type_entier + ent * taille_de(type_pointé)) comme type_ptr
        if (est_type_entier(type1)) {
            type_entier = type1;
            type_pointeur = type2;
            expr_entier = expr_bin->opérande_gauche;
            expr_pointeur = expr_bin->opérande_droite;
        }
        // ptr - ent => (ptr comme type_entier - ent * taille_de(type_pointé)) comme type_ptr
        // ptr + ent => (ptr comme type_entier + ent * taille_de(type_pointé)) comme type_ptr
        else if (est_type_entier(type2)) {
            type_entier = type2;
            type_pointeur = type1;
            expr_entier = expr_bin->opérande_droite;
            expr_pointeur = expr_bin->opérande_gauche;
        }

        auto opérande = expr_entier;

        auto type_pointé = type_pointeur->comme_type_pointeur()->type_pointé;
        if (type_pointé->taille_octet != 1) {
            auto taille_de = assem->crée_littérale_entier(
                expr_entier->lexème, type_entier, std::max(type_pointé->taille_octet, 1u));
            opérande = assem->crée_expression_binaire(expr_entier->lexème,
                                                      type_entier->table_opérateurs->opérateur_mul,
                                                      expr_entier,
                                                      taille_de);
        }

        OpérateurBinaire *op_arithm = nullptr;

        if (expr_bin->lexème->genre == GenreLexème::MOINS ||
            expr_bin->lexème->genre == GenreLexème::MOINS_EGAL) {
            op_arithm = type_entier->table_opérateurs->opérateur_sst;
        }
        else if (expr_bin->lexème->genre == GenreLexème::PLUS ||
                 expr_bin->lexème->genre == GenreLexème::PLUS_EGAL) {
            op_arithm = type_entier->table_opérateurs->opérateur_ajt;
        }

        auto arithm = assem->crée_expression_binaire(
            expr_bin->lexème, op_arithm, comme_type(expr_pointeur, type_entier), opérande);

        auto comme_pointeur = assem->crée_comme(expr_bin->lexème, arithm, nullptr);
        comme_pointeur->type = type_pointeur;
        comme_pointeur->transformation = {TypeTransformation::ENTIER_VERS_POINTEUR, type_pointeur};

        expr_bin->substitution = comme_pointeur;
    }

    if (expr_bin->possède_drapeau(DrapeauxNoeud::EST_ASSIGNATION_COMPOSEE)) {
        expr_bin->substitution = assem->crée_assignation_variable(
            expr_bin->lexème, expr_bin->opérande_gauche, expr_bin->substitution);
    }

    return expr_bin;
}

NoeudExpression *Simplificatrice::simplifie_coroutine(NoeudDéclarationEntêteFonction *corout)
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
    génère_code_C(decl->bloc, constructrice, compilatrice, false);

    if (b->aide_génération_code == REQUIERS_CODE_EXTRA_RETOUR) {
        génère_code_extra_pre_retour(decl->bloc, compilatrice, constructrice);
    }

    constructrice << "}\n";

    compilatrice.termine_fonction();
    noeud->drapeaux |= RI_FUT_GENEREE;
#endif
    return corout;
}

static NoeudExpression *est_bloc_avec_une_seule_expression_simple(NoeudExpression const *noeud)
{
    if (!noeud->est_bloc()) {
        return nullptr;
    }

    auto bloc = noeud->comme_bloc();
    if (bloc->expressions->taille() != 1) {
        return nullptr;
    }

    auto expression = bloc->expressions->a(0);
    expression = expression->substitution ? expression->substitution : expression;

    /* À FAIRE : considère plus de cas, mais il faudra détecter les dérérérencement de pointeur et
     * les divisions par zéro. */
    switch (expression->genre) {
        case GenreNoeud::EXPRESSION_LITTÉRALE_BOOLÉEN:
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_ENTIER:
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_RÉEL:
        case GenreNoeud::EXPRESSION_LITTÉRALE_CHAINE:
        case GenreNoeud::EXPRESSION_LITTÉRALE_CARACTÈRE:
        case GenreNoeud::EXPRESSION_LITTÉRALE_NUL:
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_DÉCLARATION:
        {
            break;
        }
        default:
        {
            return nullptr;
        }
    }

    return expression;
}

static NoeudExpressionSélection *peut_être_compilée_avec_sélection(NoeudSi *inst_si,
                                                                   AssembleuseArbre *assem)
{
    auto si_vrai = est_bloc_avec_une_seule_expression_simple(inst_si->bloc_si_vrai);
    if (!si_vrai) {
        return nullptr;
    }
    auto si_faux = est_bloc_avec_une_seule_expression_simple(inst_si->bloc_si_faux);
    if (!si_faux) {
        return nullptr;
    }

    auto condition = inst_si->condition;
    if (condition->est_parenthèse()) {
        condition = condition->comme_parenthèse()->expression;
    }

    if (condition->est_expression_logique()) {
        /* Nous ignorons les expressions logiques car elles doivent être compilées différements
         * lorsque dans une condition de « si ». */
        return nullptr;
    }

    condition = condition->substitution ? condition->substitution : condition;
    if (!condition->type->est_type_bool()) {
        /* À FAIRE : canonicalisation des tests de types non booléens. */
        return nullptr;
    }

    auto résultat = assem->crée_sélection(inst_si->lexème, inst_si->condition, si_vrai, si_faux);
    résultat->type = inst_si->type;
    return résultat;
}

NoeudExpression *Simplificatrice::simplifie_instruction_si(NoeudSi *inst_si)
{
    simplifie(inst_si->condition);
    simplifie(inst_si->bloc_si_vrai);
    simplifie(inst_si->bloc_si_faux);

    if (!inst_si->possède_drapeau(PositionCodeNoeud::DROITE_ASSIGNATION)) {
        return inst_si;
    }

    if (auto sélection = peut_être_compilée_avec_sélection(inst_si, assem)) {
        inst_si->substitution = sélection;
        return sélection;
    }

    /* Transforme :
     *     x := si y { z } sinon { w }
     * en :
     *     decl := ---
     *     si y { decl = z } sinon { decl = w }
     *     x := decl
     */

    auto decl_temp = crée_déclaration_variable(
        inst_si->lexème, inst_si->type, &non_initialisation);
    decl_temp->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
    auto ref_temp = assem->crée_référence_déclaration(inst_si->lexème, decl_temp);

    auto nouveau_si = assem->crée_si(inst_si->lexème, inst_si->genre);
    nouveau_si->condition = inst_si->condition;
    nouveau_si->bloc_si_vrai = inst_si->bloc_si_vrai;
    nouveau_si->bloc_si_faux = inst_si->bloc_si_faux;

    corrige_bloc_pour_assignation(nouveau_si->bloc_si_vrai, ref_temp);
    corrige_bloc_pour_assignation(nouveau_si->bloc_si_faux, ref_temp);

    ajoute_expression(decl_temp);
    ajoute_expression(nouveau_si);

    inst_si->substitution = ref_temp;
    return ref_temp;
}

NoeudExpression *Simplificatrice::simplifie_retiens(NoeudRetiens *retiens)
{
#if 0
    auto df = compilatrice.donnees_fonction;
    auto enfant = retiens->expression;

    constructrice << "pthread_mutex_lock(&__etat->mutex_coro);\n";

    auto feuilles = kuri::tablet<NoeudExpression *, 10>{};
    rassemble_feuilles(enfant, feuilles);

    for (auto i = 0l; i < feuilles.taille(); ++i) {
        auto f = feuilles[i];

        génère_code_C(f, constructrice, compilatrice, true);

        constructrice << "__etat->" << df->noms_retours[i] << " = ";
        constructrice << f->chaine_calculee();
        constructrice << ";\n";
    }

    constructrice << "pthread_mutex_lock(&__etat->mutex_boucle);\n";
    constructrice << "pthread_cond_signal(&__etat->cond_boucle);\n";
    constructrice << "pthread_mutex_unlock(&__etat->mutex_boucle);\n";
    constructrice << "pthread_cond_wait(&__etat->cond_coro, &__etat->mutex_coro);\n";
    constructrice << "pthread_mutex_unlock(&__etat->mutex_coro);\n";
#endif
    return retiens;
}

static int valeur_énum(TypeEnum *type_énum, IdentifiantCode *ident)
{
    auto index_membre = 0;

    POUR (*type_énum->bloc->membres.verrou_lecture()) {
        if (it->ident == ident) {
            break;
        }

        index_membre += 1;
    }

    return type_énum->membres[index_membre].valeur;
}

enum {
    DISCR_UNION,
    DISCR_UNION_ANONYME,
    DISCR_DEFAUT,
    DISCR_ENUM,
};

template <int N>
NoeudExpression *Simplificatrice::simplifie_discr_impl(NoeudDiscr *discr)
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

    static const Lexème lexème_ou = {",", {}, GenreLexème::BARRE_BARRE, 0, 0, 0};

    auto la_discriminée = discr->expression_discriminée;
    simplifie(la_discriminée);

    auto decl_variable = crée_déclaration_variable(
        la_discriminée->lexème, la_discriminée->type, la_discriminée);
    decl_variable->drapeaux |= DrapeauxNoeud::EST_UTILISEE;

    ajoute_expression(decl_variable);

    auto ref_decl = assem->crée_référence_déclaration(decl_variable->lexème, decl_variable);

    NoeudExpression *expression = ref_decl;

    if (N == DISCR_UNION || N == DISCR_UNION_ANONYME) {
        /* La discrimination se fait via le membre actif. Il faudra proprement gérer les unions
         * dans la RI. */
        expression = assem->crée_référence_membre(
            expression->lexème, expression, TypeBase::Z32, 1);
    }

    simplifie(discr->bloc_sinon);

    /* Nous avons une discrimination avec seulement un bloc_sinon, il est donc inutile de généré un
     * arbre. */
    if (discr->paires_discr.taille() == 0) {
        discr->substitution = discr->bloc_sinon;
        return discr->substitution;
    }

    /* Génération de l'arbre de « si ». */
    auto si_courant = assem->crée_si(discr->lexème, GenreNoeud::INSTRUCTION_SI);
    discr->substitution = si_courant;

    for (auto i = 0; i < discr->paires_discr.taille(); ++i) {
        auto &it = discr->paires_discr[i];
        auto virgule = it->expression->comme_virgule();

        /* Remplace l'expression de la variable capturée par une référence vers la variable
         * temporaire. Sinon, nous réévaluerons l'expression, ce qui en cas d'un appel créérait
         * deux appels différents. */
        if (it->variable_capturée) {
            auto init_var = it->variable_capturée->comme_déclaration_variable()->expression;
            init_var->comme_comme()->expression = ref_decl;
        }

        /* Création des comparaisons. Les expressions sont comparées avec la variable discriminée,
         * les virgules remplacées par des « || ». */
        kuri::tableau<NoeudExpressionBinaire> comparaisons;

        for (auto expr : virgule->expressions) {
            auto comparaison = NoeudExpressionBinaire();
            comparaison.lexème = discr->lexème;
            comparaison.op = discr->op;
            comparaison.opérande_gauche = expression;

            if (N == DISCR_ENUM) {
                auto valeur = valeur_énum(static_cast<TypeEnum *>(expression->type), expr->ident);
                auto constante = assem->crée_littérale_entier(
                    expr->lexème, expression->type, static_cast<uint64_t>(valeur));
                comparaison.opérande_droite = constante;
            }
            else if (N == DISCR_UNION) {
                auto const type_union = discr->expression_discriminée->type->comme_type_union();
                auto index = donne_membre_pour_nom(type_union, expr->ident)->index_membre;
                auto constante = assem->crée_littérale_entier(
                    expr->lexème, expression->type, static_cast<uint64_t>(index + 1));
                comparaison.opérande_droite = constante;
            }
            else if (N == DISCR_UNION_ANONYME) {
                auto const type_union = discr->expression_discriminée->type->comme_type_union();
                auto index = donne_membre_pour_nom(type_union, expr->ident)->index_membre;
                auto constante = assem->crée_littérale_entier(
                    expr->lexème, expression->type, static_cast<uint64_t>(index + 1));
                comparaison.opérande_droite = constante;
            }
            else {
                /* Cette expression est simplifiée via crée_expression_pour_op_chainee. */
                comparaison.opérande_droite = expr;
            }

            comparaisons.ajoute(comparaison);
        }

        si_courant->condition = crée_expression_pour_op_chainée(comparaisons, &lexème_ou);

        simplifie(it->bloc);
        si_courant->bloc_si_vrai = it->bloc;

        if (i != (discr->paires_discr.taille() - 1)) {
            auto si = assem->crée_si(discr->lexème, GenreNoeud::INSTRUCTION_SI);
            si_courant->bloc_si_faux = si;
            si_courant = si;
        }
    }

    /* Évitons d'ajouter un bloc vide, pour ne pas faire du travail inutile dans les étapes
     * suivantes. */
    if (discr->bloc_sinon && !discr->bloc_sinon->expressions->est_vide()) {
        si_courant->bloc_si_faux = discr->bloc_sinon;
    }

    return discr->substitution;
}

NoeudExpression *Simplificatrice::simplifie_discr(NoeudDiscr *discr)
{
    if (discr->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
        auto const type_union = discr->expression_discriminée->type->comme_type_union();

        if (type_union->est_anonyme) {
            return simplifie_discr_impl<DISCR_UNION_ANONYME>(discr);
        }

        return simplifie_discr_impl<DISCR_UNION>(discr);
    }

    if (discr->genre == GenreNoeud::INSTRUCTION_DISCR_ÉNUM) {
        return simplifie_discr_impl<DISCR_ENUM>(discr);
    }

    return simplifie_discr_impl<DISCR_DEFAUT>(discr);
}

NoeudSi *Simplificatrice::crée_condition_boucle(NoeudExpression *inst, GenreNoeud genre_noeud)
{
    static const Lexème lexème_arrête = {",", {}, GenreLexème::ARRÊTE, 0, 0, 0};

    /* condition d'arrêt de la boucle */
    auto condition = assem->crée_si(inst->lexème, genre_noeud);
    auto bloc_si_vrai = assem->crée_bloc_seul(inst->lexème, inst->bloc_parent);

    auto arrête = assem->crée_arrête(&lexème_arrête, nullptr);
    arrête->drapeaux |= DrapeauxNoeud::EST_IMPLICITE;
    arrête->boucle_controlée = inst;
    arrête->bloc_parent = bloc_si_vrai;

    bloc_si_vrai->ajoute_expression(arrête);
    condition->bloc_si_vrai = bloc_si_vrai;

    return condition;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Point d'entrée pour la canonicalisation.
 * \{ */

void simplifie_arbre(EspaceDeTravail *espace,
                     AssembleuseArbre *assem,
                     Typeuse &typeuse,
                     NoeudExpression *arbre)
{
    assert_rappel(!arbre->possède_drapeau(DrapeauxNoeud::FUT_SIMPLIFIÉ),
                  [&]() { dbg() << nom_humainement_lisible(arbre); });
    auto simplificatrice = Simplificatrice(espace, assem, typeuse);
    assert(assem->bloc_courant() == nullptr);
    simplificatrice.simplifie(arbre);
    assert(assem->bloc_courant() == nullptr);
    arbre->drapeaux |= DrapeauxNoeud::FUT_SIMPLIFIÉ;
}

/** \} */
