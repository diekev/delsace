/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "canonicalisation.hh"

#include <iostream>

#include "compilation/compilatrice.hh"
#include "compilation/espace_de_travail.hh"
#include "compilation/typage.hh"

#include "parsage/outils_lexemes.hh"

#include "assembleuse.hh"
#include "noeud_expression.hh"
#include "utilitaires.hh"

/* ------------------------------------------------------------------------- */
/** \name Utilitaires locaux.
 * \{ */

/* Noeud global pour les expressions de non-initialisation, utilisé afin d'économiser un peu de
 * mémoire. */
static NoeudExpressionNonIntialisation non_initialisation{};

static NoeudExpression *crée_référence_pour_membre_employé(AssembleuseArbre *assem,
                                                           Lexeme const *lexeme,
                                                           NoeudExpression *expression_accédée,
                                                           TypeCompose *type_composé,
                                                           MembreTypeComposé const &membre);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Canonicalisation.
 * \{ */

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
        case GenreNoeud::DIRECTIVE_PRE_EXECUTABLE:
        case GenreNoeud::DIRECTIVE_CORPS_BOUCLE:
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

            if (entete->possede_drapeau(DrapeauxNoeudFonction::EST_EXTERNE)) {
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
        case GenreNoeud::DECLARATION_OPERATEUR_POUR:
        {
            auto operateur_pour = noeud->comme_operateur_pour();
            fonction_courante = operateur_pour;
            simplifie(operateur_pour->corps);
            return;
        }
        case GenreNoeud::DECLARATION_CORPS_FONCTION:
        {
            auto corps = noeud->comme_corps_fonction();

            auto fut_dans_fonction = m_dans_fonction;
            m_dans_fonction = true;
            simplifie(corps->bloc);
            m_dans_fonction = fut_dans_fonction;

            if (corps->aide_generation_code == REQUIERS_CODE_EXTRA_RETOUR) {
                auto retourne = assem->cree_retourne(corps->lexeme);
                retourne->bloc_parent = corps->bloc;
                corps->bloc->ajoute_expression(retourne);
            }
            else if (corps->aide_generation_code == REQUIERS_RETOUR_UNION_VIA_RIEN) {
                cree_retourne_union_via_rien(corps->entete, corps->bloc, corps->lexeme);
            }

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

            if (expr_bin->type->est_type_type_de_donnees()) {
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
                if (type1->est_type_pointeur() && type2->est_type_pointeur()) {
                    auto const &type_z64 = TypeBase::Z64;
                    auto type_pointe = type2->comme_type_pointeur()->type_pointe;
                    auto soustraction = assem->cree_expression_binaire(
                        expr_bin->lexeme,
                        type_z64->table_opérateurs->operateur_sst,
                        comme_type(expr_bin->operande_gauche, type_z64),
                        comme_type(expr_bin->operande_droite, type_z64));
                    auto taille_de = assem->cree_litterale_entier(
                        expr_bin->lexeme, type_z64, std::max(type_pointe->taille_octet, 1u));
                    auto div = assem->cree_expression_binaire(
                        expr_bin->lexeme,
                        type_z64->table_opérateurs->operateur_div,
                        soustraction,
                        taille_de);
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

                    auto type_pointe = type_pointeur->comme_type_pointeur()->type_pointe;

                    auto taille_de = assem->cree_litterale_entier(
                        expr_entier->lexeme, type_entier, std::max(type_pointe->taille_octet, 1u));
                    auto mul = assem->cree_expression_binaire(
                        expr_entier->lexeme,
                        type_entier->table_opérateurs->operateur_mul,
                        expr_entier,
                        taille_de);

                    OperateurBinaire *op_arithm = nullptr;

                    if (expr_bin->lexeme->genre == GenreLexeme::MOINS ||
                        expr_bin->lexeme->genre == GenreLexeme::MOINS_EGAL) {
                        op_arithm = type_entier->table_opérateurs->operateur_sst;
                    }
                    else if (expr_bin->lexeme->genre == GenreLexeme::PLUS ||
                             expr_bin->lexeme->genre == GenreLexeme::PLUS_EGAL) {
                        op_arithm = type_entier->table_opérateurs->operateur_ajt;
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

                if (expr_bin->possede_drapeau(DrapeauxNoeud::EST_ASSIGNATION_COMPOSEE)) {
                    expr_bin->substitution = assem->cree_assignation_variable(
                        expr_bin->lexeme, expr_bin->operande_gauche, expr_bin->substitution);
                }

                return;
            }

            if (expr_bin->possede_drapeau(DrapeauxNoeud::EST_ASSIGNATION_COMPOSEE)) {
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

            if (expr_un->type->est_type_type_de_donnees()) {
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

            if (decl_ref->possede_drapeau(DrapeauxNoeud::EST_CONSTANTE)) {
                auto decl_const = decl_ref->comme_declaration_variable();

                if (decl_ref->type->est_type_type_de_donnees()) {
                    expr_ref->substitution = assem->cree_reference_type(
                        expr_ref->lexeme, typeuse.type_type_de_donnees(decl_ref->type));
                    return;
                }

                if (decl_ref->type->est_type_reel()) {
                    expr_ref->substitution = assem->cree_litterale_reel(
                        expr_ref->lexeme, decl_ref->type, decl_const->valeur_expression.reelle());
                    return;
                }

                if (decl_ref->type->est_type_bool()) {
                    expr_ref->substitution = assem->cree_litterale_bool(
                        expr_ref->lexeme,
                        decl_ref->type,
                        decl_const->valeur_expression.booleenne());
                    return;
                }

                if (est_type_entier(decl_ref->type) ||
                    decl_ref->type->est_type_entier_constant() ||
                    decl_ref->type->est_type_enum() || decl_ref->type->est_type_erreur()) {
                    expr_ref->substitution = assem->cree_litterale_entier(
                        expr_ref->lexeme,
                        decl_ref->type,
                        static_cast<uint64_t>(decl_const->valeur_expression.entiere()));
                    return;
                }

                if (decl_ref->type->est_type_chaine()) {
                    expr_ref->substitution = decl_const->expression;
                    return;
                }

                if (decl_ref->type->est_type_tableau_fixe()) {
                    expr_ref->substitution = decl_const->expression;
                }

                return;
            }

            if (decl_ref->est_declaration_type()) {
                expr_ref->substitution = assem->cree_reference_type(
                    expr_ref->lexeme, typeuse.type_type_de_donnees(decl_ref->type));
                return;
            }

            if (decl_ref->est_declaration_variable()) {
                auto declaration_variable = decl_ref->comme_declaration_variable();
                if (declaration_variable->declaration_vient_d_un_emploi) {
                    /* Transforme en un accès de membre. */
                    auto ref_decl_var = assem->cree_reference_declaration(
                        expr_ref->lexeme, declaration_variable->declaration_vient_d_un_emploi);

                    auto type = ref_decl_var->type;
                    while (type->est_type_pointeur() || type->est_type_reference()) {
                        type = type_dereference_pour(type);
                    }

                    auto type_composé = type->comme_type_compose();
                    auto membre =
                        type_composé->membres[declaration_variable->index_membre_employe];

                    if (membre.possède_drapeau(MembreTypeComposé::PROVIENT_D_UN_EMPOI)) {
                        auto accès_membre = crée_référence_pour_membre_employé(
                            assem, expr_ref->lexeme, ref_decl_var, type_composé, membre);
                        expr_ref->substitution = accès_membre;
                    }
                    else {
                        auto accès_membre = assem->cree_reference_membre(
                            expr_ref->lexeme,
                            ref_decl_var,
                            expr_ref->type,
                            declaration_variable->index_membre_employe);
                        expr_ref->substitution = accès_membre;
                    }
                }
            }

            return;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
        {
            simplifie_référence_membre(noeud->comme_reference_membre());
            return;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
        {
            auto ref_membre_union = noeud->comme_reference_membre_union();
            simplifie(ref_membre_union->accedee);
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

            if (expr->type->est_type_entier_constant() &&
                inst->transformation.type == TypeTransformation::ENTIER_VERS_POINTEUR) {
                expr->type = TypeBase::Z64;
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
            nouveau_bloc->ajoute_expression(boucle->bloc);
            nouveau_bloc->ajoute_expression(condition);

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
            nouveau_bloc->ajoute_expression(condition);
            nouveau_bloc->ajoute_expression(boucle->bloc);

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

                bloc->ajoute_membre(decl_temp);
                bloc->ajoute_expression(decl_temp);
                bloc->ajoute_expression(nouveau_si);
                bloc->ajoute_expression(ref_temp);

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
            simplifie_construction_structure(noeud->comme_construction_structure());
            return;
        }
        case GenreNoeud::EXPRESSION_APPEL:
        {
            auto appel = noeud->comme_appel();

            auto ancien_site_pour_position_code_source = m_site_pour_position_code_source;
            m_site_pour_position_code_source = appel;

            if (appel->aide_generation_code == CONSTRUIT_OPAQUE) {
                simplifie(appel->parametres_resolus[0]);

                auto comme = assem->cree_comme(appel->lexeme);
                comme->type = appel->type;
                comme->expression = appel->parametres_resolus[0];
                comme->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                         appel->type};
                comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;

                appel->substitution = comme;
                m_site_pour_position_code_source = ancien_site_pour_position_code_source;
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

            m_site_pour_position_code_source = ancien_site_pour_position_code_source;
            return;
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
        {
            auto assignation = noeud->comme_assignation_variable();

            simplifie(assignation->variable);

            POUR (assignation->donnees_exprs.plage()) {
                auto expression_fut_simplifiee = false;

                for (auto var : it.variables.plage()) {
                    if (var->possede_drapeau(DrapeauxNoeud::ACCES_EST_ENUM_DRAPEAU)) {
                        /* NOTE : pour le moment nous ne pouvons déclarer de nouvelle variables ici
                         * pour les valeurs temporaires, et puisque nous ne pouvons pas utiliser
                         * l'expression dans sa substitution, nous modifions l'expression
                         * directement. Ceci est plus ou moins correcte, puisque donnees_expr n'est
                         * censé être que pour la génération de code.
                         */
                        it.expression = simplifie_assignation_enum_drapeau(var, it.expression);
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

            auto bloc_substitution = assem->cree_bloc_seul(pousse_contexte->lexeme,
                                                           pousse_contexte->bloc_parent);

            auto contexte_courant = espace->compilatrice().globale_contexte_programme;
            auto ref_contexte_courant = assem->cree_reference_declaration(pousse_contexte->lexeme,
                                                                          contexte_courant);

            // sauvegarde_contexte := __contexte_fil_principal
            auto sauvegarde_contexte = assem->cree_declaration_variable(
                pousse_contexte->lexeme, contexte_courant->type, nullptr, ref_contexte_courant);
            auto ref_sauvegarde_contexte = assem->cree_reference_declaration(
                pousse_contexte->lexeme, sauvegarde_contexte);
            bloc_substitution->ajoute_membre(sauvegarde_contexte);
            bloc_substitution->ajoute_expression(sauvegarde_contexte);

            // __contexte_fil_principal = expr
            auto permute_contexte = assem->cree_assignation_variable(
                pousse_contexte->lexeme, ref_contexte_courant, pousse_contexte->expression);
            bloc_substitution->ajoute_expression(permute_contexte);

            /* Il est possible qu'une instruction de retour se trouve dans le bloc, donc nous
             * devons différer la restauration du contexte :
             *
             * diffère __contexte_fil_principal = sauvegarde_contexte
             */
            auto differe = assem->cree_differe(pousse_contexte->lexeme);
            differe->bloc_parent = bloc_substitution;
            differe->expression = assem->cree_assignation_variable(
                pousse_contexte->lexeme, ref_contexte_courant, ref_sauvegarde_contexte);
            bloc_substitution->ajoute_expression(differe);

            /* À FAIRE : surécrire le bloc_parent d'un bloc avec un bloc de substitution peut avoir
             * des conséquences incertaines mais nous avons du bloc de substitution dans la liste
             * des ancêtres du bloc afin que l'instruction diffère soit gérée dans la RI. */
            pousse_contexte->bloc->bloc_parent = bloc_substitution;

            /* Finalement ajoute le code du bloc, après l'instruction de différation. */
            bloc_substitution->ajoute_expression(pousse_contexte->bloc);

            pousse_contexte->substitution = bloc_substitution;
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
            auto structure = noeud->comme_type_structure();
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
        case GenreNoeud::EXPANSION_VARIADIQUE:
        {
            auto expr = noeud->comme_expansion_variadique();
            if (expr->type->est_type_type_de_donnees()) {
                /* Nous avons un type variadique. */
                expr->substitution = assem->cree_reference_type(expr->lexeme, expr->type);
            }
            return;
        }
        case GenreNoeud::INSTRUCTION_EMPL:
        {
            if (!m_dans_fonction) {
                /* Ne simplifie que les expressions empl dans le corps de fonctions. */
                return;
            }

            auto expr_empl = noeud->comme_empl();
            simplifie(expr_empl->expression);
            /* empl n'est pas géré dans la RI. */
            expr_empl->substitution = expr_empl->expression;
            return;
        }
        case GenreNoeud::DIRECTIVE_EXECUTE:
        case GenreNoeud::DECLARATION_ENUM:
        case GenreNoeud::DECLARATION_OPAQUE:
        case GenreNoeud::EXPRESSION_INFO_DE:
        case GenreNoeud::EXPRESSION_INIT_DE:
        case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
        case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
        case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
        case GenreNoeud::EXPRESSION_LITTERALE_NUL:
        case GenreNoeud::EXPRESSION_MEMOIRE:
        case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
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

void Simplificatrice::simplifie_boucle_pour(NoeudPour *inst)
{
    simplifie(inst->expression);
    simplifie(inst->bloc);
    simplifie(inst->bloc_sansarret);
    simplifie(inst->bloc_sinon);

    if (inst->aide_generation_code == BOUCLE_POUR_OPÉRATEUR) {
        simplifie_boucle_pour_opérateur(inst);
        return;
    }

    auto it = inst->decl_it;
    auto index_it = inst->decl_index_it;
    auto expression_iteree = inst->expression;
    auto bloc = inst->bloc;
    auto bloc_sans_arret = inst->bloc_sansarret;
    auto bloc_sinon = inst->bloc_sinon;

    auto boucle = assem->cree_boucle(inst->lexeme);
    boucle->ident = it->ident;
    boucle->bloc_parent = inst->bloc_parent;
    boucle->bloc = assem->cree_bloc_seul(inst->lexeme, boucle->bloc_parent);
    boucle->bloc_sansarret = bloc_sans_arret;
    boucle->bloc_sinon = bloc_sinon;

    auto type_index_it = index_it->type;
    auto zero = assem->cree_litterale_entier(index_it->lexeme, type_index_it, 0);

    auto ref_it = it->valeur->comme_reference_declaration();
    auto ref_index = index_it->valeur->comme_reference_declaration();

    auto bloc_pre = assem->cree_bloc_seul(nullptr, boucle->bloc_parent);
    bloc_pre->ajoute_membre(it);
    bloc_pre->ajoute_membre(index_it);

    bloc_pre->ajoute_expression(it);
    bloc_pre->ajoute_expression(index_it);

    auto bloc_inc = assem->cree_bloc_seul(nullptr, boucle->bloc_parent);

    auto condition = cree_condition_boucle(boucle, GenreNoeud::INSTRUCTION_SI);

    boucle->bloc_pre = bloc_pre;
    boucle->bloc_inc = bloc_inc;

    auto const inverse_boucle = inst->lexeme_op == GenreLexeme::SUPERIEUR;

    auto type_itere = expression_iteree->type->est_type_opaque() ?
                          expression_iteree->type->comme_type_opaque()->type_opacifie :
                          expression_iteree->type;
    expression_iteree->type = type_itere;

    /* boucle */

    switch (inst->aide_generation_code) {
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

            if (inst->aide_generation_code == GENERE_BOUCLE_PLAGE_IMPLICITE) {
                // 0 ... expr - 1
                expr_debut = assem->cree_litterale_entier(
                    expression_iteree->lexeme, expression_iteree->type, 0);

                auto valeur_un = assem->cree_litterale_entier(
                    expression_iteree->lexeme, expression_iteree->type, 1);
                expr_fin = assem->cree_expression_binaire(
                    expression_iteree->lexeme,
                    expression_iteree->type->table_opérateurs->operateur_sst,
                    expression_iteree,
                    valeur_un);
            }
            else {
                auto expr_plage = expression_iteree->comme_plage();
                expr_debut = expr_plage->debut;
                expr_fin = expr_plage->fin;
            }

            /* Calcul le nombre d'itérations pour se prémunir des débordements pour les types
             * d'entiers naturels.
             * Nombre d'itérations = (fin - début) + 1
             */
            NoeudExpression *nombre_iterations = assem->cree_expression_binaire(
                expression_iteree->lexeme,
                expression_iteree->type->table_opérateurs->operateur_sst,
                expr_fin,
                expr_debut);

            auto valeur_un = assem->cree_litterale_entier(
                expression_iteree->lexeme, expression_iteree->type, 1);
            nombre_iterations = assem->cree_expression_binaire(
                expression_iteree->lexeme,
                expression_iteree->type->table_opérateurs->operateur_ajt,
                nombre_iterations,
                valeur_un);

            /* condition */
            if (inverse_boucle) {
                std::swap(expr_debut, expr_fin);
            }

            auto init_it = assem->cree_assignation_variable(ref_it->lexeme, ref_it, expr_debut);
            bloc_pre->ajoute_expression(init_it);

            auto op_comp = index_it->type->table_opérateurs->operateur_seg;
            condition->condition = assem->cree_expression_binaire(
                inst->lexeme, op_comp, ref_index, nombre_iterations);
            boucle->bloc->ajoute_expression(condition);

            /* corps */
            boucle->bloc->ajoute_expression(bloc);

            /* suivant */
            if (inverse_boucle) {
                auto inc_it = assem->cree_decrementation(ref_it->lexeme, ref_it);
                bloc_inc->ajoute_expression(inc_it);
            }
            else {
                auto inc_it = assem->cree_incrementation(ref_it->lexeme, ref_it);
                bloc_inc->ajoute_expression(inc_it);
            }

            auto inc_it = assem->cree_incrementation(ref_index->lexeme, ref_index);
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

            if (type_itere->est_type_tableau_fixe()) {
                auto taille_tableau = type_itere->comme_type_tableau_fixe()->taille;
                expr_taille = assem->cree_litterale_entier(
                    inst->lexeme, TypeBase::Z64, static_cast<uint64_t>(taille_tableau));
            }
            else {
                expr_taille = assem->cree_reference_membre(
                    inst->lexeme, expression_iteree, TypeBase::Z64, 1);
            }

            auto type_z64 = TypeBase::Z64;
            condition->condition = assem->cree_expression_binaire(
                inst->lexeme, type_z64->table_opérateurs->operateur_seg, ref_index, expr_taille);

            auto expr_pointeur = NoeudExpression::nul();

            auto type_compose = type_itere->comme_type_compose();

            if (type_itere->est_type_tableau_fixe()) {
                auto indexage = cree_indexage(inst->lexeme, expression_iteree, zero);

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
                    inst->lexeme,
                    ref_index->type->table_opérateurs->operateur_sst,
                    expr_taille,
                    ref_index);
                expr_index = assem->cree_expression_binaire(
                    inst->lexeme,
                    ref_index->type->table_opérateurs->operateur_sst,
                    expr_index,
                    assem->cree_litterale_entier(ref_index->lexeme, ref_index->type, 1));
            }

            auto indexage = cree_indexage(inst->lexeme, expr_pointeur, expr_index);
            NoeudExpression *expression_assignee = indexage;

            if (inst->prend_reference || inst->prend_pointeur) {
                auto noeud_comme = assem->cree_comme(it->lexeme);
                noeud_comme->type = it->type;
                noeud_comme->expression = indexage;
                noeud_comme->transformation = TypeTransformation::PREND_REFERENCE;
                noeud_comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;

                expression_assignee = noeud_comme;
            }

            auto assign_it = assem->cree_assignation_variable(
                inst->lexeme, ref_it, expression_assignee);

            boucle->bloc->ajoute_expression(condition);
            boucle->bloc->ajoute_expression(assign_it);

            /* corps */
            boucle->bloc->ajoute_expression(bloc);

            /* incrémente */
            auto inc_it = assem->cree_incrementation(ref_index->lexeme, ref_index);
            bloc_inc->ajoute_expression(inc_it);
            break;
        }
        case GENERE_BOUCLE_COROUTINE:
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

            auto feuilles = kuri::tablet<NoeudExpression *, 10>{};
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
        case BOUCLE_POUR_OPÉRATEUR:
        {
            assert(false);
            break;
        }
    }

    inst->substitution = boucle;
}

void Simplificatrice::simplifie_boucle_pour_opérateur(NoeudPour *inst)
{
    simplifie(inst->corps_operateur_pour);
    auto corps_opérateur_pour = inst->corps_operateur_pour;

    auto bloc_substitution = assem->cree_bloc_seul(corps_opérateur_pour->bloc->lexeme,
                                                   inst->bloc_parent);

    /* Crée une variable temporaire pour l'expression itérée. Si l'expression est par exemple un
     * appel, il sera toujours évalué, menant potentiellement à une boucle infinie. */
    auto temporaire = assem->cree_declaration_variable(
        inst->expression->lexeme, inst->expression->type, nullptr, inst->expression);
    auto ref_temporaire = assem->cree_reference_declaration(temporaire->lexeme, temporaire);
    bloc_substitution->ajoute_expression(temporaire);

    /* Ajoute les déclarations des variables d'itération dans le corps du bloc pour que la RI les
     * trouve avant de générer le code des références. */
    bloc_substitution->ajoute_expression(inst->decl_it);
    bloc_substitution->ajoute_expression(inst->decl_index_it);
    bloc_substitution->ajoute_expression(corps_opérateur_pour->bloc);

    /* Substitutions manuelles. */
    auto entête = corps_opérateur_pour->entete;
    auto param = entête->parametre_entree(0);

    POUR (corps_opérateur_pour->arbre_aplatis) {
        /* Substitue le paramètre par la variable. */
        if (it->est_reference_declaration()) {
            auto référence = it->comme_reference_declaration();
            if (référence->declaration_referee != param) {
                continue;
            }

            référence->substitution = ref_temporaire;
            continue;
        }

        /* Substitue #corps_boucle par le bloc. */
        if (it->est_directive_corps_boucle()) {
            it->substitution = inst->bloc;
            /* Le nouveau bloc parent du bloc originel de la boucle doit être le bloc parent de
             * l'instruction qu'il remplace pour que les instructions « diffère » fonctionnent
             * proprement. */
            inst->bloc->bloc_parent = it->bloc_parent;
            continue;
        }
    }

    inst->substitution = bloc_substitution;
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
    kuri::pile<NoeudExpression *> exprs;

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

        auto di = bloc->expressions->derniere();
        di = assem->cree_assignation_variable(di->lexeme, ref_temp, di);
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

void Simplificatrice::cree_retourne_union_via_rien(NoeudDeclarationEnteteFonction *entete,
                                                   NoeudBloc *bloc_d_insertion,
                                                   Lexeme const *lexeme_reference)
{
    auto type_sortie = entete->type->comme_type_fonction()->type_sortie->comme_type_union();
    auto retourne = assem->cree_retourne(lexeme_reference);
    retourne->bloc_parent = bloc_d_insertion;

    auto param_sortie = entete->param_sortie;
    auto ref_param_sortie = assem->cree_reference_declaration(lexeme_reference, param_sortie);

    auto type_union = type_sortie;

    auto info_membre = donne_membre_pour_type(type_union, TypeBase::RIEN);
    assert(info_membre.has_value());
    auto index_membre = uint32_t(info_membre->index_membre);

    auto ref_membre = assem->cree_reference_membre(
        lexeme_reference, ref_param_sortie, TypeBase::Z32, 1);
    auto valeur_index = assem->cree_litterale_entier(
        lexeme_reference, TypeBase::Z32, index_membre + 1);

    auto assignation = assem->cree_assignation_variable(
        lexeme_reference, ref_membre, valeur_index);

    retourne->expression = ref_param_sortie;

    bloc_d_insertion->ajoute_expression(assignation);
    bloc_d_insertion->ajoute_expression(retourne);
}

/* Les retours sont simplifiés sous forme d'assignations des valeurs de retours,
 * et d'un chargement pour les retours simples. */
void Simplificatrice::simplifie_retour(NoeudRetour *inst)
{
    // crée une assignation pour chaque sortie
    auto type_fonction = fonction_courante->type->comme_type_fonction();
    auto type_sortie = type_fonction->type_sortie;

    if (type_sortie->est_type_rien()) {
        return;
    }

    if (inst->aide_generation_code == RETOURNE_UNE_UNION_VIA_RIEN) {
        auto bloc = assem->cree_bloc_seul(inst->lexeme, inst->bloc_parent);
        cree_retourne_union_via_rien(fonction_courante, bloc, inst->lexeme);
        inst->substitution = bloc;
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

    if (type_sortie->est_type_rien()) {
        retour->expression = nullptr;
    }
    else {
        retour->expression = assem->cree_reference_declaration(
            fonction_courante->param_sortie->lexeme, fonction_courante->param_sortie);
    }

    auto bloc = assem->cree_bloc_seul(inst->lexeme, inst->bloc_parent);
    bloc->ajoute_expression(assignation);
    bloc->ajoute_expression(retour);
    retour->bloc_parent = bloc;

    inst->substitution = bloc;
    return;
}

void Simplificatrice::simplifie_construction_structure(
    NoeudExpressionConstructionStructure *construction)
{
    POUR (construction->parametres_resolus) {
        simplifie(it);
    }

    if (construction->type->est_type_union()) {
        simplifie_construction_union(construction);
        return;
    }

    /* L'expression peut être nulle pour les structures anonymes crées par la compilatrice. */
    if (construction->expression && construction->expression->ident == ID::PositionCodeSource) {
        simplifie_construction_structure_position_code_source(construction);
        return;
    }

    simplifie_construction_structure_impl(construction);
}

NoeudExpressionAppel *Simplificatrice::crée_appel_fonction_init(
    Lexeme const *lexeme, NoeudExpression *expression_à_initialiser)
{
    auto type_expression = expression_à_initialiser->type;
    auto fonction_init = cree_entete_pour_initialisation_type(
        type_expression, espace->compilatrice(), assem, typeuse);

    static Lexeme lexeme_op = {};
    lexeme_op.genre = GenreLexeme::FOIS_UNAIRE;
    auto prise_adresse = assem->cree_expression_unaire(&lexeme_op);
    prise_adresse->operande = expression_à_initialiser;
    prise_adresse->type = typeuse.type_pointeur_pour(type_expression);
    auto appel = assem->cree_appel(lexeme, fonction_init, TypeBase::RIEN);
    appel->parametres_resolus.ajoute(prise_adresse);

    return appel;
}

void Simplificatrice::simplifie_construction_union(
    NoeudExpressionConstructionStructure *construction)
{
    auto const site = construction;
    auto const lexeme = construction->lexeme;
    auto type_union = construction->type->comme_type_union();

    if (construction->parametres_resolus.est_vide()) {
        /* Initialise à zéro. */

        auto decl_position = assem->cree_declaration_variable(
            lexeme, type_union, nullptr, &non_initialisation);
        auto ref_position = decl_position->valeur->comme_reference_declaration();

        auto bloc = assem->cree_bloc_seul(lexeme, site->bloc_parent);
        bloc->ajoute_membre(decl_position);
        bloc->ajoute_expression(decl_position);

        auto appel = crée_appel_fonction_init(lexeme, ref_position);
        bloc->ajoute_expression(appel);

        /* La dernière expression (une simple référence) sera utilisée lors de la génération de RI
         * pour définir la valeur à assigner. */
        bloc->ajoute_expression(ref_position);
        construction->substitution = bloc;
        return;
    }

    auto index_membre = 0u;
    auto expression_initialisation = NoeudExpression::nul();

    POUR (construction->parametres_resolus) {
        if (it != nullptr) {
            expression_initialisation = it;
            break;
        }

        index_membre += 1;
    }

    assert(expression_initialisation);

    /* Nous devons transtyper l'expression, la RI s'occupera d'initialiser le membre implicite en
     * cas d'union sûre. */
    auto comme = assem->cree_comme(lexeme);
    comme->type = type_union;
    comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;
    comme->expression = expression_initialisation;
    comme->transformation = {TypeTransformation::CONSTRUIT_UNION, type_union, index_membre};

    construction->substitution = comme;
}

void Simplificatrice::simplifie_construction_structure_position_code_source(
    NoeudExpressionConstructionStructure *construction)
{
    auto const lexeme = construction->lexeme;
    const NoeudExpression *site = m_site_pour_position_code_source ?
                                      m_site_pour_position_code_source :
                                      construction;
    auto const lexeme_site = site->lexeme;

    auto &compilatrice = espace->compilatrice();

    /* Création des valeurs pour chaque membre. */

    /* PositionCodeSource.fichier
     * À FAIRE : sécurité, n'utilise pas le chemin, mais détermine une manière fiable et robuste
     * d'obtenir le fichier, utiliser simplement le nom n'est pas fiable (d'autres fichiers du même
     * nom dans le module). */
    auto const fichier = compilatrice.fichier(lexeme_site->fichier);
    auto valeur_chemin_fichier = assem->cree_litterale_chaine(lexeme);
    valeur_chemin_fichier->valeur = compilatrice.gerante_chaine->ajoute_chaine(fichier->chemin());

    /* PositionCodeSource.fonction */
    auto nom_fonction = kuri::chaine_statique("");
    if (fonction_courante && fonction_courante->ident) {
        nom_fonction = fonction_courante->ident->nom;
    }

    auto valeur_nom_fonction = assem->cree_litterale_chaine(lexeme);
    valeur_nom_fonction->valeur = compilatrice.gerante_chaine->ajoute_chaine(nom_fonction);

    /* PositionCodeSource.ligne */
    auto pos = position_lexeme(*lexeme_site);
    auto valeur_ligne = assem->cree_litterale_entier(
        lexeme, TypeBase::Z32, static_cast<unsigned>(pos.numero_ligne));

    /* PositionCodeSource.colonne */
    auto valeur_colonne = assem->cree_litterale_entier(
        lexeme, TypeBase::Z32, static_cast<unsigned>(pos.pos));

    /* Création d'une temporaire et assignation des membres. */

    auto const type_position_code_source = typeuse.type_position_code_source;
    auto decl_position = assem->cree_declaration_variable(
        lexeme, type_position_code_source, nullptr, &non_initialisation);
    auto ref_position = decl_position->valeur->comme_reference_declaration();

    auto ref_membre_fichier = assem->cree_reference_membre(
        lexeme, ref_position, TypeBase::CHAINE, 0);
    auto ref_membre_fonction = assem->cree_reference_membre(
        lexeme, ref_position, TypeBase::CHAINE, 1);
    auto ref_membre_ligne = assem->cree_reference_membre(lexeme, ref_position, TypeBase::Z32, 2);
    auto ref_membre_colonne = assem->cree_reference_membre(lexeme, ref_position, TypeBase::Z32, 3);

    NoeudExpression *couples_ref_membre_expression[4][2] = {
        {ref_membre_fichier, valeur_chemin_fichier},
        {ref_membre_fonction, valeur_nom_fonction},
        {ref_membre_ligne, valeur_ligne},
        {ref_membre_colonne, valeur_colonne},
    };

    auto bloc = assem->cree_bloc_seul(lexeme, site->bloc_parent);
    bloc->ajoute_membre(decl_position);
    bloc->ajoute_expression(decl_position);

    for (auto couple : couples_ref_membre_expression) {
        auto assign = assem->cree_assignation_variable(lexeme, couple[0], couple[1]);
        bloc->ajoute_expression(assign);
    }

    /* La dernière expression (une simple référence) sera utilisée lors de la génération de RI pour
     * définir la valeur à assigner. */
    bloc->ajoute_expression(ref_position);
    construction->substitution = bloc;
}

void Simplificatrice::simplifie_construction_structure_impl(
    NoeudExpressionConstructionStructure *construction)
{
    auto const site = construction;
    auto const lexeme = construction->lexeme;
    auto type_struct = construction->type->comme_type_structure();

    auto decl_position = assem->cree_declaration_variable(
        lexeme, type_struct, nullptr, &non_initialisation);
    auto ref_position = decl_position->valeur->comme_reference_declaration();

    auto bloc = assem->cree_bloc_seul(lexeme, site->bloc_parent);
    bloc->ajoute_membre(decl_position);
    bloc->ajoute_expression(decl_position);

    POUR_INDEX (construction->parametres_resolus) {
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
                assem, lexeme, ref_position, type_struct, membre);
            auto assign = assem->cree_assignation_variable(lexeme, ref_membre, it);
            bloc->ajoute_expression(assign);
            continue;
        }

        auto type_membre = membre.type;
        auto ref_membre = assem->cree_reference_membre(
            lexeme, ref_position, type_membre, index_it);

        if (it != nullptr) {
            auto assign = assem->cree_assignation_variable(lexeme, ref_membre, it);
            bloc->ajoute_expression(assign);
        }
        else {
            auto appel = crée_appel_fonction_init(lexeme, ref_membre);
            bloc->ajoute_expression(appel);
        }
    }

    /* La dernière expression (une simple référence) sera utilisée lors de la génération de RI pour
     * définir la valeur à assigner. */
    bloc->ajoute_expression(ref_position);
    construction->substitution = bloc;
}

/**
 * Trouve le membre de \a type_composé ayant ajouté via `empl base: ...` la \a decl_employée et
 * retourne son #InformationMembreTypeCompose. */
static std::optional<InformationMembreTypeCompose> trouve_information_membre_ayant_ajouté_decl(
    TypeCompose *type_composé, NoeudDeclarationSymbole *decl_employée)
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
        auto decl_employée = membre_courant.decl->declaration_vient_d_un_emploi;
        auto info = trouve_information_membre_ayant_ajouté_decl(type_composé_courant,
                                                                decl_employée);
        assert(info.has_value());

        hiérarchie.ajoute(info.value());

        auto type_employé = info->membre.type->comme_type_compose();
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
                                                           Lexeme const *lexeme,
                                                           NoeudExpression *expression_accédée,
                                                           TypeCompose *type_composé,
                                                           MembreTypeComposé const &membre)
{
    auto hiérarchie = trouve_hiérarchie_emploi_membre(type_composé, membre);
    auto ref_membre_courant = expression_accédée;

    POUR (hiérarchie) {
        auto accès_base = assem->cree_reference_membre(
            lexeme, ref_membre_courant, it.membre.type, it.index_membre);

        auto ref_membre_empl = assem->cree_reference_declaration(lexeme, it.membre.decl);
        accès_base->membre = ref_membre_empl;

        ref_membre_courant = accès_base;
    }

    assert(ref_membre_courant != expression_accédée);
    return ref_membre_courant;
}

void Simplificatrice::simplifie_référence_membre(NoeudExpressionMembre *ref_membre)
{
    auto const lexeme = ref_membre->lexeme;
    auto accede = ref_membre->accedee;
    auto type_accede = accede->type;

    if (ref_membre->possede_drapeau(DrapeauxNoeud::ACCES_EST_ENUM_DRAPEAU)) {
        simplifie(accede);

        // a.DRAPEAU => (a & DRAPEAU) != 0
        auto type_enum = static_cast<TypeEnum *>(ref_membre->type);
        auto valeur_enum = type_enum->membres[ref_membre->index_membre].valeur;

        auto valeur_lit_enum = assem->cree_litterale_entier(
            lexeme, type_enum, static_cast<unsigned>(valeur_enum));
        auto op = type_enum->table_opérateurs->operateur_etb;
        auto et = assem->cree_expression_binaire(lexeme, op, accede, valeur_lit_enum);

        auto zero = assem->cree_litterale_entier(lexeme, type_enum, 0);
        op = type_enum->table_opérateurs->operateur_dif;
        auto dif = assem->cree_expression_binaire(lexeme, op, et, zero);

        ref_membre->substitution = dif;
        return;
    }

    if (accede->est_reference_declaration()) {
        if (accede->comme_reference_declaration()->declaration_referee->est_declaration_module()) {
            ref_membre->substitution = ref_membre->membre;
            simplifie(ref_membre->membre);
            return;
        }
    }

    while (type_accede->est_type_pointeur() || type_accede->est_type_reference()) {
        type_accede = type_dereference_pour(type_accede);
    }

    if (type_accede->est_type_opaque()) {
        type_accede = type_accede->comme_type_opaque()->type_opacifie;
    }

    if (type_accede->est_type_tableau_fixe()) {
        auto taille = type_accede->comme_type_tableau_fixe()->taille;
        ref_membre->substitution = assem->cree_litterale_entier(
            lexeme, ref_membre->type, static_cast<uint64_t>(taille));
        return;
    }

    if (type_accede->est_type_enum() || type_accede->est_type_erreur()) {
        auto type_enum = static_cast<TypeEnum *>(type_accede);
        auto valeur_enum = type_enum->membres[ref_membre->index_membre].valeur;
        ref_membre->substitution = assem->cree_litterale_entier(
            lexeme, type_enum, static_cast<unsigned>(valeur_enum));
        return;
    }

    if (type_accede->est_type_type_de_donnees() &&
        ref_membre->genre_valeur == GenreValeur::DROITE) {
        ref_membre->substitution = assem->cree_reference_type(
            lexeme, typeuse.type_type_de_donnees(ref_membre->type));
        return;
    }

    auto type_compose = static_cast<TypeCompose *>(type_accede);
    auto &membre = type_compose->membres[ref_membre->index_membre];

    if (membre.drapeaux == MembreTypeComposé::EST_CONSTANT) {
        simplifie(membre.expression_valeur_defaut);
        ref_membre->substitution = membre.expression_valeur_defaut;
        return;
    }

    if (membre.drapeaux & MembreTypeComposé::PROVIENT_D_UN_EMPOI) {
        /* Transforme x.y en x.base.y. */
        ref_membre->substitution = crée_référence_pour_membre_employé(
            assem, lexeme, ref_membre->accedee, type_compose, membre);
    }

    /* Pour les appels de fonctions ou les accès après des parenthèse (p.e. (x comme *TypeBase).y).
     */
    simplifie(ref_membre->accedee);
}

NoeudExpression *Simplificatrice::simplifie_assignation_enum_drapeau(NoeudExpression *var,
                                                                     NoeudExpression *expression)
{
    auto lexeme = var->lexeme;
    auto ref_membre = var->comme_reference_membre();

    // À FAIRE : référence
    // Nous prenons ref_membre->accedee directement car ce ne sera pas
    // simplifié, et qu'il faut prendre en compte les accés d'accés, les
    // expressions entre parenthèses, etc. Donc faire ceci est plus simple.
    auto nouvelle_ref = ref_membre->accedee;
    var->substitution = nouvelle_ref;

    /* Crée la conjonction d'un drapeau avec la variable (a | DRAPEAU) */
    auto cree_conjonction_drapeau =
        [&](NoeudExpression *ref_variable, TypeEnum *type_enum, unsigned valeur_enum) {
            auto valeur_lit_enum = assem->cree_litterale_entier(lexeme, type_enum, valeur_enum);
            auto op = type_enum->table_opérateurs->operateur_oub;
            return assem->cree_expression_binaire(var->lexeme, op, ref_variable, valeur_lit_enum);
        };

    /* Crée la disjonction d'un drapeau avec la variable (a & ~DRAPEAU) */
    auto cree_disjonction_drapeau =
        [&](NoeudExpression *ref_variable, TypeEnum *type_enum, unsigned valeur_enum) {
            auto valeur_lit_enum = assem->cree_litterale_entier(
                lexeme, type_enum, ~uint64_t(valeur_enum));
            auto op = type_enum->table_opérateurs->operateur_etb;
            return assem->cree_expression_binaire(var->lexeme, op, ref_variable, valeur_lit_enum);
        };

    auto type_enum = static_cast<TypeEnum *>(ref_membre->type);
    auto valeur_enum = type_enum->membres[ref_membre->index_membre].valeur;

    if (expression->est_litterale_bool()) {
        /* Nous avons une expression littérale, donc nous pouvons choisir la bonne instruction. */
        if (expression->comme_litterale_bool()->valeur) {
            // a.DRAPEAU = vrai -> a = a | DRAPEAU
            return cree_conjonction_drapeau(
                nouvelle_ref, type_enum, static_cast<unsigned>(valeur_enum));
        }
        // a.DRAPEAU = faux -> a = a & ~DRAPEAU
        return cree_disjonction_drapeau(
            nouvelle_ref, type_enum, static_cast<unsigned>(valeur_enum));
    }
    /* Transforme en une expression « ternaire » sans branche (similaire à a = b ? c : d en
     * C/C++) :
     * v1 = (a | DRAPEAU)
     * v2 = (a & ~DRAPEAU)
     * (-(b comme T) & v1) | (((b comme T) - 1) & v2)
     */

    auto v1 = cree_conjonction_drapeau(
        nouvelle_ref, type_enum, static_cast<unsigned>(valeur_enum));
    auto v2 = cree_disjonction_drapeau(
        nouvelle_ref, type_enum, static_cast<unsigned>(valeur_enum));

    /* Crée une expression pour convertir l'expression en une valeur du type sous-jacent de
     * l'énumération. */
    auto type_sous_jacent = type_enum->type_sous_jacent;

    simplifie(expression);
    auto ref_b = expression->substitution ? expression->substitution : expression;

    /* Convertis l'expression booléenne vers n8 car ils ont la même taille en octet. */
    auto comme = assem->cree_comme(var->lexeme);
    comme->type = TypeBase::N8;
    comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;
    comme->expression = ref_b;
    comme->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, TypeBase::N8};

    /* Augmente la taille du n8 si ce n'est pas le type sous-jacent de l'énum drapeau. */
    if (type_sous_jacent != TypeBase::N8) {
        auto ancien_comme = comme;
        comme = assem->cree_comme(var->lexeme);
        comme->type = type_sous_jacent;
        comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;
        comme->expression = ancien_comme;
        comme->transformation = {TypeTransformation::AUGMENTE_TAILLE_TYPE, type_sous_jacent};
    }

    /* -b */
    auto zero = assem->cree_litterale_entier(lexeme, type_enum->type_sous_jacent, 0);
    auto moins_b_type_sous_jacent = assem->cree_expression_binaire(
        lexeme, type_sous_jacent->table_opérateurs->operateur_sst, zero, comme);

    /* Convertis vers le type énum pour que la RI soit contente vis-à-vis de la sûreté de
     * type.
     */
    auto moins_b = assem->cree_comme(var->lexeme);
    moins_b->type = type_enum;
    moins_b->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;
    moins_b->expression = moins_b_type_sous_jacent;
    moins_b->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_enum};

    /* b - 1 */
    auto un = assem->cree_litterale_entier(lexeme, type_sous_jacent, 1);
    auto b_moins_un_type_sous_jacent = assem->cree_expression_binaire(
        lexeme, type_sous_jacent->table_opérateurs->operateur_sst, comme, un);

    /* Convertis vers le type énum pour que la RI soit contente vis-à-vis de la sûreté de
     * type.
     */
    auto b_moins_un = assem->cree_comme(var->lexeme);
    b_moins_un->type = type_enum;
    b_moins_un->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;
    b_moins_un->expression = b_moins_un_type_sous_jacent;
    b_moins_un->transformation = TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                                    type_enum};

    /* -b & v1 */
    auto moins_b_et_v1 = assem->cree_expression_binaire(
        lexeme, type_enum->table_opérateurs->operateur_etb, moins_b, v1);
    /* (b - 1) & v2 */
    auto b_moins_un_et_v2 = assem->cree_expression_binaire(
        lexeme, type_enum->table_opérateurs->operateur_etb, b_moins_un, v2);

    /* (-(b comme T) & v1) | (((b comme T) - 1) & v2) */
    return assem->cree_expression_binaire(
        lexeme, type_enum->table_opérateurs->operateur_oub, moins_b_et_v1, b_moins_un_et_v2);
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

    auto feuilles = kuri::tablet<NoeudExpression *, 10>{};
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

    auto la_discriminee = discr->expression_discriminee;
    simplifie(la_discriminee);

    /* Création d'un bloc afin de pouvoir déclarer une variable temporaire qui contiendra la valeur
     * discriminée. */
    auto bloc = assem->cree_bloc_seul(discr->lexeme, discr->bloc_parent);
    discr->substitution = bloc;

    auto decl_variable = assem->cree_declaration_variable(
        la_discriminee->lexeme, la_discriminee->type, nullptr, la_discriminee);

    bloc->ajoute_membre(decl_variable);
    bloc->ajoute_expression(decl_variable);

    auto ref_decl = assem->cree_reference_declaration(decl_variable->lexeme, decl_variable);

    NoeudExpression *expression = ref_decl;

    if (N == DISCR_UNION || N == DISCR_UNION_ANONYME) {
        /* La discrimination se fait via le membre actif. Il faudra proprement gérer les unions
         * dans la RI. */
        expression = assem->cree_reference_membre(
            expression->lexeme, expression, TypeBase::Z32, 1);
    }

    simplifie(discr->bloc_sinon);

    /* Nous avons une discrimination avec seulement un bloc_sinon, il est donc inutile de généré un
     * arbre. */
    if (discr->paires_discr.taille() == 0) {
        bloc->ajoute_expression(discr->bloc_sinon);
        return;
    }

    /* Génération de l'arbre de « si ». */
    auto si_courant = assem->cree_si(discr->lexeme, GenreNoeud::INSTRUCTION_SI);
    bloc->ajoute_expression(si_courant);

    for (auto i = 0; i < discr->paires_discr.taille(); ++i) {
        auto &it = discr->paires_discr[i];
        auto virgule = it->expression->comme_virgule();

        /* Remplace l'expression de la variable capturée par une référence vers la variable
         * temporaire. Sinon, nous réévaluerons l'expression, ce qui en cas d'un appel créérait
         * deux appels différents. */
        if (it->variable_capturee) {
            auto init_var = it->variable_capturee->comme_declaration_variable()->expression;
            init_var->comme_comme()->expression = ref_decl;
        }

        /* Création des comparaisons. Les expressions sont comparées avec la variable discriminée,
         * les virgules remplacées par des « || ». */
        kuri::tableau<NoeudExpressionBinaire> comparaisons;

        for (auto expr : virgule->expressions) {
            auto comparaison = NoeudExpressionBinaire();
            comparaison.lexeme = discr->lexeme;
            comparaison.op = discr->op;
            comparaison.operande_gauche = expression;

            if (N == DISCR_ENUM) {
                auto valeur = valeur_enum(static_cast<TypeEnum *>(expression->type), expr->ident);
                auto constante = assem->cree_litterale_entier(
                    expr->lexeme, expression->type, static_cast<uint64_t>(valeur));
                comparaison.operande_droite = constante;
            }
            else if (N == DISCR_UNION) {
                auto const type_union = discr->expression_discriminee->type->comme_type_union();
                auto index = donne_membre_pour_nom(type_union, expr->ident)->index_membre;
                auto constante = assem->cree_litterale_entier(
                    expr->lexeme, expression->type, static_cast<uint64_t>(index + 1));
                comparaison.operande_droite = constante;
            }
            else if (N == DISCR_UNION_ANONYME) {
                auto const type_union = discr->expression_discriminee->type->comme_type_union();
                auto index = donne_membre_pour_nom(type_union, expr->ident)->index_membre;
                auto constante = assem->cree_litterale_entier(
                    expr->lexeme, expression->type, static_cast<uint64_t>(index + 1));
                comparaison.operande_droite = constante;
            }
            else {
                /* Cette expression est simplifiée via cree_expression_pour_op_chainee. */
                comparaison.operande_droite = expr;
            }

            comparaisons.ajoute(comparaison);
        }

        si_courant->condition = cree_expression_pour_op_chainee(comparaisons, &lexeme_ou);

        simplifie(it->bloc);
        si_courant->bloc_si_vrai = it->bloc;

        if (i != (discr->paires_discr.taille() - 1)) {
            auto si = assem->cree_si(discr->lexeme, GenreNoeud::INSTRUCTION_SI);
            si_courant->bloc_si_faux = si;
            si_courant = si;
        }
    }

    si_courant->bloc_si_faux = discr->bloc_sinon;
}

void Simplificatrice::simplifie_discr(NoeudDiscr *discr)
{
    if (discr->genre == GenreNoeud::INSTRUCTION_DISCR_UNION) {
        auto const type_union = discr->expression_discriminee->type->comme_type_union();

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
    arrete->drapeaux |= DrapeauxNoeud::EST_IMPLICITE;
    arrete->boucle_controlee = inst;
    arrete->bloc_parent = bloc_si_vrai;

    bloc_si_vrai->ajoute_expression(arrete);
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
    auto simplificatrice = Simplificatrice(espace, assem, typeuse);
    assert(assem->bloc_courant() == nullptr);
    simplificatrice.simplifie(arbre);
    assert(assem->bloc_courant() == nullptr);
}

/** \} */
