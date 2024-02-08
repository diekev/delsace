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
        case GenreNoeud::DECLARATION_CONSTANTE:
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
            auto entête = noeud->comme_entete_fonction();

            if (entête->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE)) {
                return;
            }

            if (entête->est_coroutine) {
                simplifie_coroutine(entête);
                return;
            }

            fonction_courante = entête;
            simplifie(entête->corps);
            return;
        }
        case GenreNoeud::DECLARATION_OPERATEUR_POUR:
        {
            auto opérateur_pour = noeud->comme_operateur_pour();
            fonction_courante = opérateur_pour;
            simplifie(opérateur_pour->corps);
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
                auto retourne = assem->crée_retourne(corps->lexeme);
                retourne->bloc_parent = corps->bloc;
                corps->bloc->ajoute_expression(retourne);
            }
            else if (corps->aide_generation_code == REQUIERS_RETOUR_UNION_VIA_RIEN) {
                crée_retourne_union_via_rien(corps->entete, corps->bloc, corps->lexeme);
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
                noeud->substitution = assem->crée_reference_type(expr_bin->lexeme, expr_bin->type);
                return;
            }

            simplifie(expr_bin->operande_gauche);
            simplifie(expr_bin->operande_droite);

            if (expr_bin->op && expr_bin->op->est_arithmétique_pointeur) {
                auto comme_type = [&](NoeudExpression *expr_ptr, Type *type) {
                    auto comme = assem->crée_comme(expr_ptr->lexeme);
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
                    auto soustraction = assem->crée_expression_binaire(
                        expr_bin->lexeme,
                        type_z64->table_opérateurs->opérateur_sst,
                        comme_type(expr_bin->operande_gauche, type_z64),
                        comme_type(expr_bin->operande_droite, type_z64));
                    auto taille_de = assem->crée_litterale_entier(
                        expr_bin->lexeme, type_z64, std::max(type_pointe->taille_octet, 1u));
                    auto div = assem->crée_expression_binaire(
                        expr_bin->lexeme,
                        type_z64->table_opérateurs->opérateur_div,
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

                    auto taille_de = assem->crée_litterale_entier(
                        expr_entier->lexeme, type_entier, std::max(type_pointe->taille_octet, 1u));
                    auto mul = assem->crée_expression_binaire(
                        expr_entier->lexeme,
                        type_entier->table_opérateurs->opérateur_mul,
                        expr_entier,
                        taille_de);

                    OpérateurBinaire *op_arithm = nullptr;

                    if (expr_bin->lexeme->genre == GenreLexème::MOINS ||
                        expr_bin->lexeme->genre == GenreLexème::MOINS_EGAL) {
                        op_arithm = type_entier->table_opérateurs->opérateur_sst;
                    }
                    else if (expr_bin->lexeme->genre == GenreLexème::PLUS ||
                             expr_bin->lexeme->genre == GenreLexème::PLUS_EGAL) {
                        op_arithm = type_entier->table_opérateurs->opérateur_ajt;
                    }

                    auto arithm = assem->crée_expression_binaire(
                        expr_bin->lexeme, op_arithm, comme_type(expr_pointeur, type_entier), mul);

                    auto comme_pointeur = assem->crée_comme(expr_bin->lexeme);
                    comme_pointeur->type = type_pointeur;
                    comme_pointeur->expression = arithm;
                    comme_pointeur->transformation = {TypeTransformation::ENTIER_VERS_POINTEUR,
                                                      type_pointeur};

                    expr_bin->substitution = comme_pointeur;
                }

                if (expr_bin->possède_drapeau(DrapeauxNoeud::EST_ASSIGNATION_COMPOSEE)) {
                    expr_bin->substitution = assem->crée_assignation_variable(
                        expr_bin->lexeme, expr_bin->operande_gauche, expr_bin->substitution);
                }

                return;
            }

            if (expr_bin->possède_drapeau(DrapeauxNoeud::EST_ASSIGNATION_COMPOSEE)) {
                noeud->substitution = assem->crée_assignation_variable(
                    expr_bin->lexeme,
                    expr_bin->operande_gauche,
                    simplifie_opérateur_binaire(expr_bin, true));
                return;
            }

            noeud->substitution = simplifie_opérateur_binaire(expr_bin, false);
            return;
        }
        case GenreNoeud::EXPRESSION_LOGIQUE:
        {
            auto logique = noeud->comme_expression_logique();
            simplifie_expression_logique(logique);
            return;
        }
        case GenreNoeud::OPERATEUR_UNAIRE:
        {
            auto expr_un = noeud->comme_expression_unaire();

            if (expr_un->op && expr_un->op->genre == OpérateurUnaire::Genre::Complement) {
                if (expr_un->operande->est_litterale_entier()) {
                    auto littérale = expr_un->operande->comme_litterale_entier();
                    auto valeur_entière = int64_t(littérale->valeur);
                    valeur_entière = -valeur_entière;
                    littérale->valeur = uint64_t(valeur_entière);
                    expr_un->substitution = littérale;
                    return;
                }
                if (expr_un->operande->est_litterale_reel()) {
                    auto littérale = expr_un->operande->comme_litterale_reel();
                    littérale->valeur = -littérale->valeur;
                    expr_un->substitution = littérale;
                    return;
                }
            }

            simplifie(expr_un->operande);

            /* op peut être nul pour les opérateurs ! et * */
            if (expr_un->op && !expr_un->op->est_basique) {
                auto appel = assem->crée_appel(
                    expr_un->lexeme, expr_un->op->déclaration, expr_un->op->type_résultat);
                appel->parametres_resolus.ajoute(expr_un->operande);
                expr_un->substitution = appel;
                return;
            }

            return;
        }
        case GenreNoeud::EXPRESSION_PRISE_ADRESSE:
        {
            auto prise_adresse = noeud->comme_prise_adresse();
            if (prise_adresse->type->est_type_type_de_donnees()) {
                prise_adresse->substitution = assem->crée_reference_type(prise_adresse->lexeme,
                                                                         prise_adresse->type);
                return;
            }
            simplifie(prise_adresse->opérande);
            return;
        }
        case GenreNoeud::EXPRESSION_PRISE_REFERENCE:
        {
            auto prise_référence = noeud->comme_prise_reference();
            if (prise_référence->type->est_type_type_de_donnees()) {
                prise_référence->substitution = assem->crée_reference_type(prise_référence->lexeme,
                                                                           prise_référence->type);
                return;
            }
            simplifie(prise_référence->opérande);
            return;
        }
        case GenreNoeud::EXPRESSION_NEGATION_LOGIQUE:
        {
            auto négation = noeud->comme_negation_logique();
            simplifie(négation->opérande);
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
            cuisine->substitution = assem->crée_reference_declaration(
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
        case GenreNoeud::INSTRUCTION_SAUFSI_STATIQUE:
        {
            auto inst = noeud->comme_saufsi_statique();

            if (!inst->condition_est_vraie) {
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
            auto référence = noeud->comme_reference_declaration();
            auto déclaration = référence->declaration_referee;

            if (déclaration->est_declaration_constante()) {
                auto decl_const = déclaration->comme_declaration_constante();

                if (déclaration->type->est_type_type_de_donnees()) {
                    référence->substitution = assem->crée_reference_type(
                        référence->lexeme, typeuse.type_type_de_donnees(déclaration->type));
                    return;
                }

                auto type = déclaration->type;
                if (type->est_type_opaque()) {
                    type = type->comme_type_opaque()->type_opacifie;
                }

                if (type->est_type_reel()) {
                    référence->substitution = assem->crée_litterale_reel(
                        référence->lexeme,
                        déclaration->type,
                        decl_const->valeur_expression.réelle());
                    return;
                }

                if (type->est_type_bool()) {
                    référence->substitution = assem->crée_litterale_bool(
                        référence->lexeme,
                        déclaration->type,
                        decl_const->valeur_expression.booléenne());
                    return;
                }

                if (est_type_entier(type) || type->est_type_entier_constant() ||
                    type->est_type_enum() || type->est_type_erreur()) {
                    référence->substitution = assem->crée_litterale_entier(
                        référence->lexeme,
                        déclaration->type,
                        static_cast<uint64_t>(decl_const->valeur_expression.entière()));
                    return;
                }

                /* À FAIRE : test que les opaques fonctionnent ici. */
                if (déclaration->type->est_type_chaine()) {
                    référence->substitution = decl_const->expression;
                    return;
                }

                if (déclaration->type->est_type_tableau_fixe()) {
                    référence->substitution = decl_const->expression;
                    return;
                }

                assert(false);
                return;
            }

            if (déclaration->est_declaration_type()) {
                référence->substitution = assem->crée_reference_type(référence->lexeme,
                                                                     déclaration->type);
                return;
            }

            if (déclaration->est_declaration_variable()) {
                auto declaration_variable = déclaration->comme_declaration_variable();
                if (declaration_variable->declaration_vient_d_un_emploi) {
                    /* Transforme en un accès de membre. */
                    auto ref_decl_var = assem->crée_reference_declaration(
                        référence->lexeme, declaration_variable->declaration_vient_d_un_emploi);

                    auto type = ref_decl_var->type;
                    while (type->est_type_pointeur() || type->est_type_reference()) {
                        type = type_déréférencé_pour(type);
                    }

                    auto type_composé = type->comme_type_compose();
                    auto membre =
                        type_composé->membres[declaration_variable->index_membre_employe];

                    if (membre.possède_drapeau(MembreTypeComposé::PROVIENT_D_UN_EMPOI)) {
                        auto accès_membre = crée_référence_pour_membre_employé(
                            assem, référence->lexeme, ref_decl_var, type_composé, membre);
                        référence->substitution = accès_membre;
                    }
                    else {
                        auto accès_membre = assem->crée_reference_membre(
                            référence->lexeme,
                            ref_decl_var,
                            référence->type,
                            declaration_variable->index_membre_employe);
                        référence->substitution = accès_membre;
                    }
                }
            }

            if (!m_substitutions_boucles_pour.est_vide()) {
                /* Si nous somme dans le corps d'une boucle-pour personnalisée, substitue le
                 * paramètre de l'opérateur par la variable. */
                auto &données_substitution = m_substitutions_boucles_pour.haut();
                if (référence->declaration_referee == données_substitution.param) {
                    référence->substitution = données_substitution.référence_paramètre;
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

            auto nouvelle_boucle = assem->crée_boucle(noeud->lexeme);
            nouvelle_boucle->bloc_parent = boucle->bloc_parent;

            auto condition = crée_condition_boucle(nouvelle_boucle,
                                                   GenreNoeud::INSTRUCTION_SAUFSI);
            condition->condition = boucle->condition;

            auto nouveau_bloc = assem->crée_bloc_seul(nullptr, boucle->bloc_parent);
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

            auto nouvelle_boucle = assem->crée_boucle(noeud->lexeme);
            nouvelle_boucle->bloc_parent = boucle->bloc_parent;

            auto condition = crée_condition_boucle(nouvelle_boucle,
                                                   GenreNoeud::INSTRUCTION_SAUFSI);
            condition->condition = boucle->condition;

            auto nouveau_bloc = assem->crée_bloc_seul(nullptr, boucle->bloc_parent);
            nouveau_bloc->ajoute_expression(condition);
            nouveau_bloc->ajoute_expression(boucle->bloc);

            nouvelle_boucle->bloc = nouveau_bloc;
            boucle->substitution = nouvelle_boucle;
            return;
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
            auto parenthèse = noeud->comme_parenthese();
            simplifie(parenthèse->expression);
            parenthèse->substitution = parenthèse->expression;
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
        case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
        {
            auto retour = noeud->comme_retourne_multiple();
            simplifie_retour(retour);
            return;
        }
        case GenreNoeud::INSTRUCTION_SAUFSI:
        case GenreNoeud::INSTRUCTION_SI:
        {
            auto si = noeud->comme_si();
            simplifie(si->condition);
            simplifie(si->bloc_si_vrai);
            simplifie(si->bloc_si_faux);

            if (si->possède_drapeau(PositionCodeNoeud::DROITE_ASSIGNATION)) {
                /*

                  x := si y { z } sinon { w }

                  {
                    decl := XXX;
                    si y { decl = z; } sinon { decl = w; }
                    decl; // nous avons une référence simple car la RI empilera sa valeur qui
                  pourra être dépilée et utilisée pour l'assignation
                  }

                 */

                auto bloc = assem->crée_bloc_seul(si->lexeme, si->bloc_parent);

                auto decl_temp = assem->crée_declaration_variable(
                    si->lexeme, si->type, nullptr, nullptr);
                auto ref_temp = assem->crée_reference_declaration(si->lexeme, decl_temp);

                auto nouveau_si = assem->crée_si(si->lexeme, si->genre);
                nouveau_si->condition = si->condition;
                nouveau_si->bloc_si_vrai = si->bloc_si_vrai;
                nouveau_si->bloc_si_faux = si->bloc_si_faux;

                corrige_bloc_pour_assignation(nouveau_si->bloc_si_vrai, ref_temp);
                corrige_bloc_pour_assignation(nouveau_si->bloc_si_faux, ref_temp);

                bloc->ajoute_expression(decl_temp);
                bloc->ajoute_expression(nouveau_si);
                bloc->ajoute_expression(ref_temp);

                si->substitution = bloc;
            }

            return;
        }
        case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
        {
            simplifie_comparaison_chainée(noeud->comme_comparaison_chainee());
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

                auto comme = assem->crée_comme(appel->lexeme);
                comme->type = appel->type;
                comme->expression = appel->parametres_resolus[0];
                comme->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                         appel->type};
                comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;

                appel->substitution = comme;
                m_site_pour_position_code_source = ancien_site_pour_position_code_source;
                return;
            }

            if (appel->aide_generation_code == CONSTRUIT_OPAQUE_DEPUIS_STRUCTURE) {
                simplifie_construction_opaque_depuis_structure(appel);
                return;
            }

            if (appel->aide_generation_code == MONOMORPHE_TYPE_OPAQUE) {
                appel->substitution = assem->crée_reference_type(appel->lexeme, appel->type);
            }

            if (appel->noeud_fonction_appelee) {
                appel->expression->substitution = assem->crée_reference_declaration(
                    appel->lexeme, appel->noeud_fonction_appelee->comme_declaration_symbole());
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

            return;
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

            return;
        }
        case GenreNoeud::DECLARATION_VARIABLE:
        {
            auto déclaration = noeud->comme_declaration_variable();
            simplifie(déclaration->expression);
            return;
        }
        case GenreNoeud::DECLARATION_VARIABLE_MULTIPLE:
        {
            auto déclaration = noeud->comme_declaration_variable_multiple();
            POUR (déclaration->donnees_decl.plage()) {
                simplifie(it.expression);
            }
            return;
        }
        case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
        {
            auto pousse_contexte = noeud->comme_pousse_contexte();
            simplifie(pousse_contexte->bloc);

            auto bloc_substitution = assem->crée_bloc_seul(pousse_contexte->lexeme,
                                                           pousse_contexte->bloc_parent);

            auto contexte_courant = espace->compilatrice().globale_contexte_programme;
            auto ref_contexte_courant = assem->crée_reference_declaration(pousse_contexte->lexeme,
                                                                          contexte_courant);

            // sauvegarde_contexte := __contexte_fil_principal
            auto sauvegarde_contexte = assem->crée_declaration_variable(
                pousse_contexte->lexeme, contexte_courant->type, nullptr, ref_contexte_courant);
            auto ref_sauvegarde_contexte = assem->crée_reference_declaration(
                pousse_contexte->lexeme, sauvegarde_contexte);
            bloc_substitution->ajoute_expression(sauvegarde_contexte);

            // __contexte_fil_principal = expr
            auto permute_contexte = assem->crée_assignation_variable(
                pousse_contexte->lexeme, ref_contexte_courant, pousse_contexte->expression);
            bloc_substitution->ajoute_expression(permute_contexte);

            /* Il est possible qu'une instruction de retour se trouve dans le bloc, donc nous
             * devons différer la restauration du contexte :
             *
             * diffère __contexte_fil_principal = sauvegarde_contexte
             */
            auto inst_diffère = assem->crée_differe(pousse_contexte->lexeme);
            inst_diffère->bloc_parent = bloc_substitution;
            inst_diffère->expression = assem->crée_assignation_variable(
                pousse_contexte->lexeme, ref_contexte_courant, ref_sauvegarde_contexte);
            bloc_substitution->ajoute_expression(inst_diffère);

            /* À FAIRE : surécrire le bloc_parent d'un bloc avec un bloc de substitution peut avoir
             * des conséquences incertaines mais nous devons avoir le bloc de substitution dans la
             * liste des ancêtres du bloc afin que l'instruction diffère soit gérée dans la RI. */
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
                indexage->substitution = simplifie_opérateur_binaire(indexage, true);
            }

            return;
        }
        case GenreNoeud::DECLARATION_STRUCTURE:
        {
            auto structure = noeud->comme_type_structure();

            POUR (structure->membres) {
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
                expr->substitution = assem->crée_reference_type(expr->lexeme, expr->type);
            }
            else {
                simplifie(expr->expression);
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
        case GenreNoeud::EXPRESSION_MEMOIRE:
        {
            auto expr_mémoire = noeud->comme_memoire();
            simplifie(expr_mémoire->expression);
            return;
        }
        case GenreNoeud::DIRECTIVE_INTROSPECTION:
        {
            auto directive = noeud->comme_directive_instrospection();
            if (directive->ident == ID::chemin_de_ce_fichier) {
                auto &compilatrice = espace->compilatrice();
                auto littérale_chaine = assem->crée_litterale_chaine(noeud->lexeme);
                littérale_chaine->drapeaux |=
                    DrapeauxNoeud::LEXÈME_EST_RÉUTILISÉ_POUR_SUBSTITUTION;
                auto fichier = compilatrice.fichier(noeud->lexeme->fichier);
                littérale_chaine->valeur = compilatrice.gerante_chaine->ajoute_chaine(
                    fichier->chemin());
                littérale_chaine->type = TypeBase::CHAINE;
                noeud->substitution = littérale_chaine;
            }
            else if (directive->ident == ID::chemin_de_ce_module) {
                auto &compilatrice = espace->compilatrice();
                auto littérale_chaine = assem->crée_litterale_chaine(noeud->lexeme);
                littérale_chaine->drapeaux |=
                    DrapeauxNoeud::LEXÈME_EST_RÉUTILISÉ_POUR_SUBSTITUTION;
                auto fichier = compilatrice.fichier(noeud->lexeme->fichier);
                littérale_chaine->valeur = compilatrice.gerante_chaine->ajoute_chaine(
                    fichier->module->chemin());
                littérale_chaine->type = TypeBase::CHAINE;
                noeud->substitution = littérale_chaine;
            }
            else if (directive->ident == ID::nom_de_cette_fonction) {
                assert(fonction_courante);
                auto &compilatrice = espace->compilatrice();
                auto littérale_chaine = assem->crée_litterale_chaine(noeud->lexeme);
                littérale_chaine->drapeaux |=
                    DrapeauxNoeud::LEXÈME_EST_RÉUTILISÉ_POUR_SUBSTITUTION;
                littérale_chaine->valeur = compilatrice.gerante_chaine->ajoute_chaine(
                    fonction_courante->ident->nom);
                littérale_chaine->type = TypeBase::CHAINE;
                noeud->substitution = littérale_chaine;
            }
            else if (directive->ident == ID::type_de_cette_fonction ||
                     directive->ident == ID::type_de_cette_structure) {
                noeud->substitution = assem->crée_reference_type(noeud->lexeme, noeud->type);
            }

            return;
        }
        case GenreNoeud::EXPRESSION_TYPE_TABLEAU_FIXE:
        case GenreNoeud::EXPRESSION_TYPE_TABLEAU_DYNAMIQUE:
        case GenreNoeud::EXPRESSION_TYPE_TRANCHE:
        case GenreNoeud::EXPRESSION_TYPE_FONCTION:
        {
            noeud->substitution = assem->crée_reference_type(noeud->lexeme, noeud->type);
            return;
        }
        case GenreNoeud::DIRECTIVE_EXECUTE:
        {
            auto exécute = noeud->comme_execute();
            if (exécute->substitution) {
                simplifie(exécute->substitution);
            }
            return;
        }
        case GenreNoeud::DECLARATION_ENUM:
        case GenreNoeud::ERREUR:
        case GenreNoeud::ENUM_DRAPEAU:
        CAS_POUR_NOEUDS_TYPES_FONDAMENTAUX:
        case GenreNoeud::DECLARATION_UNION:
        case GenreNoeud::DECLARATION_OPAQUE:
        case GenreNoeud::EXPRESSION_INFO_DE:
        case GenreNoeud::EXPRESSION_INIT_DE:
        case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
        case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
        case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
        case GenreNoeud::EXPRESSION_LITTERALE_NUL:
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
    auto bloc_sans_arrêt = inst->bloc_sansarret;
    auto bloc_sinon = inst->bloc_sinon;

    auto boucle = assem->crée_boucle(inst->lexeme);
    boucle->ident = it->ident;
    boucle->bloc_parent = inst->bloc_parent;
    boucle->bloc = assem->crée_bloc_seul(inst->lexeme, boucle->bloc_parent);
    boucle->bloc_sansarret = bloc_sans_arrêt;
    boucle->bloc_sinon = bloc_sinon;

    auto type_index_it = index_it->type;
    auto zero = assem->crée_litterale_entier(index_it->lexeme, type_index_it, 0);

    auto ref_it = assem->crée_reference_declaration(it->lexeme, it);
    auto ref_index = assem->crée_reference_declaration(it->lexeme, index_it);

    auto bloc_pre = assem->crée_bloc_seul(nullptr, boucle->bloc_parent);

    bloc_pre->ajoute_expression(it);
    bloc_pre->ajoute_expression(index_it);

    auto bloc_inc = assem->crée_bloc_seul(nullptr, boucle->bloc_parent);

    auto condition = crée_condition_boucle(boucle, GenreNoeud::INSTRUCTION_SI);

    boucle->bloc_pre = bloc_pre;
    boucle->bloc_inc = bloc_inc;

    auto const inverse_boucle = inst->lexeme_op == GenreLexème::SUPERIEUR;

    auto type_itéré = expression_iteree->type->est_type_opaque() ?
                          expression_iteree->type->comme_type_opaque()->type_opacifie :
                          expression_iteree->type;
    expression_iteree->type = type_itéré;

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
                expr_debut = assem->crée_litterale_entier(
                    expression_iteree->lexeme, expression_iteree->type, 0);

                auto valeur_un = assem->crée_litterale_entier(
                    expression_iteree->lexeme, expression_iteree->type, 1);
                expr_fin = assem->crée_expression_binaire(
                    expression_iteree->lexeme,
                    expression_iteree->type->table_opérateurs->opérateur_sst,
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
            NoeudExpression *nombre_iterations = assem->crée_expression_binaire(
                expression_iteree->lexeme,
                expression_iteree->type->table_opérateurs->opérateur_sst,
                expr_fin,
                expr_debut);

            auto valeur_un = assem->crée_litterale_entier(
                expression_iteree->lexeme, expression_iteree->type, 1);
            nombre_iterations = assem->crée_expression_binaire(
                expression_iteree->lexeme,
                expression_iteree->type->table_opérateurs->opérateur_ajt,
                nombre_iterations,
                valeur_un);

            /* condition */
            if (inverse_boucle) {
                std::swap(expr_debut, expr_fin);
            }

            auto init_it = assem->crée_assignation_variable(ref_it->lexeme, ref_it, expr_debut);
            bloc_pre->ajoute_expression(init_it);

            auto op_comp = index_it->type->table_opérateurs->opérateur_seg;
            condition->condition = assem->crée_expression_binaire(
                inst->lexeme, op_comp, ref_index, nombre_iterations);
            boucle->bloc->ajoute_expression(condition);

            /* corps */
            boucle->bloc->ajoute_expression(bloc);

            /* suivant */
            if (inverse_boucle) {
                auto inc_it = assem->crée_decrementation(ref_it->lexeme, ref_it);
                bloc_inc->ajoute_expression(inc_it);
            }
            else {
                auto inc_it = assem->crée_incrementation(ref_it->lexeme, ref_it);
                bloc_inc->ajoute_expression(inc_it);
            }

            auto inc_it = assem->crée_incrementation(ref_index->lexeme, ref_index);
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
                expr_taille = assem->crée_litterale_entier(
                    inst->lexeme, TypeBase::Z64, static_cast<uint64_t>(taille_tableau));
            }
            else {
                expr_taille = assem->crée_reference_membre(
                    inst->lexeme, expression_iteree, TypeBase::Z64, 1);
            }

            auto type_z64 = TypeBase::Z64;
            condition->condition = assem->crée_expression_binaire(
                inst->lexeme, type_z64->table_opérateurs->opérateur_seg, ref_index, expr_taille);

            auto expr_pointeur = NoeudExpression::nul();

            auto type_compose = type_itéré->comme_type_compose();

            if (type_itéré->est_type_tableau_fixe()) {
                auto indexage = assem->crée_indexage(inst->lexeme, expression_iteree, zero, true);

                expr_pointeur = crée_prise_adresse(
                    assem, inst->lexeme, indexage, typeuse.type_pointeur_pour(indexage->type));
            }
            else {
                expr_pointeur = assem->crée_reference_membre(
                    inst->lexeme, expression_iteree, type_compose->membres[0].type, 0);
            }

            NoeudExpression *expr_index = ref_index;

            if (inverse_boucle) {
                expr_index = assem->crée_expression_binaire(
                    inst->lexeme,
                    ref_index->type->table_opérateurs->opérateur_sst,
                    expr_taille,
                    ref_index);
                expr_index = assem->crée_expression_binaire(
                    inst->lexeme,
                    ref_index->type->table_opérateurs->opérateur_sst,
                    expr_index,
                    assem->crée_litterale_entier(ref_index->lexeme, ref_index->type, 1));
            }

            auto indexage = assem->crée_indexage(inst->lexeme, expr_pointeur, expr_index, true);
            NoeudExpression *expression_assignee = indexage;

            if (inst->prend_reference || inst->prend_pointeur) {
                auto noeud_comme = assem->crée_comme(it->lexeme);
                noeud_comme->type = it->type;
                noeud_comme->expression = indexage;
                noeud_comme->transformation = TransformationType(
                    TypeTransformation::PREND_REFERENCE);
                noeud_comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;

                expression_assignee = noeud_comme;
            }

            auto assign_it = assem->crée_assignation_variable(
                inst->lexeme, ref_it, expression_assignee);

            boucle->bloc->ajoute_expression(condition);
            boucle->bloc->ajoute_expression(assign_it);

            /* corps */
            boucle->bloc->ajoute_expression(bloc);

            /* incrémente */
            auto inc_it = assem->crée_incrementation(ref_index->lexeme, ref_index);
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

            auto idx = NoeudExpression::nul();
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
    auto corps_opérateur_pour = inst->corps_operateur_pour;

    auto bloc_substitution = assem->crée_bloc_seul(corps_opérateur_pour->bloc->lexeme,
                                                   inst->bloc_parent);

    /* Crée une variable temporaire pour l'expression itérée. Si l'expression est par exemple un
     * appel, il sera toujours évalué, menant potentiellement à une boucle infinie. */
    auto temporaire = assem->crée_declaration_variable(
        inst->expression->lexeme, inst->expression->type, nullptr, inst->expression);
    auto ref_temporaire = assem->crée_reference_declaration(temporaire->lexeme, temporaire);
    bloc_substitution->ajoute_expression(temporaire);

    /* Ajoute les déclarations des variables d'itération dans le corps du bloc pour que la RI les
     * trouve avant de générer le code des références. */
    bloc_substitution->ajoute_expression(inst->decl_it);
    bloc_substitution->ajoute_expression(inst->decl_index_it);
    bloc_substitution->ajoute_expression(corps_opérateur_pour->bloc);

    /* Substitutions manuelles. */
    auto entête = corps_opérateur_pour->entete;
    auto param = entête->parametre_entree(0);

    SubstitutionBouclePourOpérée substitution_manuelle;
    substitution_manuelle.référence_paramètre = ref_temporaire;
    substitution_manuelle.param = param;
    substitution_manuelle.corps_boucle = inst->bloc;

    m_substitutions_boucles_pour.empile(substitution_manuelle);
    simplifie(inst->corps_operateur_pour);
    m_substitutions_boucles_pour.depile();

    inst->substitution = bloc_substitution;
}

static void rassemble_opérations_chainées(NoeudExpression *racine,
                                          kuri::tableau<NoeudExpressionBinaire> &comparaisons)
{
    auto expr_bin = racine->comme_expression_binaire();

    if (est_opérateur_comparaison(expr_bin->operande_gauche->lexeme->genre)) {
        rassemble_opérations_chainées(expr_bin->operande_gauche, comparaisons);

        auto expr_operande = expr_bin->operande_gauche->comme_expression_binaire();

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

NoeudExpression *Simplificatrice::crée_expression_pour_op_chainée(
    kuri::tableau<NoeudExpressionBinaire> &comparaisons, Lexème const *lexeme_op_logique)
{
    kuri::pile<NoeudExpression *> exprs;

    for (auto i = comparaisons.taille() - 1; i >= 0; --i) {
        auto &it = comparaisons[i];
        simplifie(it.operande_gauche);
        simplifie(it.operande_droite);
        exprs.empile(simplifie_opérateur_binaire(&it, true));
    }

    if (exprs.taille() == 1) {
        return exprs.depile();
    }

    auto résultat = NoeudExpression::nul();

    while (true) {
        auto a = exprs.depile();
        auto b = exprs.depile();

        auto et = assem->crée_expression_logique(lexeme_op_logique);
        et->opérande_gauche = a;
        et->opérande_droite = b;

        if (exprs.est_vide()) {
            résultat = et;
            break;
        }

        exprs.empile(et);
    }

    return résultat;
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

        di = assem->crée_assignation_variable(di->lexeme, ref_temp, di);
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

void Simplificatrice::simplifie_comparaison_chainée(NoeudExpressionBinaire *comp)
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
}

void Simplificatrice::crée_retourne_union_via_rien(NoeudDeclarationEnteteFonction *entête,
                                                   NoeudBloc *bloc_d_insertion,
                                                   Lexème const *lexeme_reference)
{
    auto type_sortie = entête->type->comme_type_fonction()->type_sortie->comme_type_union();
    auto retourne = assem->crée_retourne(lexeme_reference);
    retourne->bloc_parent = bloc_d_insertion;

    auto param_sortie = entête->param_sortie;
    auto ref_param_sortie = assem->crée_reference_declaration(lexeme_reference, param_sortie);

    auto type_union = type_sortie;

    auto info_membre = donne_membre_pour_type(type_union, TypeBase::RIEN);
    assert(info_membre.has_value());
    auto index_membre = uint32_t(info_membre->index_membre);

    auto ref_membre = assem->crée_reference_membre(
        lexeme_reference, ref_param_sortie, TypeBase::Z32, 1);
    auto valeur_index = assem->crée_litterale_entier(
        lexeme_reference, TypeBase::Z32, index_membre + 1);

    auto assignation = assem->crée_assignation_variable(
        lexeme_reference, ref_membre, valeur_index);

    retourne->expression = ref_param_sortie;

    bloc_d_insertion->ajoute_expression(assignation);
    bloc_d_insertion->ajoute_expression(retourne);
}

/* Les retours sont simplifiés sous forme d'un chargement pour les retours simples. */
void Simplificatrice::simplifie_retour(NoeudInstructionRetour *inst)
{
    if (inst->aide_generation_code == RETOURNE_UNE_UNION_VIA_RIEN) {
        auto bloc = assem->crée_bloc_seul(inst->lexeme, inst->bloc_parent);
        crée_retourne_union_via_rien(fonction_courante, bloc, inst->lexeme);
        inst->substitution = bloc;
        return;
    }

    /* Nous n'utilisons pas le type de la fonction_courante car elle peut être nulle dans le cas où
     * nous avons un #test. */
    auto type_sortie = inst->type;
    if (type_sortie->est_type_rien()) {
        return;
    }

    simplifie(inst->expression);
}

/* Les retours sont simplifiés sous forme d'assignations des valeurs de retours. */
void Simplificatrice::simplifie_retour(NoeudInstructionRetourMultiple *inst)
{
    /* Nous n'utilisons pas le type de la fonction_courante car elle peut être nulle dans le cas où
     * nous avons un #test. */
    assert(!inst->type->est_type_rien());

    /* Crée une assignation pour chaque sortie. */
    POUR (inst->donnees_exprs.plage()) {
        simplifie(it.expression);

        /* Les variables sont les déclarations des paramètres, donc crée des références. */
        for (auto &var : it.variables.plage()) {
            var = assem->crée_reference_declaration(var->lexeme,
                                                    var->comme_declaration_variable());
        }
    }

    auto assignation = assem->crée_assignation_multiple(inst->lexeme);
    assignation->expression = inst->expression;
    assignation->données_exprs = std::move(inst->donnees_exprs);

    auto retour = assem->crée_retourne(inst->lexeme);
    retour->expression = assem->crée_reference_declaration(fonction_courante->param_sortie->lexeme,
                                                           fonction_courante->param_sortie);

    auto bloc = assem->crée_bloc_seul(inst->lexeme, inst->bloc_parent);
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
    Lexème const *lexeme, NoeudExpression *expression_à_initialiser)
{
    auto type_expression = expression_à_initialiser->type;
    auto fonction_init = crée_entête_pour_initialisation_type(
        type_expression, espace->compilatrice(), assem, typeuse);

    auto prise_adresse = crée_prise_adresse(
        assem, lexeme, expression_à_initialiser, typeuse.type_pointeur_pour(type_expression));
    auto appel = assem->crée_appel(lexeme, fonction_init, TypeBase::RIEN);
    appel->parametres_resolus.ajoute(prise_adresse);

    return appel;
}

static NoeudExpression *supprime_parenthèses(NoeudExpression *expression)
{
    while (expression->est_parenthese()) {
        expression = expression->comme_parenthese()->expression;
    }
    return expression;
}

static void aplatis_expression_logique(NoeudExpressionLogique *logique,
                                       kuri::tablet<NoeudExpressionLogique *, 6> &résultat)
{
    auto opérande_gauche = supprime_parenthèses(logique->opérande_gauche);
    if (opérande_gauche->est_expression_logique()) {
        aplatis_expression_logique(opérande_gauche->comme_expression_logique(), résultat);
    }

    résultat.ajoute(logique);

    auto opérande_droite = supprime_parenthèses(logique->opérande_droite);
    if (opérande_droite->est_expression_logique()) {
        aplatis_expression_logique(opérande_droite->comme_expression_logique(), résultat);
    }
}

static kuri::tablet<NoeudExpressionLogique *, 6> aplatis_expression_logique(
    NoeudExpressionLogique *logique)
{
    kuri::tablet<NoeudExpressionLogique *, 6> résultat;
    aplatis_expression_logique(logique, résultat);
    return résultat;
}

void Simplificatrice::simplifie_expression_logique(NoeudExpressionLogique *logique)
{
#if 1
    simplifie(logique->opérande_droite);
    simplifie(logique->opérande_gauche);
#else
    // À FAIRE : simplifie les accès à des énum_drapeaux dans les expressions || ou &&,
    // il faudra également modifier la RI pour prendre en compte la substitution
    if (logique->possède_drapeau(PositionCodeNoeud::DROITE_CONDITION)) {
        simplifie(logique->opérande_droite);
        simplifie(logique->opérande_gauche);
        return;
    }

    /* À FAIRE(expression logique) : simplifie comme GCC pour les assignations
     * a := b && c ->  x := b; si x == vrai { x = c; }; a := x;
     * a := b || c ->  x := b; si x == faux { x = c; }; a := x;
     */

    dbg() << erreur::imprime_site(*espace, logique->opérande_droite);

    auto noeuds = aplatis_expression_logique(logique);
    dbg() << "Nombre de noeuds : " << noeuds.taille();

    static Lexème lexème_temp{};

    simplifie(noeuds[0]->opérande_gauche);
    auto temp = assem->crée_declaration_variable(
        &lexème_temp, TypeBase::BOOL, nullptr, noeuds[0]->opérande_gauche);

    auto bloc = assem->crée_bloc_seul(logique->lexeme, logique->bloc_parent);
    bloc->ajoute_expression(temp);

    auto bloc_courant = bloc;

    POUR (noeuds) {
        dbg() << erreur::imprime_site(*espace, it);

        auto test = (it->lexeme->genre == GenreLexème::ESP_ESP) ?
                        assem->crée_si(logique->lexeme) :
                        assem->crée_saufsi(logique->lexeme);
        bloc_courant->ajoute_expression(test);

        test->condition = temp->valeur;

        simplifie(it->opérande_droite);
        auto bloc_si_vrai = assem->crée_bloc_seul(logique->lexeme, bloc_courant);
        auto assignation = assem->crée_assignation_variable(
            logique->lexeme, temp->valeur, it->opérande_droite);
        bloc_si_vrai->ajoute_expression(assignation);

        test->bloc_si_vrai = bloc_si_vrai;
        bloc_courant = bloc_si_vrai;
    }

    bloc->ajoute_expression(temp->valeur);
    logique->substitution = bloc;
#endif
}

void Simplificatrice::simplifie_construction_union(
    NoeudExpressionConstructionStructure *construction)
{
    auto const site = construction;
    auto const lexème = construction->lexeme;
    auto type_union = construction->type->comme_type_union();

    if (construction->parametres_resolus.est_vide()) {
        /* Initialise à zéro. */

        auto decl_position = assem->crée_declaration_variable(
            lexème, type_union, nullptr, &non_initialisation);
        auto ref_position = assem->crée_reference_declaration(decl_position->lexeme,
                                                              decl_position);

        auto bloc = assem->crée_bloc_seul(lexème, site->bloc_parent);
        bloc->ajoute_expression(decl_position);

        auto appel = crée_appel_fonction_init(lexème, ref_position);
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
    auto comme = assem->crée_comme(lexème);
    comme->type = type_union;
    comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;
    comme->expression = expression_initialisation;
    comme->transformation = {TypeTransformation::CONSTRUIT_UNION, type_union, index_membre};

    construction->substitution = comme;
}

void Simplificatrice::simplifie_construction_structure_position_code_source(
    NoeudExpressionConstructionStructure *construction)
{
    auto const lexème = construction->lexeme;
    const NoeudExpression *site = m_site_pour_position_code_source ?
                                      m_site_pour_position_code_source :
                                      construction;
    auto const lexème_site = site->lexeme;

    auto &compilatrice = espace->compilatrice();

    /* Création des valeurs pour chaque membre. */

    /* PositionCodeSource.fichier
     * À FAIRE : sécurité, n'utilise pas le chemin, mais détermine une manière fiable et robuste
     * d'obtenir le fichier, utiliser simplement le nom n'est pas fiable (d'autres fichiers du même
     * nom dans le module). */
    auto const fichier = compilatrice.fichier(lexème_site->fichier);
    auto valeur_chemin_fichier = assem->crée_litterale_chaine(lexème);
    valeur_chemin_fichier->drapeaux |= DrapeauxNoeud::LEXÈME_EST_RÉUTILISÉ_POUR_SUBSTITUTION;
    valeur_chemin_fichier->valeur = compilatrice.gerante_chaine->ajoute_chaine(fichier->chemin());
    valeur_chemin_fichier->type = TypeBase::CHAINE;

    /* PositionCodeSource.fonction */
    auto nom_fonction = kuri::chaine_statique("");
    if (fonction_courante && fonction_courante->ident) {
        nom_fonction = fonction_courante->ident->nom;
    }

    auto valeur_nom_fonction = assem->crée_litterale_chaine(lexème);
    valeur_nom_fonction->drapeaux |= DrapeauxNoeud::LEXÈME_EST_RÉUTILISÉ_POUR_SUBSTITUTION;
    valeur_nom_fonction->valeur = compilatrice.gerante_chaine->ajoute_chaine(nom_fonction);
    valeur_nom_fonction->type = TypeBase::CHAINE;

    /* PositionCodeSource.ligne */
    auto pos = position_lexeme(*lexème_site);
    auto valeur_ligne = assem->crée_litterale_entier(
        lexème, TypeBase::Z32, static_cast<unsigned>(pos.numero_ligne));

    /* PositionCodeSource.colonne */
    auto valeur_colonne = assem->crée_litterale_entier(
        lexème, TypeBase::Z32, static_cast<unsigned>(pos.pos));

    /* Création d'une temporaire et assignation des membres. */

    auto const type_position_code_source = typeuse.type_position_code_source;
    auto decl_position = assem->crée_declaration_variable(
        lexème, type_position_code_source, nullptr, &non_initialisation);
    auto ref_position = assem->crée_reference_declaration(decl_position->lexeme, decl_position);

    auto ref_membre_fichier = assem->crée_reference_membre(
        lexème, ref_position, TypeBase::CHAINE, 0);
    auto ref_membre_fonction = assem->crée_reference_membre(
        lexème, ref_position, TypeBase::CHAINE, 1);
    auto ref_membre_ligne = assem->crée_reference_membre(lexème, ref_position, TypeBase::Z32, 2);
    auto ref_membre_colonne = assem->crée_reference_membre(lexème, ref_position, TypeBase::Z32, 3);

    NoeudExpression *couples_ref_membre_expression[4][2] = {
        {ref_membre_fichier, valeur_chemin_fichier},
        {ref_membre_fonction, valeur_nom_fonction},
        {ref_membre_ligne, valeur_ligne},
        {ref_membre_colonne, valeur_colonne},
    };

    auto bloc = assem->crée_bloc_seul(lexème, site->bloc_parent);
    bloc->ajoute_expression(decl_position);

    for (auto couple : couples_ref_membre_expression) {
        auto assign = assem->crée_assignation_variable(lexème, couple[0], couple[1]);
        bloc->ajoute_expression(assign);
    }

    /* La dernière expression (une simple référence) sera utilisée lors de la génération de RI pour
     * définir la valeur à assigner. */
    bloc->ajoute_expression(ref_position);
    construction->substitution = bloc;
}

NoeudExpressionReference *Simplificatrice::génère_simplification_construction_structure(
    NoeudExpressionAppel *construction, TypeStructure *type_struct, NoeudBloc *bloc)
{
    auto const lexème = construction->lexeme;
    auto déclaration = assem->crée_declaration_variable(
        lexème, type_struct, nullptr, &non_initialisation);
    auto référence = assem->crée_reference_declaration(déclaration->lexeme, déclaration);

    bloc->ajoute_expression(déclaration);

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
                assem, lexème, référence, type_struct, membre);
            auto assign = assem->crée_assignation_variable(lexème, ref_membre, it);
            bloc->ajoute_expression(assign);
            continue;
        }

        auto type_membre = membre.type;
        auto ref_membre = assem->crée_reference_membre(lexème, référence, type_membre, index_it);

        if (it != nullptr) {
            auto assign = assem->crée_assignation_variable(lexème, ref_membre, it);
            bloc->ajoute_expression(assign);
        }
        else {
            auto appel = crée_appel_fonction_init(lexème, ref_membre);
            bloc->ajoute_expression(appel);
        }
    }

    return référence;
}

void Simplificatrice::simplifie_construction_structure_impl(
    NoeudExpressionConstructionStructure *construction)
{
    auto const site = construction;
    auto const lexème = construction->lexeme;
    auto const type_struct = construction->type->comme_type_structure();

    auto bloc = assem->crée_bloc_seul(lexème, site->bloc_parent);

    auto ref_struct = génère_simplification_construction_structure(
        construction, type_struct, bloc);

    /* La dernière expression (une simple référence) sera utilisée lors de la génération de RI pour
     * définir la valeur à assigner. */
    bloc->ajoute_expression(ref_struct);
    construction->substitution = bloc;
}

void Simplificatrice::simplifie_construction_opaque_depuis_structure(NoeudExpressionAppel *appel)
{
    auto const site = appel;
    auto const lexème = appel->lexeme;
    auto type_opaque = appel->type->comme_type_opaque();
    auto type_struct =
        const_cast<Type *>(donne_type_opacifié_racine(type_opaque))->comme_type_structure();

    auto bloc = assem->crée_bloc_seul(lexème, site->bloc_parent);

    auto ref_struct = génère_simplification_construction_structure(appel, type_struct, bloc);

    /* ref_opaque := ref_struct comme TypeOpaque */
    auto comme = assem->crée_comme(appel->lexeme);
    comme->type = type_opaque;
    comme->expression = ref_struct;
    comme->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_opaque};
    comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;

    auto decl_opaque = assem->crée_declaration_variable(lexème, type_opaque, nullptr, comme);
    auto ref_opaque = assem->crée_reference_declaration(decl_opaque->lexeme, decl_opaque);
    bloc->ajoute_expression(decl_opaque);

    /* La dernière expression sera utilisée lors de la génération de RI pour
     * définir la valeur à assigner. */
    bloc->ajoute_expression(ref_opaque);
    appel->substitution = bloc;
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
        auto decl_membre = membre_courant.decl->comme_declaration_variable();
        auto decl_employée = decl_membre->declaration_vient_d_un_emploi;
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
                                                           Lexème const *lexeme,
                                                           NoeudExpression *expression_accédée,
                                                           TypeCompose *type_composé,
                                                           MembreTypeComposé const &membre)
{
    auto hiérarchie = trouve_hiérarchie_emploi_membre(type_composé, membre);
    auto ref_membre_courant = expression_accédée;

    POUR (hiérarchie) {
        auto accès_base = assem->crée_reference_membre(
            lexeme, ref_membre_courant, it.membre.type, it.index_membre);
        accès_base->ident = it.membre.nom;
        ref_membre_courant = accès_base;
    }

    assert(ref_membre_courant != expression_accédée);
    return ref_membre_courant;
}

void Simplificatrice::simplifie_référence_membre(NoeudExpressionMembre *ref_membre)
{
    auto const lexème = ref_membre->lexeme;
    auto accédée = ref_membre->accedee;

    if (ref_membre->possède_drapeau(DrapeauxNoeud::ACCES_EST_ENUM_DRAPEAU)) {
        simplifie(accédée);

        // a.DRAPEAU => (a & DRAPEAU) != 0
        auto type_enum = static_cast<TypeEnum *>(ref_membre->type);
        auto valeur_énum = type_enum->membres[ref_membre->index_membre].valeur;

        auto valeur_lit_enum = assem->crée_litterale_entier(
            lexème, type_enum, static_cast<unsigned>(valeur_énum));
        auto op = type_enum->table_opérateurs->opérateur_etb;
        auto et = assem->crée_expression_binaire(lexème, op, accédée, valeur_lit_enum);

        auto zero = assem->crée_litterale_entier(lexème, type_enum, 0);
        op = type_enum->table_opérateurs->opérateur_dif;
        auto dif = assem->crée_expression_binaire(lexème, op, et, zero);

        ref_membre->substitution = dif;
        return;
    }

    if (accédée->est_reference_declaration()) {
        if (accédée->comme_reference_declaration()
                ->declaration_referee->est_declaration_module()) {
            ref_membre->substitution = assem->crée_reference_declaration(
                lexème, ref_membre->déclaration_référée);
            simplifie(ref_membre->substitution);
            return;
        }
    }

    auto type_accédé = donne_type_accédé_effectif(accédée->type);

    if (type_accédé->est_type_tableau_fixe()) {
        auto taille = type_accédé->comme_type_tableau_fixe()->taille;
        ref_membre->substitution = assem->crée_litterale_entier(
            lexème, ref_membre->type, static_cast<uint64_t>(taille));
        return;
    }

    if (type_accédé->est_type_enum() || type_accédé->est_type_erreur()) {
        auto type_enum = static_cast<TypeEnum *>(type_accédé);
        auto valeur_énum = type_enum->membres[ref_membre->index_membre].valeur;
        ref_membre->substitution = assem->crée_litterale_entier(
            lexème, type_enum, static_cast<unsigned>(valeur_énum));
        return;
    }

    if (type_accédé->est_type_type_de_donnees() &&
        ref_membre->genre_valeur == GenreValeur::DROITE) {
        ref_membre->substitution = assem->crée_reference_type(
            lexème, typeuse.type_type_de_donnees(ref_membre->type));
        return;
    }

    auto type_composé = type_accédé->comme_type_compose();
    auto &membre = type_composé->membres[ref_membre->index_membre];

    if (membre.drapeaux == MembreTypeComposé::EST_CONSTANT) {
        simplifie(membre.expression_valeur_defaut);
        ref_membre->substitution = membre.expression_valeur_defaut;
        if (ref_membre->substitution->type->est_type_entier_constant()) {
            ref_membre->substitution->type = membre.type;
        }
        return;
    }

    if (membre.drapeaux & MembreTypeComposé::PROVIENT_D_UN_EMPOI) {
        /* Transforme x.y en x.base.y. */
        ref_membre->substitution = crée_référence_pour_membre_employé(
            assem, lexème, ref_membre->accedee, type_composé, membre);
    }

    /* Pour les appels de fonctions ou les accès après des parenthèse (p.e. (x comme *TypeBase).y).
     */
    simplifie(ref_membre->accedee);
}

NoeudExpression *Simplificatrice::simplifie_assignation_énum_drapeau(NoeudExpression *var,
                                                                     NoeudExpression *expression)
{
    auto lexème = var->lexeme;
    auto ref_membre = var->comme_reference_membre();

    // À FAIRE : référence
    // Nous prenons ref_membre->accedee directement car ce ne sera pas
    // simplifié, et qu'il faut prendre en compte les accés d'accés, les
    // expressions entre parenthèses, etc. Donc faire ceci est plus simple.
    auto nouvelle_ref = ref_membre->accedee;
    var->substitution = nouvelle_ref;

    /* Crée la conjonction d'un drapeau avec la variable (a | DRAPEAU) */
    auto crée_conjonction_drapeau =
        [&](NoeudExpression *ref_variable, TypeEnum *type_enum, unsigned valeur_énum) {
            auto valeur_lit_enum = assem->crée_litterale_entier(lexème, type_enum, valeur_énum);
            auto op = type_enum->table_opérateurs->opérateur_oub;
            return assem->crée_expression_binaire(var->lexeme, op, ref_variable, valeur_lit_enum);
        };

    /* Crée la disjonction d'un drapeau avec la variable (a & ~DRAPEAU) */
    auto crée_disjonction_drapeau =
        [&](NoeudExpression *ref_variable, TypeEnum *type_enum, unsigned valeur_énum) {
            auto valeur_lit_enum = assem->crée_litterale_entier(
                lexème, type_enum, ~uint64_t(valeur_énum));
            auto op = type_enum->table_opérateurs->opérateur_etb;
            return assem->crée_expression_binaire(var->lexeme, op, ref_variable, valeur_lit_enum);
        };

    auto type_énum = static_cast<TypeEnum *>(ref_membre->type);
    auto valeur_énum = type_énum->membres[ref_membre->index_membre].valeur;

    if (expression->est_litterale_bool()) {
        /* Nous avons une expression littérale, donc nous pouvons choisir la bonne instruction. */
        if (expression->comme_litterale_bool()->valeur) {
            // a.DRAPEAU = vrai -> a = a | DRAPEAU
            return crée_conjonction_drapeau(
                nouvelle_ref, type_énum, static_cast<unsigned>(valeur_énum));
        }
        // a.DRAPEAU = faux -> a = a & ~DRAPEAU
        return crée_disjonction_drapeau(
            nouvelle_ref, type_énum, static_cast<unsigned>(valeur_énum));
    }
    /* Transforme en une expression « ternaire » sans branche (similaire à a = b ? c : d en
     * C/C++) :
     * v1 = (a | DRAPEAU)
     * v2 = (a & ~DRAPEAU)
     * (-(b comme T) & v1) | (((b comme T) - 1) & v2)
     */

    auto v1 = crée_conjonction_drapeau(
        nouvelle_ref, type_énum, static_cast<unsigned>(valeur_énum));
    auto v2 = crée_disjonction_drapeau(
        nouvelle_ref, type_énum, static_cast<unsigned>(valeur_énum));

    /* Crée une expression pour convertir l'expression en une valeur du type sous-jacent de
     * l'énumération. */
    auto type_sous_jacent = type_énum->type_sous_jacent;

    simplifie(expression);
    auto ref_b = expression->substitution ? expression->substitution : expression;

    /* Convertis l'expression booléenne vers n8 car ils ont la même taille en octet. */
    auto comme = assem->crée_comme(var->lexeme);
    comme->type = TypeBase::N8;
    comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;
    comme->expression = ref_b;
    comme->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, TypeBase::N8};

    /* Augmente la taille du n8 si ce n'est pas le type sous-jacent de l'énum drapeau. */
    if (type_sous_jacent != TypeBase::N8) {
        auto ancien_comme = comme;
        comme = assem->crée_comme(var->lexeme);
        comme->type = type_sous_jacent;
        comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;
        comme->expression = ancien_comme;
        comme->transformation = {TypeTransformation::AUGMENTE_TAILLE_TYPE, type_sous_jacent};
    }

    /* -b */
    auto zero = assem->crée_litterale_entier(lexème, type_énum->type_sous_jacent, 0);
    auto moins_b_type_sous_jacent = assem->crée_expression_binaire(
        lexème, type_sous_jacent->table_opérateurs->opérateur_sst, zero, comme);

    /* Convertis vers le type énum pour que la RI soit contente vis-à-vis de la sûreté de
     * type.
     */
    auto moins_b = assem->crée_comme(var->lexeme);
    moins_b->type = type_énum;
    moins_b->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;
    moins_b->expression = moins_b_type_sous_jacent;
    moins_b->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_énum};

    /* b - 1 */
    auto un = assem->crée_litterale_entier(lexème, type_sous_jacent, 1);
    auto b_moins_un_type_sous_jacent = assem->crée_expression_binaire(
        lexème, type_sous_jacent->table_opérateurs->opérateur_sst, comme, un);

    /* Convertis vers le type énum pour que la RI soit contente vis-à-vis de la sûreté de
     * type.
     */
    auto b_moins_un = assem->crée_comme(var->lexeme);
    b_moins_un->type = type_énum;
    b_moins_un->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;
    b_moins_un->expression = b_moins_un_type_sous_jacent;
    b_moins_un->transformation = TransformationType{TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                                    type_énum};

    /* -b & v1 */
    auto moins_b_et_v1 = assem->crée_expression_binaire(
        lexème, type_énum->table_opérateurs->opérateur_etb, moins_b, v1);
    /* (b - 1) & v2 */
    auto b_moins_un_et_v2 = assem->crée_expression_binaire(
        lexème, type_énum->table_opérateurs->opérateur_etb, b_moins_un, v2);

    /* (-(b comme T) & v1) | (((b comme T) - 1) & v2) */
    return assem->crée_expression_binaire(
        lexème, type_énum->table_opérateurs->opérateur_oub, moins_b_et_v1, b_moins_un_et_v2);
}

NoeudExpression *Simplificatrice::simplifie_opérateur_binaire(NoeudExpressionBinaire *expr_bin,
                                                              bool pour_operande)
{
    if (expr_bin->op->est_basique) {
        if (!pour_operande) {
            return nullptr;
        }

        /* Crée une nouvelle expression binaire afin d'éviter les dépassements de piles car
         * sinon la substitution serait toujours réévaluée lors de l'évaluation de l'expression
         * d'assignation. */
        return assem->crée_expression_binaire(
            expr_bin->lexeme, expr_bin->op, expr_bin->operande_gauche, expr_bin->operande_droite);
    }

    auto appel = assem->crée_appel(
        expr_bin->lexeme, expr_bin->op->decl, expr_bin->op->type_résultat);

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
    auto df = compilatrice.donnees_fonction;
    auto enfant = retiens->expression;

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

    static const Lexème lexème_ou = {",", {}, GenreLexème::BARRE_BARRE, 0, 0, 0};

    auto la_discriminée = discr->expression_discriminee;
    simplifie(la_discriminée);

    /* Création d'un bloc afin de pouvoir déclarer une variable temporaire qui contiendra la valeur
     * discriminée. */
    auto bloc = assem->crée_bloc_seul(discr->lexeme, discr->bloc_parent);
    discr->substitution = bloc;

    auto decl_variable = assem->crée_declaration_variable(
        la_discriminée->lexeme, la_discriminée->type, nullptr, la_discriminée);

    bloc->ajoute_expression(decl_variable);

    auto ref_decl = assem->crée_reference_declaration(decl_variable->lexeme, decl_variable);

    NoeudExpression *expression = ref_decl;

    if (N == DISCR_UNION || N == DISCR_UNION_ANONYME) {
        /* La discrimination se fait via le membre actif. Il faudra proprement gérer les unions
         * dans la RI. */
        expression = assem->crée_reference_membre(
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
    auto si_courant = assem->crée_si(discr->lexeme, GenreNoeud::INSTRUCTION_SI);
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
                auto valeur = valeur_énum(static_cast<TypeEnum *>(expression->type), expr->ident);
                auto constante = assem->crée_litterale_entier(
                    expr->lexeme, expression->type, static_cast<uint64_t>(valeur));
                comparaison.operande_droite = constante;
            }
            else if (N == DISCR_UNION) {
                auto const type_union = discr->expression_discriminee->type->comme_type_union();
                auto index = donne_membre_pour_nom(type_union, expr->ident)->index_membre;
                auto constante = assem->crée_litterale_entier(
                    expr->lexeme, expression->type, static_cast<uint64_t>(index + 1));
                comparaison.operande_droite = constante;
            }
            else if (N == DISCR_UNION_ANONYME) {
                auto const type_union = discr->expression_discriminee->type->comme_type_union();
                auto index = donne_membre_pour_nom(type_union, expr->ident)->index_membre;
                auto constante = assem->crée_litterale_entier(
                    expr->lexeme, expression->type, static_cast<uint64_t>(index + 1));
                comparaison.operande_droite = constante;
            }
            else {
                /* Cette expression est simplifiée via crée_expression_pour_op_chainee. */
                comparaison.operande_droite = expr;
            }

            comparaisons.ajoute(comparaison);
        }

        si_courant->condition = crée_expression_pour_op_chainée(comparaisons, &lexème_ou);

        simplifie(it->bloc);
        si_courant->bloc_si_vrai = it->bloc;

        if (i != (discr->paires_discr.taille() - 1)) {
            auto si = assem->crée_si(discr->lexeme, GenreNoeud::INSTRUCTION_SI);
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

NoeudSi *Simplificatrice::crée_condition_boucle(NoeudExpression *inst, GenreNoeud genre_noeud)
{
    static const Lexème lexème_arrête = {",", {}, GenreLexème::ARRETE, 0, 0, 0};

    /* condition d'arrêt de la boucle */
    auto condition = assem->crée_si(inst->lexeme, genre_noeud);
    auto bloc_si_vrai = assem->crée_bloc_seul(inst->lexeme, inst->bloc_parent);

    auto arrête = assem->crée_arrete(&lexème_arrête);
    arrête->drapeaux |= DrapeauxNoeud::EST_IMPLICITE;
    arrête->boucle_controlee = inst;
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
