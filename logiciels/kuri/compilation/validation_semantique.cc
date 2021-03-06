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

#include "validation_semantique.hh"

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/outils/assert.hh"
#include "biblinternes/outils/garde_portee.h"
#include "biblinternes/structures/file_fixe.hh"

#include "parsage/outils_lexemes.hh"

#include "arbre_syntaxique/assembleuse.hh"
#include "arbre_syntaxique/noeud_expression.hh"

#include "broyage.hh"
#include "compilatrice.hh"
#include "erreur.h"
#include "espace_de_travail.hh"
#include "monomorphisations.hh"
#include "portee.hh"
#include "tacheronne.hh"
#include "validation_expression_appel.hh"

using dls::outils::possede_drapeau;

/* ************************************************************************** */

#define VERIFIE_INTERFACE_KURI_CHARGEE(nom)                                                       \
    if (espace->interface_kuri->decl_##nom == nullptr) {                                          \
        unite->attend_sur_interface_kuri(#nom);                                                   \
        return CodeRetourValidation::Erreur;                                                      \
    }                                                                                             \
    else if (espace->interface_kuri->decl_##nom->corps->unite == nullptr) {                       \
        m_compilatrice.ordonnanceuse->cree_tache_pour_typage(                                     \
            espace, espace->interface_kuri->decl_##nom->corps);                                   \
    }

#define VERIFIE_UNITE_TYPAGE(type)                                                                \
    if (type->est_enum() || type->est_erreur()) {                                                 \
        if (static_cast<TypeEnum *>(type)->decl->unite == nullptr) {                              \
            m_compilatrice.ordonnanceuse->cree_tache_pour_typage(                                 \
                espace, static_cast<TypeEnum *>(type)->decl);                                     \
        }                                                                                         \
    }                                                                                             \
    else if (type->est_structure()) {                                                             \
        if (type->comme_structure()->decl->unite == nullptr) {                                    \
            m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace,                          \
                                                                 type->comme_structure()->decl);  \
        }                                                                                         \
    }                                                                                             \
    else if (type->est_union()) {                                                                 \
        if (type->comme_union()->decl->unite == nullptr) {                                        \
            m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace,                          \
                                                                 type->comme_union()->decl);      \
        }                                                                                         \
    }

ContexteValidationCode::ContexteValidationCode(Compilatrice &compilatrice,
                                               Tacheronne &tacheronne,
                                               UniteCompilation &u)
    : m_compilatrice(compilatrice), m_tacheronne(tacheronne), unite(&u), espace(unite->espace)
{
}

void ContexteValidationCode::commence_fonction(NoeudDeclarationEnteteFonction *fonction)
{
    donnees_dependance.types_utilises.efface();
    donnees_dependance.fonctions_utilisees.efface();
    donnees_dependance.globales_utilisees.efface();
    fonction_courante = fonction;
}

void ContexteValidationCode::termine_fonction()
{
    fonction_courante = nullptr;
}

CodeRetourValidation ContexteValidationCode::valide_semantique_noeud(NoeudExpression *noeud)
{
    switch (noeud->genre) {
        case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
        case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
        case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
        case GenreNoeud::INSTRUCTION_BOUCLE:
        case GenreNoeud::EXPRESSION_VIRGULE:
        case GenreNoeud::DECLARATION_MODULE:
        case GenreNoeud::EXPRESSION_PAIRE_DISCRIMINATION:
        case GenreNoeud::INSTRUCTION_DIFFERE:
        {
            break;
        }
        case GenreNoeud::INSTRUCTION_CHARGE:
        {
            const auto inst = noeud->comme_charge();
            const auto lexeme = inst->expression->lexeme;
            const auto fichier = espace->fichier(inst->lexeme->fichier);
            const auto temps = dls::chrono::compte_seconde();
            m_compilatrice.ajoute_fichier_a_la_compilation(
                espace, lexeme->chaine, fichier->module, inst->expression);
            temps_chargement += temps.temps();
            break;
        }
        case GenreNoeud::INSTRUCTION_IMPORTE:
        {
            const auto inst = noeud->comme_importe();
            const auto lexeme = inst->expression->lexeme;
            const auto fichier = espace->fichier(inst->lexeme->fichier);
            const auto temps = dls::chrono::compte_seconde();
            const auto module = m_compilatrice.importe_module(
                espace, kuri::chaine(lexeme->chaine), inst->expression);
            temps_chargement += temps.temps();

            if (!module) {
                return CodeRetourValidation::Erreur;
            }

            // @concurrence critique
            if (fichier->importe_module(module->nom())) {
                espace->rapporte_avertissement(inst, "Importation superflux du module");
            }
            else if (fichier->module == module) {
                espace->rapporte_erreur(inst, "Importation d'un module dans lui-même !\n");
            }
            else {
                fichier->modules_importes.insere(module);
                auto noeud_module = m_tacheronne.assembleuse
                                        ->cree_noeud<GenreNoeud::DECLARATION_MODULE>(inst->lexeme)
                                        ->comme_declaration_module();
                noeud_module->module = module;
                noeud_module->ident = inst->expression->ident;
                noeud_module->bloc_parent = inst->bloc_parent;
                noeud_module->bloc_parent->membres->ajoute(noeud_module);
                noeud_module->drapeaux |= DECLARATION_FUT_VALIDEE;
            }

            break;
        }
        case GenreNoeud::DECLARATION_ENTETE_FONCTION:
        {
            auto decl = noeud->comme_entete_fonction();

            if (decl->est_declaration_type) {
                aplatis_arbre(decl);
                POUR (decl->arbre_aplatis) {
                    if (valide_semantique_noeud(it) == CodeRetourValidation::Erreur) {
                        return CodeRetourValidation::Erreur;
                    }
                }

                auto requiers_contexte = !decl->possede_drapeau(FORCE_NULCTX);
                auto types_entrees = dls::tablet<Type *, 6>(decl->params.taille() +
                                                            requiers_contexte);

                if (requiers_contexte) {
                    types_entrees[0] = espace->typeuse.type_contexte;
                }

                for (auto i = 0; i < decl->params.taille(); ++i) {
                    NoeudExpression *type_entree = decl->params[i];

                    if (resoud_type_final(type_entree, types_entrees[i + requiers_contexte]) ==
                        CodeRetourValidation::Erreur) {
                        return CodeRetourValidation::Erreur;
                    }
                }

                Type *type_sortie = nullptr;

                if (decl->params_sorties.taille() == 1) {
                    if (resoud_type_final(decl->params_sorties[0], type_sortie) ==
                        CodeRetourValidation::Erreur) {
                        return CodeRetourValidation::Erreur;
                    }
                }
                else {
                    dls::tablet<TypeCompose::Membre, 6> membres;
                    membres.reserve(decl->params_sorties.taille());

                    for (auto &type_declare : decl->params_sorties) {
                        if (resoud_type_final(type_declare, type_sortie) ==
                            CodeRetourValidation::Erreur) {
                            return CodeRetourValidation::Erreur;
                        }

                        // À FAIRE(état validation)
                        if ((type_sortie->drapeaux & TYPE_FUT_VALIDE) == 0) {
                            unite->attend_sur_type(type_sortie);
                            return CodeRetourValidation::Erreur;
                        }

                        membres.ajoute({type_sortie});
                    }

                    type_sortie = espace->typeuse.cree_tuple(membres);
                }

                auto type_fonction = espace->typeuse.type_fonction(types_entrees, type_sortie);
                decl->type = espace->typeuse.type_type_de_donnees(type_fonction);
                return CodeRetourValidation::OK;
            }

            break;
        }
        case GenreNoeud::DECLARATION_CORPS_FONCTION:
        {
            auto decl = noeud->comme_corps_fonction();

            if (decl->entete->est_operateur) {
                return valide_operateur(decl);
            }

            return valide_fonction(decl);
        }
        case GenreNoeud::EXPRESSION_APPEL:
        {
            auto expr = noeud->comme_appel();
            expr->genre_valeur = GenreValeur::DROITE;
            // @réinitialise en cas d'erreurs passées
            expr->parametres_resolus.efface();
            return valide_appel_fonction(m_compilatrice, *espace, *this, expr);
        }
        case GenreNoeud::DIRECTIVE_CUISINE:
        {
            return valide_cuisine(noeud->comme_cuisine());
        }
        case GenreNoeud::DIRECTIVE_EXECUTE:
        {
            auto noeud_directive = noeud->comme_execute();

            // crée une fonction pour l'exécution
            auto decl_entete = m_tacheronne.assembleuse->cree_entete_fonction(noeud->lexeme);
            auto decl_corps = decl_entete->corps;

            decl_entete->bloc_parent = noeud->bloc_parent;
            decl_corps->bloc_parent = noeud->bloc_parent;

            decl_entete->drapeaux |= FORCE_NULCTX;
            decl_entete->est_metaprogramme = true;

            // le type de la fonction est fonc () -> (type_expression)

            auto type_expression = noeud_directive->expression->type;

            if (type_expression->est_tuple()) {
                auto tuple = type_expression->comme_tuple();

                POUR (tuple->membres) {
                    auto decl_sortie = m_tacheronne.assembleuse->cree_declaration_variable(
                        noeud->lexeme);
                    decl_sortie->ident =
                        m_compilatrice.table_identifiants->identifiant_pour_chaine("__ret0");
                    decl_sortie->type = it.type;
                    decl_sortie->drapeaux |= DECLARATION_FUT_VALIDEE;
                    decl_entete->params_sorties.ajoute(decl_sortie);
                }

                decl_entete->param_sortie = m_tacheronne.assembleuse->cree_declaration_variable(
                    noeud->lexeme);
                decl_entete->param_sortie->ident =
                    m_compilatrice.table_identifiants->identifiant_pour_chaine("valeur_de_retour");
                decl_entete->param_sortie->type = type_expression;
            }
            else {
                auto decl_sortie = m_tacheronne.assembleuse->cree_declaration_variable(
                    noeud->lexeme);
                decl_sortie->ident = m_compilatrice.table_identifiants->identifiant_pour_chaine(
                    "__ret0");
                decl_sortie->type = type_expression;
                decl_sortie->drapeaux |= DECLARATION_FUT_VALIDEE;

                decl_entete->params_sorties.ajoute(decl_sortie);
                decl_entete->param_sortie = m_tacheronne.assembleuse->cree_declaration_variable(
                    noeud->lexeme);
                decl_entete->param_sortie->type = type_expression;
            }

            auto types_entrees = dls::tablet<Type *, 6>(0);

            auto type_fonction = espace->typeuse.type_fonction(types_entrees, type_expression);
            decl_entete->type = type_fonction;

            decl_corps->bloc = m_tacheronne.assembleuse->cree_bloc_seul(noeud->lexeme, nullptr);

            static Lexeme lexeme_retourne = {"retourne", {}, GenreLexeme::RETOURNE, 0, 0, 0};
            auto expr_ret = m_tacheronne.assembleuse->cree_retourne(&lexeme_retourne);

            simplifie_arbre(
                espace, m_tacheronne.assembleuse, espace->typeuse, noeud_directive->expression);

            if (type_expression != espace->typeuse[TypeBase::RIEN]) {
                expr_ret->genre = GenreNoeud::INSTRUCTION_RETOUR;
                expr_ret->expression = noeud_directive->expression;

                /* besoin de valider pour mettre en place les informations de retour */
                auto ancienne_fonction_courante = fonction_courante;
                fonction_courante = decl_entete;
                valide_expression_retour(expr_ret);
                fonction_courante = ancienne_fonction_courante;
            }
            else {
                decl_corps->bloc->expressions->ajoute(noeud_directive->expression);
            }

            decl_corps->bloc->expressions->ajoute(expr_ret);

            auto graphe = espace->graphe_dependance.verrou_ecriture();
            auto noeud_dep = graphe->cree_noeud_fonction(decl_entete);
            /* préserve les données, car il nous les faut également pour la fonction_courante */
            graphe->ajoute_dependances(
                *noeud_dep, donnees_dependance, fonction_courante == nullptr);

            decl_entete->drapeaux |= DECLARATION_FUT_VALIDEE;

            auto metaprogramme = espace->cree_metaprogramme();
            metaprogramme->directive = noeud_directive;
            metaprogramme->fonction = decl_entete;

            m_compilatrice.ordonnanceuse->cree_tache_pour_execution(espace, metaprogramme);
            m_compilatrice.ordonnanceuse->cree_tache_pour_generation_ri(espace, decl_corps);

            noeud->type = noeud_directive->expression->type;

            // À FAIRE: ceci ne prend en compte les expressions pour les variables globales
            if (fonction_courante) {
                /* avance l'index car il est inutile de revalider ce noeud */
                unite->index_courant += 1;
                unite->attend_sur_metaprogramme(metaprogramme);
                graphe->cree_noeud_fonction(metaprogramme->fonction);
                graphe->ajoute_dependances(*metaprogramme->fonction->noeud_dependance,
                                           donnees_dependance);
                return CodeRetourValidation::Erreur;
            }

            break;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
        {
            return valide_reference_declaration(noeud->comme_reference_declaration(),
                                                noeud->bloc_parent);
        }
        case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
        {
            auto type_connu = espace->typeuse.type_pour_lexeme(noeud->lexeme->genre);
            auto type_type = espace->typeuse.type_type_de_donnees(type_connu);
            noeud->type = type_type;
            break;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
        {
            auto inst = noeud->comme_reference_membre();
            noeud->genre_valeur = GenreValeur::TRANSCENDANTALE;

            if (valide_acces_membre(inst) == CodeRetourValidation::Erreur) {
                return CodeRetourValidation::Erreur;
            }

            donnees_dependance.types_utilises.insere(inst->type);
            break;
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
        {
            auto inst = noeud->comme_assignation_variable();
            return valide_assignation(inst);
        }
        case GenreNoeud::DECLARATION_VARIABLE:
        {
            return valide_declaration_variable(noeud->comme_declaration_variable());
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
        {
            noeud->genre_valeur = GenreValeur::DROITE;
            noeud->type = espace->typeuse[TypeBase::R32];
            noeud->comme_litterale_reel()->valeur = noeud->lexeme->valeur_reelle;

            donnees_dependance.types_utilises.insere(noeud->type);
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
        {
            noeud->genre_valeur = GenreValeur::DROITE;
            noeud->type = espace->typeuse[TypeBase::ENTIER_CONSTANT];
            noeud->comme_litterale_entier()->valeur = noeud->lexeme->valeur_entiere;

            donnees_dependance.types_utilises.insere(noeud->type);
            break;
        }
        case GenreNoeud::OPERATEUR_BINAIRE:
        case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
        {
            auto expr = static_cast<NoeudExpressionBinaire *>(noeud);
            expr->genre_valeur = GenreValeur::DROITE;
            CHRONO_TYPAGE(m_tacheronne.stats_typage.operateurs_binaire, "opérateur binaire");

            auto enfant1 = expr->operande_gauche;
            auto enfant2 = expr->operande_droite;

            if (expr->lexeme->genre == GenreLexeme::TABLEAU) {
                auto expression_taille = enfant1;
                auto expression_type = enfant2;

                auto type2 = expression_type->type;

                if (type2->genre != GenreType::TYPE_DE_DONNEES) {
                    rapporte_erreur(
                        "Attendu une expression de type après la déclaration de type tableau",
                        enfant2);
                    return CodeRetourValidation::Erreur;
                }

                auto type_de_donnees = type2->comme_type_de_donnees();
                auto type_connu = type_de_donnees->type_connu ? type_de_donnees->type_connu :
                                                                type_de_donnees;

                auto taille_tableau = 0l;

                if (expression_taille) {
                    auto res = evalue_expression(
                        espace, expression_taille->bloc_parent, expression_taille);

                    if (res.est_errone) {
                        rapporte_erreur("Impossible d'évaluer la taille du tableau",
                                        expression_taille);
                        return CodeRetourValidation::Erreur;
                    }

                    if (res.valeur.type != TypeExpression::ENTIER) {
                        rapporte_erreur("L'expression n'est pas de type entier",
                                        expression_taille);
                        return CodeRetourValidation::Erreur;
                    }

                    if (res.valeur.entier == 0) {
                        espace->rapporte_erreur(
                            expression_taille,
                            "Impossible de définir un tableau fixe de taille 0 !\n");
                    }

                    taille_tableau = res.valeur.entier;
                }

                if (taille_tableau != 0) {
                    // À FAIRE: détermine proprement que nous avons un type s'utilisant par valeur
                    // via un membre
                    if ((type_connu->drapeaux & TYPE_FUT_VALIDE) == 0) {
                        unite->attend_sur_type(type_connu);
                        return CodeRetourValidation::Erreur;
                    }

                    auto type_tableau = espace->typeuse.type_tableau_fixe(
                        type_connu, static_cast<int>(taille_tableau));
                    expr->type = espace->typeuse.type_type_de_donnees(type_tableau);
                    donnees_dependance.types_utilises.insere(type_tableau);
                }
                else {
                    auto type_tableau = espace->typeuse.type_tableau_dynamique(type_connu);
                    expr->type = espace->typeuse.type_type_de_donnees(type_tableau);
                    donnees_dependance.types_utilises.insere(type_tableau);
                }

                return CodeRetourValidation::OK;
            }

            auto type_op = expr->lexeme->genre;

            auto assignation_composee = est_assignation_composee(type_op);

            auto type1 = enfant1->type;
            auto type2 = enfant2->type;

            if (type1->genre == GenreType::TYPE_DE_DONNEES) {
                if (type2->genre != GenreType::TYPE_DE_DONNEES) {
                    rapporte_erreur("Opération impossible entre un type et autre chose", expr);
                    return CodeRetourValidation::Erreur;
                }

                auto type_type1 = type1->comme_type_de_donnees();
                auto type_type2 = type2->comme_type_de_donnees();

                switch (expr->lexeme->genre) {
                    default:
                    {
                        rapporte_erreur("Opérateur inapplicable sur des types", expr);
                        return CodeRetourValidation::Erreur;
                    }
                    case GenreLexeme::BARRE:
                    {
                        if (type_type1->type_connu == nullptr) {
                            rapporte_erreur("Opération impossible car le type n'est pas connu",
                                            noeud);
                            return CodeRetourValidation::Erreur;
                        }

                        if (type_type2->type_connu == nullptr) {
                            rapporte_erreur("Opération impossible car le type n'est pas connu",
                                            noeud);
                            return CodeRetourValidation::Erreur;
                        }

                        if (type_type1->type_connu == type_type2->type_connu) {
                            rapporte_erreur(
                                "Impossible de créer une union depuis des types similaires\n",
                                expr);
                            return CodeRetourValidation::Erreur;
                        }

                        if ((type_type1->type_connu->drapeaux & TYPE_FUT_VALIDE) == 0) {
                            unite->attend_sur_type(type_type1->type_connu);
                            return CodeRetourValidation::Erreur;
                        }

                        if ((type_type2->type_connu->drapeaux & TYPE_FUT_VALIDE) == 0) {
                            unite->attend_sur_type(type_type2->type_connu);
                            return CodeRetourValidation::Erreur;
                        }

                        auto membres = dls::tablet<TypeCompose::Membre, 6>(2);
                        membres[0] = {type_type1->type_connu, ID::_0};
                        membres[1] = {type_type2->type_connu, ID::_1};

                        auto type_union = espace->typeuse.union_anonyme(membres);
                        expr->type = espace->typeuse.type_type_de_donnees(type_union);
                        donnees_dependance.types_utilises.insere(type_union);

                        // @concurrence critique
                        if (type_union->decl == nullptr) {
                            static Lexeme lexeme_union = {
                                "anonyme", {}, GenreLexeme::CHAINE_CARACTERE, 0, 0, 0};
                            auto decl_struct = m_tacheronne.assembleuse->cree_structure(
                                &lexeme_union);
                            decl_struct->type = type_union;

                            type_union->decl = decl_struct;

                            m_compilatrice.ordonnanceuse->cree_tache_pour_generation_ri(
                                espace, decl_struct);
                        }

                        return CodeRetourValidation::OK;
                    }
                    case GenreLexeme::EGALITE:
                    {
                        // XXX - aucune raison de prendre un verrou ici
                        auto op = espace->operateurs->op_comp_egal_types;
                        expr->type = op->type_resultat;
                        expr->op = op;
                        donnees_dependance.types_utilises.insere(expr->type);
                        return CodeRetourValidation::OK;
                    }
                    case GenreLexeme::DIFFERENCE:
                    {
                        // XXX - aucune raison de prendre un verrou ici
                        auto op = espace->operateurs->op_comp_diff_types;
                        expr->type = op->type_resultat;
                        expr->op = op;
                        donnees_dependance.types_utilises.insere(expr->type);
                        return CodeRetourValidation::OK;
                    }
                }
            }

            /* détecte a comp b comp c */
            if (est_operateur_comparaison(type_op) &&
                est_operateur_comparaison(enfant1->lexeme->genre)) {
                expr->genre = GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE;
                expr->type = espace->typeuse[TypeBase::BOOL];

                auto enfant_expr = static_cast<NoeudExpressionBinaire *>(enfant1);
                type1 = enfant_expr->operande_droite->type;

                auto candidats = dls::tablet<OperateurCandidat, 10>();
                if (cherche_candidats_operateurs(
                        *espace, *this, type1, type2, type_op, candidats)) {
                    return CodeRetourValidation::Erreur;
                }
                auto meilleur_candidat = OperateurCandidat::nul_const();
                auto poids = 0.0;

                for (auto const &candidat : candidats) {
                    if (candidat.poids > poids) {
                        poids = candidat.poids;
                        meilleur_candidat = &candidat;
                    }
                }

                if (meilleur_candidat == nullptr) {
                    unite->attend_sur_operateur(noeud);
                    return CodeRetourValidation::Erreur;
                }

                expr->op = meilleur_candidat->op;
                transtype_si_necessaire(expr->operande_gauche,
                                        meilleur_candidat->transformation_type1);
                transtype_si_necessaire(expr->operande_droite,
                                        meilleur_candidat->transformation_type2);

                if (!expr->op->est_basique) {
                    auto decl_op = expr->op->decl;
                    donnees_dependance.fonctions_utilisees.insere(expr->op->decl);

                    if (decl_op->corps->unite == nullptr) {
                        m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace,
                                                                             decl_op->corps);
                    }
                }
            }
            else if (dls::outils::est_element(
                         noeud->lexeme->genre, GenreLexeme::BARRE_BARRE, GenreLexeme::ESP_ESP)) {
                if (!est_type_conditionnable(enfant1->type) &&
                    !enfant1->possede_drapeau(ACCES_EST_ENUM_DRAPEAU)) {
                    espace->rapporte_erreur(
                        enfant1,
                        "Expression non conditionnable à gauche de l'opérateur logique !");
                }

                if (!est_type_conditionnable(enfant2->type) &&
                    !enfant2->possede_drapeau(ACCES_EST_ENUM_DRAPEAU)) {
                    espace->rapporte_erreur(
                        enfant2,
                        "Expression non conditionnable à droite de l'opérateur logique !");
                }

                /* Les expressions de types a && b || c ou a || b && c ne sont pas valides
                 * car nous ne pouvons déterminer le bon ordre d'exécution. */
                if (noeud->lexeme->genre == GenreLexeme::BARRE_BARRE) {
                    if (enfant1->lexeme->genre == GenreLexeme::ESP_ESP) {
                        espace
                            ->rapporte_erreur(
                                enfant1,
                                "Utilisation ambigüe de l'opérateur « && » à gauche de « || » !")
                            .ajoute_message("Veuillez utiliser des parenthèses pour clarifier "
                                            "l'ordre des comparisons.");
                    }

                    if (enfant2->lexeme->genre == GenreLexeme::ESP_ESP) {
                        espace
                            ->rapporte_erreur(
                                enfant2,
                                "Utilisation ambigüe de l'opérateur « && » à droite de « || » !")
                            .ajoute_message("Veuillez utiliser des parenthèses pour clarifier "
                                            "l'ordre des comparisons.");
                    }
                }

                noeud->type = espace->typeuse[TypeBase::BOOL];
            }
            else {
                bool type_gauche_est_reference = false;
                if (assignation_composee) {
                    type_op = operateur_pour_assignation_composee(type_op);

                    if (type1->est_reference()) {
                        type_gauche_est_reference = true;
                        type1 = type1->comme_reference()->type_pointe;
                        transtype_si_necessaire(expr->operande_gauche,
                                                TypeTransformation::DEREFERENCE);
                    }
                }

                auto candidats = dls::tablet<OperateurCandidat, 10>();
                if (cherche_candidats_operateurs(
                        *espace, *this, type1, type2, type_op, candidats)) {
                    return CodeRetourValidation::Erreur;
                }
                auto meilleur_candidat = OperateurCandidat::nul_const();
                auto poids = 0.0;

                for (auto const &candidat : candidats) {
                    if (candidat.poids > poids) {
                        poids = candidat.poids;
                        meilleur_candidat = &candidat;
                    }
                }

                if (meilleur_candidat == nullptr) {
                    unite->attend_sur_operateur(noeud);
                    return CodeRetourValidation::Erreur;
                }

                expr->type = meilleur_candidat->op->type_resultat;
                expr->op = meilleur_candidat->op;
                expr->permute_operandes = meilleur_candidat->permute_operandes;

                if (type_gauche_est_reference &&
                    meilleur_candidat->transformation_type1.type != TypeTransformation::INUTILE) {
                    espace->rapporte_erreur(expr->operande_gauche,
                                            "Impossible de transtyper la valeur à gauche pour une "
                                            "assignation composée.");
                }

                transtype_si_necessaire(expr->operande_gauche,
                                        meilleur_candidat->transformation_type1);
                transtype_si_necessaire(expr->operande_droite,
                                        meilleur_candidat->transformation_type2);

                if (!expr->op->est_basique) {
                    auto decl_op = expr->op->decl;
                    donnees_dependance.fonctions_utilisees.insere(decl_op);

                    if (decl_op->corps->unite == nullptr) {
                        m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace,
                                                                             decl_op->corps);
                    }
                }

                if (assignation_composee) {
                    expr->drapeaux |= EST_ASSIGNATION_COMPOSEE;

                    auto resultat = cherche_transformation(*espace, *this, expr->type, type1);

                    if (std::holds_alternative<Attente>(resultat)) {
                        unite->marque_attente(std::get<Attente>(resultat));
                        return CodeRetourValidation::Erreur;
                    }

                    auto transformation = std::get<TransformationType>(resultat);

                    if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                        rapporte_erreur_assignation_type_differents(type1, expr->type, enfant2);
                        return CodeRetourValidation::Erreur;
                    }
                }
            }

            donnees_dependance.types_utilises.insere(expr->type);
            break;
        }
        case GenreNoeud::OPERATEUR_UNAIRE:
        {
            CHRONO_TYPAGE(m_tacheronne.stats_typage.operateurs_unaire, "opérateur unaire");
            auto expr = noeud->comme_expression_unaire();
            expr->genre_valeur = GenreValeur::DROITE;

            auto enfant = expr->operande;
            auto type = enfant->type;

            if (type->est_type_de_donnees() && dls::outils::est_element(expr->lexeme->genre,
                                                                        GenreLexeme::FOIS_UNAIRE,
                                                                        GenreLexeme::ESP_UNAIRE)) {
                auto type_de_donnees = type->comme_type_de_donnees();
                auto type_connu = type_de_donnees->type_connu;

                if (type_connu == nullptr) {
                    type_connu = type_de_donnees;
                }

                if (expr->lexeme->genre == GenreLexeme::FOIS_UNAIRE) {
                    type_connu = espace->typeuse.type_pointeur_pour(type_connu);
                }
                else if (expr->lexeme->genre == GenreLexeme::ESP_UNAIRE) {
                    type_connu = espace->typeuse.type_reference_pour(type_connu);
                }

                noeud->type = espace->typeuse.type_type_de_donnees(type_connu);
                break;
            }

            if (type->genre == GenreType::REFERENCE) {
                type = type_dereference_pour(type);
                transtype_si_necessaire(expr->operande, TypeTransformation::DEREFERENCE);
            }

            if (expr->type == nullptr) {
                if (expr->lexeme->genre == GenreLexeme::FOIS_UNAIRE) {
                    if (!est_valeur_gauche(enfant->genre_valeur)) {
                        rapporte_erreur("Ne peut pas prendre l'adresse d'une valeur-droite.",
                                        enfant);
                        return CodeRetourValidation::Erreur;
                    }

                    expr->type = espace->typeuse.type_pointeur_pour(type);
                }
                else if (expr->lexeme->genre == GenreLexeme::EXCLAMATION) {
                    if (!est_type_conditionnable(enfant->type)) {
                        rapporte_erreur(
                            "Ne peut pas appliquer l'opérateur « ! » au type de l'expression",
                            enfant);
                        return CodeRetourValidation::Erreur;
                    }

                    expr->type = espace->typeuse[TypeBase::BOOL];
                }
                else {
                    if (type->genre == GenreType::ENTIER_CONSTANT) {
                        type = espace->typeuse[TypeBase::Z32];
                        transtype_si_necessaire(
                            expr->operande, {TypeTransformation::CONVERTI_ENTIER_CONSTANT, type});
                    }

                    auto operateurs = espace->operateurs.verrou_lecture();
                    auto op = cherche_operateur_unaire(*operateurs, type, expr->lexeme->genre);

                    if (op == nullptr) {
                        unite->attend_sur_operateur(noeud);
                        return CodeRetourValidation::Erreur;
                    }

                    expr->type = op->type_resultat;
                    expr->op = op;
                }
            }

            donnees_dependance.types_utilises.insere(expr->type);
            break;
        }
        case GenreNoeud::EXPRESSION_INDEXAGE:
        {
            auto expr = noeud->comme_indexage();
            expr->genre_valeur = GenreValeur::TRANSCENDANTALE;

            auto enfant1 = expr->operande_gauche;
            auto enfant2 = expr->operande_droite;
            auto type1 = enfant1->type;
            auto type2 = enfant2->type;

            if (type1->genre == GenreType::REFERENCE) {
                transtype_si_necessaire(expr->operande_gauche, TypeTransformation::DEREFERENCE);
                type1 = type_dereference_pour(type1);
            }

            // À FAIRE : vérifie qu'aucun opérateur ne soit définie sur le type opaque
            if (type1->est_opaque()) {
                type1 = type1->comme_opaque()->type_opacifie;
            }

            switch (type1->genre) {
                case GenreType::VARIADIQUE:
                case GenreType::TABLEAU_DYNAMIQUE:
                {
                    expr->type = type_dereference_pour(type1);
                    VERIFIE_INTERFACE_KURI_CHARGEE(panique_tableau);
                    donnees_dependance.fonctions_utilisees.insere(
                        espace->interface_kuri->decl_panique_tableau);
                    break;
                }
                case GenreType::TABLEAU_FIXE:
                {
                    auto type_tabl = type1->comme_tableau_fixe();
                    expr->type = type_dereference_pour(type1);

                    auto res = evalue_expression(espace, enfant2->bloc_parent, enfant2);

                    if (!res.est_errone) {
                        if (res.valeur.entier >= type_tabl->taille) {
                            rapporte_erreur_acces_hors_limites(
                                enfant2, type_tabl, res.valeur.entier);
                            return CodeRetourValidation::Erreur;
                        }

                        /* nous savons que l'accès est dans les limites,
                         * évite d'émettre le code de vérification */
                        expr->aide_generation_code = IGNORE_VERIFICATION;
                    }

                    if (expr->aide_generation_code != IGNORE_VERIFICATION) {
                        VERIFIE_INTERFACE_KURI_CHARGEE(panique_tableau);
                        donnees_dependance.fonctions_utilisees.insere(
                            espace->interface_kuri->decl_panique_tableau);
                    }

                    break;
                }
                case GenreType::POINTEUR:
                {
                    expr->type = type_dereference_pour(type1);
                    break;
                }
                case GenreType::CHAINE:
                {
                    expr->type = espace->typeuse[TypeBase::Z8];
                    VERIFIE_INTERFACE_KURI_CHARGEE(panique_chaine);
                    donnees_dependance.fonctions_utilisees.insere(
                        espace->interface_kuri->decl_panique_chaine);
                    break;
                }
                default:
                {
                    auto candidats = dls::tablet<OperateurCandidat, 10>();
                    if (cherche_candidats_operateurs(*espace,
                                                     *this,
                                                     type1,
                                                     type2,
                                                     GenreLexeme::CROCHET_OUVRANT,
                                                     candidats)) {
                        return CodeRetourValidation::Erreur;
                    }
                    auto meilleur_candidat = OperateurCandidat::nul_const();
                    auto poids = 0.0;

                    for (auto const &candidat : candidats) {
                        if (candidat.poids > poids) {
                            poids = candidat.poids;
                            meilleur_candidat = &candidat;
                        }
                    }

                    if (meilleur_candidat == nullptr) {
                        unite->attend_sur_operateur(noeud);
                        return CodeRetourValidation::Erreur;
                    }

                    expr->type = meilleur_candidat->op->type_resultat;
                    expr->op = meilleur_candidat->op;
                    expr->permute_operandes = meilleur_candidat->permute_operandes;

                    transtype_si_necessaire(expr->operande_gauche,
                                            meilleur_candidat->transformation_type1);
                    transtype_si_necessaire(expr->operande_droite,
                                            meilleur_candidat->transformation_type2);

                    if (!expr->op->est_basique) {
                        auto decl_op = expr->op->decl;
                        donnees_dependance.fonctions_utilisees.insere(decl_op);

                        if (decl_op->corps->unite == nullptr) {
                            m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace,
                                                                                 decl_op->corps);
                        }
                    }
                }
            }

            auto type_cible = espace->typeuse[TypeBase::Z64];
            auto type_index = enfant2->type;

            if (type_index->est_entier_naturel() || type_index->est_octet()) {
                transtype_si_necessaire(
                    expr->operande_droite,
                    {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_cible});
            }
            else {
                if (type_index->genre == GenreType::ENUM) {
                    type_index = static_cast<TypeEnum *>(type_index)->type_donnees;
                }

                if (transtype_si_necessaire(expr->operande_droite, type_cible) ==
                    CodeRetourValidation::Erreur) {
                    return CodeRetourValidation::Erreur;
                }
            }

            donnees_dependance.types_utilises.insere(expr->type);
            break;
        }
        case GenreNoeud::INSTRUCTION_RETOUR:
        {
            auto inst = noeud->comme_retourne();
            noeud->genre_valeur = GenreValeur::DROITE;
            return valide_expression_retour(inst);
        }
        case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
        {
            noeud->type = espace->typeuse[TypeBase::CHAINE];
            noeud->genre_valeur = GenreValeur::DROITE;
            donnees_dependance.types_utilises.insere(noeud->type);
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
        {
            noeud->genre_valeur = GenreValeur::DROITE;
            noeud->type = espace->typeuse[TypeBase::BOOL];
            noeud->comme_litterale_bool()->valeur = noeud->lexeme->chaine == "vrai";

            donnees_dependance.types_utilises.insere(noeud->type);
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
        {
            noeud->genre_valeur = GenreValeur::DROITE;
            noeud->type = espace->typeuse[TypeBase::Z8];
            noeud->comme_litterale_caractere()->valeur = static_cast<uint32_t>(
                noeud->lexeme->valeur_entiere);
            donnees_dependance.types_utilises.insere(noeud->type);
            break;
        }
        case GenreNoeud::INSTRUCTION_SAUFSI:
        case GenreNoeud::INSTRUCTION_SI:
        {
            auto inst = static_cast<NoeudSi *>(noeud);

            auto type_condition = inst->condition->type;

            if (type_condition == nullptr && !est_operateur_bool(inst->condition->lexeme->genre)) {
                rapporte_erreur("Attendu un opérateur booléen pour la condition", inst->condition);
                return CodeRetourValidation::Erreur;
            }

            if (!est_type_conditionnable(type_condition) &&
                !inst->condition->possede_drapeau(ACCES_EST_ENUM_DRAPEAU)) {
                rapporte_erreur("Impossible de conditionner le type de l'expression 'si'",
                                inst->condition,
                                erreur::Genre::TYPE_DIFFERENTS);
                return CodeRetourValidation::Erreur;
            }

            /* pour les expressions x = si y { z } sinon { w } */
            if (inst->possede_drapeau(DrapeauxNoeud::DROITE_ASSIGNATION)) {
                inst->type = inst->bloc_si_vrai->type;

                // À FAIRE : vérifie que tous les blocs ont le même type

                // vérifie que l'arbre s'arrête sur un sinon
                auto racine = inst;
                while (true) {
                    if (!inst->bloc_si_faux) {
                        espace->rapporte_erreur(racine,
                                                "Bloc « sinon » manquant dans la condition si "
                                                "utilisée comme expression !");
                        return CodeRetourValidation::Erreur;
                    }

                    if (inst->bloc_si_faux->est_si() || inst->bloc_si_faux->est_saufsi()) {
                        inst = static_cast<NoeudSi *>(inst->bloc_si_faux);
                        continue;
                    }

                    break;
                }
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_SI_STATIQUE:
        {
            auto inst = noeud->comme_si_statique();

            if (inst->visite == false) {
                auto res = evalue_expression(espace, inst->bloc_parent, inst->condition);

                if (res.est_errone) {
                    rapporte_erreur(
                        res.message_erreur, res.noeud_erreur, erreur::Genre::VARIABLE_REDEFINIE);
                    return CodeRetourValidation::Erreur;
                }

                auto condition_est_vraie = res.valeur.entier != 0;
                inst->condition_est_vraie = condition_est_vraie;

                if (!condition_est_vraie) {
                    // dis à l'unité de sauter les instructions jusqu'au prochain point
                    unite->index_courant = inst->index_bloc_si_faux;
                }

                inst->visite = true;
            }
            else {
                // dis à l'unité de sauter les instructions jusqu'au prochain point
                unite->index_courant = inst->index_apres;
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_COMPOSEE:
        {
            auto inst = noeud->comme_bloc();

            auto expressions = inst->expressions.verrou_lecture();

            if (expressions->est_vide()) {
                noeud->type = espace->typeuse[TypeBase::RIEN];
            }
            else {
                noeud->type = expressions->a(expressions->taille() - 1)->type;
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_POUR:
        {
            auto inst = noeud->comme_pour();

            auto enfant1 = inst->variable;
            auto enfant2 = inst->expression;
            auto enfant3 = inst->bloc;
            auto feuilles = enfant1->comme_virgule();

            for (auto &f : feuilles->expressions) {
                /* transforme les références en déclarations, nous faisons ça ici et non lors
                 * du syntaxage ou de la simplification de l'arbre afin de prendre en compte
                 * les cas où nous avons une fonction polymorphique : les données des déclarations
                 * ne sont pas copiées */
                f = m_tacheronne.assembleuse->cree_declaration_variable(
                    f->comme_reference_declaration(), nullptr);
                auto decl_f = trouve_dans_bloc(noeud->bloc_parent, f->ident);

                if (decl_f != nullptr) {
                    if (f->lexeme->ligne > decl_f->lexeme->ligne) {
                        rapporte_erreur(
                            "Redéfinition de la variable", f, erreur::Genre::VARIABLE_REDEFINIE);
                        return CodeRetourValidation::Erreur;
                    }
                }
            }

            auto variable = feuilles->expressions[0];
            inst->ident = variable->ident;

            auto requiers_index = feuilles->expressions.taille() == 2;

            auto type = enfant2->type;
            if (type->est_opaque()) {
                type = type->comme_opaque()->type_opacifie;
                enfant2->type = type;
            }

            auto determine_iterande = [&, this](NoeudExpression *iterand) -> char {
                /* NOTE : nous testons le type des noeuds d'abord pour ne pas que le
                 * type de retour d'une coroutine n'interfère avec le type d'une
                 * variable (par exemple quand nous retournons une chaine). */
                if (iterand->est_plage()) {
                    if (requiers_index) {
                        return GENERE_BOUCLE_PLAGE_INDEX;
                    }

                    return GENERE_BOUCLE_PLAGE;
                }

                if (iterand->est_appel()) {
                    auto appel = iterand->comme_appel();
                    auto fonction_appelee = appel->noeud_fonction_appelee;

                    if (fonction_appelee->est_entete_fonction()) {
                        auto entete = fonction_appelee->comme_entete_fonction();

                        if (entete->est_coroutine) {
                            espace->rapporte_erreur(enfant2,
                                                    "Les coroutines ne sont plus supportées dans "
                                                    "le langage pour le moment");
                            //							enfant1->type = enfant2->type;

                            //							df = enfant2->df;
                            //							auto nombre_vars_ret =
                            // df->idx_types_retours.taille();

                            //							if (feuilles.taille() == nombre_vars_ret) {
                            //								requiers_index = false;
                            //								noeud->aide_generation_code =
                            // GENERE_BOUCLE_COROUTINE;
                            //							}
                            //							else if (feuilles.taille() ==
                            //nombre_vars_ret
                            //+ 1) { 								requiers_index = true;
                            // noeud->aide_generation_code = GENERE_BOUCLE_COROUTINE_INDEX;
                            //							}
                            //							else {
                            //								rapporte_erreur(
                            //											"Mauvais compte d'arguments
                            //à déployer", compilatrice,
                            // *enfant1->lexeme);
                            //							}
                        }
                    }
                }

                if (type->genre == GenreType::TABLEAU_DYNAMIQUE ||
                    type->genre == GenreType::TABLEAU_FIXE ||
                    type->genre == GenreType::VARIADIQUE) {
                    type = type_dereference_pour(type);

                    if (requiers_index) {
                        return GENERE_BOUCLE_TABLEAU_INDEX;
                    }
                    else {
                        return GENERE_BOUCLE_TABLEAU;
                    }
                }
                else if (type->genre == GenreType::CHAINE) {
                    type = espace->typeuse[TypeBase::Z8];
                    enfant1->type = type;

                    if (requiers_index) {
                        return GENERE_BOUCLE_TABLEAU_INDEX;
                    }
                    else {
                        return GENERE_BOUCLE_TABLEAU;
                    }
                }
                else if (est_type_entier(type) || type->est_entier_constant()) {
                    if (type->est_entier_constant()) {
                        enfant1->type = espace->typeuse[TypeBase::Z32];
                        type = enfant1->type;
                    }
                    else {
                        enfant1->type = type;
                    }

                    if (requiers_index) {
                        return GENERE_BOUCLE_PLAGE_IMPLICITE_INDEX;
                    }
                    else {
                        return GENERE_BOUCLE_PLAGE_IMPLICITE;
                    }
                }
                else {
                    espace->rapporte_erreur(enfant2, "Le type de la variable n'est pas itérable")
                        .ajoute_message("Note : le type de la variable est ")
                        .ajoute_message(chaine_type(type))
                        .ajoute_message("\n");
                    return -1;
                }
            };

            auto aide_generation_code = determine_iterande(enfant2);

            if (aide_generation_code == -1) {
                return CodeRetourValidation::Erreur;
            }

            /* Le type ne doit plus être un entier_constant après determine_itérande,
             * donc nous pouvons directement l'assigner à enfant2->type.
             * Ceci est nécessaire car la simplification du code accède aux opérateurs
             * selon le type de enfant2. */
            if (enfant2->type->est_entier_constant()) {
                assert(!type->est_entier_constant());
                enfant2->type = type;
            }

            /* il faut attendre de vérifier que le type est itérable avant de prendre cette
             * indication en compte */
            if (inst->prend_reference) {
                type = espace->typeuse.type_reference_pour(type);
            }
            else if (inst->prend_pointeur) {
                type = espace->typeuse.type_pointeur_pour(type);
            }

            noeud->aide_generation_code = aide_generation_code;

            donnees_dependance.types_utilises.insere(type);
            enfant3->membres->reserve(feuilles->expressions.taille());

            auto nombre_feuilles = feuilles->expressions.taille() - requiers_index;

            for (auto i = 0; i < nombre_feuilles; ++i) {
                auto decl_f = feuilles->expressions[i]->comme_declaration_variable();

                decl_f->type = type;
                decl_f->valeur->type = type;
                decl_f->drapeaux |= DECLARATION_FUT_VALIDEE;

                enfant3->membres->ajoute(decl_f);
            }

            if (requiers_index) {
                auto decl_idx = feuilles->expressions[feuilles->expressions.taille() - 1]
                                    ->comme_declaration_variable();

                if (noeud->aide_generation_code == GENERE_BOUCLE_PLAGE_INDEX) {
                    decl_idx->type = espace->typeuse[TypeBase::Z32];
                }
                else {
                    decl_idx->type = espace->typeuse[TypeBase::Z64];
                }

                decl_idx->valeur->type = decl_idx->type;
                decl_idx->drapeaux |= DECLARATION_FUT_VALIDEE;
                enfant3->membres->ajoute(decl_idx);
            }

            break;
        }
        case GenreNoeud::EXPRESSION_COMME:
        {
            auto expr = noeud->comme_comme();
            expr->genre_valeur = GenreValeur::DROITE;

            if (resoud_type_final(expr->expression_type, expr->type) ==
                CodeRetourValidation::Erreur) {
                return CodeRetourValidation::Erreur;
            }

            if (noeud->type == nullptr) {
                rapporte_erreur(
                    "Ne peut transtyper vers un type invalide", expr, erreur::Genre::TYPE_INCONNU);
                return CodeRetourValidation::Erreur;
            }

            donnees_dependance.types_utilises.insere(noeud->type);

            auto enfant = expr->expression;
            if (enfant->type == nullptr) {
                rapporte_erreur(
                    "Ne peut calculer le type d'origine", enfant, erreur::Genre::TYPE_INCONNU);
                return CodeRetourValidation::Erreur;
            }

            if (enfant->type->est_reference() && !noeud->type->est_reference()) {
                transtype_si_necessaire(expr->expression, TypeTransformation::DEREFERENCE);
            }

            auto resultat = cherche_transformation_pour_transtypage(
                *espace, *this, expr->expression->type, noeud->type);

            if (std::holds_alternative<Attente>(resultat)) {
                unite->marque_attente(std::get<Attente>(resultat));
                return CodeRetourValidation::Erreur;
            }

            auto transformation = std::get<TransformationType>(resultat);

            if (transformation.type == TypeTransformation::INUTILE) {
                espace->rapporte_avertissement(expr, "transtypage inutile");
            }

            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_type_arguments(noeud, expr->expression);
                return CodeRetourValidation::Erreur;
            }

            expr->transformation = transformation;

            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NUL:
        {
            noeud->genre_valeur = GenreValeur::DROITE;
            noeud->type = espace->typeuse[TypeBase::PTR_NUL];

            donnees_dependance.types_utilises.insere(noeud->type);
            break;
        }
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        {
            auto expr = noeud->comme_taille_de();
            expr->genre_valeur = GenreValeur::DROITE;
            expr->type = espace->typeuse[TypeBase::N32];

            auto expr_type = expr->expression;
            if (resoud_type_final(expr_type, expr_type->type) == CodeRetourValidation::Erreur) {
                return CodeRetourValidation::Erreur;
            }

            if ((expr_type->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
                /* ce n'est plus la peine de revenir ici une fois que le type sera validé */
                unite->index_courant += 1;
                unite->attend_sur_type(expr_type->type);
                return CodeRetourValidation::Erreur;
            }

            break;
        }
        case GenreNoeud::EXPRESSION_PLAGE:
        {
            auto inst = noeud->comme_plage();
            auto enfant1 = inst->debut;
            auto enfant2 = inst->fin;

            auto type_debut = enfant1->type;
            auto type_fin = enfant2->type;

            assert(type_debut);
            assert(type_fin);

            if (type_debut != type_fin) {
                if (type_debut->genre == GenreType::ENTIER_CONSTANT && est_type_entier(type_fin)) {
                    type_debut = type_fin;
                    enfant1->type = type_debut;
                    transtype_si_necessaire(
                        inst->debut, {TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_debut});
                }
                else if (type_fin->genre == GenreType::ENTIER_CONSTANT &&
                         est_type_entier(type_debut)) {
                    type_fin = type_debut;
                    enfant2->type = type_fin;
                    transtype_si_necessaire(
                        inst->fin, {TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_fin});
                }
                else {
                    rapporte_erreur_type_operation(type_debut, type_fin, noeud);
                    return CodeRetourValidation::Erreur;
                }
            }
            else if (type_debut->genre == GenreType::ENTIER_CONSTANT) {
                type_debut = espace->typeuse[TypeBase::Z32];
                transtype_si_necessaire(
                    inst->debut, {TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_debut});
                transtype_si_necessaire(
                    inst->fin, {TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_debut});
            }

            if (type_debut->genre != GenreType::ENTIER_NATUREL &&
                type_debut->genre != GenreType::ENTIER_RELATIF &&
                type_debut->genre != GenreType::REEL) {
                rapporte_erreur("Attendu des types réguliers dans la plage de la boucle 'pour'",
                                noeud,
                                erreur::Genre::TYPE_DIFFERENTS);
                return CodeRetourValidation::Erreur;
            }

            noeud->type = type_debut;

            break;
        }
        case GenreNoeud::INSTRUCTION_ARRETE:
        {
            return valide_controle_boucle(noeud->comme_arrete());
        }
        case GenreNoeud::INSTRUCTION_CONTINUE:
        {
            return valide_controle_boucle(noeud->comme_continue());
        }
        case GenreNoeud::INSTRUCTION_REPRENDS:
        {
            return valide_controle_boucle(noeud->comme_reprends());
        }
        case GenreNoeud::INSTRUCTION_REPETE:
        {
            auto inst = noeud->comme_repete();
            if (inst->condition->type == nullptr &&
                !est_operateur_bool(inst->condition->lexeme->genre)) {
                rapporte_erreur("Attendu un opérateur booléen pour la condition", inst->condition);
                return CodeRetourValidation::Erreur;
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_TANTQUE:
        {
            auto inst = noeud->comme_tantque();

            if (inst->condition->type == nullptr &&
                !est_operateur_bool(inst->condition->lexeme->genre)) {
                rapporte_erreur("Attendu un opérateur booléen pour la condition", inst->condition);
                return CodeRetourValidation::Erreur;
            }

            if (inst->condition->type->genre != GenreType::BOOL) {
                rapporte_erreur("Une expression booléenne est requise pour la boucle 'tantque'",
                                inst->condition,
                                erreur::Genre::TYPE_ARGUMENT);
                return CodeRetourValidation::Erreur;
            }

            break;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
        {
            auto expr = noeud->comme_construction_tableau();
            noeud->genre_valeur = GenreValeur::DROITE;

            auto feuilles = expr->expression->comme_virgule();

            if (feuilles->expressions.est_vide()) {
                return CodeRetourValidation::OK;
            }

            auto premiere_feuille = feuilles->expressions[0];

            auto type_feuille = premiere_feuille->type;

            if (type_feuille->genre == GenreType::ENTIER_CONSTANT) {
                type_feuille = espace->typeuse[TypeBase::Z32];
                transtype_si_necessaire(
                    feuilles->expressions[0],
                    {TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_feuille});
            }

            for (auto i = 1; i < feuilles->expressions.taille(); ++i) {
                if (transtype_si_necessaire(feuilles->expressions[i], type_feuille) ==
                    CodeRetourValidation::Erreur) {
                    return CodeRetourValidation::Erreur;
                }
            }

            noeud->type = espace->typeuse.type_tableau_fixe(type_feuille,
                                                            feuilles->expressions.taille());

            /* ajoute également le type de pointeur pour la génération de code C */
            auto type_ptr = espace->typeuse.type_pointeur_pour(type_feuille);

            donnees_dependance.types_utilises.insere(noeud->type);
            donnees_dependance.types_utilises.insere(type_ptr);
            break;
        }
        case GenreNoeud::EXPRESSION_INFO_DE:
        {
            auto noeud_expr = noeud->comme_info_de();
            auto expr = noeud_expr->expression;

            if (resoud_type_final(noeud_expr->expression, expr->type) ==
                CodeRetourValidation::Erreur) {
                return CodeRetourValidation::Erreur;
            }

            auto type_info_type = Type::nul();

            switch (expr->type->genre) {
                case GenreType::POLYMORPHIQUE:
                case GenreType::TUPLE:
                {
                    break;
                }
                case GenreType::EINI:
                case GenreType::CHAINE:
                case GenreType::RIEN:
                case GenreType::BOOL:
                case GenreType::OCTET:
                case GenreType::TYPE_DE_DONNEES:
                case GenreType::REEL:
                {
                    type_info_type = espace->typeuse.type_info_type_;
                    break;
                }
                case GenreType::ENTIER_CONSTANT:
                case GenreType::ENTIER_NATUREL:
                case GenreType::ENTIER_RELATIF:
                {
                    type_info_type = espace->typeuse.type_info_type_entier;
                    break;
                }
                case GenreType::REFERENCE:
                case GenreType::POINTEUR:
                {
                    type_info_type = espace->typeuse.type_info_type_pointeur;
                    break;
                }
                case GenreType::STRUCTURE:
                {
                    type_info_type = espace->typeuse.type_info_type_structure;
                    break;
                }
                case GenreType::UNION:
                {
                    type_info_type = espace->typeuse.type_info_type_union;
                    break;
                }
                case GenreType::VARIADIQUE:
                case GenreType::TABLEAU_DYNAMIQUE:
                case GenreType::TABLEAU_FIXE:
                {
                    type_info_type = espace->typeuse.type_info_type_tableau;
                    break;
                }
                case GenreType::FONCTION:
                {
                    type_info_type = espace->typeuse.type_info_type_fonction;
                    break;
                }
                case GenreType::ENUM:
                case GenreType::ERREUR:
                {
                    type_info_type = espace->typeuse.type_info_type_enum;
                    break;
                }
                case GenreType::OPAQUE:
                {
                    type_info_type = espace->typeuse.type_info_type_opaque;
                    break;
                }
            }

            noeud->genre_valeur = GenreValeur::DROITE;
            noeud->type = espace->typeuse.type_pointeur_pour(type_info_type);

            break;
        }
        case GenreNoeud::EXPRESSION_INIT_DE:
        {
            auto init_de = noeud->comme_init_de();
            Type *type = nullptr;

            if (resoud_type_final(init_de->expression, type) == CodeRetourValidation::Erreur) {
                rapporte_erreur("impossible de définir le type de init_de", noeud);
                return CodeRetourValidation::Erreur;
            }

            if (type->genre != GenreType::STRUCTURE && type->genre != GenreType::UNION) {
                rapporte_erreur("init_de doit prendre le type d'une structure ou d'une union",
                                noeud);
                return CodeRetourValidation::Erreur;
            }

            auto types_entrees = dls::tablet<Type *, 6>(2);
            types_entrees[0] = espace->typeuse.type_contexte;
            types_entrees[1] = espace->typeuse.type_pointeur_pour(type);

            auto type_fonction = espace->typeuse.type_fonction(types_entrees,
                                                               espace->typeuse[TypeBase::RIEN]);
            noeud->type = type_fonction;

            donnees_dependance.types_utilises.insere(noeud->type);

            noeud->genre_valeur = GenreValeur::DROITE;
            break;
        }
        case GenreNoeud::EXPRESSION_TYPE_DE:
        {
            auto expr = noeud->comme_type_de();
            auto expr_type = expr->expression;

            if (expr_type->type == nullptr) {
                rapporte_erreur("impossible de définir le type de l'expression de type_de",
                                expr_type);
                return CodeRetourValidation::Erreur;
            }

            if (expr_type->type->genre == GenreType::TYPE_DE_DONNEES) {
                noeud->type = expr_type->type;
            }
            else {
                noeud->type = espace->typeuse.type_type_de_donnees(expr_type->type);
            }

            break;
        }
        case GenreNoeud::EXPRESSION_MEMOIRE:
        {
            auto expr = noeud->comme_memoire();

            auto type = expr->expression->type;

            if (type->genre != GenreType::POINTEUR) {
                rapporte_erreur("Un pointeur est requis pour le déréférencement via 'mémoire'",
                                expr->expression,
                                erreur::Genre::TYPE_DIFFERENTS);
                return CodeRetourValidation::Erreur;
            }

            auto type_pointeur = type->comme_pointeur();
            noeud->genre_valeur = GenreValeur::TRANSCENDANTALE;
            noeud->type = type_pointeur->type_pointe;

            break;
        }
        case GenreNoeud::DECLARATION_STRUCTURE:
        {
            return valide_structure(noeud->comme_structure());
        }
        case GenreNoeud::DECLARATION_ENUM:
        {
            return valide_enum(noeud->comme_enum());
        }
        case GenreNoeud::INSTRUCTION_DISCR:
        case GenreNoeud::INSTRUCTION_DISCR_ENUM:
        case GenreNoeud::INSTRUCTION_DISCR_UNION:
        {
            auto inst = noeud->comme_discr();

            auto expression = inst->expression_discriminee;

            auto type = expression->type;

            if (type->genre == GenreType::REFERENCE) {
                transtype_si_necessaire(inst->expression_discriminee,
                                        TypeTransformation::DEREFERENCE);
                type = type->comme_reference()->type_pointe;
            }

            if ((type->drapeaux & TYPE_FUT_VALIDE) == 0) {
                VERIFIE_UNITE_TYPAGE(type)
                unite->attend_sur_type(type);
                return CodeRetourValidation::Erreur;
            }

            if (type->genre == GenreType::UNION && type->comme_union()->est_anonyme) {
                // vérifie que tous les membres sont discriminés
                auto type_union = type->comme_union();
                inst->op = espace->typeuse[TypeBase::Z32]->operateur_egt;

                auto membres_rencontres = dls::ensemblon<Type *, 16>();

                noeud->genre = GenreNoeud::INSTRUCTION_DISCR_UNION;

                for (int i = 0; i < inst->paires_discr.taille(); ++i) {
                    auto expr_paire =
                        inst->paires_discr[i]->expression->comme_virgule()->expressions[0];

                    valide_semantique_noeud(expr_paire);

                    Type *type_expr;
                    if (resoud_type_final(expr_paire, type_expr) == CodeRetourValidation::Erreur) {
                        rapporte_erreur("Ne peut résoudre le type", expr_paire);
                        return CodeRetourValidation::Erreur;
                    }

                    expr_paire->type = type_expr;

                    auto trouve = false;

                    POUR (type_union->membres) {
                        if (it.type == type_expr) {
                            membres_rencontres.insere(type_expr);
                            trouve = true;
                            break;
                        }
                    }

                    if (!trouve) {
                        rapporte_erreur("Le type n'est pas membre de l'union", expr_paire);
                    }

                    /* À FAIRE(union) : ajoute la variable dans le bloc suivant, il nous faudra un
                     * système de capture dans le cas où la variable est un accès */
                }

                if (inst->bloc_sinon == nullptr) {
                    // return valide_presence_membres();
                }
            }
            else if (type->genre == GenreType::UNION) {
                auto type_union = type->comme_union();
                auto decl = type_union->decl;
                inst->op = espace->typeuse[TypeBase::Z32]->operateur_egt;

                if (decl->est_nonsure) {
                    rapporte_erreur("« discr » ne peut prendre une union nonsûre", expression);
                    return CodeRetourValidation::Erreur;
                }

                auto membres_rencontres = dls::ensemblon<IdentifiantCode *, 16>();

                auto valide_presence_membres =
                    [&membres_rencontres, &type_union, this, &expression]() {
                        auto valeurs_manquantes = dls::ensemble<kuri::chaine_statique>();

                        POUR (type_union->membres) {
                            if (!membres_rencontres.possede(it.nom)) {
                                valeurs_manquantes.insere(it.nom->nom);
                            }
                        }

                        if (valeurs_manquantes.taille() != 0) {
                            rapporte_erreur_valeur_manquante_discr(expression, valeurs_manquantes);
                            return CodeRetourValidation::Erreur;
                        }

                        return CodeRetourValidation::OK;
                    };

                noeud->genre = GenreNoeud::INSTRUCTION_DISCR_UNION;

                for (int i = 0; i < inst->paires_discr.taille(); ++i) {
                    auto expr_paire =
                        inst->paires_discr[i]->expression->comme_virgule()->expressions[0];
                    auto bloc_paire = inst->paires_discr[i]->bloc;

                    /* vérifie que toutes les expressions des paires sont bel et
                     * bien des membres */
                    if (expr_paire->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
                        espace
                            ->rapporte_erreur(expr_paire,
                                              "Attendu une référence à un membre de l'union")
                            .ajoute_message(
                                "L'expression est de genre : ", expr_paire->genre, "\n");
                        return CodeRetourValidation::Erreur;
                    }

                    if (membres_rencontres.possede(expr_paire->ident)) {
                        rapporte_erreur("Redéfinition de l'expression", expr_paire);
                        return CodeRetourValidation::Erreur;
                    }

                    membres_rencontres.insere(expr_paire->ident);

                    auto decl_var = trouve_dans_bloc_seul(decl->bloc, expr_paire);

                    if (decl_var == nullptr) {
                        rapporte_erreur_membre_inconnu(noeud, expression, expr_paire, type_union);
                        return CodeRetourValidation::Erreur;
                    }

                    auto decl_prec = trouve_dans_bloc(inst->bloc_parent, expr_paire->ident);

                    /* Pousse la variable comme étant employée, puisque nous savons ce qu'elle est
                     */
                    if (decl_prec != nullptr) {
                        rapporte_erreur("Ne peut pas utiliser implicitement le membre car une "
                                        "variable de ce nom existe déjà",
                                        expr_paire);
                        return CodeRetourValidation::Erreur;
                    }

                    /* pousse la variable dans le bloc suivant */
                    auto decl_expr = m_tacheronne.assembleuse->cree_declaration_variable(
                        expr_paire->lexeme);
                    decl_expr->ident = expr_paire->ident;
                    decl_expr->lexeme = expr_paire->lexeme;
                    decl_expr->bloc_parent = bloc_paire;
                    decl_expr->drapeaux |= EMPLOYE;
                    decl_expr->type = decl_var->type;
                    // À FAIRE: il semblerait que l'absence de ceci ajout une tache de typage
                    decl_expr->drapeaux |= DECLARATION_FUT_VALIDEE;
                    // À FAIRE(emploi): mise en place des informations d'emploi

                    bloc_paire->membres->ajoute(decl_expr);
                }

                if (inst->bloc_sinon == nullptr) {
                    return valide_presence_membres();
                }
            }
            else if (type->genre == GenreType::ENUM || type->genre == GenreType::ERREUR) {
                auto type_enum = static_cast<TypeEnum *>(type);
                inst->op = type_enum->operateur_egt;

                auto membres_rencontres = dls::ensemblon<IdentifiantCode *, 16>();
                noeud->genre = GenreNoeud::INSTRUCTION_DISCR_ENUM;

                for (int i = 0; i < inst->paires_discr.taille(); ++i) {
                    auto expr_paire = inst->paires_discr[i]->expression;

                    auto feuilles = expr_paire->comme_virgule();

                    for (auto f : feuilles->expressions) {
                        if (f->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
                            rapporte_erreur(
                                "expression inattendue dans la discrimination, seules les "
                                "références de déclarations sont supportées pour le moment",
                                f);
                            return CodeRetourValidation::Erreur;
                        }

                        auto nom_membre = f->ident;

                        auto nom_trouve = false;

                        POUR (type_enum->membres) {
                            if (it.nom == nom_membre) {
                                nom_trouve = true;
                                break;
                            }
                        }

                        if (!nom_trouve) {
                            rapporte_erreur_membre_inconnu(noeud, expression, f, type_enum);
                            return CodeRetourValidation::Erreur;
                        }

                        if (membres_rencontres.possede(nom_membre)) {
                            rapporte_erreur("Redéfinition de l'expression", f);
                            return CodeRetourValidation::Erreur;
                        }

                        membres_rencontres.insere(nom_membre);
                    }
                }

                if (inst->bloc_sinon == nullptr) {
                    auto valeurs_manquantes = dls::ensemble<kuri::chaine_statique>();

                    POUR (type_enum->membres) {
                        if (!membres_rencontres.possede(it.nom) &&
                            (it.drapeaux & TypeCompose::Membre::EST_IMPLICITE) == 0) {
                            valeurs_manquantes.insere(it.nom->nom);
                        }
                    }

                    if (valeurs_manquantes.taille() != 0) {
                        rapporte_erreur_valeur_manquante_discr(expression, valeurs_manquantes);
                        return CodeRetourValidation::Erreur;
                    }
                }
            }
            else {
                auto type_pour_la_recherche = type;
                if (type->genre == GenreType::TYPE_DE_DONNEES) {
                    type_pour_la_recherche = espace->typeuse.type_type_de_donnees_;
                }

                auto candidats = dls::tablet<OperateurCandidat, 10>();
                if (cherche_candidats_operateurs(*espace,
                                                 *this,
                                                 type_pour_la_recherche,
                                                 type_pour_la_recherche,
                                                 GenreLexeme::EGALITE,
                                                 candidats)) {
                    return CodeRetourValidation::Erreur;
                }

                auto meilleur_candidat = OperateurCandidat::nul_const();
                auto poids = 0.0;

                for (auto const &candidat : candidats) {
                    if (candidat.poids > poids) {
                        poids = candidat.poids;
                        meilleur_candidat = &candidat;
                    }
                }

                if (meilleur_candidat == nullptr) {
                    ::rapporte_erreur(unite->espace,
                                      noeud,
                                      "Je ne peux pas valider l'expression de discrimination car "
                                      "je n'arrive à trouver un opérateur de comparaison pour le "
                                      "type de l'expression. ")
                        .ajoute_message("Le type de l'expression est : ")
                        .ajoute_message(chaine_type(type))
                        .ajoute_message(".\n\n")
                        .ajoute_conseil(
                            "Les discriminations ont besoin d'un opérateur « == » défini pour le "
                            "type afin de pouvoir comparer les valeurs,"
                            " donc si vous voulez utiliser une discrimination sur un type "
                            "personnalisé, vous pouvez définir l'opérateur comme ceci :\n\n"
                            "\topérateur == :: fonc (a: MonType, b: MonType) -> bool\n\t{\n\t\t "
                            "/* logique de comparaison */\n\t}\n");
                    return CodeRetourValidation::Erreur;
                }

                inst->op = meilleur_candidat->op;

                if (!inst->op->est_basique) {
                    donnees_dependance.fonctions_utilisees.insere(inst->op->decl);
                }

                for (int i = 0; i < inst->paires_discr.taille(); ++i) {
                    auto expr_paire = inst->paires_discr[i]->expression;

                    auto feuilles = expr_paire->comme_virgule();

                    for (auto j = 0; j < feuilles->expressions.taille(); ++j) {
                        if (valide_semantique_noeud(feuilles->expressions[j]) ==
                            CodeRetourValidation::Erreur) {
                            return CodeRetourValidation::Erreur;
                        }

                        if (transtype_si_necessaire(feuilles->expressions[j], expression->type) ==
                            CodeRetourValidation::Erreur) {
                            return CodeRetourValidation::Erreur;
                        }
                    }
                }

                if (inst->bloc_sinon == nullptr) {
                    espace->rapporte_erreur(noeud,
                                            "Les discriminations de valeurs scalaires doivent "
                                            "avoir un bloc « sinon »");
                }
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_RETIENS:
        {
            if (!fonction_courante->est_coroutine) {
                rapporte_erreur("'retiens' hors d'une coroutine", noeud);
                return CodeRetourValidation::Erreur;
            }

            return valide_expression_retour(static_cast<NoeudRetour *>(noeud));
        }
        case GenreNoeud::EXPRESSION_PARENTHESE:
        {
            auto expr = noeud->comme_parenthese();
            noeud->type = expr->expression->type;
            noeud->genre_valeur = expr->expression->genre_valeur;
            break;
        }
        case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
        {
            auto inst = noeud->comme_pousse_contexte();
            auto variable = inst->expression;

            if (variable->type != espace->typeuse.type_contexte) {
                espace
                    ->rapporte_erreur(variable, "La variable doit être de type ContexteProgramme")
                    .ajoute_message("Note : la variable est de type ")
                    .ajoute_message(chaine_type(variable->type))
                    .ajoute_message("\n");
                return CodeRetourValidation::Erreur;
            }

            break;
        }
        case GenreNoeud::EXPANSION_VARIADIQUE:
        {
            auto expr = noeud->comme_expansion_variadique();

            if (expr->expression == nullptr) {
                // nous avons un type variadique
                auto type_var = espace->typeuse.type_variadique(nullptr);
                expr->type = espace->typeuse.type_type_de_donnees(type_var);
                return CodeRetourValidation::OK;
            }

            auto type_expr = expr->expression->type;

            if (type_expr->genre == GenreType::TYPE_DE_DONNEES) {
                auto type_de_donnees = type_expr->comme_type_de_donnees();
                auto type_var = espace->typeuse.type_variadique(type_de_donnees->type_connu);
                expr->type = espace->typeuse.type_type_de_donnees(type_var);
            }
            else {
                if (!dls::outils::est_element(type_expr->genre,
                                              GenreType::TABLEAU_FIXE,
                                              GenreType::TABLEAU_DYNAMIQUE,
                                              GenreType::VARIADIQUE)) {
                    espace
                        ->rapporte_erreur(expr,
                                          "Type invalide pour l'expansion variadique, je requiers "
                                          "un type de tableau ou un type variadique")
                        .ajoute_message("Note : le type de l'expression est ")
                        .ajoute_message(chaine_type(type_expr))
                        .ajoute_message("\n");
                }

                if (type_expr->est_tableau_fixe()) {
                    auto type_tableau_fixe = type_expr->comme_tableau_fixe();
                    type_expr = espace->typeuse.type_tableau_dynamique(
                        type_tableau_fixe->type_pointe);
                    transtype_si_necessaire(expr->expression,
                                            {TypeTransformation::CONVERTI_TABLEAU, type_expr});
                }

                expr->type = type_expr;
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_TENTE:
        {
            auto inst = noeud->comme_tente();
            inst->type = inst->expression_appelee->type;
            inst->genre_valeur = GenreValeur::DROITE;

            auto type_de_l_erreur = Type::nul();

            // voir ce que l'on retourne
            // - si aucun type erreur -> erreur ?
            // - si erreur seule -> il faudra vérifier l'erreur
            // - si union -> voir si l'union est sûre et contient une erreur, dépaquete celle-ci
            // dans le génération de code

            if ((inst->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
                VERIFIE_UNITE_TYPAGE(inst->type)
                unite->attend_sur_type(inst->type);
                return CodeRetourValidation::Erreur;
            }

            if (inst->type->genre == GenreType::ERREUR) {
                type_de_l_erreur = inst->type;
            }
            else if (inst->type->genre == GenreType::UNION) {
                auto type_union = inst->type->comme_union();
                auto possede_type_erreur = false;

                POUR (type_union->membres) {
                    if (it.type->genre == GenreType::ERREUR) {
                        possede_type_erreur = true;
                    }
                }

                if (!possede_type_erreur) {
                    rapporte_erreur(
                        "Utilisation de « tente » sur une fonction qui ne retourne pas d'erreur",
                        inst);
                    return CodeRetourValidation::Erreur;
                }

                if (type_union->membres.taille() == 2) {
                    if (type_union->membres[0].type->genre == GenreType::ERREUR) {
                        type_de_l_erreur = type_union->membres[0].type;
                        inst->type = type_union->membres[1].type;
                    }
                    else {
                        inst->type = type_union->membres[0].type;
                        type_de_l_erreur = type_union->membres[1].type;
                    }
                }
                else {
                    espace
                        ->rapporte_erreur(inst,
                                          "Les instructions tentes ne sont pas encore définies "
                                          "pour les unions n'ayant pas 2 membres uniquement.")
                        .ajoute_message("Le type du l'union est ")
                        .ajoute_message(chaine_type(type_union))
                        .ajoute_message("\n");
                }
            }
            else {
                rapporte_erreur(
                    "Utilisation de « tente » sur une fonction qui ne retourne pas d'erreur",
                    inst);
                return CodeRetourValidation::Erreur;
            }

            if (inst->expression_piegee) {
                if (inst->expression_piegee->genre !=
                    GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
                    rapporte_erreur("Expression inattendu dans l'expression de piège, nous devons "
                                    "avoir une référence à une variable",
                                    inst->expression_piegee);
                    return CodeRetourValidation::Erreur;
                }

                auto var_piege = inst->expression_piegee->comme_reference_declaration();

                auto decl = trouve_dans_bloc(var_piege->bloc_parent, var_piege->ident);

                if (decl != nullptr) {
                    rapporte_erreur_redefinition_symbole(var_piege, decl);
                }

                var_piege->type = type_de_l_erreur;

                auto decl_var_piege = m_tacheronne.assembleuse->cree_declaration_variable(
                    var_piege->lexeme);
                decl_var_piege->bloc_parent = inst->bloc;
                decl_var_piege->valeur = var_piege;
                decl_var_piege->type = var_piege->type;
                decl_var_piege->ident = var_piege->ident;
                decl_var_piege->drapeaux |= DECLARATION_FUT_VALIDEE;

                inst->expression_piegee->comme_reference_declaration()->declaration_referee =
                    decl_var_piege;

                // ne l'ajoute pas aux expressions, car nous devons l'initialiser manuellement
                inst->bloc->membres->pousse_front(decl_var_piege);

                auto di = derniere_instruction(inst->bloc);

                if (di == nullptr || !dls::outils::est_element(di->genre,
                                                               GenreNoeud::INSTRUCTION_RETOUR,
                                                               GenreNoeud::INSTRUCTION_ARRETE,
                                                               GenreNoeud::INSTRUCTION_CONTINUE,
                                                               GenreNoeud::INSTRUCTION_REPRENDS)) {
                    rapporte_erreur("Un bloc de piège doit obligatoirement retourner, ou si dans "
                                    "une boucle, la continuer, l'arrêter, ou la reprendre",
                                    inst);
                    return CodeRetourValidation::Erreur;
                }
            }
            else {
                VERIFIE_INTERFACE_KURI_CHARGEE(panique_erreur);
                donnees_dependance.fonctions_utilisees.insere(
                    espace->interface_kuri->decl_panique_erreur);
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_EMPL:
        {
            auto empl = noeud->comme_empl();
            auto decl = empl->expression->comme_declaration_variable();

            empl->type = decl->type;
            decl->drapeaux |= EMPLOYE;
            auto type_employe = decl->type;

            // permet le déréférencement de pointeur, mais uniquement sur un niveau
            if (type_employe->est_pointeur() || type_employe->est_reference()) {
                type_employe = type_dereference_pour(type_employe);
            }

            if (type_employe->genre != GenreType::STRUCTURE) {
                ::rapporte_erreur(unite->espace,
                                  decl,
                                  "Impossible d'employer une variable n'étant pas une structure.")
                    .ajoute_message("Le type de la variable est : ")
                    .ajoute_message(chaine_type(type_employe))
                    .ajoute_message(".\n\n");
                return CodeRetourValidation::Erreur;
            }

            if ((type_employe->drapeaux & TYPE_FUT_VALIDE) == 0) {
                VERIFIE_UNITE_TYPAGE(type_employe)
                unite->attend_sur_type(type_employe);
                return CodeRetourValidation::Erreur;
            }

            auto type_structure = type_employe->comme_structure();

            auto index_membre = 0;

            // pour les structures, prend le bloc_parent qui sera celui de la structure
            auto bloc_parent = decl->bloc_parent;

            // pour les fonctions, utilisent leurs blocs si le bloc_parent est le bloc_parent de la
            // fonction (ce qui est le cas pour les paramètres...)
            if (fonction_courante && bloc_parent == fonction_courante->corps->bloc->bloc_parent) {
                bloc_parent = fonction_courante->corps->bloc;
            }

            POUR (type_structure->membres) {
                if (it.drapeaux & TypeCompose::Membre::EST_CONSTANT) {
                    continue;
                }

                auto decl_membre = m_tacheronne.assembleuse->cree_declaration_variable(
                    decl->lexeme);
                decl_membre->ident = it.nom;
                decl_membre->type = it.type;
                decl_membre->bloc_parent = bloc_parent;
                decl_membre->drapeaux |= DECLARATION_FUT_VALIDEE;
                decl_membre->declaration_vient_d_un_emploi = decl;
                decl_membre->index_membre_employe = index_membre++;
                decl_membre->expression = it.expression_valeur_defaut;

                bloc_parent->membres->ajoute(decl_membre);
            }
            break;
        }
    }

    return CodeRetourValidation::OK;
}

CodeRetourValidation ContexteValidationCode::valide_acces_membre(
    NoeudExpressionMembre *expression_membre)
{
    auto structure = expression_membre->accedee;
    auto membre = expression_membre->membre;

    if (structure->est_reference_declaration()) {
        auto decl = structure->comme_reference_declaration()->declaration_referee;

        if (decl->est_declaration_module()) {
            auto ref = membre->comme_reference_declaration();
            auto res = valide_reference_declaration(
                ref, decl->comme_declaration_module()->module->bloc);

            if (res != CodeRetourValidation::OK) {
                return res;
            }

            expression_membre->type = membre->type;
            expression_membre->genre_valeur = membre->genre_valeur;
            return res;
        }
    }

    auto type = structure->type;

    /* nous pouvons avoir une référence d'un pointeur, donc déréférence au plus */
    while (type->genre == GenreType::POINTEUR || type->genre == GenreType::REFERENCE) {
        type = type_dereference_pour(type);
    }

    if (type->est_opaque()) {
        type = type->comme_opaque()->type_opacifie;
    }

    // Il est possible d'avoir une chaine de type : Struct1.Struct2.Struct3...
    if (type->genre == GenreType::TYPE_DE_DONNEES) {
        auto type_de_donnees = type->comme_type_de_donnees();

        if (type_de_donnees->type_connu != nullptr) {
            type = type_de_donnees->type_connu;
            // change le type de la structure également pour simplifier la génération
            // de la RI (nous nous basons sur le type pour ça)
            structure->type = type;
        }
    }

    if (est_type_compose(type)) {
        if ((type->drapeaux & TYPE_FUT_VALIDE) == 0) {
            VERIFIE_UNITE_TYPAGE(type)
            unite->attend_sur_type(type);
            return CodeRetourValidation::Erreur;
        }

        auto type_compose = static_cast<TypeCompose *>(type);

        auto membre_trouve = false;
        auto index_membre = 0;
        auto membre_est_constant = false;
        auto membre_est_implicite = false;

        POUR (type_compose->membres) {
            if (it.nom == membre->ident) {
                expression_membre->type = it.type;
                membre_trouve = true;
                membre_est_constant = it.drapeaux == TypeCompose::Membre::EST_CONSTANT;
                membre_est_implicite = it.drapeaux == TypeCompose::Membre::EST_IMPLICITE;
                break;
            }

            index_membre += 1;
        }

        if (membre_trouve == false) {
            rapporte_erreur_membre_inconnu(expression_membre, structure, membre, type_compose);
            return CodeRetourValidation::Erreur;
        }

        expression_membre->index_membre = index_membre;

        if (type->genre == GenreType::ENUM || type->genre == GenreType::ERREUR) {
            expression_membre->genre_valeur = GenreValeur::DROITE;

            /* Nous voulons détecter les accès à des constantes d'énumérations via une variable,
             * mais nous devons également prendre en compte le fait que la variable peut-être une
             * référence à un type énumération.
             *
             * Par exemple :
             * - a.CONSTANTE, où a est une variable qui n'est pas de type énum_drapeau -> erreur de
             * compilation
             * - MonÉnum.CONSTANTE, où MonÉnum est un type -> OK
             */
            if (structure->est_reference_declaration() &&
                !structure->comme_reference_declaration()->declaration_referee->est_enum() &&
                !expression_membre->type->est_type_de_donnees()) {
                if (type->est_enum() && static_cast<TypeEnum *>(type)->est_drapeau) {
                    expression_membre->genre_valeur = GenreValeur::TRANSCENDANTALE;
                    if (!membre_est_implicite) {
                        expression_membre->drapeaux |= ACCES_EST_ENUM_DRAPEAU;
                    }
                }
                else {
                    espace->rapporte_erreur(
                        expression_membre,
                        "Impossible d'accéder à une variable de type énumération");
                    return CodeRetourValidation::Erreur;
                }
            }
        }
        else if (membre_est_constant) {
            expression_membre->genre_valeur = GenreValeur::DROITE;
        }
        else if (type->genre == GenreType::UNION) {
            expression_membre->genre = GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION;

            VERIFIE_INTERFACE_KURI_CHARGEE(panique_membre_union);
            donnees_dependance.fonctions_utilisees.insere(
                espace->interface_kuri->decl_panique_membre_union);
        }

        return CodeRetourValidation::OK;
    }

    espace
        ->rapporte_erreur(structure,
                          "Impossible de référencer un membre d'un type n'étant pas une structure")
        .ajoute_message("Note: le type est « ", chaine_type(type), " »");
    return CodeRetourValidation::Erreur;
}

CodeRetourValidation ContexteValidationCode::valide_type_fonction(
    NoeudDeclarationEnteteFonction *decl)
{
#ifdef STATISTIQUES_DETAILLEES
    auto possede_erreur = true;
    dls::chrono::chrono_rappel_milliseconde chrono_([&](double temps) {
        if (possede_erreur) {
            m_tacheronne.stats_typage.fonctions.fusionne_entree({"tentatives râtées", temps});
        }
    });
#endif

    CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide_type_fonction");
    commence_fonction(decl);

    auto &graphe = espace->graphe_dependance;
    auto noeud_dep = graphe->cree_noeud_fonction(decl);

    /* Valide les constantes polymorphiques. */
    if (decl->est_polymorphe) {
        decl->bloc_constantes->membres.avec_verrou_ecriture(
            [this](kuri::tableau<NoeudDeclaration *, int> &membres) {
                POUR (membres) {
                    auto type_poly = espace->typeuse.cree_polymorphique(it->ident);
                    it->type = espace->typeuse.type_type_de_donnees(type_poly);
                    it->drapeaux |= DECLARATION_FUT_VALIDEE;
                }
            });

        if (!decl->monomorphisations) {
            decl->monomorphisations =
                m_tacheronne.allocatrice_noeud.cree_monomorphisations_fonction();
        }
    }

    {
        CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide_type_fonction (arbre aplatis)");
        if (valide_arbre_aplatis(decl, decl->arbre_aplatis) == CodeRetourValidation::Erreur) {
            graphe->ajoute_dependances(*noeud_dep, donnees_dependance);
            return CodeRetourValidation::Erreur;
        }
    }

    // -----------------------------------
    {
        CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions,
                      "valide_type_fonction (validation paramètres)");
        auto noms = dls::ensemblon<IdentifiantCode *, 16>();
        auto dernier_est_variadic = false;

        for (auto i = 0; i < decl->params.taille(); ++i) {
            if (!decl->params[i]->est_declaration_variable() && !decl->params[i]->est_empl()) {
                unite->espace->rapporte_erreur(
                    decl->params[i], "Le paramètre n'est ni une déclaration, ni un emploi");
                return CodeRetourValidation::Erreur;
            }

            auto param = decl->parametre_entree(i);
            auto variable = param->valeur;
            auto expression = param->expression;

            if (noms.possede(variable->ident)) {
                rapporte_erreur(
                    "Redéfinition de l'argument", variable, erreur::Genre::ARGUMENT_REDEFINI);
                return CodeRetourValidation::Erreur;
            }

            if (dernier_est_variadic) {
                rapporte_erreur("Argument déclaré après un argument variadic", variable);
                return CodeRetourValidation::Erreur;
            }

            if (expression != nullptr) {
                if (decl->est_operateur) {
                    rapporte_erreur("Un paramètre d'une surcharge d'opérateur ne peut avoir de "
                                    "valeur par défaut",
                                    param);
                    return CodeRetourValidation::Erreur;
                }
            }

            noms.insere(variable->ident);

            if (param->type->genre == GenreType::VARIADIQUE) {
                param->drapeaux |= EST_VARIADIQUE;
                decl->est_variadique = true;
                dernier_est_variadic = true;

                auto type_var = param->type->comme_variadique();

                if (!decl->est_externe && type_var->type_pointe == nullptr) {
                    rapporte_erreur("La déclaration de fonction variadique sans type n'est"
                                    " implémentée que pour les fonctions externes",
                                    param);
                    return CodeRetourValidation::Erreur;
                }
            }
        }

        if (decl->est_polymorphe) {
            decl->drapeaux |= DECLARATION_FUT_VALIDEE;
            return CodeRetourValidation::OK;
        }
    }

    // -----------------------------------

    TypeFonction *type_fonc = nullptr;
    auto possede_contexte = !decl->est_externe && !decl->possede_drapeau(FORCE_NULCTX);
    {
        CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide_type_fonction (typage)");

        dls::tablet<Type *, 6> types_entrees;
        types_entrees.reserve(decl->params.taille() + possede_contexte);

        if (possede_contexte) {
            types_entrees.ajoute(espace->typeuse.type_contexte);
        }

        POUR (decl->params) {
            types_entrees.ajoute(it->type);
        }

        Type *type_sortie = nullptr;

        if (decl->params_sorties.taille() == 1) {
            if (resoud_type_final(
                    decl->params_sorties[0]->comme_declaration_variable()->expression_type,
                    type_sortie) == CodeRetourValidation::Erreur) {
                return CodeRetourValidation::Erreur;
            }
        }
        else {
            dls::tablet<TypeCompose::Membre, 6> membres;
            membres.reserve(decl->params_sorties.taille());

            for (auto &expr : decl->params_sorties) {
                auto type_declare = expr->comme_declaration_variable();
                if (resoud_type_final(type_declare->expression_type, type_sortie) ==
                    CodeRetourValidation::Erreur) {
                    return CodeRetourValidation::Erreur;
                }

                // À FAIRE(état validation) : nous ne devrions pas revalider les paramètres
                if ((type_sortie->drapeaux & TYPE_FUT_VALIDE) == 0) {
                    unite->attend_sur_type(type_sortie);
                    return CodeRetourValidation::Erreur;
                }

                membres.ajoute({type_sortie});
            }

            type_sortie = espace->typeuse.cree_tuple(membres);
        }

        decl->param_sortie->type = type_sortie;

        if (decl->ident == ID::principale) {
            if (decl->params.taille() != 0) {
                espace->rapporte_erreur(
                    decl->params[0],
                    "La fonction principale ne doit pas prendre de paramètres d'entrée !");
                return CodeRetourValidation::Erreur;
            }

            if (decl->param_sortie->type->est_tuple()) {
                espace->rapporte_erreur(
                    decl->param_sortie,
                    "La fonction principale ne peut retourner qu'une seule valeur !");
                return CodeRetourValidation::Erreur;
            }

            if (decl->param_sortie->type != espace->typeuse[TypeBase::Z32]) {
                espace->rapporte_erreur(decl->param_sortie,
                                        "La fonction principale doit retourner un z32 !");
                return CodeRetourValidation::Erreur;
            }
        }

        CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide_type_fonction (type_fonction)");
        type_fonc = espace->typeuse.type_fonction(types_entrees, type_sortie);
        decl->type = type_fonc;
        donnees_dependance.types_utilises.insere(decl->type);
    }

    if (decl->est_operateur) {
        CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide_type_fonction (opérateurs)");
        auto type_resultat = type_fonc->type_sortie;

        if (type_resultat == espace->typeuse[TypeBase::RIEN]) {
            rapporte_erreur("Un opérateur ne peut retourner 'rien'", decl);
            return CodeRetourValidation::Erreur;
        }

        if (est_operateur_bool(decl->lexeme->genre) &&
            type_resultat != espace->typeuse[TypeBase::BOOL]) {
            rapporte_erreur("Un opérateur de comparaison doit retourner 'bool'", decl);
            return CodeRetourValidation::Erreur;
        }

        auto operateurs = espace->operateurs.verrou_ecriture();

        if (decl->params.taille() == 1) {
            auto &iter_op = operateurs->trouve_unaire(decl->lexeme->genre);
            auto type1 = type_fonc->types_entrees[0 + possede_contexte];

            for (auto i = 0; i < iter_op.taille(); ++i) {
                auto op = &iter_op[i];

                if (op->type_operande == type1) {
                    if (op->est_basique) {
                        rapporte_erreur("redéfinition de l'opérateur basique", decl);
                        return CodeRetourValidation::Erreur;
                    }

                    espace->rapporte_erreur(decl, "Redéfinition de l'opérateur")
                        .ajoute_message("L'opérateur fut déjà défini ici :\n")
                        .ajoute_site(op->decl);
                    return CodeRetourValidation::Erreur;
                }
            }

            operateurs->ajoute_perso_unaire(decl->lexeme->genre, type1, type_resultat, decl);
        }
        else if (decl->params.taille() == 2) {
            auto type1 = type_fonc->types_entrees[0 + possede_contexte];
            auto type2 = type_fonc->types_entrees[1 + possede_contexte];

            for (auto &op : type1->operateurs.operateurs(decl->lexeme->genre).plage()) {
                if (op->type2 == type2) {
                    if (op->est_basique) {
                        rapporte_erreur("redéfinition de l'opérateur basique", decl);
                        return CodeRetourValidation::Erreur;
                    }

                    espace->rapporte_erreur(decl, "Redéfinition de l'opérateur")
                        .ajoute_message("L'opérateur fut déjà défini ici :\n")
                        .ajoute_site(op->decl);
                    return CodeRetourValidation::Erreur;
                }
            }

            operateurs->ajoute_perso(decl->lexeme->genre, type1, type2, type_resultat, decl);
        }
    }
    else {
        // À FAIRE(moultfilage) : vérifie l'utilisation des synchrones pour les tableaux
        {
            CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions,
                          "valide_type_fonction (redéfinition)");
            auto eu_erreur = false;
            decl->bloc_parent->membres.avec_verrou_lecture(
                [&](const kuri::tableau<NoeudDeclaration *, int> &membres) {
                    POUR (membres) {
                        if (it == decl) {
                            continue;
                        }

                        if (it->genre != GenreNoeud::DECLARATION_ENTETE_FONCTION) {
                            continue;
                        }

                        if (it->ident != decl->ident) {
                            continue;
                        }

                        if (it->type == decl->type) {
                            rapporte_erreur_redefinition_fonction(decl, it);
                            eu_erreur = true;
                            break;
                        }
                    }
                });

            if (eu_erreur) {
                return CodeRetourValidation::Erreur;
            }
        }
    }

    graphe->ajoute_dependances(*noeud_dep, donnees_dependance);
    decl->drapeaux |= DECLARATION_FUT_VALIDEE;

#ifdef STATISTIQUES_DETAILLEES
    possede_erreur = false;
#endif

    return CodeRetourValidation::OK;
}

CodeRetourValidation ContexteValidationCode::valide_arbre_aplatis(
    NoeudExpression *declaration, kuri::tableau<NoeudExpression *, int> &arbre_aplatis)
{
    aplatis_arbre(declaration);

    for (; unite->index_courant < arbre_aplatis.taille(); ++unite->index_courant) {
        auto noeud_enfant = arbre_aplatis[unite->index_courant];

        if (noeud_enfant->est_structure()) {
            // les structures ont leurs propres unités de compilation
            if (!noeud_enfant->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                if (noeud_enfant->comme_structure()->unite == nullptr) {
                    m_compilatrice.ordonnanceuse->cree_tache_pour_typage(
                        espace, noeud_enfant->comme_structure());
                }
                unite->attend_sur_declaration(noeud_enfant->comme_structure());
                return CodeRetourValidation::Erreur;
            }

            continue;
        }

        if (valide_semantique_noeud(noeud_enfant) == CodeRetourValidation::Erreur) {
            return CodeRetourValidation::Erreur;
        }
    }

    return CodeRetourValidation::OK;
}

static void rassemble_expressions(NoeudExpression *expr,
                                  dls::tablet<NoeudExpression *, 6> &expressions)
{
    if (expr == nullptr) {
        return;
    }

    /* pour les directives d'exécutions nous devons directement utiliser le résultat afin
     * d'éviter les problèmes si la substitution est une virgule (plusieurs résultats) */
    if (expr->est_execute() && expr->substitution) {
        expr = expr->substitution;
    }

    if (expr->est_virgule()) {
        auto virgule = expr->comme_virgule();

        POUR (virgule->expressions) {
            expressions.ajoute(it);
        }
    }
    else {
        expressions.ajoute(expr);
    }
}

struct VariableEtExpression {
    IdentifiantCode *ident = nullptr;
    NoeudExpression *expression = nullptr;
};

static void rassemble_expressions(NoeudExpression *expr,
                                  dls::tablet<VariableEtExpression, 6> &expressions)
{
    /* pour les directives d'exécutions nous devons directement utiliser le résultat afin
     * d'éviter les problèmes si la substitution est une virgule (plusieurs résultats) */
    if (expr->est_execute() && expr->substitution) {
        expr = expr->substitution;
    }

    if (expr->est_virgule()) {
        auto virgule = expr->comme_virgule();

        POUR (virgule->expressions) {
            if (it->est_assignation_variable()) {
                auto assignation = it->comme_assignation_variable();
                expressions.ajoute({assignation->variable->ident, assignation->expression});
            }
            else {
                expressions.ajoute({nullptr, it});
            }
        }
    }
    else {
        if (expr->est_assignation_variable()) {
            auto assignation = expr->comme_assignation_variable();
            expressions.ajoute({assignation->variable->ident, assignation->expression});
        }
        else {
            expressions.ajoute({nullptr, expr});
        }
    }
}

CodeRetourValidation ContexteValidationCode::valide_expression_retour(NoeudRetour *inst)
{
    auto type_fonc = fonction_courante->type->comme_fonction();
    auto est_corps_texte = fonction_courante->corps->est_corps_texte;

    if (inst->expression == nullptr) {
        inst->type = espace->typeuse[TypeBase::RIEN];

        if ((!fonction_courante->est_coroutine && type_fonc->type_sortie != inst->type) ||
            est_corps_texte) {
            rapporte_erreur("Expression de retour manquante", inst);
            return CodeRetourValidation::Erreur;
        }

        donnees_dependance.types_utilises.insere(inst->type);
        return CodeRetourValidation::OK;
    }

    if (est_corps_texte) {
        if (inst->expression->est_virgule()) {
            rapporte_erreur("Trop d'expression de retour pour le corps texte", inst->expression);
            return CodeRetourValidation::Erreur;
        }

        auto expr = inst->expression;

        if (expr->est_assignation_variable()) {
            rapporte_erreur("Impossible d'assigner la valeur de retour pour un #corps_texte",
                            inst->expression);
            return CodeRetourValidation::Erreur;
        }

        if (!expr->type->est_chaine()) {
            rapporte_erreur("Attendu un type chaine pour le retour de #corps_texte",
                            inst->expression);
            return CodeRetourValidation::Erreur;
        }

        inst->type = espace->typeuse[TypeBase::CHAINE];

        DonneesAssignations donnees;
        donnees.expression = inst->expression;
        donnees.variables.ajoute(fonction_courante->params_sorties[0]);
        donnees.transformations.ajoute({});

        inst->donnees_exprs.ajoute(std::move(donnees));

        donnees_dependance.types_utilises.insere(inst->type);

        return CodeRetourValidation::OK;
    }

    if (type_fonc->type_sortie->est_rien()) {
        espace->rapporte_erreur(inst->expression,
                                "Retour d'une valeur d'une fonction qui ne retourne rien");
        return CodeRetourValidation::Erreur;
    }

    dls::file_fixe<NoeudExpression *, 6> variables;

    POUR (fonction_courante->params_sorties) {
        variables.enfile(it);
    }

    /* tri les expressions selon les noms */
    dls::tablet<VariableEtExpression, 6> vars_et_exprs;
    rassemble_expressions(inst->expression, vars_et_exprs);

    dls::tablet<NoeudExpression *, 6> expressions;
    expressions.redimensionne(vars_et_exprs.taille());

    POUR (expressions) {
        it = nullptr;
    }

    auto index_courant = 0;
    auto eu_nom = false;
    POUR (vars_et_exprs) {
        auto expr = it.expression;

        if (it.ident) {
            eu_nom = true;

            if (expr->type->est_tuple()) {
                espace->rapporte_erreur(
                    it.expression,
                    "Impossible de nommer les variables de retours si l'expression retourne "
                    "plusieurs valeurs");
                return CodeRetourValidation::Erreur;
            }

            for (auto i = 0; i < fonction_courante->params_sorties.taille(); ++i) {
                if (it.ident == fonction_courante->params_sorties[i]->ident) {
                    if (expressions[i] != nullptr) {
                        espace->rapporte_erreur(
                            it.expression,
                            "Redéfinition d'une expression pour un paramètre de retour");
                        return CodeRetourValidation::Erreur;
                    }

                    expressions[i] = it.expression;
                    break;
                }
            }
        }
        else {
            if (eu_nom) {
                espace->rapporte_erreur(
                    it.expression,
                    "L'expressoin doit avoir un nom si elle suit une autre ayant déjà un nom");
            }

            if (expressions[index_courant] != nullptr) {
                espace->rapporte_erreur(
                    it.expression, "Redéfinition d'une expression pour un paramètre de retour");
                return CodeRetourValidation::Erreur;
            }

            expressions[index_courant] = it.expression;
        }

        index_courant += 1;
    }

    auto valide_typage_et_ajoute = [this](DonneesAssignations &donnees,
                                          NoeudExpression *variable,
                                          NoeudExpression *expression,
                                          Type *type_de_l_expression) {
        auto resultat = cherche_transformation(
            *espace, *this, type_de_l_expression, variable->type);

        if (std::holds_alternative<Attente>(resultat)) {
            unite->marque_attente(std::get<Attente>(resultat));
            return false;
        }

        auto transformation = std::get<TransformationType>(resultat);

        if (transformation.type == TypeTransformation::IMPOSSIBLE) {
            rapporte_erreur_assignation_type_differents(
                variable->type, type_de_l_expression, expression);
            return false;
        }

        donnees.variables.ajoute(variable);
        donnees.transformations.ajoute(transformation);
        return true;
    };

    dls::tablet<DonneesAssignations, 6> donnees_retour;

    POUR (expressions) {
        DonneesAssignations donnees;
        donnees.expression = it;

        if (it->type->est_rien()) {
            rapporte_erreur(
                "impossible de retourner une expression de type « rien » à une variable",
                it,
                erreur::Genre::ASSIGNATION_RIEN);
            return CodeRetourValidation::Erreur;
        }
        else if (it->type->est_tuple()) {
            auto type_tuple = it->type->comme_tuple();

            donnees.multiple_retour = true;

            for (auto &membre : type_tuple->membres) {
                if (variables.est_vide()) {
                    espace->rapporte_erreur(it, "Trop d'expressions de retour");
                    break;
                }

                if (!valide_typage_et_ajoute(donnees, variables.defile(), it, membre.type)) {
                    return CodeRetourValidation::Erreur;
                }
            }
        }
        else {
            if (variables.est_vide()) {
                espace->rapporte_erreur(it, "Trop d'expressions de retour");
                return CodeRetourValidation::Erreur;
            }

            if (!valide_typage_et_ajoute(donnees, variables.defile(), it, it->type)) {
                return CodeRetourValidation::Erreur;
            }
        }

        donnees_retour.ajoute(std::move(donnees));
    }

    // À FAIRE : valeur par défaut des expressions
    if (!variables.est_vide()) {
        espace->rapporte_erreur(inst, "Expressions de retour manquante");
        return CodeRetourValidation::Erreur;
    }

    inst->type = type_fonc->type_sortie;

    inst->donnees_exprs.reserve(static_cast<int>(donnees_retour.taille()));
    POUR (donnees_retour) {
        inst->donnees_exprs.ajoute(std::move(it));
    }

    donnees_dependance.types_utilises.insere(inst->type);
    return CodeRetourValidation::OK;
}

CodeRetourValidation ContexteValidationCode::valide_cuisine(NoeudDirectiveCuisine *directive)
{
    auto expr = directive->expression;

    if (!expr->est_appel()) {
        espace->rapporte_erreur(
            expr, "L'expression d'une directive de cuisson doit être une expression d'appel !");
    }

    if (!expr->type->est_fonction()) {
        espace->rapporte_erreur(
            expr, "La cuisson d'autre chose qu'une fonction n'est pas encore supportée !");
    }

    directive->type = expr->type;
    donnees_dependance.fonctions_utilisees.insere(
        expr->comme_appel()->expression->comme_entete_fonction());

    return CodeRetourValidation::OK;
}

CodeRetourValidation ContexteValidationCode::valide_reference_declaration(
    NoeudExpressionReference *expr, NoeudBloc *bloc_recherche)
{
    CHRONO_TYPAGE(m_tacheronne.stats_typage.ref_decl, "valide référence déclaration");

    expr->genre_valeur = GenreValeur::TRANSCENDANTALE;

    assert(bloc_recherche != nullptr);
    auto fichier = espace->fichier(expr->lexeme->fichier);

#if 0
	/* À FAIRE : nous devrions gérer les déclarations multiples, mais il est possible
	 * qu'un module soit nommé comme une structure dans celui-ci, donc une expression
	 * du style X.X sera toujours erronnée.
	 */
	auto declarations = dls::tablet<NoeudDeclaration *, 10>();
	trouve_declarations_dans_bloc_ou_module(declarations, bloc_recherche, expr->ident, fichier);

	if (declarations.taille() == 0) {
		unite->attend_sur_symbole(expr);
		return CodeRetourValidation::Erreur;
	}

	if (declarations.taille() > 1) {
		auto e = espace->rapporte_erreur(expr, "Plusieurs déclaration sont possibles pour le symbole !");
		e.ajoute_message("Candidates possibles :\n");

		POUR (declarations) {
			e.ajoute_site(it);
		}

		return CodeRetourValidation::Erreur;
	}

	auto decl = declarations[0];
#else
    auto decl = trouve_dans_bloc_ou_module(bloc_recherche, expr->ident, fichier);

    if (decl == nullptr) {
        unite->attend_sur_symbole(expr);
        return CodeRetourValidation::Erreur;
    }
#endif

    if (decl->lexeme->fichier == expr->lexeme->fichier &&
        decl->genre == GenreNoeud::DECLARATION_VARIABLE && !decl->possede_drapeau(EST_GLOBALE)) {
        if (decl->lexeme->ligne > expr->lexeme->ligne) {
            rapporte_erreur("Utilisation d'une variable avant sa déclaration", expr);
            return CodeRetourValidation::Erreur;
        }
    }

    if (dls::outils::est_element(
            decl->genre, GenreNoeud::DECLARATION_ENUM, GenreNoeud::DECLARATION_STRUCTURE) &&
        expr->aide_generation_code != EST_NOEUD_ACCES) {
        if (decl->unite == nullptr) {
            m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace, decl);
        }
        expr->type = espace->typeuse.type_type_de_donnees(decl->type);
        expr->declaration_referee = decl;
    }
    else {
        if (!decl->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
            if (decl->unite == nullptr) {
                m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace, decl);
            }
            // À FAIRE : curseur := curseur.curseurs[0] -> il faut pouvoir déterminer si la
            // référence est celle de la variable que l'on valide, ceci ne fonctionnera pas pour
            // les déclarations multiples, ou les types étant référencés dans les expressions de
            // leurs membres
            if (decl == unite->noeud) {
                espace->rapporte_erreur(expr, "Utilisation d'une variable dans sa définition !\n");
                return CodeRetourValidation::Erreur;
            }
            unite->attend_sur_declaration(decl);
            return CodeRetourValidation::Erreur;
        }

        if (decl->type && decl->type->est_opaque() && decl->est_declaration_variable() &&
            (decl->drapeaux & EST_DECLARATION_TYPE_OPAQUE)) {
            expr->type = espace->typeuse.type_type_de_donnees(decl->type);
            expr->declaration_referee = decl;
        }
        else {
            // les fonctions peuvent ne pas avoir de type au moment si elles sont des appels
            // polymorphiques
            assert(decl->type || decl->genre == GenreNoeud::DECLARATION_ENTETE_FONCTION ||
                   decl->est_declaration_module());
            expr->declaration_referee = decl;
            expr->type = decl->type;

            /* si nous avons une valeur polymorphique, crée un type de données
             * temporaire pour que la validation soit contente, elle sera
             * remplacée par une constante appropriée lors de la validation
             * de l'appel */
            if (decl->drapeaux & EST_VALEUR_POLYMORPHIQUE) {
                expr->type = espace->typeuse.type_type_de_donnees(expr->type);
            }
        }
    }

    if (decl->possede_drapeau(EST_CONSTANTE)) {
        expr->genre_valeur = GenreValeur::DROITE;
    }

    // uniquement pour les fonctions polymorphiques
    if (expr->type) {
        donnees_dependance.types_utilises.insere(expr->type);
    }

    if (decl->est_entete_fonction() && !decl->comme_entete_fonction()->est_polymorphe) {
        expr->genre_valeur = GenreValeur::DROITE;
        auto decl_fonc = decl->comme_entete_fonction();
        donnees_dependance.fonctions_utilisees.insere(decl_fonc);

        if (decl_fonc->corps->unite == nullptr && !decl_fonc->est_externe) {
            m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace, decl_fonc->corps);
        }
    }
    else if (decl->genre == GenreNoeud::DECLARATION_VARIABLE) {
        if (decl->possede_drapeau(EST_GLOBALE)) {
            donnees_dependance.globales_utilisees.insere(decl->comme_declaration_variable());
        }
    }

    return CodeRetourValidation::OK;
}

MetaProgramme *ContexteValidationCode::cree_metaprogramme_corps_texte(NoeudBloc *bloc_corps_texte,
                                                                      NoeudBloc *bloc_parent,
                                                                      const Lexeme *lexeme)
{
    auto fonction = m_tacheronne.assembleuse->cree_entete_fonction(lexeme);
    auto nouveau_corps = fonction->corps;

    fonction->bloc_constantes = m_tacheronne.assembleuse->cree_bloc_seul(lexeme, bloc_parent);
    fonction->bloc_parametres = m_tacheronne.assembleuse->cree_bloc_seul(
        lexeme, fonction->bloc_constantes);

    fonction->bloc_parent = bloc_parent;
    nouveau_corps->bloc_parent = fonction->bloc_parametres;
    nouveau_corps->bloc = bloc_corps_texte;

    /* mise en place du type de la fonction : () -> chaine */
    fonction->est_metaprogramme = true;

    auto decl_sortie = m_tacheronne.assembleuse->cree_declaration_variable(lexeme);
    decl_sortie->ident = m_compilatrice.table_identifiants->identifiant_pour_chaine("__ret0");
    decl_sortie->type = espace->typeuse[TypeBase::CHAINE];
    decl_sortie->drapeaux |= DECLARATION_FUT_VALIDEE;

    fonction->params_sorties.ajoute(decl_sortie);
    fonction->param_sortie = decl_sortie;

    auto types_entrees = dls::tablet<Type *, 6>(0);

    auto type_sortie = espace->typeuse[TypeBase::CHAINE];

    fonction->type = espace->typeuse.type_fonction(types_entrees, type_sortie);
    fonction->drapeaux |= DECLARATION_FUT_VALIDEE;

    auto metaprogramme = espace->cree_metaprogramme();
    metaprogramme->corps_texte = bloc_corps_texte;
    metaprogramme->fonction = fonction;

    return metaprogramme;
}

CodeRetourValidation ContexteValidationCode::valide_fonction(NoeudDeclarationCorpsFonction *decl)
{
    auto entete = decl->entete;

    if (entete->est_polymorphe && !entete->est_monomorphisation) {
        // nous ferons l'analyse sémantique plus tard
        decl->drapeaux |= DECLARATION_FUT_VALIDEE;
        return CodeRetourValidation::OK;
    }

    decl->type = entete->type;

    auto est_corps_texte = decl->est_corps_texte;
    MetaProgramme *metaprogramme = nullptr;

    if (unite->index_courant == 0 && est_corps_texte) {
        metaprogramme = cree_metaprogramme_corps_texte(
            decl->bloc, entete->bloc_parent, decl->lexeme);
        metaprogramme->corps_texte_pour_fonction = entete;

        auto fonction = metaprogramme->fonction;
        auto nouveau_corps = fonction->corps;

        /* échange les corps */
        entete->corps = nouveau_corps;
        nouveau_corps->entete = entete;

        fonction->corps = decl;
        decl->entete = fonction;

        fonction->bloc_parent = entete->bloc_parent;
        nouveau_corps->bloc_parent = decl->bloc_parent;

        fonction->est_monomorphisation = entete->est_monomorphisation;

        // préserve les constantes polymorphiques
        if (fonction->est_monomorphisation) {
            POUR (*entete->bloc_constantes->membres.verrou_lecture()) {
                fonction->bloc_constantes->membres->ajoute(it);
            }
        }

        entete = fonction;
    }

    auto &graphe = espace->graphe_dependance;
    auto noeud_dep = graphe->cree_noeud_fonction(entete);

    commence_fonction(entete);

    if (unite->index_courant == 0) {
        auto requiers_contexte = !decl->entete->possede_drapeau(FORCE_NULCTX);

        if (requiers_contexte) {
            auto val_ctx = m_tacheronne.assembleuse->cree_reference_declaration(decl->lexeme);
            val_ctx->type = espace->typeuse.type_contexte;
            val_ctx->bloc_parent = decl->bloc_parent;
            val_ctx->ident = ID::contexte;

            auto decl_ctx = m_tacheronne.assembleuse->cree_declaration_variable(decl->lexeme);
            decl_ctx->bloc_parent = decl->bloc_parent;
            decl_ctx->valeur = val_ctx;
            decl_ctx->type = val_ctx->type;
            decl_ctx->ident = val_ctx->ident;
            decl_ctx->drapeaux |= DECLARATION_FUT_VALIDEE;

            decl->bloc->membres->ajoute(decl_ctx);
        }
    }

    CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide fonction");

    if (valide_arbre_aplatis(decl, decl->arbre_aplatis) == CodeRetourValidation::Erreur) {
        graphe->ajoute_dependances(*noeud_dep, donnees_dependance);
        return CodeRetourValidation::Erreur;
    }

    auto bloc = decl->bloc;
    auto inst_ret = derniere_instruction(bloc);

    /* si aucune instruction de retour -> vérifie qu'aucun type n'a été spécifié */
    if (inst_ret == nullptr) {
        auto type_fonc = entete->type->comme_fonction();

        if ((type_fonc->type_sortie->genre != GenreType::RIEN && !entete->est_coroutine) ||
            est_corps_texte) {
            rapporte_erreur(
                "Instruction de retour manquante", decl, erreur::Genre::TYPE_DIFFERENTS);
            return CodeRetourValidation::Erreur;
        }

        if (entete != espace->interface_kuri->decl_creation_contexte) {
            decl->aide_generation_code = REQUIERS_CODE_EXTRA_RETOUR;
        }
    }

    graphe->ajoute_dependances(*noeud_dep, donnees_dependance);

    simplifie_arbre(unite->espace, m_tacheronne.assembleuse, espace->typeuse, entete);

    if (est_corps_texte) {
        /* Le dreapeaux nulctx est pour la génération de RI de l'entête, donc il
         * faut le mettre après avoir validé le corps, la création d'un contexte
         * au début de la fonction sera ajouté avant l'exécution du code donc il
         * est possible d'utiliser le contexte dans le métaprogramme. */
        entete->drapeaux |= FORCE_NULCTX;
        m_compilatrice.ordonnanceuse->cree_tache_pour_execution(espace, metaprogramme);
    }

    termine_fonction();
    decl->drapeaux |= DECLARATION_FUT_VALIDEE;
    return CodeRetourValidation::OK;
}

CodeRetourValidation ContexteValidationCode::valide_operateur(NoeudDeclarationCorpsFonction *decl)
{
    auto entete = decl->entete;
    commence_fonction(entete);

    decl->type = entete->type;

    auto &graphe = espace->graphe_dependance;
    auto noeud_dep = graphe->cree_noeud_fonction(entete);

    if (unite->index_courant == 0) {
        auto requiers_contexte = !decl->possede_drapeau(FORCE_NULCTX);

        decl->bloc->membres->reserve(entete->params.taille() + requiers_contexte);

        if (requiers_contexte) {
            auto val_ctx = m_tacheronne.assembleuse->cree_reference_declaration(decl->lexeme);
            val_ctx->type = espace->typeuse.type_contexte;
            val_ctx->bloc_parent = decl->bloc_parent;
            val_ctx->ident = ID::contexte;

            auto decl_ctx = m_tacheronne.assembleuse->cree_declaration_variable(decl->lexeme);
            decl_ctx->bloc_parent = decl->bloc_parent;
            decl_ctx->valeur = val_ctx;
            decl_ctx->type = val_ctx->type;
            decl_ctx->ident = val_ctx->ident;
            decl_ctx->drapeaux |= DECLARATION_FUT_VALIDEE;

            decl->bloc->membres->ajoute(decl_ctx);
        }
    }

    if (valide_arbre_aplatis(decl, decl->arbre_aplatis) == CodeRetourValidation::Erreur) {
        graphe->ajoute_dependances(*noeud_dep, donnees_dependance);
        return CodeRetourValidation::Erreur;
    }

    auto inst_ret = derniere_instruction(decl->bloc);

    if (inst_ret == nullptr) {
        rapporte_erreur("Instruction de retour manquante", decl, erreur::Genre::TYPE_DIFFERENTS);
        return CodeRetourValidation::Erreur;
    }

    graphe->ajoute_dependances(*noeud_dep, donnees_dependance);

    termine_fonction();
    simplifie_arbre(unite->espace, m_tacheronne.assembleuse, espace->typeuse, entete);
    decl->drapeaux |= DECLARATION_FUT_VALIDEE;
    return CodeRetourValidation::OK;
}

enum {
    VALIDE_ENUM_ERREUR,
    VALIDE_ENUM_DRAPEAU,
    VALIDE_ENUM_NORMAL,
};

template <typename T>
static inline bool est_puissance_de_2(T x)
{
    return (x != 0) && (x & (x - 1)) == 0;
}

static bool est_hors_des_limites(long valeur, Type *type)
{
    if (type->est_entier_naturel()) {
        if (type->taille_octet == 1) {
            return valeur >= std::numeric_limits<unsigned char>::max();
        }

        if (type->taille_octet == 2) {
            return valeur > std::numeric_limits<unsigned short>::max();
        }

        if (type->taille_octet == 4) {
            return valeur > std::numeric_limits<unsigned int>::max();
        }

        // À FAIRE : trouve une bonne manière de détecter ceci
        return false;
    }

    if (type->taille_octet == 1) {
        return valeur < std::numeric_limits<char>::min() ||
               valeur > std::numeric_limits<char>::max();
    }

    if (type->taille_octet == 2) {
        return valeur < std::numeric_limits<short>::min() ||
               valeur > std::numeric_limits<short>::max();
    }

    if (type->taille_octet == 4) {
        return valeur < std::numeric_limits<int>::min() ||
               valeur > std::numeric_limits<int>::max();
    }

    // À FAIRE : trouve une bonne manière de détecter ceci
    return false;
}

static long valeur_min(Type *type)
{
    if (type->est_entier_naturel()) {
        if (type->taille_octet == 1) {
            return std::numeric_limits<unsigned char>::min();
        }

        if (type->taille_octet == 2) {
            return std::numeric_limits<unsigned short>::min();
        }

        if (type->taille_octet == 4) {
            return std::numeric_limits<unsigned int>::min();
        }

        return std::numeric_limits<unsigned long>::min();
    }

    if (type->taille_octet == 1) {
        return std::numeric_limits<char>::min();
    }

    if (type->taille_octet == 2) {
        return std::numeric_limits<short>::min();
    }

    if (type->taille_octet == 4) {
        return std::numeric_limits<int>::min();
    }

    return std::numeric_limits<long>::min();
}

static unsigned long valeur_max(Type *type)
{
    if (type->est_entier_naturel()) {
        if (type->taille_octet == 1) {
            return std::numeric_limits<unsigned char>::max();
        }

        if (type->taille_octet == 2) {
            return std::numeric_limits<unsigned short>::max();
        }

        if (type->taille_octet == 4) {
            return std::numeric_limits<unsigned int>::max();
        }

        return std::numeric_limits<unsigned long>::max();
    }

    if (type->taille_octet == 1) {
        return std::numeric_limits<char>::max();
    }

    if (type->taille_octet == 2) {
        return std::numeric_limits<short>::max();
    }

    if (type->taille_octet == 4) {
        return std::numeric_limits<int>::max();
    }

    return std::numeric_limits<long>::max();
}

template <int N>
CodeRetourValidation ContexteValidationCode::valide_enum_impl(NoeudEnum *decl, TypeEnum *type_enum)
{
    auto &graphe = espace->graphe_dependance;
    graphe->connecte_type_type(type_enum, type_enum->type_donnees);

    type_enum->taille_octet = type_enum->type_donnees->taille_octet;
    type_enum->alignement = type_enum->type_donnees->alignement;

    espace->operateurs->ajoute_operateur_basique_enum(espace->typeuse, type_enum);

    auto noms_rencontres = dls::ensemblon<IdentifiantCode *, 32>();

    auto derniere_valeur = ValeurExpression();

    auto &membres = type_enum->membres;
    membres.reserve(decl->bloc->expressions->taille());
    decl->bloc->membres->reserve(decl->bloc->expressions->taille());

    long valeur_enum_min = std::numeric_limits<long>::max();
    long valeur_enum_max = std::numeric_limits<long>::min();
    long valeurs_legales = 0;

    POUR (*decl->bloc->expressions.verrou_ecriture()) {
        if (it->genre != GenreNoeud::DECLARATION_VARIABLE) {
            rapporte_erreur("Type d'expression inattendu dans l'énum", it);
            return CodeRetourValidation::Erreur;
        }

        auto decl_expr = it->comme_declaration_variable();
        decl_expr->type = type_enum;

        decl->bloc->membres->ajoute(decl_expr);

        auto var = decl_expr->valeur;

        if (decl_expr->expression_type != nullptr) {
            rapporte_erreur("Expression d'énumération déclarée avec un type", it);
            return CodeRetourValidation::Erreur;
        }

        if (var->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
            rapporte_erreur("Expression invalide dans la déclaration du membre de l'énumération",
                            var);
            return CodeRetourValidation::Erreur;
        }

        if (noms_rencontres.possede(var->ident)) {
            rapporte_erreur("Redéfinition du membre", var);
            return CodeRetourValidation::Erreur;
        }

        noms_rencontres.insere(var->ident);

        auto expr = decl_expr->expression;

        it->ident = var->ident;

        auto valeur = ValeurExpression();

        if (expr != nullptr) {
            auto res = evalue_expression(espace, decl->bloc, expr);

            if (res.est_errone) {
                espace->rapporte_erreur(res.noeud_erreur, res.message_erreur);
                return CodeRetourValidation::Erreur;
            }

            if (N == VALIDE_ENUM_ERREUR) {
                if (res.valeur.entier == 0) {
                    espace->rapporte_erreur(
                        expr,
                        "L'expression d'une enumération erreur ne peut s'évaluer à 0 (cette "
                        "valeur est réservée par la compilatrice).");
                    return CodeRetourValidation::Erreur;
                }
            }

            if (res.valeur.type != TypeExpression::ENTIER) {
                espace->rapporte_erreur(expr,
                                        "L'expression d'une énumération doit être de type entier");
                return CodeRetourValidation::Erreur;
            }

            valeur = res.valeur;
        }
        else {
            /* TypeExpression::INVALIDE indique que nous sommes dans la première itération. */
            if (derniere_valeur.type == TypeExpression::INVALIDE) {
                valeur.type = TypeExpression::ENTIER;
                valeur.entier = (N == VALIDE_ENUM_DRAPEAU || N == VALIDE_ENUM_ERREUR) ? 1 : 0;
            }
            else {
                valeur.type = derniere_valeur.type;

                if (N == VALIDE_ENUM_DRAPEAU) {
                    valeur.entier = derniere_valeur.entier * 2;

                    if (!est_puissance_de_2(valeur.entier)) {
                        espace->rapporte_erreur(decl_expr,
                                                "La valeur implicite d'une énumération drapeau "
                                                "doit être une puissance de 2 !");
                        return CodeRetourValidation::Erreur;
                    }
                }
                else {
                    valeur.entier = derniere_valeur.entier + 1;
                }
            }
        }

        if (est_hors_des_limites(valeur.entier, type_enum->type_donnees)) {
            auto e = espace->rapporte_erreur(
                decl_expr, "Valeur hors des limites pour le type de l'énumération");
            e.ajoute_message("Le type des données de l'énumération est « ",
                             chaine_type(type_enum->type_donnees),
                             " ».");
            e.ajoute_message("Les valeurs légales pour un tel type se trouvent entre ",
                             valeur_min(type_enum->type_donnees),
                             " et ",
                             valeur_max(type_enum->type_donnees),
                             ".\n");
            e.ajoute_message("Or, la valeur courante est de ", valeur.entier, ".\n");
            return CodeRetourValidation::Erreur;
        }

        valeur_enum_min = std::min(valeur.entier, valeur_enum_min);
        valeur_enum_max = std::max(valeur.entier, valeur_enum_max);

        if (N == VALIDE_ENUM_DRAPEAU) {
            valeurs_legales |= valeur.entier;
        }

        membres.ajoute({type_enum, var->ident, 0, static_cast<int>(valeur.entier)});

        derniere_valeur = valeur;
    }

    membres.ajoute({espace->typeuse[TypeBase::Z32],
                    ID::nombre_elements,
                    0,
                    membres.taille(),
                    nullptr,
                    TypeCompose::Membre::EST_IMPLICITE});
    membres.ajoute({type_enum,
                    ID::min,
                    0,
                    static_cast<int>(valeur_enum_min),
                    nullptr,
                    TypeCompose::Membre::EST_IMPLICITE});
    membres.ajoute({type_enum,
                    ID::max,
                    0,
                    static_cast<int>(valeur_enum_max),
                    nullptr,
                    TypeCompose::Membre::EST_IMPLICITE});

    if (N == VALIDE_ENUM_DRAPEAU) {
        membres.ajoute({type_enum,
                        ID::valeurs_legales,
                        0,
                        static_cast<int>(valeurs_legales),
                        nullptr,
                        TypeCompose::Membre::EST_IMPLICITE});
        membres.ajoute({type_enum,
                        ID::valeurs_illegales,
                        0,
                        static_cast<int>(~valeurs_legales),
                        nullptr,
                        TypeCompose::Membre::EST_IMPLICITE});
    }

    decl->drapeaux |= DECLARATION_FUT_VALIDEE;
    decl->type->drapeaux |= TYPE_FUT_VALIDE;
    return CodeRetourValidation::OK;
}

CodeRetourValidation ContexteValidationCode::valide_enum(NoeudEnum *decl)
{
    CHRONO_TYPAGE(m_tacheronne.stats_typage.enumerations, "valide énum");
    auto type_enum = static_cast<TypeEnum *>(decl->type);

    if (type_enum->est_erreur) {
        type_enum->type_donnees = espace->typeuse[TypeBase::Z32];
    }
    else if (decl->expression_type != nullptr) {
        if (valide_semantique_noeud(decl->expression_type) == CodeRetourValidation::Erreur) {
            return CodeRetourValidation::Erreur;
        }

        if (resoud_type_final(decl->expression_type, type_enum->type_donnees) ==
            CodeRetourValidation::Erreur) {
            return CodeRetourValidation::Erreur;
        }

        /* les énum_drapeaux doivent être des types naturels pour éviter les problèmes
         * d'arithmétiques binaire */
        if (type_enum->est_drapeau && !type_enum->type_donnees->est_entier_naturel()) {
            espace
                ->rapporte_erreur(decl->expression_type,
                                  "Les énum_drapeaux doivent être de type entier naturel (n8, "
                                  "n16, n32, ou n64).\n",
                                  erreur::Genre::TYPE_DIFFERENTS)
                .ajoute_message(
                    "Note : un entier naturel est requis car certaines manipulations de bits en "
                    "complément à deux, par exemple les décalages à droite avec l'opérateur >>, "
                    "préserve le signe de la valeur. "
                    "Un décalage à droite sur l'octet de type relatif 10101010 produirait "
                    "10010101 et non 01010101 comme attendu. Ainsi, pour que je puisse garantir "
                    "un programme bienformé, un type naturel doit être utilisé.\n");
            return CodeRetourValidation::Erreur;
        }

        if (!est_type_entier(type_enum->type_donnees)) {
            espace->rapporte_erreur(
                decl->expression_type,
                "Le type de données d'une énumération doit être de type entier");
            return CodeRetourValidation::Erreur;
        }
    }
    else if (type_enum->est_drapeau) {
        type_enum->type_donnees = espace->typeuse[TypeBase::N32];
    }
    else {
        type_enum->type_donnees = espace->typeuse[TypeBase::Z32];
    }

    if (type_enum->est_erreur) {
        return valide_enum_impl<VALIDE_ENUM_ERREUR>(decl, type_enum);
    }

    if (type_enum->est_drapeau) {
        return valide_enum_impl<VALIDE_ENUM_DRAPEAU>(decl, type_enum);
    }

    return valide_enum_impl<VALIDE_ENUM_NORMAL>(decl, type_enum);
}

/* À FAIRE: les héritages dans les structures externes :
 *
 * BaseExterne :: struct #externe
 *
 * DérivéeExterne :: struct #externe {
 *	  empl base: BaseExterne
 * }
 *
 * Ici nous n'aurons aucun membre.
 *
 * Il nous faudra une meilleure manière de gérer ce cas, peut-être via une
 * erreur de compilation si nous tentons d'utiliser un tel type par valeur.
 * Il faudra également proprement gérer le cas pour les infos types.
 */
CodeRetourValidation ContexteValidationCode::valide_structure(NoeudStruct *decl)
{
    auto &graphe = espace->graphe_dependance;

    auto noeud_dependance = graphe->cree_noeud_type(decl->type);
    decl->noeud_dependance = noeud_dependance;

    union_ou_structure_courante = decl->type;
    DIFFERE {
        union_ou_structure_courante = nullptr;
    };

    if (decl->est_externe && decl->bloc == nullptr) {
        decl->drapeaux |= DECLARATION_FUT_VALIDEE;
        decl->type->drapeaux |= TYPE_FUT_VALIDE;
        return CodeRetourValidation::OK;
    }

    if (decl->est_polymorphe) {
        if (valide_arbre_aplatis(decl, decl->arbre_aplatis_params) ==
            CodeRetourValidation::Erreur) {
            graphe->ajoute_dependances(*noeud_dependance, donnees_dependance);
            return CodeRetourValidation::Erreur;
        }

        if (!decl->monomorphisations) {
            decl->monomorphisations =
                m_tacheronne.allocatrice_noeud.cree_monomorphisations_struct();
        }

        // nous validerons les membres lors de la monomorphisation
        decl->drapeaux |= DECLARATION_FUT_VALIDEE;
        decl->type->drapeaux |= TYPE_FUT_VALIDE;
        return CodeRetourValidation::OK;
    }

    if (decl->est_corps_texte) {
        auto metaprogramme = cree_metaprogramme_corps_texte(
            decl->bloc, decl->bloc_parent, decl->lexeme);
        auto fonction = metaprogramme->fonction;
        fonction->corps->arbre_aplatis = decl->arbre_aplatis;

        metaprogramme->corps_texte_pour_structure = decl;

        if (decl->est_monomorphisation) {
            decl->bloc_constantes->membres.avec_verrou_ecriture(
                [fonction](kuri::tableau<NoeudDeclaration *, int> &membres) {
                    POUR (membres) {
                        fonction->bloc_constantes->membres->ajoute(it);
                    }
                });
        }

        m_compilatrice.ordonnanceuse->cree_tache_pour_typage(espace, fonction->corps);
        m_compilatrice.ordonnanceuse->cree_tache_pour_execution(espace, metaprogramme);

        unite->attend_sur_metaprogramme(metaprogramme);

        // retourne faux, nous retyperons quand le corps sera généré et parsé
        return CodeRetourValidation::Erreur;
    }

    if (valide_arbre_aplatis(decl, decl->arbre_aplatis) == CodeRetourValidation::Erreur) {
        graphe->ajoute_dependances(*noeud_dependance, donnees_dependance);
        return CodeRetourValidation::Erreur;
    }

    CHRONO_TYPAGE(m_tacheronne.stats_typage.structures, "valide structure");

    if (!decl->est_monomorphisation) {
        auto decl_precedente = trouve_dans_bloc(decl->bloc_parent, decl);

        // la bibliothèque C a des symboles qui peuvent être les mêmes pour les fonctions et les
        // structres (p.e. stat)
        if (decl_precedente != nullptr && decl_precedente->genre == decl->genre) {
            rapporte_erreur_redefinition_symbole(decl, decl_precedente);
            return CodeRetourValidation::Erreur;
        }
    }

    auto type_compose = decl->type->comme_compose();
    // @réinitialise en cas d'erreurs passées
    type_compose->membres.efface();
    type_compose->membres.reserve(decl->bloc->membres->taille());

    auto verifie_inclusion_valeur = [&decl, this](NoeudExpression *enf) {
        if (enf->type == decl->type) {
            rapporte_erreur("Ne peut inclure la structure dans elle-même par valeur",
                            enf,
                            erreur::Genre::TYPE_ARGUMENT);
            return CodeRetourValidation::Erreur;
        }

        auto type_base = enf->type;

        if (type_base->genre == GenreType::TABLEAU_FIXE) {
            auto type_deref = type_dereference_pour(type_base);

            if (type_deref == decl->type) {
                rapporte_erreur("Ne peut inclure la structure dans elle-même par valeur",
                                enf,
                                erreur::Genre::TYPE_ARGUMENT);
                return CodeRetourValidation::Erreur;
            }
        }

        return CodeRetourValidation::OK;
    };

    auto ajoute_donnees_membre = [&, this](NoeudExpression *enfant, NoeudExpression *expr_valeur) {
        auto type_membre = enfant->type;
        auto align_type = type_membre->alignement;

        // À FAIRE: ceci devrait plutôt être déplacé dans la validation des déclarations, mais nous
        // finissons sur une erreur de compilation à cause d'une attente
        if ((type_membre->drapeaux & TYPE_FUT_VALIDE) == 0) {
            VERIFIE_UNITE_TYPAGE(type_membre)
            unite->attend_sur_type(type_membre);
            return CodeRetourValidation::Erreur;
        }

        if (align_type == 0) {
            unite->espace->rapporte_erreur(enfant, "impossible de définir l'alignement du type")
                .ajoute_message("Le type est « ", chaine_type(type_membre), " »\n");
            return CodeRetourValidation::Erreur;
        }

        if (type_membre->taille_octet == 0) {
            rapporte_erreur("impossible de définir la taille du type", enfant);
            return CodeRetourValidation::Erreur;
        }

        type_compose->membres.ajoute({enfant->type, enfant->ident, 0, 0, expr_valeur});

        donnees_dependance.types_utilises.insere(type_membre);

        return CodeRetourValidation::OK;
    };

    if (decl->est_union) {
        auto type_union = decl->type->comme_union();
        type_union->est_nonsure = decl->est_nonsure;

        POUR (*decl->bloc->membres.verrou_ecriture()) {
            auto decl_var = it->comme_declaration_variable();

            for (auto &donnees : decl_var->donnees_decl.plage()) {
                for (auto i = 0; i < donnees.variables.taille(); ++i) {
                    auto var = donnees.variables[i];

                    if (var->type->est_rien()) {
                        rapporte_erreur("Ne peut avoir un type « rien » dans une union",
                                        decl_var,
                                        erreur::Genre::TYPE_DIFFERENTS);
                        return CodeRetourValidation::Erreur;
                    }

                    if (var->type->est_structure() || var->type->est_union()) {
                        if ((var->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
                            VERIFIE_UNITE_TYPAGE(var->type)
                            unite->attend_sur_type(var->type);
                            return CodeRetourValidation::Erreur;
                        }
                    }

                    if (var->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
                        rapporte_erreur(
                            "Expression invalide dans la déclaration du membre de l'union", var);
                        return CodeRetourValidation::Erreur;
                    }

                    if (verifie_inclusion_valeur(var) == CodeRetourValidation::Erreur) {
                        return CodeRetourValidation::Erreur;
                    }

                    /* l'arbre syntaxique des expressions par défaut doivent contenir
                     * la transformation puisque nous n'utilisons pas la déclaration
                     * pour générer la RI */
                    auto expression = donnees.expression;
                    transtype_si_necessaire(expression, donnees.transformations[i]);

                    // À FAIRE(emploi) : préserve l'emploi dans les données types
                    if (ajoute_donnees_membre(var, expression) == CodeRetourValidation::Erreur) {
                        return CodeRetourValidation::Erreur;
                    }
                }
            }
        }

        calcule_taille_type_compose(type_union, false, 0);

        if (!decl->est_nonsure) {
            type_union->cree_type_structure(espace->typeuse, type_union->decalage_index);
        }

        decl->type->drapeaux |= TYPE_FUT_VALIDE;

        POUR (type_compose->membres) {
            graphe->connecte_type_type(type_compose, it.type);
        }

        graphe->ajoute_dependances(*noeud_dependance, donnees_dependance);
        return CodeRetourValidation::OK;
    }

    auto type_struct = type_compose->comme_structure();

    POUR (*decl->bloc->membres.verrou_lecture()) {
        if (dls::outils::est_element(
                it->genre, GenreNoeud::DECLARATION_STRUCTURE, GenreNoeud::DECLARATION_ENUM)) {
            // utilisation d'un type de données afin de pouvoir automatiquement déterminer un type
            auto type_de_donnees = espace->typeuse.type_type_de_donnees(it->type);
            type_compose->membres.ajoute(
                {type_de_donnees, it->ident, 0, 0, nullptr, TypeCompose::Membre::EST_CONSTANT});

            // l'utilisation d'un type de données brise le graphe de dépendance
            donnees_dependance.types_utilises.insere(it->type);
            continue;
        }

        if (it->possede_drapeau(EMPLOYE)) {
            type_struct->types_employes.ajoute(it->type->comme_structure());
            continue;
        }

        if (it->genre != GenreNoeud::DECLARATION_VARIABLE) {
            rapporte_erreur("Déclaration inattendu dans le bloc de la structure", it);
            return CodeRetourValidation::Erreur;
        }

        auto decl_var = it->comme_declaration_variable();

        if (decl_var->possede_drapeau(EST_CONSTANTE)) {
            type_compose->membres.ajoute({it->type,
                                          it->ident,
                                          0,
                                          0,
                                          decl_var->expression,
                                          TypeCompose::Membre::EST_CONSTANT});
            continue;
        }

        if (decl_var->declaration_vient_d_un_emploi) {
            // À FAIRE(emploi) : préserve l'emploi dans les données types
            if (ajoute_donnees_membre(decl_var, decl_var->expression) ==
                CodeRetourValidation::Erreur) {
                return CodeRetourValidation::Erreur;
            }

            continue;
        }

        for (auto &donnees : decl_var->donnees_decl.plage()) {
            for (auto i = 0; i < donnees.variables.taille(); ++i) {
                auto var = donnees.variables[i];

                if (var->type->est_rien()) {
                    rapporte_erreur("Ne peut avoir un type « rien » dans une structure",
                                    decl_var,
                                    erreur::Genre::TYPE_DIFFERENTS);
                    return CodeRetourValidation::Erreur;
                }

                if (var->type->est_structure() || var->type->est_union()) {
                    if ((var->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
                        VERIFIE_UNITE_TYPAGE(var->type)
                        unite->attend_sur_type(var->type);
                        return CodeRetourValidation::Erreur;
                    }
                }

                if (var->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
                    rapporte_erreur(
                        "Expression invalide dans la déclaration du membre de la structure", var);
                    return CodeRetourValidation::Erreur;
                }

                if (verifie_inclusion_valeur(var) == CodeRetourValidation::Erreur) {
                    return CodeRetourValidation::Erreur;
                }

                /* l'arbre syntaxique des expressions par défaut doivent contenir
                 * la transformation puisque nous n'utilisons pas la déclaration
                 * pour générer la RI */
                auto expression = donnees.expression;
                transtype_si_necessaire(expression, donnees.transformations[i]);

                // À FAIRE(emploi) : préserve l'emploi dans les données types
                if (ajoute_donnees_membre(var, expression) == CodeRetourValidation::Erreur) {
                    return CodeRetourValidation::Erreur;
                }
            }
        }
    }

    POUR (*decl->bloc->expressions.verrou_ecriture()) {
        if (it->est_assignation_variable()) {
            auto expr_assign = it->comme_assignation_variable();
            auto variable = expr_assign->variable;

            for (auto &membre : type_compose->membres) {
                if (membre.nom == variable->ident) {
                    membre.expression_valeur_defaut = expr_assign->expression;
                    break;
                }
            }
        }
    }

    auto nombre_membres_non_constants = 0;

    POUR (type_compose->membres) {
        if (it.drapeaux &
            (TypeCompose::Membre::EST_CONSTANT | TypeCompose::Membre::EST_IMPLICITE)) {
            continue;
        }

        ++nombre_membres_non_constants;
    }

    if (nombre_membres_non_constants == 0) {
        if (!decl->est_externe) {
            /* Ajoute un membre, d'un octet de taille. */
            type_compose->membres.ajoute(
                {espace->typeuse[TypeBase::BOOL], ID::chaine_vide, 0, 0, nullptr});
            calcule_taille_type_compose(type_compose, decl->est_compacte, decl->alignement_desire);
        }
    }
    else {
        calcule_taille_type_compose(type_compose, decl->est_compacte, decl->alignement_desire);
    }

    decl->type->drapeaux |= TYPE_FUT_VALIDE;
    decl->drapeaux |= DECLARATION_FUT_VALIDEE;

    POUR (type_struct->types_employes) {
        graphe->connecte_type_type(type_struct, it);
    }

    POUR (type_compose->membres) {
        graphe->connecte_type_type(type_compose, it.type);
    }

    graphe->ajoute_dependances(*noeud_dependance, donnees_dependance);
    simplifie_arbre(unite->espace, m_tacheronne.assembleuse, espace->typeuse, decl);
    return CodeRetourValidation::OK;
}

CodeRetourValidation ContexteValidationCode::valide_declaration_variable(
    NoeudDeclarationVariable *decl)
{
    if (decl->drapeaux & EST_DECLARATION_TYPE_OPAQUE) {
        auto type_opacifie = Type::nul();

        if (!decl->expression->possede_drapeau(DECLARATION_TYPE_POLYMORPHIQUE)) {
            resoud_type_final(decl->expression, type_opacifie);

            if ((type_opacifie->drapeaux & TYPE_FUT_VALIDE) == 0) {
                unite->attend_sur_type(type_opacifie);
                return CodeRetourValidation::Erreur;
            }
        }
        else {
            type_opacifie = espace->typeuse.cree_polymorphique(decl->expression->ident);
        }

        auto type_opaque = espace->typeuse.cree_opaque(decl, type_opacifie);
        decl->type = type_opaque;
        decl->drapeaux |= DECLARATION_FUT_VALIDEE;
        return CodeRetourValidation::OK;
    }

    auto &ctx = m_tacheronne.contexte_validation_declaration;
    ctx.variables.efface();
    ctx.donnees_temp.efface();
    ctx.decls_et_refs.efface();
    ctx.feuilles_variables.efface();
    ctx.feuilles_expressions.efface();
    ctx.donnees_assignations.efface();

    auto &feuilles_variables = ctx.feuilles_variables;
    {
        CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "rassemble variables");
        rassemble_expressions(decl->valeur, feuilles_variables);
    }

    /* Rassemble les variables, et crée des déclarations si nécessaire. */
    auto &decls_et_refs = ctx.decls_et_refs;
    decls_et_refs.redimensionne(feuilles_variables.taille());
    {
        CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "préparation");

        if (feuilles_variables.taille() == 1) {
            auto variable = feuilles_variables[0]->comme_reference_declaration();

            decls_et_refs[0].ref_decl = variable;
            decls_et_refs[0].decl = variable->declaration_referee->comme_declaration_variable();
        }
        else {
            for (auto i = 0; i < feuilles_variables.taille(); ++i) {
                auto variable = feuilles_variables[i]->comme_reference_declaration();

                decls_et_refs[i].ref_decl = variable;
                decls_et_refs[i].decl =
                    variable->declaration_referee->comme_declaration_variable();
            }
        }
    }

    {
        CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "typage et redéfinition");

        POUR (decls_et_refs) {
            auto decl_prec = trouve_dans_bloc(it.decl->bloc_parent, it.decl);

            if (decl_prec != nullptr && decl_prec->genre == decl->genre) {
                if (decl->lexeme->ligne > decl_prec->lexeme->ligne) {
                    rapporte_erreur_redefinition_symbole(it.ref_decl, decl_prec);
                    return CodeRetourValidation::Erreur;
                }
            }

            if (resoud_type_final(it.decl->expression_type, it.ref_decl->type) ==
                CodeRetourValidation::Erreur) {
                return CodeRetourValidation::Erreur;
            }
        }
    }

    auto &feuilles_expressions = ctx.feuilles_expressions;
    {
        CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "rassemble expressions");
        rassemble_expressions(decl->expression, feuilles_expressions);
    }

    // pour chaque expression, associe les variables qui doivent recevoir leurs résultats
    // si une variable n'a pas de valeur, prend la valeur de la dernière expression

    auto &variables = ctx.variables;
    {
        CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "enfile variables");

        POUR (feuilles_variables) {
            variables.enfile(it);
        }
    }

    auto &donnees_assignations = ctx.donnees_assignations;

    auto ajoute_variable = [this, decl](DonneesAssignations &donnees,
                                        NoeudExpression *variable,
                                        NoeudExpression *expression,
                                        Type *type_de_l_expression) -> bool {
        if (variable->type == nullptr) {
            if (type_de_l_expression->genre == GenreType::ENTIER_CONSTANT) {
                variable->type = espace->typeuse[TypeBase::Z32];
                donnees.variables.ajoute(variable);
                donnees.transformations.ajoute(
                    {TypeTransformation::CONVERTI_ENTIER_CONSTANT, variable->type});
            }
            else {
                if (type_de_l_expression->est_reference()) {
                    variable->type = type_de_l_expression->comme_reference()->type_pointe;
                    donnees.variables.ajoute(variable);
                    donnees.transformations.ajoute({TypeTransformation::DEREFERENCE});
                }
                else {
                    variable->type = type_de_l_expression;
                    donnees.variables.ajoute(variable);
                    donnees.transformations.ajoute({TypeTransformation::INUTILE});
                }
            }
        }
        else {
            auto resultat = cherche_transformation(
                *espace, *this, type_de_l_expression, variable->type);

            if (std::holds_alternative<Attente>(resultat)) {
                unite->marque_attente(std::get<Attente>(resultat));
                return false;
            }

            auto transformation = std::get<TransformationType>(resultat);
            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_assignation_type_differents(
                    variable->type, type_de_l_expression, expression);
                return false;
            }

            donnees.variables.ajoute(variable);
            donnees.transformations.ajoute(transformation);
        }

        if (decl->drapeaux & EST_CONSTANTE && !type_de_l_expression->est_type_de_donnees()) {
            auto res_exec = evalue_expression(espace, decl->bloc_parent, expression);

            if (res_exec.est_errone) {
                rapporte_erreur("Impossible d'évaluer l'expression de la constante", expression);
                return false;
            }

            decl->valeur_expression = res_exec.valeur;
        }

        return true;
    };

    {
        CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "assignation expressions");

        POUR (feuilles_expressions) {
            auto &donnees = ctx.donnees_temp;
            donnees.expression = it;

            // il est possible d'ignorer les variables
            if (variables.est_vide()) {
                espace->rapporte_erreur(decl, "Trop d'expressions ou de types pour l'assignation");
                return CodeRetourValidation::Erreur;
            }

            if ((decl->drapeaux & EST_CONSTANTE) && it->est_non_initialisation()) {
                rapporte_erreur("Impossible de ne pas initialiser une constante", it);
                return CodeRetourValidation::Erreur;
            }

            if (decl->drapeaux & EST_EXTERNE) {
                rapporte_erreur(
                    "Ne peut pas assigner une variable globale externe dans sa déclaration", decl);
                return CodeRetourValidation::Erreur;
            }

            if (it->type == nullptr && !it->est_non_initialisation()) {
                rapporte_erreur("impossible de définir le type de l'expression", it);
                return CodeRetourValidation::Erreur;
            }

            if (it->est_non_initialisation()) {
                donnees.variables.ajoute(variables.defile());
                donnees.transformations.ajoute({TypeTransformation::INUTILE});
            }
            else if (it->type->est_tuple()) {
                auto type_tuple = it->type->comme_tuple();

                donnees.multiple_retour = true;

                for (auto &membre : type_tuple->membres) {
                    if (variables.est_vide()) {
                        break;
                    }

                    if (!ajoute_variable(donnees, variables.defile(), it, membre.type)) {
                        return CodeRetourValidation::Erreur;
                    }
                }
            }
            else if (it->type->est_rien()) {
                rapporte_erreur(
                    "impossible d'assigner une expression de type « rien » à une variable",
                    it,
                    erreur::Genre::ASSIGNATION_RIEN);
                return CodeRetourValidation::Erreur;
            }
            else {
                if (!ajoute_variable(donnees, variables.defile(), it, it->type)) {
                    return CodeRetourValidation::Erreur;
                }
            }

            donnees_assignations.ajoute(std::move(donnees));
        }

        if (donnees_assignations.est_vide()) {
            donnees_assignations.ajoute({});
        }

        // a, b := c
        auto donnees = &donnees_assignations.back();
        while (!variables.est_vide()) {
            auto var = variables.defile();
            auto transformation = TransformationType(TypeTransformation::INUTILE);

            if (donnees->expression) {
                var->type = donnees->expression->type;

                if (var->type->est_entier_constant()) {
                    var->type = espace->typeuse[TypeBase::Z32];
                    transformation = {TypeTransformation::CONVERTI_ENTIER_CONSTANT, var->type};
                }
            }

            donnees->variables.ajoute(var);
            donnees->transformations.ajoute(transformation);
        }
    }

    {
        CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "validation finale");

        POUR (decls_et_refs) {
            auto decl_var = it.decl;
            auto variable = it.ref_decl;

            if (variable->type == nullptr) {
                rapporte_erreur("variable déclarée sans type", variable);
                return CodeRetourValidation::Erreur;
            }

            decl_var->type = variable->type;

            if (decl_var->drapeaux & EST_GLOBALE) {
                auto graphe = espace->graphe_dependance.verrou_ecriture();
                graphe->cree_noeud_globale(decl_var);
            }

            auto bloc_parent = decl_var->bloc_parent;
            bloc_parent->membres->ajoute(decl_var);

            decl_var->drapeaux |= DECLARATION_FUT_VALIDEE;
            donnees_dependance.types_utilises.insere(decl_var->type);
        }
    }

    /* Les paramètres de fonctions n'ont pas besoin de données pour les assignations d'expressions.
     */
    if (!decl->possede_drapeau(EST_PARAMETRE)) {
        CHRONO_TYPAGE(m_tacheronne.stats_typage.validation_decl, "copie données");

        decl->donnees_decl.reserve(static_cast<int>(donnees_assignations.taille()));

        POUR (donnees_assignations) {
            decl->donnees_decl.ajoute(std::move(it));
        }
    }

    if (!fonction_courante) {
        simplifie_arbre(unite->espace, m_tacheronne.assembleuse, espace->typeuse, decl);
    }

    return CodeRetourValidation::OK;
}

CodeRetourValidation ContexteValidationCode::valide_assignation(NoeudAssignation *inst)
{
    CHRONO_TYPAGE(m_tacheronne.stats_typage.assignations, "valide assignation");
    auto variable = inst->variable;

    dls::file_fixe<NoeudExpression *, 6> variables;

    if (variable->est_virgule()) {
        auto virgule = variable->comme_virgule();
        POUR (virgule->expressions) {
            if (!est_valeur_gauche(it->genre_valeur)) {
                rapporte_erreur("Impossible d'assigner une expression à une valeur-droite !",
                                inst,
                                erreur::Genre::ASSIGNATION_INVALIDE);
                return CodeRetourValidation::Erreur;
            }

            variables.enfile(it);
        }
    }
    else {
        if (!est_valeur_gauche(variable->genre_valeur)) {
            rapporte_erreur("Impossible d'assigner une expression à une valeur-droite !",
                            inst,
                            erreur::Genre::ASSIGNATION_INVALIDE);
            return CodeRetourValidation::Erreur;
        }

        variables.enfile(variable);
    }

    dls::tablet<NoeudExpression *, 6> expressions;
    rassemble_expressions(inst->expression, expressions);

    auto ajoute_variable = [this](DonneesAssignations &donnees,
                                  NoeudExpression *var,
                                  NoeudExpression *expression,
                                  Type *type_de_l_expression) -> bool {
        auto type_de_la_variable = var->type;
        auto var_est_reference = type_de_la_variable->est_reference();
        auto expr_est_reference = type_de_l_expression->est_reference();

        auto transformation = TransformationType();

        if (var->possede_drapeau(ACCES_EST_ENUM_DRAPEAU) && expression->type->est_bool()) {
            if (!expression->est_litterale_bool()) {
                espace->rapporte_erreur(expression,
                                        "L'assignation d'une valeur d'une énum_drapeau doit être "
                                        "une littérale booléenne");
                return false;
            }

            donnees.variables.ajoute(var);
            donnees.transformations.ajoute(transformation);
            return true;
        }

        if (var_est_reference && expr_est_reference) {
            // déréférence les deux côtés
            auto resultat = cherche_transformation(
                *espace, *this, type_de_l_expression, var->type);

            if (std::holds_alternative<Attente>(resultat)) {
                unite->marque_attente(std::get<Attente>(resultat));
                return false;
            }

            transformation = std::get<TransformationType>(resultat);
            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_assignation_type_differents(
                    var->type, type_de_l_expression, expression);
                return false;
            }

            transtype_si_necessaire(var, TypeTransformation::DEREFERENCE);
            transformation = TypeTransformation::DEREFERENCE;
        }
        else if (var_est_reference) {
            // déréférence var
            type_de_la_variable = type_de_la_variable->comme_reference()->type_pointe;

            auto resultat = cherche_transformation(
                *espace, *this, type_de_l_expression, type_de_la_variable);

            if (std::holds_alternative<Attente>(resultat)) {
                unite->marque_attente(std::get<Attente>(resultat));
                return false;
            }

            transformation = std::get<TransformationType>(resultat);
            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_assignation_type_differents(
                    var->type, type_de_l_expression, expression);
                return false;
            }

            transtype_si_necessaire(var, TypeTransformation::DEREFERENCE);
        }
        else if (expr_est_reference) {
            // déréférence expr
            auto resultat = cherche_transformation(
                *espace, *this, type_de_l_expression, var->type);

            if (std::holds_alternative<Attente>(resultat)) {
                unite->marque_attente(std::get<Attente>(resultat));
                return false;
            }

            transformation = std::get<TransformationType>(resultat);
            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_assignation_type_differents(
                    var->type, type_de_l_expression, expression);
                return false;
            }
        }
        else {
            auto resultat = cherche_transformation(
                *espace, *this, type_de_l_expression, var->type);

            if (std::holds_alternative<Attente>(resultat)) {
                unite->marque_attente(std::get<Attente>(resultat));
                return false;
            }

            transformation = std::get<TransformationType>(resultat);
            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_assignation_type_differents(
                    var->type, type_de_l_expression, expression);
                return false;
            }
        }

        donnees.variables.ajoute(var);
        donnees.transformations.ajoute(transformation);
        return true;
    };

    dls::tablet<DonneesAssignations, 6> donnees_assignations;

    POUR (expressions) {
        if (it->est_non_initialisation()) {
            rapporte_erreur("Impossible d'utiliser '---' dans une expression d'assignation",
                            inst->expression);
            return CodeRetourValidation::Erreur;
        }

        if (it->type == nullptr) {
            rapporte_erreur("Impossible de définir le type de la variable !",
                            inst,
                            erreur::Genre::TYPE_INCONNU);
            return CodeRetourValidation::Erreur;
        }

        if (it->type->est_rien()) {
            rapporte_erreur("Impossible d'assigner une expression de type 'rien' à une variable !",
                            inst,
                            erreur::Genre::ASSIGNATION_RIEN);
            return CodeRetourValidation::Erreur;
        }

        auto donnees = DonneesAssignations();
        donnees.expression = it;

        if (it->type->est_tuple()) {
            auto type_tuple = it->type->comme_tuple();

            donnees.multiple_retour = true;

            for (auto &membre : type_tuple->membres) {
                if (variables.est_vide()) {
                    break;
                }

                if (!ajoute_variable(donnees, variables.defile(), it, membre.type)) {
                    return CodeRetourValidation::Erreur;
                }
            }
        }
        else {
            if (!ajoute_variable(donnees, variables.defile(), it, it->type)) {
                return CodeRetourValidation::Erreur;
            }
        }

        donnees_assignations.ajoute(std::move(donnees));
    }

    // a, b = c
    auto donnees = &donnees_assignations.back();
    while (!variables.est_vide()) {
        if (!ajoute_variable(
                *donnees, variables.defile(), donnees->expression, donnees->expression->type)) {
            return CodeRetourValidation::Erreur;
        }
    }

    inst->donnees_exprs.reserve(static_cast<int>(donnees_assignations.taille()));
    POUR (donnees_assignations) {
        inst->donnees_exprs.ajoute(std::move(it));
    }

    return CodeRetourValidation::OK;
}

template <typename TypeControleBoucle>
CodeRetourValidation ContexteValidationCode::valide_controle_boucle(TypeControleBoucle *inst)
{
    auto chaine_var = inst->expression == nullptr ? nullptr : inst->expression->ident;
    auto boucle = bloc_est_dans_boucle(inst->bloc_parent, chaine_var);

    if (!boucle) {
        if (!chaine_var) {
            espace->rapporte_erreur(
                inst, "« continue » en dehors d'une boucle", erreur::Genre::CONTROLE_INVALIDE);
            return CodeRetourValidation::Erreur;
        }

        espace->rapporte_erreur(inst->expression,
                                "La variable ne réfère à aucune boucle",
                                erreur::Genre::VARIABLE_INCONNUE);
        return CodeRetourValidation::Erreur;
    }

    inst->boucle_controlee = boucle;
    return CodeRetourValidation::OK;
}

/* ************************************************************************** */

CodeRetourValidation ContexteValidationCode::resoud_type_final(NoeudExpression *expression_type,
                                                               Type *&type_final)
{
    if (expression_type == nullptr) {
        type_final = nullptr;
        return CodeRetourValidation::OK;
    }

    auto type_var = expression_type->type;

    if (type_var == nullptr) {
        espace->rapporte_erreur(expression_type,
                                "Erreur interne, le type de l'expression est nul !");
        return CodeRetourValidation::Erreur;
    }

    if (type_var->genre != GenreType::TYPE_DE_DONNEES) {
        rapporte_erreur("attendu un type de données", expression_type);
        return CodeRetourValidation::Erreur;
    }

    auto type_de_donnees = type_var->comme_type_de_donnees();

    if (type_de_donnees->type_connu == nullptr) {
        rapporte_erreur("impossible de définir le type selon l'expression", expression_type);
        return CodeRetourValidation::Erreur;
    }

    type_final = type_de_donnees->type_connu;
    return CodeRetourValidation::OK;
}

void ContexteValidationCode::rapporte_erreur(const char *message, NoeudExpression *noeud)
{
    erreur::lance_erreur(message, *espace, noeud);
}

void ContexteValidationCode::rapporte_erreur(const char *message,
                                             NoeudExpression *noeud,
                                             erreur::Genre genre)
{
    erreur::lance_erreur(message, *espace, noeud, genre);
}

void ContexteValidationCode::rapporte_erreur_redefinition_symbole(NoeudExpression *decl,
                                                                  NoeudDeclaration *decl_prec)
{
    erreur::redefinition_symbole(*espace, decl, decl_prec);
}

void ContexteValidationCode::rapporte_erreur_redefinition_fonction(
    NoeudDeclarationEnteteFonction *decl, NoeudDeclaration *decl_prec)
{
    erreur::redefinition_fonction(*espace, decl_prec, decl);
}

void ContexteValidationCode::rapporte_erreur_type_arguments(NoeudExpression *type_arg,
                                                            NoeudExpression *type_enf)
{
    erreur::lance_erreur_transtypage_impossible(
        type_arg->type, type_enf->type, *espace, type_enf, type_arg);
}

void ContexteValidationCode::rapporte_erreur_assignation_type_differents(const Type *type_gauche,
                                                                         const Type *type_droite,
                                                                         NoeudExpression *noeud)
{
    erreur::lance_erreur_assignation_type_differents(type_gauche, type_droite, *espace, noeud);
}

void ContexteValidationCode::rapporte_erreur_type_operation(const Type *type_gauche,
                                                            const Type *type_droite,
                                                            NoeudExpression *noeud)
{
    erreur::lance_erreur_type_operation(type_gauche, type_droite, *espace, noeud);
}

void ContexteValidationCode::rapporte_erreur_acces_hors_limites(NoeudExpression *b,
                                                                TypeTableauFixe *type_tableau,
                                                                long index_acces)
{
    erreur::lance_erreur_acces_hors_limites(
        *espace, b, type_tableau->taille, type_tableau, index_acces);
}

void ContexteValidationCode::rapporte_erreur_membre_inconnu(NoeudExpression *acces,
                                                            NoeudExpression *structure,
                                                            NoeudExpression *membre,
                                                            TypeCompose *type)
{
    erreur::membre_inconnu(*espace, acces, structure, membre, type);
}

void ContexteValidationCode::rapporte_erreur_valeur_manquante_discr(
    NoeudExpression *expression, dls::ensemble<kuri::chaine_statique> const &valeurs_manquantes)
{
    erreur::valeur_manquante_discr(*espace, expression, valeurs_manquantes);
}

void ContexteValidationCode::rapporte_erreur_fonction_nulctx(const NoeudExpression *appl_fonc,
                                                             const NoeudExpression *decl_fonc,
                                                             const NoeudExpression *decl_appel)
{
    erreur::lance_erreur_fonction_nulctx(*espace, appl_fonc, decl_fonc, decl_appel);
}

CodeRetourValidation ContexteValidationCode::transtype_si_necessaire(NoeudExpression *&expression,
                                                                     Type *type_cible)
{
    auto resultat = cherche_transformation(*espace, *this, expression->type, type_cible);

    if (std::holds_alternative<Attente>(resultat)) {
        unite->marque_attente(std::get<Attente>(resultat));
        return CodeRetourValidation::Erreur;
    }

    auto transformation = std::get<TransformationType>(resultat);
    if (transformation.type == TypeTransformation::IMPOSSIBLE) {
        rapporte_erreur_assignation_type_differents(type_cible, expression->type, expression);
        return CodeRetourValidation::Erreur;
    }

    transtype_si_necessaire(expression, transformation);
    return CodeRetourValidation::OK;
}

void ContexteValidationCode::transtype_si_necessaire(NoeudExpression *&expression,
                                                     TransformationType const &transformation)
{
    if (transformation.type == TypeTransformation::INUTILE) {
        return;
    }

    if (transformation.type == TypeTransformation::CONVERTI_ENTIER_CONSTANT) {
        expression->type = transformation.type_cible;
        return;
    }

    auto type_cible = transformation.type_cible;

    if (type_cible == nullptr) {
        if (transformation.type == TypeTransformation::CONSTRUIT_EINI) {
            type_cible = espace->typeuse[TypeBase::EINI];
        }
        else if (transformation.type == TypeTransformation::CONVERTI_VERS_PTR_RIEN) {
            type_cible = espace->typeuse[TypeBase::PTR_RIEN];
        }
        else if (transformation.type == TypeTransformation::PREND_REFERENCE) {
            type_cible = espace->typeuse.type_reference_pour(expression->type);
        }
        else if (transformation.type == TypeTransformation::DEREFERENCE) {
            type_cible = type_dereference_pour(expression->type);
        }
        else if (transformation.type == TypeTransformation::CONSTRUIT_TABL_OCTET) {
            type_cible = espace->typeuse[TypeBase::TABL_OCTET];
        }
        else if (transformation.type == TypeTransformation::CONVERTI_TABLEAU) {
            auto type_tableau_fixe = expression->type->comme_tableau_fixe();
            type_cible = espace->typeuse.type_tableau_dynamique(type_tableau_fixe->type_pointe);
        }
        else {
            assert_rappel(false, [&]() {
                std::cerr << "Type Transformation non géré : " << transformation.type << '\n';
            });
        }
    }

    auto tfm = transformation;

    if (transformation.type == TypeTransformation::CONVERTI_REFERENCE_VERS_TYPE_CIBLE) {
        auto noeud_comme = m_tacheronne.assembleuse->cree_comme(expression->lexeme);
        noeud_comme->type = espace->typeuse.type_reference_pour(expression->type);
        noeud_comme->expression = expression;
        noeud_comme->transformation = TypeTransformation::PREND_REFERENCE;
        noeud_comme->drapeaux |= TRANSTYPAGE_IMPLICITE;

        expression = noeud_comme;
        tfm.type = TypeTransformation::CONVERTI_VERS_TYPE_CIBLE;
    }

    auto noeud_comme = m_tacheronne.assembleuse->cree_comme(expression->lexeme);
    noeud_comme->type = type_cible;
    noeud_comme->expression = expression;
    noeud_comme->transformation = tfm;
    noeud_comme->drapeaux |= TRANSTYPAGE_IMPLICITE;

    expression = noeud_comme;
}
