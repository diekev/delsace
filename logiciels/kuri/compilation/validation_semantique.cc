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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "validation_semantique.hh"

#include "arbre_syntaxique/assembleuse.hh"

#include "parsage/outils_lexemes.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "portee.hh"
#include "tacheronne.hh"
#include "unite_compilation.hh"

/* ************************************************************************** */

#define VERIFIE_INTERFACE_KURI_CHARGEE(nom, id)                                                   \
    if (m_compilatrice.interface_kuri->decl_##nom == nullptr) {                                   \
        return Attente::sur_interface_kuri(id);                                                   \
    }

ContexteValidationCode::ContexteValidationCode(Compilatrice &compilatrice,
                                               Tacheronne &tacheronne,
                                               UniteCompilation &u)
    : m_compilatrice(compilatrice), m_tacheronne(tacheronne), unite(&u), espace(unite->espace)
{
}

/* Point d'entrée pour la validation sémantique. Nous utilisons ceci au lieu de directement appeler
 * valide_semantique_noeud, puisque la validation des arbres aplatis pourrait résulter en un
 * dépassement de pile dans le cas où l'arbre aplatis contient également la fonction racine.
 * En outre, ceci nous permet de mieux controler les racines de validations, qui doivent être
 * des déclarations ou directives globales. */
ResultatValidation ContexteValidationCode::valide()
{
    if (racine_validation()->est_entete_fonction()) {
        return valide_entete_fonction(racine_validation()->comme_entete_fonction());
    }

    if (racine_validation()->est_corps_fonction()) {
        auto corps = racine_validation()->comme_corps_fonction();
        if (corps->entete->est_operateur) {
            return valide_operateur(corps);
        }
        return valide_fonction(corps);
    }

    if (racine_validation()->est_enum()) {
        auto enumeration = racine_validation()->comme_enum();
        return valide_enum(enumeration);
    }

    if (racine_validation()->est_structure()) {
        auto structure = racine_validation()->comme_structure();
        return valide_structure(structure);
    }

    if (racine_validation()->est_type_opaque()) {
        auto opaque = racine_validation()->comme_type_opaque();
        return valide_arbre_aplatis(opaque, opaque->arbre_aplatis);
    }

    if (racine_validation()->est_declaration_variable()) {
        auto decl = racine_validation()->comme_declaration_variable();
        return valide_arbre_aplatis(decl, decl->arbre_aplatis);
    }

    if (racine_validation()->est_execute()) {
        auto execute = racine_validation()->comme_execute();
        return valide_arbre_aplatis(execute, execute->arbre_aplatis);
    }

    if (racine_validation()->est_importe() || racine_validation()->est_charge()) {
        return valide_semantique_noeud(racine_validation());
    }

    if (racine_validation()->est_ajoute_fini()) {
        auto ajoute_fini = racine_validation()->comme_ajoute_fini();
        return valide_arbre_aplatis(ajoute_fini, ajoute_fini->arbre_aplatis);
    }

    if (racine_validation()->est_ajoute_init()) {
        auto ajoute_init = racine_validation()->comme_ajoute_init();
        return valide_arbre_aplatis(ajoute_init, ajoute_init->arbre_aplatis);
    }

    unite->espace->rapporte_erreur_sans_site("Erreur interne : aucune racine de typage valide");
    return CodeRetourValidation::Erreur;
}

static Attente attente_sur_operateur_ou_type(NoeudExpressionBinaire *noeud)
{
    auto est_enum_ou_reference_enum = [](Type *t) -> TypeEnum * {
        if (t->est_enum()) {
            return t->comme_enum();
        }

        if (t->est_reference() && t->comme_reference()->type_pointe->est_enum()) {
            return t->comme_reference()->type_pointe->comme_enum();
        }

        return nullptr;
    };

    auto type1 = noeud->operande_gauche->type;
    auto type1_est_enum = est_enum_ou_reference_enum(type1);
    if (type1_est_enum && (type1_est_enum->drapeaux & TYPE_FUT_VALIDE) == 0) {
        return Attente::sur_type(type1_est_enum);
    }
    auto type2 = noeud->operande_droite->type;
    auto type2_est_enum = est_enum_ou_reference_enum(type2);
    if (type2_est_enum && (type2_est_enum->drapeaux & TYPE_FUT_VALIDE) == 0) {
        return Attente::sur_type(type2_est_enum);
    }
    return Attente::sur_operateur(noeud);
}

MetaProgramme *ContexteValidationCode::cree_metaprogramme_pour_directive(
    NoeudDirectiveExecute *directive)
{
    // crée une fonction pour l'exécution
    auto decl_entete = m_tacheronne.assembleuse->cree_entete_fonction(directive->lexeme);
    auto decl_corps = decl_entete->corps;

    decl_entete->bloc_parent = directive->bloc_parent;
    decl_corps->bloc_parent = directive->bloc_parent;

    m_tacheronne.assembleuse->bloc_courant(decl_corps->bloc_parent);
    decl_entete->bloc_constantes = m_tacheronne.assembleuse->empile_bloc(directive->lexeme);
    decl_entete->bloc_parametres = m_tacheronne.assembleuse->empile_bloc(directive->lexeme);

    decl_entete->est_metaprogramme = true;

    // le type de la fonction est fonc () -> (type_expression)
    auto expression = directive->expression;
    auto type_expression = expression->type;

    /* Le type peut être nul pour les #tests. */
    if (!type_expression && directive->ident == ID::test) {
        type_expression = m_compilatrice.typeuse[TypeBase::RIEN];
    }

    if (type_expression->est_tuple()) {
        auto tuple = type_expression->comme_tuple();

        POUR (tuple->membres) {
            auto decl_sortie = m_tacheronne.assembleuse->cree_declaration_variable(
                directive->lexeme);
            decl_sortie->ident = m_compilatrice.table_identifiants->identifiant_pour_chaine(
                "__ret0");
            decl_sortie->type = it.type;
            decl_sortie->drapeaux |= DECLARATION_FUT_VALIDEE;
            decl_entete->params_sorties.ajoute(decl_sortie);
        }

        decl_entete->param_sortie = m_tacheronne.assembleuse->cree_declaration_variable(
            directive->lexeme);
        decl_entete->param_sortie->ident =
            m_compilatrice.table_identifiants->identifiant_pour_chaine("valeur_de_retour");
        decl_entete->param_sortie->type = type_expression;
    }
    else {
        auto decl_sortie = m_tacheronne.assembleuse->cree_declaration_variable(directive->lexeme);
        decl_sortie->ident = m_compilatrice.table_identifiants->identifiant_pour_chaine("__ret0");
        decl_sortie->type = type_expression;
        decl_sortie->drapeaux |= DECLARATION_FUT_VALIDEE;

        decl_entete->params_sorties.ajoute(decl_sortie);
        decl_entete->param_sortie = m_tacheronne.assembleuse->cree_declaration_variable(
            directive->lexeme);
        decl_entete->param_sortie->type = type_expression;
    }

    auto types_entrees = kuri::tablet<Type *, 6>(0);

    auto type_fonction = m_compilatrice.typeuse.type_fonction(types_entrees, type_expression);
    decl_entete->type = type_fonction;

    decl_corps->bloc = m_tacheronne.assembleuse->empile_bloc(directive->lexeme);

    static Lexeme lexeme_retourne = {"retourne", {}, GenreLexeme::RETOURNE, 0, 0, 0};
    auto expr_ret = m_tacheronne.assembleuse->cree_retourne(&lexeme_retourne);

    simplifie_arbre(espace, m_tacheronne.assembleuse, m_compilatrice.typeuse, expression);

    if (type_expression != m_compilatrice.typeuse[TypeBase::RIEN]) {
        expr_ret->genre = GenreNoeud::INSTRUCTION_RETOUR;
        expr_ret->expression = expression;

        /* besoin de valider pour mettre en place les informations de retour */
        auto ancienne_racine = unite->noeud;
        unite->noeud = decl_entete;
        valide_expression_retour(expr_ret);
        unite->noeud = ancienne_racine;
    }
    else {
        decl_corps->bloc->expressions->ajoute(expression);
    }

    decl_corps->bloc->expressions->ajoute(expr_ret);

    /* Bloc corps. */
    m_tacheronne.assembleuse->depile_bloc();
    /* Bloc paramètres. */
    m_tacheronne.assembleuse->depile_bloc();
    /* Bloc constantes. */
    m_tacheronne.assembleuse->depile_bloc();

    decl_entete->drapeaux |= DECLARATION_FUT_VALIDEE;
    decl_corps->drapeaux |= DECLARATION_FUT_VALIDEE;

    auto metaprogramme = m_compilatrice.cree_metaprogramme(espace);
    metaprogramme->fonction = decl_entete;
    metaprogramme->directive = directive;
    directive->metaprogramme = metaprogramme;
    return metaprogramme;
}

static inline bool est_expression_convertible_en_bool(NoeudExpression *expression)
{
    return est_type_booleen_implicite(expression->type) ||
           expression->possede_drapeau(ACCES_EST_ENUM_DRAPEAU);
}

/* Décide si le type peut être utilisé pour les expressions d'indexages basiques du langage.
 * NOTE : les entiers relatifs ne sont pas considérées ici car nous utilisons cette décision pour
 * transtyper automatiquement vers le type cible (z64), et nous les gérons séparément. */
static inline bool est_type_implicitement_utilisable_pour_indexage(Type *type)
{
    if (type->est_entier_naturel()) {
        return true;
    }

    if (type->est_octet()) {
        return true;
    }

    if (type->est_enum()) {
        /* Pour l'instant, les énum_drapeaux ne sont pas utilisable, car les index peuvent être
         * arbitrairement larges. */
        return !type->comme_enum()->est_drapeau;
    }

    if (type->est_bool()) {
        return true;
    }

    if (type->est_opaque()) {
        return est_type_implicitement_utilisable_pour_indexage(
            type->comme_opaque()->type_opacifie);
    }

    return false;
}

ResultatValidation ContexteValidationCode::valide_semantique_noeud(NoeudExpression *noeud)
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
        case GenreNoeud::DIRECTIVE_DEPENDANCE_BIBLIOTHEQUE:
        {
            break;
        }
        case GenreNoeud::DIRECTIVE_AJOUTE_FINI:
        {
            auto fini_execution = m_compilatrice.interface_kuri->decl_fini_execution_kuri;
            if (fini_execution == nullptr) {
                return Attente::sur_interface_kuri(ID::fini_execution_kuri);
            }
            if (!fini_execution->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(fini_execution);
            }
            auto corps = fini_execution->corps;
            if (!corps->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(corps);
            }
            auto ajoute_fini = noeud->comme_ajoute_fini();
            corps->bloc->expressions->pousse_front(ajoute_fini->expression);
            ajoute_fini->drapeaux |= DECLARATION_FUT_VALIDEE;
            break;
        }
        case GenreNoeud::DIRECTIVE_AJOUTE_INIT:
        {
            auto init_execution = m_compilatrice.interface_kuri->decl_init_execution_kuri;
            if (init_execution == nullptr) {
                return Attente::sur_interface_kuri(ID::init_execution_kuri);
            }
            if (!init_execution->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(init_execution);
            }
            auto corps = init_execution->corps;
            if (!corps->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(corps);
            }
            auto ajoute_init = noeud->comme_ajoute_init();
            corps->bloc->expressions->pousse_front(ajoute_init->expression);
            ajoute_init->drapeaux |= DECLARATION_FUT_VALIDEE;
            break;
        }
        case GenreNoeud::DIRECTIVE_PRE_EXECUTABLE:
        {
            auto pre_executable = noeud->comme_pre_executable();
            auto fichier = m_compilatrice.fichier(pre_executable->lexeme->fichier);
            auto module = fichier->module;
            if (module->directive_pre_executable) {
                espace
                    ->rapporte_erreur(
                        noeud, "Le module possède déjà une directive d'exécution pré-exécutable")
                    .ajoute_message("La première directive fut déclarée ici :")
                    .ajoute_site(module->directive_pre_executable);
                return CodeRetourValidation::Erreur;
            }
            module->directive_pre_executable = pre_executable;
            /* NOTE : le métaprogramme ne sera exécuté qu'à la fin de la génération de code. */
            cree_metaprogramme_pour_directive(pre_executable);
            pre_executable->drapeaux |= DECLARATION_FUT_VALIDEE;
            break;
        }
        case GenreNoeud::INSTRUCTION_CHARGE:
        {
            const auto inst = noeud->comme_charge();
            const auto lexeme = inst->expression->lexeme;
            const auto fichier = m_compilatrice.fichier(inst->lexeme->fichier);
            const auto temps = dls::chrono::compte_seconde();
            m_compilatrice.ajoute_fichier_a_la_compilation(
                espace, lexeme->chaine, fichier->module, inst->expression);
            noeud->drapeaux |= DECLARATION_FUT_VALIDEE;
            temps_chargement += temps.temps();
            break;
        }
        case GenreNoeud::INSTRUCTION_IMPORTE:
        {
            const auto inst = noeud->comme_importe();
            const auto lexeme = inst->expression->lexeme;
            const auto fichier = m_compilatrice.fichier(inst->lexeme->fichier);
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
                noeud_module->ident = module->nom();
                noeud_module->bloc_parent = inst->bloc_parent;
                noeud_module->bloc_parent->membres->ajoute(noeud_module);
                noeud_module->drapeaux |= DECLARATION_FUT_VALIDEE;
            }

            noeud->drapeaux |= DECLARATION_FUT_VALIDEE;
            break;
        }
        case GenreNoeud::DECLARATION_BIBLIOTHEQUE:
        {
            noeud->drapeaux |= DECLARATION_FUT_VALIDEE;
            break;
        }
        case GenreNoeud::DECLARATION_ENTETE_FONCTION:
        {
            auto decl = noeud->comme_entete_fonction();

            if (!decl->est_declaration_type) {
                return valide_entete_fonction(decl);
            }

            aplatis_arbre(decl);
            POUR (decl->arbre_aplatis) {
                auto resultat_validation = valide_semantique_noeud(it);
                if (!est_ok(resultat_validation)) {
                    return resultat_validation;
                }
            }

            auto types_entrees = kuri::tablet<Type *, 6>(decl->params.taille());

            for (auto i = 0; i < decl->params.taille(); ++i) {
                NoeudExpression *type_entree = decl->params[i];

                if (resoud_type_final(type_entree, types_entrees[i]) ==
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
                kuri::tablet<TypeCompose::Membre, 6> membres;
                membres.reserve(decl->params_sorties.taille());

                for (auto &type_declare : decl->params_sorties) {
                    if (resoud_type_final(type_declare, type_sortie) ==
                        CodeRetourValidation::Erreur) {
                        return CodeRetourValidation::Erreur;
                    }

                    // À FAIRE(état validation)
                    if ((type_sortie->drapeaux & TYPE_FUT_VALIDE) == 0) {
                        return Attente::sur_type(type_sortie);
                    }

                    membres.ajoute({nullptr, type_sortie});
                }

                type_sortie = m_compilatrice.typeuse.cree_tuple(membres);
            }

            auto type_fonction = m_compilatrice.typeuse.type_fonction(types_entrees, type_sortie);
            decl->type = m_compilatrice.typeuse.type_type_de_donnees(type_fonction);
            return CodeRetourValidation::OK;
        }
        case GenreNoeud::DECLARATION_CORPS_FONCTION:
        {
            auto decl = noeud->comme_corps_fonction();

            if (!decl->entete->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(decl->entete);
            }

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

            auto metaprogramme = cree_metaprogramme_pour_directive(noeud_directive);

            m_compilatrice.gestionnaire_code->requiers_compilation_metaprogramme(espace,
                                                                                 metaprogramme);

            noeud->type = noeud_directive->expression->type;

            if (racine_validation() != noeud) {
                /* avance l'index car il est inutile de revalider ce noeud */
                unite->index_courant += 1;
                return Attente::sur_metaprogramme(metaprogramme);
            }

            noeud->drapeaux |= DECLARATION_FUT_VALIDEE;
            break;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
        {
            return valide_reference_declaration(noeud->comme_reference_declaration(),
                                                noeud->bloc_parent);
        }
        case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
        {
            auto type_connu = m_compilatrice.typeuse.type_pour_lexeme(noeud->lexeme->genre);
            auto type_type = m_compilatrice.typeuse.type_type_de_donnees(type_connu);
            noeud->type = type_type;
            break;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
        {
            auto inst = noeud->comme_reference_membre();
            noeud->genre_valeur = GenreValeur::TRANSCENDANTALE;
            return valide_acces_membre(inst);
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
        case GenreNoeud::DECLARATION_OPAQUE:
        {
            return valide_type_opaque(noeud->comme_type_opaque());
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
        {
            noeud->genre_valeur = GenreValeur::DROITE;
            noeud->type = m_compilatrice.typeuse[TypeBase::R32];
            noeud->comme_litterale_reel()->valeur = noeud->lexeme->valeur_reelle;
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
        {
            noeud->genre_valeur = GenreValeur::DROITE;
            noeud->type = m_compilatrice.typeuse[TypeBase::ENTIER_CONSTANT];
            noeud->comme_litterale_entier()->valeur = noeud->lexeme->valeur_entiere;
            break;
        }
        case GenreNoeud::OPERATEUR_BINAIRE:
        {
            return valide_operateur_binaire(static_cast<NoeudExpressionBinaire *>(noeud));
        }
        case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
        {
            /* Nous devrions être ici uniquement si nous avions une attente. */
            return valide_operateur_binaire_chaine(static_cast<NoeudExpressionBinaire *>(noeud));
        }
        case GenreNoeud::OPERATEUR_UNAIRE:
        {
            CHRONO_TYPAGE(m_tacheronne.stats_typage.operateurs_unaire, "opérateur unaire");
            auto expr = noeud->comme_expression_unaire();
            expr->genre_valeur = GenreValeur::DROITE;

            auto enfant = expr->operande;
            auto type = enfant->type;

            if (type == nullptr) {
                espace->rapporte_erreur(
                    enfant, "Erreur interne : type nul pour l'opérande d'un opérateur unaire !");
                return CodeRetourValidation::Erreur;
            }

            if (type->est_type_de_donnees() && dls::outils::est_element(expr->lexeme->genre,
                                                                        GenreLexeme::FOIS_UNAIRE,
                                                                        GenreLexeme::ESP_UNAIRE)) {
                auto type_de_donnees = type->comme_type_de_donnees();
                auto type_connu = type_de_donnees->type_connu;

                if (type_connu == nullptr) {
                    type_connu = type_de_donnees;
                }

                if (expr->lexeme->genre == GenreLexeme::FOIS_UNAIRE) {
                    type_connu = m_compilatrice.typeuse.type_pointeur_pour(type_connu);
                }
                else if (expr->lexeme->genre == GenreLexeme::ESP_UNAIRE) {
                    type_connu = m_compilatrice.typeuse.type_reference_pour(type_connu);
                }

                noeud->type = m_compilatrice.typeuse.type_type_de_donnees(type_connu);
                break;
            }

            if (type->genre == GenreType::REFERENCE) {
                type = type_dereference_pour(type);

                /* Les références sont des pointeurs implicites, la prise d'adresse ne doit pas
                 * déréférencer. À FAIRE : ajout d'un transtypage référence -> pointeur */
                if (expr->lexeme->genre != GenreLexeme::FOIS_UNAIRE) {
                    transtype_si_necessaire(expr->operande, TypeTransformation::DEREFERENCE);
                }
            }

            if (expr->type == nullptr) {
                if (expr->lexeme->genre == GenreLexeme::FOIS_UNAIRE) {
                    if (!est_valeur_gauche(enfant->genre_valeur)) {
                        rapporte_erreur("Ne peut pas prendre l'adresse d'une valeur-droite.",
                                        enfant);
                        return CodeRetourValidation::Erreur;
                    }

                    expr->type = m_compilatrice.typeuse.type_pointeur_pour(type);
                }
                else if (expr->lexeme->genre == GenreLexeme::EXCLAMATION) {
                    if (!est_expression_convertible_en_bool(enfant)) {
                        rapporte_erreur(
                            "Ne peut pas appliquer l'opérateur « ! » au type de l'expression",
                            enfant);
                        return CodeRetourValidation::Erreur;
                    }

                    expr->type = m_compilatrice.typeuse[TypeBase::BOOL];
                }
                else {
                    if (type->genre == GenreType::ENTIER_CONSTANT) {
                        type = m_compilatrice.typeuse[TypeBase::Z32];
                        transtype_si_necessaire(
                            expr->operande, {TypeTransformation::CONVERTI_ENTIER_CONSTANT, type});
                    }

                    auto operateurs = m_compilatrice.operateurs.verrou_lecture();
                    auto op = cherche_operateur_unaire(*operateurs, type, expr->lexeme->genre);

                    if (op == nullptr) {
                        return Attente::sur_operateur(noeud);
                    }

                    expr->type = op->type_resultat;
                    expr->op = op;
                }
            }

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
                    VERIFIE_INTERFACE_KURI_CHARGEE(panique_tableau,
                                                   ID::panique_depassement_limites_tableau);
                    break;
                }
                case GenreType::TABLEAU_FIXE:
                {
                    auto type_tabl = type1->comme_tableau_fixe();
                    expr->type = type_dereference_pour(type1);

                    auto res = evalue_expression(m_compilatrice, enfant2->bloc_parent, enfant2);

                    if (!res.est_errone && res.valeur.est_entiere()) {
                        if (res.valeur.entiere() >= type_tabl->taille) {
                            rapporte_erreur_acces_hors_limites(
                                enfant2, type_tabl, res.valeur.entiere());
                            return CodeRetourValidation::Erreur;
                        }

                        /* nous savons que l'accès est dans les limites,
                         * évite d'émettre le code de vérification */
                        expr->aide_generation_code = IGNORE_VERIFICATION;
                    }

                    if (expr->aide_generation_code != IGNORE_VERIFICATION) {
                        VERIFIE_INTERFACE_KURI_CHARGEE(panique_tableau,
                                                       ID::panique_depassement_limites_tableau);
                    }

                    break;
                }
                case GenreType::POINTEUR:
                {
                    expr->type = type_dereference_pour(type1);
                    break;
                }
                default:
                {
                    auto candidats = kuri::tablet<OperateurCandidat, 10>();
                    auto resultat = cherche_candidats_operateurs(
                        *espace, type1, type2, GenreLexeme::CROCHET_OUVRANT, candidats);
                    if (resultat.has_value()) {
                        return resultat.value();
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
                        return attente_sur_operateur_ou_type(expr);
                    }

                    expr->type = meilleur_candidat->op->type_resultat;
                    expr->op = meilleur_candidat->op;
                    expr->permute_operandes = meilleur_candidat->permute_operandes;

                    transtype_si_necessaire(expr->operande_gauche,
                                            meilleur_candidat->transformation_type1);
                    transtype_si_necessaire(expr->operande_droite,
                                            meilleur_candidat->transformation_type2);
                }
            }

            auto type_cible = m_compilatrice.typeuse[TypeBase::Z64];
            auto type_index = enfant2->type;

            if (est_type_implicitement_utilisable_pour_indexage(type_index)) {
                transtype_si_necessaire(
                    expr->operande_droite,
                    {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_cible});
            }
            else {
                auto const resultat_transtype = transtype_si_necessaire(expr->operande_droite,
                                                                        type_cible);
                if (!est_ok(resultat_transtype)) {
                    return resultat_transtype;
                }
            }

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
            noeud->type = m_compilatrice.typeuse[TypeBase::CHAINE];
            noeud->genre_valeur = GenreValeur::DROITE;
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
        {
            noeud->genre_valeur = GenreValeur::DROITE;
            noeud->type = m_compilatrice.typeuse[TypeBase::BOOL];
            noeud->comme_litterale_bool()->valeur = noeud->lexeme->chaine == "vrai";
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
        {
            noeud->genre_valeur = GenreValeur::DROITE;
            noeud->type = m_compilatrice.typeuse[TypeBase::Z8];
            noeud->comme_litterale_caractere()->valeur = static_cast<uint32_t>(
                noeud->lexeme->valeur_entiere);
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

            if (!est_expression_convertible_en_bool(inst->condition)) {
                espace
                    ->rapporte_erreur(inst->condition,
                                      "Impossible de convertir implicitement l'expression vers "
                                      "une expression booléenne",
                                      erreur::Genre::TYPE_DIFFERENTS)
                    .ajoute_message(
                        "Le type de l'expression est ", chaine_type(type_condition), "\n");
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
                auto res = evalue_expression(m_compilatrice, inst->bloc_parent, inst->condition);

                if (res.est_errone) {
                    rapporte_erreur(
                        res.message_erreur, res.noeud_erreur, erreur::Genre::VARIABLE_REDEFINIE);
                    return CodeRetourValidation::Erreur;
                }

                if (!res.valeur.est_booleenne()) {
                    rapporte_erreur("L'expression d'un #si doit être de type booléenne", noeud);
                    return CodeRetourValidation::Erreur;
                }

                auto condition_est_vraie = res.valeur.booleenne();
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
                noeud->type = m_compilatrice.typeuse[TypeBase::RIEN];
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

            auto requiers_index = feuilles->expressions.taille() == 2;

            for (auto &f : feuilles->expressions) {
                /* Transforme les références en déclarations, nous faisons ça ici et non lors
                 * du syntaxage ou de la simplification de l'arbre afin de prendre en compte
                 * les cas où nous avons une fonction polymorphique : les données des déclarations
                 * ne sont pas copiées.
                 * Afin de ne pas faire de travail inutile, toutes les variables, saufs les
                 * variables d'indexage ne sont pas initialisées. Les variables d'indexages doivent
                 * l'être puisqu'elles sont directement testées avec la condition de fin de la
                 * boucle.
                 */
                auto init = NoeudExpression::nul();
                if (requiers_index && f != feuilles->expressions.derniere()) {
                    init = m_tacheronne.assembleuse->cree_non_initialisation(f->lexeme);
                }

                f = m_tacheronne.assembleuse->cree_declaration_variable(
                    f->comme_reference_declaration(), init);
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
#if 0
                            enfant1->type = enfant2->type;

                            df = enfant2->df;
                            auto nombre_vars_ret = df->idx_types_retours.taille();

                            if (feuilles.taille() == nombre_vars_ret) {
                                requiers_index = false;
                                noeud->aide_generation_code = GENERE_BOUCLE_COROUTINE;
                            }
                            else if (feuilles.taille() == nombre_vars_ret + 1) {
                                requiers_index = true;
                                noeud->aide_generation_code = GENERE_BOUCLE_COROUTINE_INDEX;
                            }
                            else {
                                rapporte_erreur("Mauvais compte d'arguments à déployer ",
                                                compilatrice,
                                                *enfant1->lexeme);
                            }
#endif
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
                    type = m_compilatrice.typeuse[TypeBase::Z8];
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
                        enfant1->type = m_compilatrice.typeuse[TypeBase::Z32];
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
                type = m_compilatrice.typeuse.type_reference_pour(type);
            }
            else if (inst->prend_pointeur) {
                type = m_compilatrice.typeuse.type_pointeur_pour(type);
            }

            noeud->aide_generation_code = aide_generation_code;

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
                    decl_idx->type = m_compilatrice.typeuse[TypeBase::Z32];
                }
                else {
                    decl_idx->type = m_compilatrice.typeuse[TypeBase::Z64];
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
                m_compilatrice, expr->expression->type, noeud->type);

            if (std::holds_alternative<Attente>(resultat)) {
                return std::get<Attente>(resultat);
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
            noeud->type = m_compilatrice.typeuse[TypeBase::PTR_NUL];
            break;
        }
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        {
            auto expr = noeud->comme_taille_de();
            expr->genre_valeur = GenreValeur::DROITE;
            expr->type = m_compilatrice.typeuse[TypeBase::N32];

            auto expr_type = expr->expression;
            if (resoud_type_final(expr_type, expr_type->type) == CodeRetourValidation::Erreur) {
                return CodeRetourValidation::Erreur;
            }

            if ((expr_type->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
                /* ce n'est plus la peine de revenir ici une fois que le type sera validé */
                unite->index_courant += 1;
                return Attente::sur_type(expr_type->type);
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
                type_debut = m_compilatrice.typeuse[TypeBase::Z32];
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
                type_feuille = m_compilatrice.typeuse[TypeBase::Z32];
                transtype_si_necessaire(
                    feuilles->expressions[0],
                    {TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_feuille});
            }

            for (auto i = 1; i < feuilles->expressions.taille(); ++i) {
                auto const resultat_transtype = transtype_si_necessaire(feuilles->expressions[i],
                                                                        type_feuille);
                if (!est_ok(resultat_transtype)) {
                    return resultat_transtype;
                }
            }

            noeud->type = m_compilatrice.typeuse.type_tableau_fixe(type_feuille,
                                                                   feuilles->expressions.taille());
            break;
        }
        case GenreNoeud::EXPRESSION_INFO_DE:
        {
            auto noeud_expr = noeud->comme_info_de();
            auto expr = noeud_expr->expression;

            Type *type = Type::nul();
            if (resoud_type_final(expr, type) == CodeRetourValidation::Erreur) {
                return CodeRetourValidation::Erreur;
            }

            /* Visite récursivement le type pour s'assurer que tous les types dépendants sont
             * validés, ceci est nécessaire pour garantir que les infos types seront générés avec
             * les bonnes données. À FAIRE : permet l'ajournement des infos-types afin de ne pas
             * avoir à attendre. */
            kuri::ensemblon<Type *, 16> types_utilises;
            types_utilises.insere(type);
            auto attente_possible = attente_sur_type_si_drapeau_manquant(types_utilises,
                                                                         TYPE_FUT_VALIDE);

            if (attente_possible && attente_possible->est<AttenteSurType>() &&
                attente_possible->type() != racine_validation()->type) {
                return attente_possible.value();
            }

            expr->type = type;

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
                    type_info_type = m_compilatrice.typeuse.type_info_type_;
                    break;
                }
                case GenreType::ENTIER_CONSTANT:
                case GenreType::ENTIER_NATUREL:
                case GenreType::ENTIER_RELATIF:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_entier;
                    break;
                }
                case GenreType::REFERENCE:
                case GenreType::POINTEUR:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_pointeur;
                    break;
                }
                case GenreType::STRUCTURE:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_structure;
                    break;
                }
                case GenreType::UNION:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_union;
                    break;
                }
                case GenreType::VARIADIQUE:
                case GenreType::TABLEAU_DYNAMIQUE:
                case GenreType::TABLEAU_FIXE:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_tableau;
                    break;
                }
                case GenreType::FONCTION:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_fonction;
                    break;
                }
                case GenreType::ENUM:
                case GenreType::ERREUR:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_enum;
                    break;
                }
                case GenreType::OPAQUE:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_opaque;
                    break;
                }
            }

            noeud->genre_valeur = GenreValeur::DROITE;
            noeud->type = m_compilatrice.typeuse.type_pointeur_pour(type_info_type);

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

            auto types_entrees = kuri::tablet<Type *, 6>(1);
            types_entrees[0] = m_compilatrice.typeuse.type_pointeur_pour(type);

            auto type_fonction = m_compilatrice.typeuse.type_fonction(
                types_entrees, m_compilatrice.typeuse[TypeBase::RIEN]);
            noeud->type = type_fonction;
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
                noeud->type = m_compilatrice.typeuse.type_type_de_donnees(expr_type->type);
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
            return valide_discrimination(inst);
        }
        case GenreNoeud::INSTRUCTION_RETIENS:
        {
            if (!fonction_courante() || !fonction_courante()->est_coroutine) {
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

            if (m_compilatrice.typeuse.type_contexte == nullptr) {
                return Attente::sur_interface_kuri(ID::ContexteProgramme);
            }

            if (!m_compilatrice.globale_contexte_programme_est_disponible()) {
                static auto noeud_expr = NoeudExpressionReference();
                noeud_expr.ident = ID::__contexte_fil_principal;
                return Attente::sur_symbole(&noeud_expr);
            }

            if (!m_compilatrice.globale_contexte_programme->possede_drapeau(
                    DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(m_compilatrice.globale_contexte_programme);
            }

            if (variable->type != m_compilatrice.typeuse.type_contexte) {
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
                auto type_var = m_compilatrice.typeuse.type_variadique(nullptr);
                expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_var);
                return CodeRetourValidation::OK;
            }

            auto type_expr = expr->expression->type;

            if (type_expr->genre == GenreType::TYPE_DE_DONNEES) {
                auto type_de_donnees = type_expr->comme_type_de_donnees();
                auto type_var = m_compilatrice.typeuse.type_variadique(
                    type_de_donnees->type_connu);
                expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_var);
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
                    type_expr = m_compilatrice.typeuse.type_tableau_dynamique(
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
                return Attente::sur_type(inst->type);
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
                VERIFIE_INTERFACE_KURI_CHARGEE(panique_erreur, ID::panique_erreur_non_geree);
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
                unite->espace
                    ->rapporte_erreur(
                        decl, "Impossible d'employer une variable n'étant pas une structure.")
                    .ajoute_message("Le type de la variable est : ")
                    .ajoute_message(chaine_type(type_employe))
                    .ajoute_message(".\n\n");
                return CodeRetourValidation::Erreur;
            }

            if ((type_employe->drapeaux & TYPE_FUT_VALIDE) == 0) {
                return Attente::sur_type(type_employe);
            }

            auto type_structure = type_employe->comme_structure();

            auto index_membre = 0;

            // pour les structures, prend le bloc_parent qui sera celui de la structure
            auto bloc_parent = decl->bloc_parent;

            // pour les fonctions, utilisent leurs blocs si le bloc_parent est le bloc_parent de la
            // fonction (ce qui est le cas pour les paramètres...)
            if (fonction_courante() &&
                bloc_parent == fonction_courante()->corps->bloc->bloc_parent) {
                bloc_parent = fonction_courante()->corps->bloc;
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

ResultatValidation ContexteValidationCode::valide_acces_membre(
    NoeudExpressionMembre *expression_membre)
{
    auto structure = expression_membre->accedee;
    auto membre = expression_membre->membre;

    if (structure->est_reference_declaration()) {
        auto decl = structure->comme_reference_declaration()->declaration_referee;

        if (decl->est_declaration_module()) {
            auto ref = membre->comme_reference_declaration();

            auto module_ref = decl->comme_declaration_module()->module;
            /* À FAIRE(gestion) : attente spécifique sur tout le module. */
            if (module_ref->bloc == nullptr) {
                return Attente::sur_symbole(ref);
            }

            auto res = valide_reference_declaration(ref, module_ref->bloc);
            if (!est_ok(res)) {
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
            return Attente::sur_type(type);
        }

        auto type_compose = static_cast<TypeCompose *>(type);

        auto membre_trouve = false;
        auto index_membre = 0;
        auto membre_est_constant = false;
        auto membre_est_implicite = false;
        auto decl_membre = NoeudDeclarationVariable::nul();

        POUR (type_compose->membres) {
            if (it.nom == membre->ident) {
                expression_membre->type = it.type;
                membre_trouve = true;
                membre_est_constant = it.drapeaux == TypeCompose::Membre::EST_CONSTANT;
                membre_est_implicite = it.drapeaux == TypeCompose::Membre::EST_IMPLICITE;
                decl_membre = it.decl;
                break;
            }

            index_membre += 1;
        }

        if (membre_trouve == false) {
            rapporte_erreur_membre_inconnu(expression_membre, membre, type_compose);
            return CodeRetourValidation::Erreur;
        }

        membre->comme_reference_declaration()->declaration_referee = decl_membre;

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
            if (structure->type->est_enum() && structure->genre_valeur != GenreValeur::DROITE) {
                if (type->est_enum() && static_cast<TypeEnum *>(type)->est_drapeau) {
                    if (!membre_est_implicite) {
                        expression_membre->genre_valeur = GenreValeur::TRANSCENDANTALE;
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
            VERIFIE_INTERFACE_KURI_CHARGEE(panique_membre_union, ID::panique_membre_union);
        }

        return CodeRetourValidation::OK;
    }

    espace
        ->rapporte_erreur(structure,
                          "Impossible de référencer un membre d'un type n'étant pas une structure")
        .ajoute_message("Note: le type est « ", chaine_type(type), " »");
    return CodeRetourValidation::Erreur;
}

static bool fonctions_ont_memes_definitions(NoeudDeclarationEnteteFonction const &fonction1,
                                            NoeudDeclarationEnteteFonction const &fonction2)
{
    if (fonction1.ident != fonction2.ident) {
        return false;
    }

    /* À FAIRE(bibliothèque) : stocke les fonctions des bibliothèques dans celles-ci, afin de
     * pouvoir comparer des fonctions externes même si elles sont définies par des modules
     * différents. */
    if (fonction1.possede_drapeau(EST_EXTERNE) && fonction2.possede_drapeau(EST_EXTERNE) &&
        fonction1.ident_bibliotheque == fonction2.ident_bibliotheque) {
        return true;
    }

    if (fonction1.type != fonction2.type) {
        return false;
    }

    /* Il est valide de redéfinir la fonction principale dans un autre espace. */
    if (fonction1.ident == ID::principale) {
        if (!fonction1.unite || !fonction2.unite) {
            /* S'il manque une unité, nous revérifierons lors de la validation de la deuxième
             * fonction. */
            return false;
        }

        if (fonction1.unite->espace != fonction2.unite->espace) {
            return false;
        }
    }

    return true;
}

ResultatValidation ContexteValidationCode::valide_entete_fonction(
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

    /* Valide les constantes polymorphiques. */
    if (decl->est_polymorphe) {
        decl->bloc_constantes->membres.avec_verrou_ecriture(
            [this](kuri::tableau<NoeudDeclaration *, int> &membres) {
                POUR (membres) {
                    auto type_poly = m_compilatrice.typeuse.cree_polymorphique(it->ident);
                    it->type = m_compilatrice.typeuse.type_type_de_donnees(type_poly);
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
        auto resultat_validation = valide_arbre_aplatis(decl, decl->arbre_aplatis);
        if (!est_ok(resultat_validation)) {
            return resultat_validation;
        }
    }

    // -----------------------------------
    {
        CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions,
                      "valide_type_fonction (validation paramètres)");
        auto noms = kuri::ensemblon<IdentifiantCode *, 16>();
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
    {
        CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide_type_fonction (typage)");

        kuri::tablet<Type *, 6> types_entrees;
        types_entrees.reserve(decl->params.taille());

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
            kuri::tablet<TypeCompose::Membre, 6> membres;
            membres.reserve(decl->params_sorties.taille());

            for (auto &expr : decl->params_sorties) {
                auto type_declare = expr->comme_declaration_variable();
                if (resoud_type_final(type_declare->expression_type, type_sortie) ==
                    CodeRetourValidation::Erreur) {
                    return CodeRetourValidation::Erreur;
                }

                // À FAIRE(état validation) : nous ne devrions pas revalider les paramètres
                if ((type_sortie->drapeaux & TYPE_FUT_VALIDE) == 0) {
                    return Attente::sur_type(type_sortie);
                }

                membres.ajoute({nullptr, type_sortie});
            }

            type_sortie = m_compilatrice.typeuse.cree_tuple(membres);
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

            if (decl->param_sortie->type != m_compilatrice.typeuse[TypeBase::Z32]) {
                espace->rapporte_erreur(decl->param_sortie,
                                        "La fonction principale doit retourner un z32 !");
                return CodeRetourValidation::Erreur;
            }
        }

        CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide_type_fonction (type_fonction)");
        type_fonc = m_compilatrice.typeuse.type_fonction(types_entrees, type_sortie);
        decl->type = type_fonc;
    }

    if (decl->est_operateur) {
        CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide_type_fonction (opérateurs)");
        auto type_resultat = type_fonc->type_sortie;

        if (type_resultat == m_compilatrice.typeuse[TypeBase::RIEN]) {
            rapporte_erreur("Un opérateur ne peut retourner 'rien'", decl);
            return CodeRetourValidation::Erreur;
        }

        if (est_operateur_bool(decl->lexeme->genre) &&
            type_resultat != m_compilatrice.typeuse[TypeBase::BOOL]) {
            rapporte_erreur("Un opérateur de comparaison doit retourner 'bool'", decl);
            return CodeRetourValidation::Erreur;
        }

        auto operateurs = m_compilatrice.operateurs.verrou_ecriture();

        if (decl->params.taille() == 1) {
            auto &iter_op = operateurs->trouve_unaire(decl->lexeme->genre);
            auto type1 = type_fonc->types_entrees[0];

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
            auto type1 = type_fonc->types_entrees[0];
            auto type2 = type_fonc->types_entrees[1];

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
        // Le point d'entrée est copié pour chaque espace, donc il est possible qu'il existe
        // plusieurs fois dans le bloc.
        if (decl->ident != ID::__point_d_entree_systeme) {
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

                        auto decl_it = it->comme_entete_fonction();

                        if (fonctions_ont_memes_definitions(*decl, *decl_it)) {
                            rapporte_erreur_redefinition_fonction(decl, decl_it);
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

    // À FAIRE: n'utilise externe que pour les fonctions vraiment externes...
    if (decl->est_externe && decl->ident && decl->ident->nom != "__principale" &&
        !decl->possede_drapeau(COMPILATRICE)) {
        auto bibliotheque = m_compilatrice.gestionnaire_bibliotheques->trouve_bibliotheque(
            decl->ident_bibliotheque);

        if (!bibliotheque) {
            espace
                ->rapporte_erreur(decl,
                                  "Impossible de définir la bibliothèque où trouver la fonction")
                .ajoute_message(
                    "« ", decl->ident_bibliotheque->nom, " » ne réfère à aucune bibliothèque !");
            return CodeRetourValidation::Erreur;
        }

        decl->symbole = bibliotheque->cree_symbole(decl->nom_symbole);
    }

    decl->drapeaux |= DECLARATION_FUT_VALIDEE;

#ifdef STATISTIQUES_DETAILLEES
    possede_erreur = false;
#endif

    return CodeRetourValidation::OK;
}

ResultatValidation ContexteValidationCode::valide_arbre_aplatis(
    NoeudExpression *declaration, kuri::tableau<NoeudExpression *, int> &arbre_aplatis)
{
    aplatis_arbre(declaration);

    for (; unite->index_courant < arbre_aplatis.taille(); ++unite->index_courant) {
        auto noeud_enfant = arbre_aplatis[unite->index_courant];

        if (noeud_enfant->est_structure()) {
            /* Les structures nichées ont leurs propres unités de compilation */
            if (!noeud_enfant->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(noeud_enfant->comme_structure());
            }

            continue;
        }

        if (noeud_enfant->est_entete_fonction() &&
            !noeud_enfant->comme_entete_fonction()->est_declaration_type &&
            noeud_enfant != fonction_courante()) {
            /* Les fonctions nichées dans d'autres fonctions ont leurs propres unités de
             * compilation. */
            if (!noeud_enfant->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(noeud_enfant->comme_entete_fonction());
            }

            continue;
        }

        auto resultat_validation = valide_semantique_noeud(noeud_enfant);

        if (std::holds_alternative<Attente>(resultat_validation)) {
            return std::get<Attente>(resultat_validation);
        }

        auto code_etat = std::get<CodeRetourValidation>(resultat_validation);

        if (code_etat == CodeRetourValidation::Erreur) {
            return CodeRetourValidation::Erreur;
        }
    }

    return CodeRetourValidation::OK;
}

static void rassemble_expressions(NoeudExpression *expr,
                                  kuri::tablet<NoeudExpression *, 6> &expressions)
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
                                  kuri::tablet<VariableEtExpression, 6> &expressions)
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

static bool peut_construire_union_via_rien(TypeUnion *type_union)
{
    POUR (type_union->membres) {
        if (it.type->est_rien()) {
            return true;
        }
    }

    return false;
}

ResultatValidation ContexteValidationCode::valide_expression_retour(NoeudRetour *inst)
{
    auto type_fonc = fonction_courante()->type->comme_fonction();
    auto est_corps_texte = fonction_courante()->corps->est_corps_texte;

    auto const bloc_parent = inst->bloc_parent;
    if (bloc_est_dans_differe(bloc_parent)) {
        rapporte_erreur("« retourne » utilisée dans un bloc « diffère »", inst);
        return CodeRetourValidation::Erreur;
    }

    if (inst->expression == nullptr) {
        inst->type = m_compilatrice.typeuse[TypeBase::RIEN];

        auto type_sortie = type_fonc->type_sortie;

        /* Vérifie si le type de sortie est une union, auquel cas nous pouvons retourner une valeur
         * du type ayant le membre « rien » actif. */
        if (type_sortie->est_union() && !type_sortie->comme_union()->est_nonsure) {
            if (peut_construire_union_via_rien(type_fonc->type_sortie->comme_union())) {
                inst->aide_generation_code = RETOURNE_UNE_UNION_VIA_RIEN;
                return CodeRetourValidation::OK;
            }
        }

        if ((!fonction_courante()->est_coroutine && type_sortie != inst->type) ||
            est_corps_texte) {
            rapporte_erreur("Expression de retour manquante", inst);
            return CodeRetourValidation::Erreur;
        }

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

        inst->type = m_compilatrice.typeuse[TypeBase::CHAINE];

        DonneesAssignations donnees;
        donnees.expression = inst->expression;
        donnees.variables.ajoute(fonction_courante()->params_sorties[0]);
        donnees.transformations.ajoute({});

        inst->donnees_exprs.ajoute(std::move(donnees));
        return CodeRetourValidation::OK;
    }

    if (type_fonc->type_sortie->est_rien()) {
        espace->rapporte_erreur(inst->expression,
                                "Retour d'une valeur d'une fonction qui ne retourne rien");
        return CodeRetourValidation::Erreur;
    }

    kuri::file_fixe<NoeudExpression *, 6> variables;

    POUR (fonction_courante()->params_sorties) {
        variables.enfile(it);
    }

    /* tri les expressions selon les noms */
    kuri::tablet<VariableEtExpression, 6> vars_et_exprs;
    rassemble_expressions(inst->expression, vars_et_exprs);

    kuri::tablet<NoeudExpression *, 6> expressions;
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

            for (auto i = 0; i < fonction_courante()->params_sorties.taille(); ++i) {
                if (it.ident == fonction_courante()->params_sorties[i]->ident) {
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
                                          Type *type_de_l_expression) -> ResultatValidation {
        auto resultat = cherche_transformation(
            m_compilatrice, type_de_l_expression, variable->type);

        if (std::holds_alternative<Attente>(resultat)) {
            return std::get<Attente>(resultat);
        }

        auto transformation = std::get<TransformationType>(resultat);

        if (transformation.type == TypeTransformation::IMPOSSIBLE) {
            rapporte_erreur_assignation_type_differents(
                variable->type, type_de_l_expression, expression);
            return CodeRetourValidation::Erreur;
        }

        donnees.variables.ajoute(variable);
        donnees.transformations.ajoute(transformation);
        return CodeRetourValidation::OK;
    };

    kuri::tablet<DonneesAssignations, 6> donnees_retour;

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

                auto resultat = valide_typage_et_ajoute(
                    donnees, variables.defile(), it, membre.type);
                if (!est_ok(resultat)) {
                    return resultat;
                }
            }
        }
        else {
            if (variables.est_vide()) {
                espace->rapporte_erreur(it, "Trop d'expressions de retour");
                return CodeRetourValidation::Erreur;
            }

            auto resultat = valide_typage_et_ajoute(donnees, variables.defile(), it, it->type);
            if (!est_ok(resultat)) {
                return resultat;
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

    return CodeRetourValidation::OK;
}

ResultatValidation ContexteValidationCode::valide_cuisine(NoeudDirectiveCuisine *directive)
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
    return CodeRetourValidation::OK;
}

static bool est_declaration_polymorphique(NoeudDeclaration const *decl)
{
    if (decl->est_entete_fonction()) {
        auto const entete = decl->comme_entete_fonction();
        return entete->est_polymorphe;
    }

    if (decl->est_structure()) {
        auto const structure = decl->comme_structure();
        return structure->est_polymorphe;
    }

    return false;
}

ResultatValidation ContexteValidationCode::valide_reference_declaration(
    NoeudExpressionReference *expr, NoeudBloc *bloc_recherche)
{
    CHRONO_TYPAGE(m_tacheronne.stats_typage.ref_decl, "valide référence déclaration");

    expr->genre_valeur = GenreValeur::TRANSCENDANTALE;

    assert_rappel(bloc_recherche != nullptr, [&]() { erreur::imprime_site(*espace, expr); });
    auto fichier = m_compilatrice.fichier(expr->lexeme->fichier);

#if 0
	/* À FAIRE : nous devrions gérer les déclarations multiples, mais il est possible
	 * qu'un module soit nommé comme une structure dans celui-ci, donc une expression
	 * du style X.X sera toujours erronnée.
	 */
 auto declarations = kuri::tablet<NoeudDeclaration *, 10>();
	trouve_declarations_dans_bloc_ou_module(declarations, bloc_recherche, expr->ident, fichier);

	if (declarations.taille() == 0) {
        return Attente::sur_symbole(expr);
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
        if (fonction_courante() && fonction_courante()->est_monomorphisation) {
            auto site_monomorphisation = fonction_courante()->site_monomorphisation;

            fichier = m_compilatrice.fichier(site_monomorphisation->lexeme->fichier);
            decl = trouve_dans_bloc_ou_module(
                site_monomorphisation->bloc_parent, expr->ident, fichier);
        }
        if (decl == nullptr) {
            return Attente::sur_symbole(expr);
        }
    }
#endif

    if (decl->lexeme->fichier == expr->lexeme->fichier &&
        decl->genre == GenreNoeud::DECLARATION_VARIABLE && !decl->possede_drapeau(EST_GLOBALE)) {
        if (decl->lexeme->ligne > expr->lexeme->ligne) {
            rapporte_erreur("Utilisation d'une variable avant sa déclaration", expr);
            return CodeRetourValidation::Erreur;
        }
    }

    if (decl->est_declaration_type()) {
        if (decl->est_type_opaque() && !decl->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
            return Attente::sur_declaration(decl);
        }

        /* Ne vérifions pas seulement le drapeau DECLARATION_FUT_VALIDEE, car la référence peut
         * être vers le type en validation (p.e. un pointeur vers une autre instance de la
         * structure). */
        if (!decl->type) {
            return Attente::sur_declaration(decl);
        }

        expr->type = m_compilatrice.typeuse.type_type_de_donnees(decl->type);
        expr->declaration_referee = decl;
        expr->genre_valeur = GenreValeur::DROITE;
    }
    else {
        if (!decl->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
            // À FAIRE : curseur := curseur.curseurs[0] -> il faut pouvoir déterminer si la
            // référence est celle de la variable que l'on valide, ceci ne fonctionnera pas pour
            // les déclarations multiples, ou les types étant référencés dans les expressions de
            // leurs membres
            if (decl == unite->noeud) {
                espace->rapporte_erreur(expr, "Utilisation d'une variable dans sa définition !\n");
                return CodeRetourValidation::Erreur;
            }
            return Attente::sur_declaration(decl);
        }

        if (est_declaration_polymorphique(decl) &&
            !expr->possede_drapeau(GAUCHE_EXPRESSION_APPEL)) {
            espace->rapporte_erreur(
                expr,
                "Référence d'une déclaration polymorphique en dehors d'une expression d'appel");
            return CodeRetourValidation::Erreur;
        }

        // les fonctions peuvent ne pas avoir de type au moment si elles sont des appels
        // polymorphiques
        assert(decl->type || decl->est_entete_fonction() || decl->est_declaration_module());
        expr->declaration_referee = decl;
        decl->drapeaux |= EST_UTILISEE;
        if (decl->est_declaration_variable()) {
            auto decl_var = decl->comme_declaration_variable();
            if (decl_var->declaration_vient_d_un_emploi) {
                decl_var->declaration_vient_d_un_emploi->drapeaux |= EST_UTILISEE;
            }
        }
        expr->type = decl->type;

        /* si nous avons une valeur polymorphique, crée un type de données
         * temporaire pour que la validation soit contente, elle sera
         * remplacée par une constante appropriée lors de la validation
         * de l'appel */
        if (decl->drapeaux & EST_VALEUR_POLYMORPHIQUE) {
            expr->type = m_compilatrice.typeuse.type_type_de_donnees(expr->type);
        }
    }

    if (decl->possede_drapeau(EST_CONSTANTE)) {
        if (decl->est_declaration_variable()) {
            auto valeur = decl->comme_declaration_variable()->valeur_expression;
            /* Remplace tout de suite les constantes de fonctions par les fonctions, pour ne pas
             * avoir à s'en soucier plus tard. */
            if (valeur.est_fonction()) {
                decl = valeur.fonction();
                expr->declaration_referee = decl;
            }
        }

        expr->genre_valeur = GenreValeur::DROITE;
    }

    if (decl->est_entete_fonction() && !decl->comme_entete_fonction()->est_polymorphe) {
        expr->genre_valeur = GenreValeur::DROITE;
    }

    return CodeRetourValidation::OK;
}

ResultatValidation ContexteValidationCode::valide_type_opaque(NoeudDeclarationTypeOpaque *decl)
{
    auto type_opacifie = Type::nul();

    if (!decl->expression_type->possede_drapeau(DECLARATION_TYPE_POLYMORPHIQUE)) {
        if (resoud_type_final(decl->expression_type, type_opacifie) ==
            CodeRetourValidation::Erreur) {
            return CodeRetourValidation::Erreur;
        }

        if ((type_opacifie->drapeaux & TYPE_FUT_VALIDE) == 0) {
            return Attente::sur_type(type_opacifie);
        }
    }
    else {
        type_opacifie = m_compilatrice.typeuse.cree_polymorphique(decl->expression_type->ident);
    }

    auto type_opaque = m_compilatrice.typeuse.cree_opaque(decl, type_opacifie);
    decl->type = type_opaque;
    decl->drapeaux |= DECLARATION_FUT_VALIDEE;
    return CodeRetourValidation::OK;
}

MetaProgramme *ContexteValidationCode::cree_metaprogramme_corps_texte(NoeudBloc *bloc_corps_texte,
                                                                      NoeudBloc *bloc_parent,
                                                                      const Lexeme *lexeme)
{
    auto fonction = m_tacheronne.assembleuse->cree_entete_fonction(lexeme);
    auto nouveau_corps = fonction->corps;

    m_tacheronne.assembleuse->bloc_courant(bloc_parent);

    fonction->bloc_constantes = m_tacheronne.assembleuse->empile_bloc(lexeme);
    fonction->bloc_parametres = m_tacheronne.assembleuse->empile_bloc(lexeme);

    fonction->bloc_parent = bloc_parent;
    nouveau_corps->bloc_parent = fonction->bloc_parametres;
    /* Le corps de la fonction pour les #corps_texte des structures est celui de la déclaration. */
    nouveau_corps->bloc = bloc_corps_texte;

    /* mise en place du type de la fonction : () -> chaine */
    fonction->est_metaprogramme = true;

    auto decl_sortie = m_tacheronne.assembleuse->cree_declaration_variable(lexeme);
    decl_sortie->ident = m_compilatrice.table_identifiants->identifiant_pour_chaine("__ret0");
    decl_sortie->type = m_compilatrice.typeuse[TypeBase::CHAINE];
    decl_sortie->drapeaux |= DECLARATION_FUT_VALIDEE;

    fonction->params_sorties.ajoute(decl_sortie);
    fonction->param_sortie = decl_sortie;

    auto types_entrees = kuri::tablet<Type *, 6>(0);

    auto type_sortie = m_compilatrice.typeuse[TypeBase::CHAINE];

    fonction->type = m_compilatrice.typeuse.type_fonction(types_entrees, type_sortie);
    fonction->drapeaux |= DECLARATION_FUT_VALIDEE;

    auto metaprogramme = m_compilatrice.cree_metaprogramme(espace);
    metaprogramme->corps_texte = bloc_corps_texte;
    metaprogramme->fonction = fonction;

    m_tacheronne.assembleuse->depile_bloc();
    m_tacheronne.assembleuse->depile_bloc();

    return metaprogramme;
}

NoeudExpression *ContexteValidationCode::racine_validation() const
{
    assert(unite->noeud);
    return unite->noeud;
}

NoeudDeclarationEnteteFonction *ContexteValidationCode::fonction_courante() const
{
    if (racine_validation()->est_entete_fonction()) {
        return racine_validation()->comme_entete_fonction();
    }

    if (racine_validation()->est_corps_fonction()) {
        return racine_validation()->comme_corps_fonction()->entete;
    }

    return nullptr;
}

Type *ContexteValidationCode::union_ou_structure_courante() const
{
    if (racine_validation()->est_structure()) {
        return racine_validation()->type;
    }

    return nullptr;
}

static void avertis_declarations_inutilisees(EspaceDeTravail const &espace,
                                             NoeudDeclarationEnteteFonction const &entete)
{
    if (entete.est_externe) {
        return;
    }

    /* Les paramètres de sortie sont toujours utilisés.
     * À FAIRE : nous avons les paramètres de sorties des fonctions nichées dans le bloc de cette
     * fonction ?
     */
    POUR (entete.params_sorties) {
        it->comme_declaration_variable()->drapeaux |= EST_UTILISEE;
    }
    entete.param_sortie->drapeaux |= EST_UTILISEE;

    for (int i = 0; i < entete.params.taille(); ++i) {
        auto decl_param = entete.parametre_entree(i);
        if (possede_annotation(decl_param, "inutilisée")) {
            continue;
        }

        if (!decl_param->possede_drapeau(EST_UTILISEE)) {
            espace.rapporte_avertissement(decl_param, "Paramètre inutilisé");
        }
    }

    auto const &corps = *entete.corps;

    visite_noeud(corps.bloc,
                 PreferenceVisiteNoeud::ORIGINAL,
                 [&espace, entete](const NoeudExpression *noeud) {
                     if (noeud->est_structure()) {
                         return DecisionVisiteNoeud::IGNORE_ENFANTS;
                     }

                     /* À FAIRE(visite noeud) : évaluation des #si pour savoir quel bloc traverser.
                      */
                     if (noeud->est_si_statique()) {
                         return DecisionVisiteNoeud::IGNORE_ENFANTS;
                     }

                     if (!noeud->est_declaration()) {
                         return DecisionVisiteNoeud::CONTINUE;
                     }

                     /* Ignore les variables implicites des boucles « pour ». */
                     if (noeud->ident == ID::it || noeud->ident == ID::index_it) {
                         return DecisionVisiteNoeud::CONTINUE;
                     }

                     /* '_' est un peu spécial, il sers à définir une variable qui ne sera pas
                      * utilisée, bien que ceci ne soit pas en score formalisé dans le langage. */
                     if (noeud->ident && noeud->ident->nom == "_") {
                         return DecisionVisiteNoeud::CONTINUE;
                     }

                     if (noeud->est_declaration_variable()) {
                         auto const decl_var = noeud->comme_declaration_variable();
                         /* Les déclarations multiples comme « a, b := ... » ont les déclarations
                          * ajoutées séparément aux membres du bloc. */
                         if (decl_var->valeur->est_virgule()) {
                             return DecisionVisiteNoeud::CONTINUE;
                         }

                         if (possede_annotation(decl_var, "inutilisée")) {
                             return DecisionVisiteNoeud::CONTINUE;
                         }
                     }

                     /* Les corps fonctions sont des déclarations et sont visités, mais ne sont pas
                      * marqués comme utilisés car seules les entêtes le sont. Évitons d'émettre un
                      * avertissement pour rien. */
                     if (noeud->est_corps_fonction()) {
                         return DecisionVisiteNoeud::CONTINUE;
                     }

                     if (!noeud->possede_drapeau(EST_UTILISEE)) {
                         if (noeud->est_entete_fonction()) {
                             auto entete_ = noeud->comme_entete_fonction();
                             if (!entete_->est_declaration_type) {
                                 auto message = enchaine("Dans la fonction ",
                                                         entete.ident->nom,
                                                         " : fonction « ",
                                                         (noeud->ident ?
                                                              noeud->ident->nom :
                                                              kuri::chaine_statique("")),
                                                         " » inutilisée");
                                 espace.rapporte_avertissement(noeud, message);
                             }

                             /* Ne traverse pas la fonction nichée. */
                             return DecisionVisiteNoeud::IGNORE_ENFANTS;
                         }

                         auto message = enchaine(
                             "Dans la fonction ",
                             entete.ident->nom,
                             " : déclaration « ",
                             (noeud->ident ? noeud->ident->nom : kuri::chaine_statique("")),
                             " » inutilisée");
                         espace.rapporte_avertissement(noeud, message);
                     }

                     /* Ne traversons pas les fonctions nichées. Nous arrivons ici uniquement si la
                      * fonction fut utilisée. */
                     if (noeud->est_entete_fonction()) {
                         return DecisionVisiteNoeud::IGNORE_ENFANTS;
                     }

                     return DecisionVisiteNoeud::CONTINUE;
                 });
}

ResultatValidation ContexteValidationCode::valide_fonction(NoeudDeclarationCorpsFonction *decl)
{
    auto entete = decl->entete;

    if (entete->est_polymorphe && !entete->est_monomorphisation) {
        // nous ferons l'analyse sémantique plus tard
        decl->drapeaux |= DECLARATION_FUT_VALIDEE;
        return CodeRetourValidation::OK;
    }

    decl->type = entete->type;

    auto est_corps_texte = decl->est_corps_texte;

    if (est_corps_texte && !decl->possede_drapeau(METAPROGRAMME_CORPS_TEXTE_FUT_CREE)) {
        auto metaprogramme = cree_metaprogramme_corps_texte(
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
        fonction->site_monomorphisation = entete->site_monomorphisation;

        // préserve les constantes polymorphiques
        if (fonction->est_monomorphisation) {
            POUR (*entete->bloc_constantes->membres.verrou_lecture()) {
                fonction->bloc_constantes->membres->ajoute(it);
            }
        }

        decl->drapeaux |= METAPROGRAMME_CORPS_TEXTE_FUT_CREE;

        /* Puisque nous validons le #corps_texte, l'entête pour la fonction courante doit être
         * celle de la fonction de métaprogramme. */
        entete = fonction;
    }

    CHRONO_TYPAGE(m_tacheronne.stats_typage.fonctions, "valide fonction");

    auto resultat_validation = valide_arbre_aplatis(decl, decl->arbre_aplatis);
    if (!est_ok(resultat_validation)) {
        return resultat_validation;
    }

    auto bloc = decl->bloc;
    auto inst_ret = derniere_instruction(bloc);

    /* si aucune instruction de retour -> vérifie qu'aucun type n'a été spécifié */
    if (inst_ret == nullptr) {
        auto type_fonc = entete->type->comme_fonction();
        auto type_sortie = type_fonc->type_sortie;

        if (type_sortie->est_union() && !type_sortie->comme_union()->est_nonsure) {
            if (peut_construire_union_via_rien(type_sortie->comme_union())) {
                decl->aide_generation_code = REQUIERS_RETOUR_UNION_VIA_RIEN;
            }
        }
        else {
            if ((type_fonc->type_sortie->genre != GenreType::RIEN && !entete->est_coroutine) ||
                est_corps_texte) {
                rapporte_erreur(
                    "Instruction de retour manquante", decl, erreur::Genre::TYPE_DIFFERENTS);
                return CodeRetourValidation::Erreur;
            }
        }

        if (decl->aide_generation_code != REQUIERS_RETOUR_UNION_VIA_RIEN &&
            entete != m_compilatrice.interface_kuri->decl_creation_contexte) {
            decl->aide_generation_code = REQUIERS_CODE_EXTRA_RETOUR;
        }
    }

    simplifie_arbre(unite->espace, m_tacheronne.assembleuse, m_compilatrice.typeuse, entete);

    if (est_corps_texte) {
        /* Puisque la validation du #corps_texte peut être interrompue, nous devons retrouver le
         * métaprogramme : nous ne pouvons pas prendre l'adresse du métaprogramme créé ci-dessus.
         * À FAIRE : considère réusiner la gestion des métaprogrammes dans le GestionnaireCode afin
         * de pouvoir requérir la compilation du métaprogramme dès sa création, mais d'attendre que
         * la fonction soit validée afin de le compiler.
         */
        auto metaprogramme = m_compilatrice.metaprogramme_pour_fonction(entete);
        m_compilatrice.gestionnaire_code->requiers_compilation_metaprogramme(espace,
                                                                             metaprogramme);
    }

    decl->drapeaux |= DECLARATION_FUT_VALIDEE;

    avertis_declarations_inutilisees(*espace, *entete);

    return CodeRetourValidation::OK;
}

ResultatValidation ContexteValidationCode::valide_operateur(NoeudDeclarationCorpsFonction *decl)
{
    auto entete = decl->entete;
    decl->type = entete->type;

    auto resultat_validation = valide_arbre_aplatis(decl, decl->arbre_aplatis);
    if (!est_ok(resultat_validation)) {
        return resultat_validation;
    }

    auto inst_ret = derniere_instruction(decl->bloc);

    if (inst_ret == nullptr) {
        rapporte_erreur("Instruction de retour manquante", decl, erreur::Genre::TYPE_DIFFERENTS);
        return CodeRetourValidation::Erreur;
    }

    simplifie_arbre(unite->espace, m_tacheronne.assembleuse, m_compilatrice.typeuse, entete);
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
ResultatValidation ContexteValidationCode::valide_enum_impl(NoeudEnum *decl, TypeEnum *type_enum)
{
    auto &graphe = m_compilatrice.graphe_dependance;
    graphe->connecte_type_type(type_enum, type_enum->type_donnees);

    type_enum->taille_octet = type_enum->type_donnees->taille_octet;
    type_enum->alignement = type_enum->type_donnees->alignement;

    m_compilatrice.operateurs->ajoute_operateur_basique_enum(m_compilatrice.typeuse, type_enum);

    auto noms_rencontres = kuri::ensemblon<IdentifiantCode *, 32>();

    auto derniere_valeur = ValeurExpression();
    assert(!derniere_valeur.est_valide());

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
        assert(!valeur.est_valide());

        if (expr != nullptr) {
            auto res = evalue_expression(m_compilatrice, decl->bloc, expr);

            if (res.est_errone) {
                espace->rapporte_erreur(res.noeud_erreur, res.message_erreur);
                return CodeRetourValidation::Erreur;
            }

            if (N == VALIDE_ENUM_ERREUR) {
                if (res.valeur.entiere() == 0) {
                    espace->rapporte_erreur(
                        expr,
                        "L'expression d'une enumération erreur ne peut s'évaluer à 0 (cette "
                        "valeur est réservée par la compilatrice).");
                    return CodeRetourValidation::Erreur;
                }
            }

            if (!res.valeur.est_entiere()) {
                espace->rapporte_erreur(expr,
                                        "L'expression d'une énumération doit être de type entier");
                return CodeRetourValidation::Erreur;
            }

            valeur = res.valeur;
        }
        else {
            /* Une expression invalide indique que nous sommes dans la première itération. */
            if (!derniere_valeur.est_valide()) {
                valeur = (N == VALIDE_ENUM_DRAPEAU || N == VALIDE_ENUM_ERREUR) ? 1 : 0;
            }
            else {
                if (N == VALIDE_ENUM_DRAPEAU) {
                    valeur = derniere_valeur.entiere() * 2;

                    if (!est_puissance_de_2(valeur.entiere())) {
                        espace->rapporte_erreur(decl_expr,
                                                "La valeur implicite d'une énumération drapeau "
                                                "doit être une puissance de 2 !");
                        return CodeRetourValidation::Erreur;
                    }
                }
                else {
                    valeur = derniere_valeur.entiere() + 1;
                }
            }
        }

        if (est_hors_des_limites(valeur.entiere(), type_enum->type_donnees)) {
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
            e.ajoute_message("Or, la valeur courante est de ", valeur.entiere(), ".\n");
            return CodeRetourValidation::Erreur;
        }

        valeur_enum_min = std::min(valeur.entiere(), valeur_enum_min);
        valeur_enum_max = std::max(valeur.entiere(), valeur_enum_max);

        if (N == VALIDE_ENUM_DRAPEAU) {
            valeurs_legales |= valeur.entiere();
        }

        membres.ajoute({nullptr, type_enum, var->ident, 0, static_cast<int>(valeur.entiere())});

        derniere_valeur = valeur;
    }

    membres.ajoute({nullptr,
                    m_compilatrice.typeuse[TypeBase::Z32],
                    ID::nombre_elements,
                    0,
                    membres.taille(),
                    nullptr,
                    TypeCompose::Membre::EST_IMPLICITE});
    membres.ajoute({nullptr,
                    type_enum,
                    ID::min,
                    0,
                    static_cast<int>(valeur_enum_min),
                    nullptr,
                    TypeCompose::Membre::EST_IMPLICITE});
    membres.ajoute({nullptr,
                    type_enum,
                    ID::max,
                    0,
                    static_cast<int>(valeur_enum_max),
                    nullptr,
                    TypeCompose::Membre::EST_IMPLICITE});

    if (N == VALIDE_ENUM_DRAPEAU) {
        membres.ajoute({nullptr,
                        type_enum,
                        ID::valeurs_legales,
                        0,
                        static_cast<int>(valeurs_legales),
                        nullptr,
                        TypeCompose::Membre::EST_IMPLICITE});
        membres.ajoute({nullptr,
                        type_enum,
                        ID::valeurs_illegales,
                        0,
                        static_cast<int>(~valeurs_legales),
                        nullptr,
                        TypeCompose::Membre::EST_IMPLICITE});
        membres.ajoute(
            {nullptr, type_enum, ID::zero, 0, 0, nullptr, TypeCompose::Membre::EST_IMPLICITE});
    }

    decl->drapeaux |= DECLARATION_FUT_VALIDEE;
    decl->type->drapeaux |= TYPE_FUT_VALIDE;
    return CodeRetourValidation::OK;
}

ResultatValidation ContexteValidationCode::valide_enum(NoeudEnum *decl)
{
    CHRONO_TYPAGE(m_tacheronne.stats_typage.enumerations, "valide énum");
    auto type_enum = static_cast<TypeEnum *>(decl->type);

    if (type_enum->est_erreur) {
        type_enum->type_donnees = m_compilatrice.typeuse[TypeBase::Z32];
    }
    else if (decl->expression_type != nullptr) {
        auto resultat_validation = valide_semantique_noeud(decl->expression_type);
        if (!est_ok(resultat_validation)) {
            return resultat_validation;
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
        type_enum->type_donnees = m_compilatrice.typeuse[TypeBase::N32];
    }
    else {
        type_enum->type_donnees = m_compilatrice.typeuse[TypeBase::Z32];
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
ResultatValidation ContexteValidationCode::valide_structure(NoeudStruct *decl)
{
    auto &graphe = m_compilatrice.graphe_dependance;

    /* Les structures copiées n'ont pas de types (la copie ne fait que copier le pointeur, ce qui
     * nous ferait modifier l'original). */
    if (decl->type == nullptr) {
        if (decl->est_union) {
            decl->type = m_compilatrice.typeuse.reserve_type_union(decl);
        }
        else {
            decl->type = m_compilatrice.typeuse.reserve_type_structure(decl);
        }
    }

    auto noeud_dependance = graphe->cree_noeud_type(decl->type);
    decl->noeud_dependance = noeud_dependance;

    if (decl->est_externe && decl->bloc == nullptr) {
        decl->drapeaux |= DECLARATION_FUT_VALIDEE;
        decl->type->drapeaux |= TYPE_FUT_VALIDE;
        return CodeRetourValidation::OK;
    }

    if (decl->est_polymorphe) {
        auto resultat_validation = valide_arbre_aplatis(decl, decl->arbre_aplatis_params);
        if (!est_ok(resultat_validation)) {
            return resultat_validation;
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
        /* Nous devons avoir deux passes : une pour créer la fonction du métaprogramme, une autre
         * pour requérir la compilation dudit métaprogramme. */
        if (!decl->metaprogramme_corps_texte) {
            auto metaprogramme = cree_metaprogramme_corps_texte(
                decl->bloc, decl->bloc_parent, decl->lexeme);
            auto fonction = metaprogramme->fonction;
            fonction->corps->arbre_aplatis = decl->arbre_aplatis;
            assert(fonction->corps->bloc);

            decl->metaprogramme_corps_texte = metaprogramme;
            metaprogramme->corps_texte_pour_structure = decl;

            if (decl->est_monomorphisation) {
                decl->bloc_constantes->membres.avec_verrou_ecriture(
                    [fonction](kuri::tableau<NoeudDeclaration *, int> &membres) {
                        POUR (membres) {
                            fonction->bloc_constantes->membres->ajoute(it);
                        }
                    });
            }

            m_compilatrice.gestionnaire_code->requiers_typage(espace, fonction->corps);
            return Attente::sur_declaration(fonction->corps);
        }

        auto metaprogramme = decl->metaprogramme_corps_texte;
        auto fichier = m_compilatrice.cree_fichier_pour_metaprogramme(metaprogramme);
        m_compilatrice.gestionnaire_code->requiers_compilation_metaprogramme(espace,
                                                                             metaprogramme);
        return Attente::sur_parsage(fichier);
    }

    auto resultat_validation = valide_arbre_aplatis(decl, decl->arbre_aplatis);
    if (!est_ok(resultat_validation)) {
        return resultat_validation;
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

    auto ajoute_donnees_membre = [&, this](NoeudExpression *enfant,
                                           NoeudExpression *expr_valeur) -> ResultatValidation {
        auto type_membre = enfant->type;

        // À FAIRE: ceci devrait plutôt être déplacé dans la validation des déclarations, mais nous
        // finissons sur une erreur de compilation à cause d'une attente
        if ((type_membre->drapeaux & TYPE_FUT_VALIDE) == 0) {
            return Attente::sur_type(type_membre);
        }

        if (type_membre != m_compilatrice.typeuse[TypeBase::RIEN]) {
            if (type_membre->alignement == 0) {
                unite->espace
                    ->rapporte_erreur(enfant, "impossible de définir l'alignement du type")
                    .ajoute_message("Le type est « ", chaine_type(type_membre), " »\n");
                return CodeRetourValidation::Erreur;
            }

            if (type_membre->taille_octet == 0) {
                rapporte_erreur("impossible de définir la taille du type", enfant);
                return CodeRetourValidation::Erreur;
            }
        }

        auto decl_var_enfant = NoeudDeclarationVariable::nul();
        if (enfant->est_declaration_variable()) {
            decl_var_enfant = enfant->comme_declaration_variable();
        }
        else if (enfant->est_reference_declaration()) {
            auto ref = enfant->comme_reference_declaration();
            if (ref->declaration_referee->est_declaration_variable()) {
                decl_var_enfant = ref->declaration_referee->comme_declaration_variable();
            }
        }

        type_compose->membres.ajoute(
            {decl_var_enfant, enfant->type, enfant->ident, 0, 0, expr_valeur});
        return CodeRetourValidation::OK;
    };

    if (decl->est_union) {
        auto type_union = decl->type->comme_union();
        type_union->est_nonsure = decl->est_nonsure;

        POUR (*decl->bloc->membres.verrou_ecriture()) {
            if (it->est_declaration_type()) {
                // utilisation d'un type de données afin de pouvoir automatiquement déterminer un
                // type
                auto type_de_donnees = m_compilatrice.typeuse.type_type_de_donnees(it->type);
                type_compose->membres.ajoute({nullptr,
                                              type_de_donnees,
                                              it->ident,
                                              0,
                                              0,
                                              nullptr,
                                              TypeCompose::Membre::EST_CONSTANT});
                continue;
            }

            auto decl_var = it->comme_declaration_variable();

            if (decl_var->possede_drapeau(EST_CONSTANTE)) {
                type_compose->membres.ajoute({decl_var,
                                              it->type,
                                              it->ident,
                                              0,
                                              0,
                                              decl_var->expression,
                                              TypeCompose::Membre::EST_CONSTANT});
                continue;
            }

            for (auto &donnees : decl_var->donnees_decl.plage()) {
                for (auto i = 0; i < donnees.variables.taille(); ++i) {
                    auto var = donnees.variables[i];

                    if (var->type->est_rien() && decl->est_nonsure) {
                        rapporte_erreur("Ne peut avoir un type « rien » dans une union nonsûre",
                                        decl_var,
                                        erreur::Genre::TYPE_DIFFERENTS);
                        return CodeRetourValidation::Erreur;
                    }

                    if (var->type->est_variadique()) {
                        rapporte_erreur("Ne peut avoir un type variadique dans une union",
                                        decl_var,
                                        erreur::Genre::TYPE_DIFFERENTS);
                        return CodeRetourValidation::Erreur;
                    }

                    if (var->type->est_structure() || var->type->est_union()) {
                        if ((var->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
                            return Attente::sur_type(var->type);
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
                    auto const resultat_ajout = ajoute_donnees_membre(var, expression);
                    if (!est_ok(resultat_ajout)) {
                        return resultat_ajout;
                    }
                }
            }
        }

        calcule_taille_type_compose(type_union, false, 0);

        if (!decl->est_nonsure) {
            type_union->cree_type_structure(m_compilatrice.typeuse, type_union->decalage_index);
        }

        decl->drapeaux |= DECLARATION_FUT_VALIDEE;
        decl->type->drapeaux |= TYPE_FUT_VALIDE;

        POUR (type_compose->membres) {
            graphe->connecte_type_type(type_compose, it.type);
        }

        return CodeRetourValidation::OK;
    }

    auto type_struct = type_compose->comme_structure();

    POUR (*decl->bloc->membres.verrou_lecture()) {
        if (it->est_declaration_type()) {
            // utilisation d'un type de données afin de pouvoir automatiquement déterminer un type
            auto type_de_donnees = m_compilatrice.typeuse.type_type_de_donnees(it->type);
            type_compose->membres.ajoute({nullptr,
                                          type_de_donnees,
                                          it->ident,
                                          0,
                                          0,
                                          nullptr,
                                          TypeCompose::Membre::EST_CONSTANT});
            continue;
        }

        if (it->possede_drapeau(EMPLOYE)) {
            if (!it->type->est_structure()) {
                espace->rapporte_erreur(it,
                                        "Ne peut pas employer un type n'étant pas une structure");
                return CodeRetourValidation::Erreur;
            }

            type_struct->types_employes.ajoute(it->type->comme_structure());
            continue;
        }

        if (it->genre != GenreNoeud::DECLARATION_VARIABLE) {
            rapporte_erreur("Déclaration inattendu dans le bloc de la structure", it);
            return CodeRetourValidation::Erreur;
        }

        auto decl_var = it->comme_declaration_variable();

        if (decl_var->possede_drapeau(EST_CONSTANTE)) {
            type_compose->membres.ajoute({decl_var,
                                          it->type,
                                          it->ident,
                                          0,
                                          0,
                                          decl_var->expression,
                                          TypeCompose::Membre::EST_CONSTANT});
            continue;
        }

        if (decl_var->declaration_vient_d_un_emploi) {
            // À FAIRE(emploi) : préserve l'emploi dans les données types
            auto const resultat_ajout = ajoute_donnees_membre(decl_var, decl_var->expression);
            if (!est_ok(resultat_ajout)) {
                return resultat_ajout;
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

                if (var->type->est_variadique()) {
                    rapporte_erreur("Ne peut avoir un type variadique dans une structure",
                                    decl_var,
                                    erreur::Genre::TYPE_DIFFERENTS);
                    return CodeRetourValidation::Erreur;
                }

                if (var->type->est_structure() || var->type->est_union()) {
                    if ((var->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
                        return Attente::sur_type(var->type);
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
                auto const resultat_ajout = ajoute_donnees_membre(var, expression);
                if (!est_ok(resultat_ajout)) {
                    return resultat_ajout;
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
                {nullptr, m_compilatrice.typeuse[TypeBase::BOOL], ID::chaine_vide, 0, 0, nullptr});
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

    simplifie_arbre(unite->espace, m_tacheronne.assembleuse, m_compilatrice.typeuse, decl);
    return CodeRetourValidation::OK;
}

static bool peut_etre_type_constante(Type *type)
{
    switch (type->genre) {
        /* Possible mais non supporté pour le moment. */
        case GenreType::STRUCTURE:
        /* Il n'est pas encore clair comment prendre le pointeur de la constante pour les tableaux
         * dynamiques. */
        case GenreType::TABLEAU_DYNAMIQUE:
        /* Sémantiquement, les variadiques ne peuvent être utilisées que pour les paramètres de
         * fonctions. */
        case GenreType::VARIADIQUE:
        /* Il n'est pas claire comment gérer les unions, les sûres doivent avoir un membre
         * actif, et les valeurs pour les sûres ou nonsûres doivent être transtypées sur le
         * lieu d'utilisation. */
        case GenreType::UNION:
        /* Un eini doit avoir une info-type, et prendre une valeur par pointeur, qui n'est pas
         * encore supporté pour les constantes. */
        case GenreType::EINI:
        /* Les tuples ne sont que pour les retours de fonctions. */
        case GenreType::TUPLE:
        case GenreType::REFERENCE:
        case GenreType::POINTEUR:
        case GenreType::POLYMORPHIQUE:
        case GenreType::RIEN:
        {
            return false;
        }
        default:
        {
            return true;
        }
    }
}

ResultatValidation ContexteValidationCode::valide_declaration_variable(
    NoeudDeclarationVariable *decl)
{
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
            auto bloc_final = NoeudBloc::nul();
            if (it.decl->possede_drapeau(EST_PARAMETRE) ||
                it.decl->possede_drapeau(EST_MEMBRE_STRUCTURE)) {
                bloc_final = it.decl->bloc_parent->bloc_parent;
            }
            auto decl_prec = trouve_dans_bloc(it.decl->bloc_parent, it.decl, bloc_final);

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
                                        Type *type_de_l_expression) -> ResultatValidation {
        if (variable->type == nullptr) {
            if (type_de_l_expression->genre == GenreType::ENTIER_CONSTANT) {
                variable->type = m_compilatrice.typeuse[TypeBase::Z32];
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
                m_compilatrice, type_de_l_expression, variable->type);

            if (std::holds_alternative<Attente>(resultat)) {
                return std::get<Attente>(resultat);
            }

            auto transformation = std::get<TransformationType>(resultat);
            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_assignation_type_differents(
                    variable->type, type_de_l_expression, expression);
                return CodeRetourValidation::Erreur;
            }

            donnees.variables.ajoute(variable);
            donnees.transformations.ajoute(transformation);
        }

        if (decl->drapeaux & EST_CONSTANTE && !type_de_l_expression->est_type_de_donnees()) {
            if (!peut_etre_type_constante(type_de_l_expression)) {
                rapporte_erreur("L'expression de la constante n'a pas un type pouvant être celui "
                                "d'une expression constante",
                                expression);
                return CodeRetourValidation::Erreur;
            }

            auto res_exec = evalue_expression(m_compilatrice, decl->bloc_parent, expression);

            if (res_exec.est_errone) {
                rapporte_erreur("Impossible d'évaluer l'expression de la constante", expression);
                return CodeRetourValidation::Erreur;
            }

            decl->valeur_expression = res_exec.valeur;
        }

        return CodeRetourValidation::OK;
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

                    auto resultat = ajoute_variable(donnees, variables.defile(), it, membre.type);
                    if (!est_ok(resultat)) {
                        return resultat;
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
                auto resultat = ajoute_variable(donnees, variables.defile(), it, it->type);
                if (!est_ok(resultat)) {
                    return resultat;
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
                    var->type = m_compilatrice.typeuse[TypeBase::Z32];
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
                auto graphe = m_compilatrice.graphe_dependance.verrou_ecriture();
                graphe->cree_noeud_globale(decl_var);
            }
            else {
                /* Les globales sont ajoutées au bloc parent par la syntaxeuse. */
                auto bloc_parent = decl_var->bloc_parent;
                bloc_parent->membres->ajoute(decl_var);
            }

            decl_var->drapeaux |= DECLARATION_FUT_VALIDEE;
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

    if (!fonction_courante()) {
        simplifie_arbre(unite->espace, m_tacheronne.assembleuse, m_compilatrice.typeuse, decl);

        /* Pour la génération de RI pour les globales, nous devons attendre que le type fut validé.
         */
        if ((decl->type->drapeaux & TYPE_FUT_VALIDE) == 0) {
            /* Ne revalide pas ce noeud. */
            unite->index_courant += 1;
            return Attente::sur_type(decl->type);
        }
    }

    return CodeRetourValidation::OK;
}

ResultatValidation ContexteValidationCode::valide_assignation(NoeudAssignation *inst)
{
    CHRONO_TYPAGE(m_tacheronne.stats_typage.assignations, "valide assignation");
    auto variable = inst->variable;

    kuri::file_fixe<NoeudExpression *, 6> variables;

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

    kuri::tablet<NoeudExpression *, 6> expressions;
    rassemble_expressions(inst->expression, expressions);

    auto ajoute_variable = [this](DonneesAssignations &donnees,
                                  NoeudExpression *var,
                                  NoeudExpression *expression,
                                  Type *type_de_l_expression) -> ResultatValidation {
        auto type_de_la_variable = var->type;
        auto var_est_reference = type_de_la_variable->est_reference();
        auto expr_est_reference = type_de_l_expression->est_reference();

        auto transformation = TransformationType();

        if (var->possede_drapeau(ACCES_EST_ENUM_DRAPEAU)) {
            if (!expression->type->est_bool()) {
                espace
                    ->rapporte_erreur(expression,
                                      "L'assignation d'une valeur d'une énum_drapeau doit être "
                                      "une valeur booléenne")
                    .ajoute_message(
                        "Le type de l'expression est ", chaine_type(expression->type), "\n");
                return CodeRetourValidation::Erreur;
            }

            donnees.variables.ajoute(var);
            donnees.transformations.ajoute(transformation);
            return CodeRetourValidation::OK;
        }

        if (var_est_reference && expr_est_reference) {
            // déréférence les deux côtés
            auto resultat = cherche_transformation(
                m_compilatrice, type_de_l_expression, var->type);

            if (std::holds_alternative<Attente>(resultat)) {
                return std::get<Attente>(resultat);
            }

            transformation = std::get<TransformationType>(resultat);
            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_assignation_type_differents(
                    var->type, type_de_l_expression, expression);
                return CodeRetourValidation::Erreur;
            }

            transtype_si_necessaire(var, TypeTransformation::DEREFERENCE);
            transformation = TypeTransformation::DEREFERENCE;
        }
        else if (var_est_reference) {
            // déréférence var
            type_de_la_variable = type_de_la_variable->comme_reference()->type_pointe;

            auto resultat = cherche_transformation(
                m_compilatrice, type_de_l_expression, type_de_la_variable);

            if (std::holds_alternative<Attente>(resultat)) {
                return std::get<Attente>(resultat);
            }

            transformation = std::get<TransformationType>(resultat);
            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_assignation_type_differents(
                    var->type, type_de_l_expression, expression);
                return CodeRetourValidation::Erreur;
            }

            transtype_si_necessaire(var, TypeTransformation::DEREFERENCE);
        }
        else if (expr_est_reference) {
            // déréférence expr
            auto resultat = cherche_transformation(
                m_compilatrice, type_de_l_expression, var->type);

            if (std::holds_alternative<Attente>(resultat)) {
                return std::get<Attente>(resultat);
            }

            transformation = std::get<TransformationType>(resultat);
            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_assignation_type_differents(
                    var->type, type_de_l_expression, expression);
                return CodeRetourValidation::Erreur;
            }
        }
        else {
            auto resultat = cherche_transformation(
                m_compilatrice, type_de_l_expression, var->type);

            if (std::holds_alternative<Attente>(resultat)) {
                return std::get<Attente>(resultat);
            }

            transformation = std::get<TransformationType>(resultat);
            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_assignation_type_differents(
                    var->type, type_de_l_expression, expression);
                return CodeRetourValidation::Erreur;
            }
        }

        donnees.variables.ajoute(var);
        donnees.transformations.ajoute(transformation);
        return CodeRetourValidation::OK;
    };

    kuri::tablet<DonneesAssignations, 6> donnees_assignations;

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

                auto resultat = ajoute_variable(donnees, variables.defile(), it, membre.type);
                if (!est_ok(resultat)) {
                    return resultat;
                }
            }
        }
        else {
            auto resultat = ajoute_variable(donnees, variables.defile(), it, it->type);
            if (!est_ok(resultat)) {
                return resultat;
            }
        }

        donnees_assignations.ajoute(std::move(donnees));
    }

    // a, b = c
    auto donnees = &donnees_assignations.back();
    while (!variables.est_vide()) {
        auto resultat = ajoute_variable(
            *donnees, variables.defile(), donnees->expression, donnees->expression->type);
        if (!est_ok(resultat)) {
            return resultat;
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
                                                            NoeudExpression *membre,
                                                            TypeCompose *type)
{
    erreur::membre_inconnu(*espace, acces, membre, type);
}

void ContexteValidationCode::rapporte_erreur_valeur_manquante_discr(
    NoeudExpression *expression, kuri::ensemble<kuri::chaine_statique> const &valeurs_manquantes)
{
    erreur::valeur_manquante_discr(*espace, expression, valeurs_manquantes);
}

void ContexteValidationCode::rapporte_erreur_fonction_nulctx(const NoeudExpression *appl_fonc,
                                                             const NoeudExpression *decl_fonc,
                                                             const NoeudExpression *decl_appel)
{
    erreur::lance_erreur_fonction_nulctx(*espace, appl_fonc, decl_fonc, decl_appel);
}

ResultatValidation ContexteValidationCode::transtype_si_necessaire(NoeudExpression *&expression,
                                                                   Type *type_cible)
{
    auto resultat = cherche_transformation(m_compilatrice, expression->type, type_cible);

    if (std::holds_alternative<Attente>(resultat)) {
        return std::get<Attente>(resultat);
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
            type_cible = m_compilatrice.typeuse[TypeBase::EINI];
        }
        else if (transformation.type == TypeTransformation::CONVERTI_VERS_PTR_RIEN) {
            type_cible = m_compilatrice.typeuse[TypeBase::PTR_RIEN];
        }
        else if (transformation.type == TypeTransformation::PREND_REFERENCE) {
            type_cible = m_compilatrice.typeuse.type_reference_pour(expression->type);
        }
        else if (transformation.type == TypeTransformation::DEREFERENCE) {
            type_cible = type_dereference_pour(expression->type);
        }
        else if (transformation.type == TypeTransformation::CONSTRUIT_TABL_OCTET) {
            type_cible = m_compilatrice.typeuse[TypeBase::TABL_OCTET];
        }
        else if (transformation.type == TypeTransformation::CONVERTI_TABLEAU) {
            auto type_tableau_fixe = expression->type->comme_tableau_fixe();
            type_cible = m_compilatrice.typeuse.type_tableau_dynamique(
                type_tableau_fixe->type_pointe);
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
        noeud_comme->type = m_compilatrice.typeuse.type_reference_pour(expression->type);
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

ResultatValidation ContexteValidationCode::valide_operateur_binaire(NoeudExpressionBinaire *expr)
{
    expr->genre_valeur = GenreValeur::DROITE;
    CHRONO_TYPAGE(m_tacheronne.stats_typage.operateurs_binaire, "opérateur binaire");

    if (expr->lexeme->genre == GenreLexeme::TABLEAU) {
        return valide_operateur_binaire_tableau(expr);
    }

    auto enfant1 = expr->operande_gauche;
    auto enfant2 = expr->operande_droite;
    auto type1 = enfant1->type;

    if (type1->genre == GenreType::TYPE_DE_DONNEES) {
        return valide_operateur_binaire_type(expr);
    }

    auto type_op = expr->lexeme->genre;

    /* détecte a comp b comp c */
    if (est_operateur_comparaison(type_op) && est_operateur_comparaison(enfant1->lexeme->genre)) {
        return valide_operateur_binaire_chaine(expr);
    }

    if (dls::outils::est_element(type_op, GenreLexeme::BARRE_BARRE, GenreLexeme::ESP_ESP)) {
        if (!est_expression_convertible_en_bool(enfant1)) {
            espace->rapporte_erreur(
                enfant1, "Expression non conditionnable à gauche de l'opérateur logique !");
        }

        if (!est_expression_convertible_en_bool(enfant2)) {
            espace->rapporte_erreur(
                enfant2, "Expression non conditionnable à droite de l'opérateur logique !");
        }

        /* Les expressions de types a && b || c ou a || b && c ne sont pas valides
         * car nous ne pouvons déterminer le bon ordre d'exécution. */
        if (expr->lexeme->genre == GenreLexeme::BARRE_BARRE) {
            if (enfant1->lexeme->genre == GenreLexeme::ESP_ESP) {
                espace
                    ->rapporte_erreur(
                        enfant1, "Utilisation ambigüe de l'opérateur « && » à gauche de « || » !")
                    .ajoute_message("Veuillez utiliser des parenthèses pour clarifier "
                                    "l'ordre des comparisons.");
            }

            if (enfant2->lexeme->genre == GenreLexeme::ESP_ESP) {
                espace
                    ->rapporte_erreur(
                        enfant2, "Utilisation ambigüe de l'opérateur « && » à droite de « || » !")
                    .ajoute_message("Veuillez utiliser des parenthèses pour clarifier "
                                    "l'ordre des comparisons.");
            }
        }

        expr->type = m_compilatrice.typeuse[TypeBase::BOOL];
        return CodeRetourValidation::OK;
    }

    if (enfant1->possede_drapeau(ACCES_EST_ENUM_DRAPEAU) && enfant2->est_litterale_bool()) {
        return valide_comparaison_enum_drapeau_bool(
            expr, enfant1->comme_reference_membre(), enfant2->comme_litterale_bool());
    }

    if (enfant2->possede_drapeau(ACCES_EST_ENUM_DRAPEAU) && enfant1->est_litterale_bool()) {
        return valide_comparaison_enum_drapeau_bool(
            expr, enfant2->comme_reference_membre(), enfant1->comme_litterale_bool());
    }

    return valide_operateur_binaire_generique(expr);
}

ResultatValidation ContexteValidationCode::valide_operateur_binaire_chaine(
    NoeudExpressionBinaire *expr)
{
    auto type_op = expr->lexeme->genre;
    auto enfant1 = expr->operande_gauche;
    auto enfant2 = expr->operande_droite;
    auto type1 = enfant1->type;
    auto type2 = enfant2->type;
    expr->genre = GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE;
    expr->type = m_compilatrice.typeuse[TypeBase::BOOL];

    auto enfant_expr = static_cast<NoeudExpressionBinaire *>(enfant1);
    type1 = enfant_expr->operande_droite->type;

    auto candidats = kuri::tablet<OperateurCandidat, 10>();
    auto resultat = cherche_candidats_operateurs(*espace, type1, type2, type_op, candidats);
    if (resultat.has_value()) {
        return resultat.value();
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
        return attente_sur_operateur_ou_type(expr);
    }

    expr->op = meilleur_candidat->op;
    transtype_si_necessaire(expr->operande_gauche, meilleur_candidat->transformation_type1);
    transtype_si_necessaire(expr->operande_droite, meilleur_candidat->transformation_type2);
    return CodeRetourValidation::OK;
}

ResultatValidation ContexteValidationCode::valide_operateur_binaire_tableau(
    NoeudExpressionBinaire *expr)
{
    auto enfant1 = expr->operande_gauche;
    auto enfant2 = expr->operande_droite;
    auto expression_taille = enfant1;
    auto expression_type = enfant2;

    auto type2 = expression_type->type;

    if (type2->genre != GenreType::TYPE_DE_DONNEES) {
        rapporte_erreur("Attendu une expression de type après la déclaration de type tableau",
                        enfant2);
        return CodeRetourValidation::Erreur;
    }

    auto type_de_donnees = type2->comme_type_de_donnees();
    auto type_connu = type_de_donnees->type_connu ? type_de_donnees->type_connu : type_de_donnees;

    auto taille_tableau = 0l;

    if (expression_taille) {
        auto res = evalue_expression(
            m_compilatrice, expression_taille->bloc_parent, expression_taille);

        if (res.est_errone) {
            rapporte_erreur("Impossible d'évaluer la taille du tableau", expression_taille);
            return CodeRetourValidation::Erreur;
        }

        if (!res.valeur.est_entiere()) {
            rapporte_erreur("L'expression n'est pas de type entier", expression_taille);
            return CodeRetourValidation::Erreur;
        }

        if (res.valeur.entiere() == 0) {
            espace->rapporte_erreur(expression_taille,
                                    "Impossible de définir un tableau fixe de taille 0 !\n");
            return CodeRetourValidation::Erreur;
        }

        taille_tableau = res.valeur.entiere();
    }

    if (taille_tableau != 0) {
        // À FAIRE: détermine proprement que nous avons un type s'utilisant par valeur
        // via un membre
        if ((type_connu->drapeaux & TYPE_FUT_VALIDE) == 0) {
            return Attente::sur_type(type_connu);
        }

        auto type_tableau = m_compilatrice.typeuse.type_tableau_fixe(
            type_connu, static_cast<int>(taille_tableau));
        expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_tableau);
    }
    else {
        auto type_tableau = m_compilatrice.typeuse.type_tableau_dynamique(type_connu);
        expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_tableau);
    }

    return CodeRetourValidation::OK;
}

ResultatValidation ContexteValidationCode::valide_operateur_binaire_type(
    NoeudExpressionBinaire *expr)
{
    auto enfant1 = expr->operande_gauche;
    auto enfant2 = expr->operande_droite;
    auto type1 = enfant1->type;
    auto type2 = enfant2->type;

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
                rapporte_erreur("Opération impossible car le type n'est pas connu", expr);
                return CodeRetourValidation::Erreur;
            }

            if (type_type2->type_connu == nullptr) {
                rapporte_erreur("Opération impossible car le type n'est pas connu", expr);
                return CodeRetourValidation::Erreur;
            }

            if (type_type1->type_connu == type_type2->type_connu) {
                rapporte_erreur("Impossible de créer une union depuis des types similaires\n",
                                expr);
                return CodeRetourValidation::Erreur;
            }

            if ((type_type1->type_connu->drapeaux & TYPE_FUT_VALIDE) == 0) {
                return Attente::sur_type(type_type1->type_connu);
            }

            if ((type_type2->type_connu->drapeaux & TYPE_FUT_VALIDE) == 0) {
                return Attente::sur_type(type_type2->type_connu);
            }

            auto membres = kuri::tablet<TypeCompose::Membre, 6>(2);
            membres[0] = {nullptr, type_type1->type_connu, ID::_0};
            membres[1] = {nullptr, type_type2->type_connu, ID::_1};

            auto type_union = m_compilatrice.typeuse.union_anonyme(membres);
            expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_union);

            // @concurrence critique
            if (type_union->decl == nullptr) {
                static Lexeme lexeme_union = {
                    "anonyme", {}, GenreLexeme::CHAINE_CARACTERE, 0, 0, 0};
                auto decl_struct = m_tacheronne.assembleuse->cree_structure(&lexeme_union);
                decl_struct->type = type_union;
                type_union->decl = decl_struct;
            }

            return CodeRetourValidation::OK;
        }
        case GenreLexeme::EGALITE:
        {
            // XXX - aucune raison de prendre un verrou ici
            auto op = m_compilatrice.operateurs->op_comp_egal_types;
            expr->type = op->type_resultat;
            expr->op = op;
            return CodeRetourValidation::OK;
        }
        case GenreLexeme::DIFFERENCE:
        {
            // XXX - aucune raison de prendre un verrou ici
            auto op = m_compilatrice.operateurs->op_comp_diff_types;
            expr->type = op->type_resultat;
            expr->op = op;
            return CodeRetourValidation::OK;
        }
    }
}

ResultatValidation ContexteValidationCode::valide_operateur_binaire_generique(
    NoeudExpressionBinaire *expr)
{
    auto type_op = expr->lexeme->genre;
    auto assignation_composee = est_assignation_composee(type_op);
    auto enfant1 = expr->operande_gauche;
    auto enfant2 = expr->operande_droite;
    auto type1 = enfant1->type;
    auto type2 = enfant2->type;

    bool type_gauche_est_reference = false;
    if (assignation_composee) {
        type_op = operateur_pour_assignation_composee(type_op);

        if (type1->est_reference()) {
            type_gauche_est_reference = true;
            type1 = type1->comme_reference()->type_pointe;
            transtype_si_necessaire(expr->operande_gauche, TypeTransformation::DEREFERENCE);
        }
    }

    auto candidats = kuri::tablet<OperateurCandidat, 10>();
    auto resultat = cherche_candidats_operateurs(*espace, type1, type2, type_op, candidats);
    if (resultat.has_value()) {
        return resultat.value();
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
        return attente_sur_operateur_ou_type(expr);
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

    transtype_si_necessaire(expr->operande_gauche, meilleur_candidat->transformation_type1);
    transtype_si_necessaire(expr->operande_droite, meilleur_candidat->transformation_type2);

    if (assignation_composee) {
        expr->drapeaux |= EST_ASSIGNATION_COMPOSEE;

        auto resultat_tfm = cherche_transformation(m_compilatrice, expr->type, type1);

        if (std::holds_alternative<Attente>(resultat_tfm)) {
            return std::get<Attente>(resultat_tfm);
        }

        auto transformation = std::get<TransformationType>(resultat_tfm);

        if (transformation.type == TypeTransformation::IMPOSSIBLE) {
            rapporte_erreur_assignation_type_differents(type1, expr->type, enfant2);
            return CodeRetourValidation::Erreur;
        }
    }

    return CodeRetourValidation::OK;
}

ResultatValidation ContexteValidationCode::valide_comparaison_enum_drapeau_bool(
    NoeudExpressionBinaire *expr,
    NoeudExpressionMembre * /*expr_acces_enum*/,
    NoeudExpressionLitteraleBool *expr_bool)
{
    auto type_op = expr->lexeme->genre;

    if (type_op != GenreLexeme::EGALITE && type_op != GenreLexeme::DIFFERENCE) {
        espace->rapporte_erreur(expr,
                                "Une comparaison entre une valeur d'énumération drapeau et une "
                                "littérale booléenne doit se faire via « == » ou « != »");
        return CodeRetourValidation::Erreur;
    }

    auto type_bool = expr_bool->type;

    auto candidats = kuri::tablet<OperateurCandidat, 10>();
    auto resultat = cherche_candidats_operateurs(
        *espace, type_bool, type_bool, type_op, candidats);
    if (resultat.has_value()) {
        return resultat.value();
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
        return attente_sur_operateur_ou_type(expr);
    }

    expr->op = meilleur_candidat->op;
    expr->type = type_bool;
    return CodeRetourValidation::OK;
}
