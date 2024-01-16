/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#include "validation_semantique.hh"

#include <iostream>

#include "biblinternes/outils/conditions.h"

#include "arbre_syntaxique/assembleuse.hh"
#include "arbre_syntaxique/canonicalisation.hh"
#include "arbre_syntaxique/cas_genre_noeud.hh"
#include "arbre_syntaxique/copieuse.hh"

#include "parsage/outils_lexemes.hh"

#include "utilitaires/macros.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "numerique.hh"
#include "portee.hh"
#include "tacheronne.hh"
#include "unite_compilation.hh"
#include "utilitaires/log.hh"

/* ************************************************************************** */

#define TENTE_IMPL(var, x)                                                                        \
    auto var = x;                                                                                 \
    if (!est_ok(var)) {                                                                           \
        return var;                                                                               \
    }

#define TENTE(x) TENTE_IMPL(VARIABLE_ANONYME(résultat), x)

Sémanticienne::Sémanticienne(Compilatrice &compilatrice) : m_compilatrice(compilatrice)
{
}

Sémanticienne::~Sémanticienne()
{
    POUR (m_arbres_aplatis) {
        memoire::deloge("ArbreAplatis", it);
    }
}

void Sémanticienne::réinitialise()
{
    m_tacheronne = nullptr;
    m_espace = nullptr;
    m_unité = nullptr;
    m_arbre_courant = nullptr;
}

void Sémanticienne::définis_tacheronne(Tacheronne &tacheronne)
{
    m_tacheronne = &tacheronne;
}

AssembleuseArbre *Sémanticienne::donne_assembleuse()
{
    return m_tacheronne->assembleuse;
}

StatistiquesTypage &Sémanticienne::donne_stats_typage()
{
    return m_stats_typage;
}

void Sémanticienne::rassemble_statistiques(Statistiques &stats)
{
    auto mémoire_utilisée = 0l;
    POUR (m_arbres_aplatis) {
        mémoire_utilisée += it->noeuds.taille_mémoire();
        mémoire_utilisée += taille_de(ArbreAplatis);
    }

    mémoire_utilisée += m_arbres_aplatis.taille_mémoire();

    stats.ajoute_mémoire_utilisée("Compilatrice", mémoire_utilisée);
}

/* Point d'entrée pour la validation sémantique. Nous utilisons ceci au lieu de directement appeler
 * valide_semantique_noeud, puisque la validation des arbres aplatis pourrait résulter en un
 * dépassement de pile dans le cas où l'arbre aplatis contient également la fonction racine.
 * En outre, ceci nous permet de mieux controler les racines de validations, qui doivent être
 * des déclarations ou directives globales. */
RésultatValidation Sémanticienne::valide(UniteCompilation *unité)
{
    m_unité = unité;
    m_espace = unité->espace;

    if (!unité->arbre_aplatis) {
        unité->arbre_aplatis = donne_un_arbre_aplatis();
    }

    m_arbre_courant = unité->arbre_aplatis;

    if (racine_validation()->est_entete_fonction()) {
        return valide_entête_fonction(racine_validation()->comme_entete_fonction());
    }

    if (racine_validation()->est_corps_fonction()) {
        auto corps = racine_validation()->comme_corps_fonction();
        if (corps->entete->est_operateur) {
            return valide_opérateur(corps);
        }
        return valide_fonction(corps);
    }

    if (racine_validation()->est_type_enum()) {
        auto enumeration = racine_validation()->comme_type_enum();
        return valide_énum(enumeration);
    }

    if (racine_validation()->est_type_structure()) {
        auto structure = racine_validation()->comme_type_structure();
        return valide_structure(structure);
    }

    if (racine_validation()->est_type_union()) {
        auto type_union = racine_validation()->comme_type_union();
        return valide_union(type_union);
    }

    if (racine_validation()->est_type_opaque()) {
        auto opaque = racine_validation()->comme_type_opaque();
        return valide_arbre_aplatis(opaque);
    }

    if (racine_validation()->est_declaration_variable()) {
        auto decl = racine_validation()->comme_declaration_variable();
        return valide_arbre_aplatis(decl);
    }

    if (racine_validation()->est_declaration_variable_multiple()) {
        auto decl = racine_validation()->comme_declaration_variable_multiple();
        return valide_arbre_aplatis(decl);
    }

    if (racine_validation()->est_declaration_constante()) {
        auto decl = racine_validation()->comme_declaration_constante();
        return valide_arbre_aplatis(decl);
    }

    if (racine_validation()->est_execute()) {
        auto execute = racine_validation()->comme_execute();
        return valide_arbre_aplatis(execute);
    }

    if (racine_validation()->est_importe() || racine_validation()->est_charge()) {
        return valide_sémantique_noeud(racine_validation());
    }

    if (racine_validation()->est_ajoute_fini()) {
        auto ajoute_fini = racine_validation()->comme_ajoute_fini();
        return valide_arbre_aplatis(ajoute_fini);
    }

    if (racine_validation()->est_ajoute_init()) {
        auto ajoute_init = racine_validation()->comme_ajoute_init();
        return valide_arbre_aplatis(ajoute_init);
    }

    if (racine_validation()->est_declaration_bibliotheque()) {
        return valide_sémantique_noeud(racine_validation());
    }

    if (racine_validation()->est_dependance_bibliotheque()) {
        return valide_sémantique_noeud(racine_validation());
    }

    m_unité->espace->rapporte_erreur_sans_site("Erreur interne : aucune racine de typage valide");
    return CodeRetourValidation::Erreur;
}

MetaProgramme *Sémanticienne::crée_métaprogramme_pour_directive(NoeudDirectiveExecute *directive)
{
    auto assembleuse = m_tacheronne->assembleuse;
    assert(assembleuse->bloc_courant() == nullptr);

    // crée une fonction pour l'exécution
    auto decl_entete = assembleuse->crée_entete_fonction(directive->lexeme);
    auto decl_corps = decl_entete->corps;

    decl_entete->bloc_parent = directive->bloc_parent;
    decl_corps->bloc_parent = directive->bloc_parent;

    assembleuse->bloc_courant(decl_corps->bloc_parent);
    decl_entete->bloc_constantes = assembleuse->empile_bloc(directive->lexeme, decl_entete);
    decl_entete->bloc_parametres = assembleuse->empile_bloc(directive->lexeme, decl_entete);

    decl_entete->drapeaux_fonction |= (DrapeauxNoeudFonction::EST_MÉTAPROGRAMME |
                                       DrapeauxNoeudFonction::FUT_GÉNÉRÉE_PAR_LA_COMPILATRICE);

    // le type de la fonction est fonc () -> (type_expression)
    auto expression = directive->expression;
    auto type_expression = expression->type;

    /* Les #tests ne doivent retourner rien, mais l'expression étant un bloc prend le type de la
     * dernière expression du bloc qui peut être d'autre type que « rien ».
     * À FAIRE : garantis que le #test ne retourne rien différement (valide proprement, vérifie
     * s'il est utile d'assigner un type aux blocs, etc.). */
    if (directive->ident == ID::test) {
        type_expression = TypeBase::RIEN;
    }

    if (type_expression->est_type_tuple()) {
        auto tuple = type_expression->comme_type_tuple();

        POUR (tuple->membres) {
            auto decl_sortie = assembleuse->crée_declaration_variable(directive->lexeme);
            decl_sortie->ident = m_compilatrice.table_identifiants->identifiant_pour_chaine(
                "__ret0");
            decl_sortie->type = it.type;
            decl_sortie->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
            decl_entete->params_sorties.ajoute(decl_sortie);
        }

        decl_entete->param_sortie = assembleuse->crée_declaration_variable(directive->lexeme);
        decl_entete->param_sortie->ident =
            m_compilatrice.table_identifiants->identifiant_pour_chaine("valeur_de_retour");
        decl_entete->param_sortie->type = type_expression;
    }
    else {
        auto decl_sortie = assembleuse->crée_declaration_variable(directive->lexeme);
        decl_sortie->ident = m_compilatrice.table_identifiants->identifiant_pour_chaine("__ret0");
        decl_sortie->type = type_expression;
        decl_sortie->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

        decl_entete->params_sorties.ajoute(decl_sortie);
        decl_entete->param_sortie = assembleuse->crée_declaration_variable(directive->lexeme);
        decl_entete->param_sortie->type = type_expression;
    }

    auto types_entrees = kuri::tablet<Type *, 6>(0);

    auto type_fonction = m_compilatrice.typeuse.type_fonction(types_entrees, type_expression);
    decl_entete->type = type_fonction;

    decl_corps->bloc = assembleuse->empile_bloc(directive->lexeme, decl_entete);

    static Lexème lexème_retourne = {"retourne", {}, GenreLexème::RETOURNE, 0, 0, 0};
    auto expr_ret = assembleuse->crée_retourne(&lexème_retourne);

#ifndef NDEBUG
    /* Dépile manuellement en mode débogage afin de vérifier que les assembleuses sont proprement
     * réinitialisées. */

    /* Bloc corps. */
    assembleuse->dépile_bloc();
    /* Bloc paramètres. */
    assembleuse->dépile_bloc();
    /* Bloc constantes. */
    assembleuse->dépile_bloc();
    /* Bloc parent. */
    assembleuse->dépile_bloc();
#else
    assembleuse->dépile_tout();
#endif

    simplifie_arbre(m_espace, assembleuse, m_compilatrice.typeuse, expression);

    if (type_expression != TypeBase::RIEN) {
        expr_ret->genre = GenreNoeud::INSTRUCTION_RETOUR;
        expr_ret->expression = expression;

        /* besoin de valider pour mettre en place les informations de retour */
        auto ancienne_racine = m_unité->noeud;
        m_unité->noeud = decl_entete;
        valide_expression_retour(expr_ret);
        m_unité->noeud = ancienne_racine;
    }
    else {
        decl_corps->bloc->ajoute_expression(expression);
    }

    decl_corps->bloc->ajoute_expression(expr_ret);

    decl_entete->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    decl_corps->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

    auto metaprogramme = m_compilatrice.crée_metaprogramme(m_espace);
    metaprogramme->fonction = decl_entete;
    metaprogramme->directive = directive;
    directive->metaprogramme = metaprogramme;
    return metaprogramme;
}

static inline bool est_expression_convertible_en_bool(NoeudExpression *expression)
{
    auto type = expression->type;
    if (type->est_type_opaque()) {
        if (est_type_booléen_implicite(type->comme_type_opaque()->type_opacifie)) {
            return true;
        }
    }

    while (expression->est_parenthese()) {
        expression = expression->comme_parenthese()->expression;
    }

    return est_type_booléen_implicite(type) ||
           expression->possède_drapeau(DrapeauxNoeud::ACCES_EST_ENUM_DRAPEAU);
}

RésultatValidation Sémanticienne::valide_sémantique_noeud(NoeudExpression *noeud)
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
        case GenreNoeud::DIRECTIVE_DEPENDANCE_BIBLIOTHEQUE:
        {
            auto noeud_dépendance_bibliothèque = noeud->comme_dependance_bibliotheque();
            return valide_dépendance_bibliothèque(noeud_dépendance_bibliothèque);
        }
        case GenreNoeud::DIRECTIVE_CORPS_BOUCLE:
        {
            if (!fonction_courante()) {
                m_espace->rapporte_erreur(
                    noeud, "Utilisation de #corps_boucle en dehors d'une fonction.");
                return CodeRetourValidation::Erreur;
            }
            auto corps = fonction_courante()->corps;
            if (!corps->est_macro_boucle_pour) {
                m_espace->rapporte_erreur(
                    noeud, "Utilisation de #corps_boucle en dehors d'un opérateur pour.");
                return CodeRetourValidation::Erreur;
            }
            auto boucle_parent = bloc_est_dans_boucle(noeud->bloc_parent, nullptr);
            if (!boucle_parent) {
                m_espace->rapporte_erreur(
                    noeud, "Il est impossible d'utiliser #corps_boucle en dehors d'une boucle.");
                return CodeRetourValidation::Erreur;
            }
            corps->corps_boucle = noeud->comme_directive_corps_boucle();
            break;
        }
        case GenreNoeud::DIRECTIVE_AJOUTE_FINI:
        {
            auto fini_execution = m_compilatrice.interface_kuri->decl_fini_execution_kuri;
            assert(fini_execution);
            if (!fini_execution->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(fini_execution);
            }
            auto corps = fini_execution->corps;
            if (!corps->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(corps);
            }
            auto ajoute_fini = noeud->comme_ajoute_fini();
            corps->bloc->expressions->ajoute_au_début(ajoute_fini->expression);
            ajoute_fini->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
            break;
        }
        case GenreNoeud::DIRECTIVE_AJOUTE_INIT:
        {
            auto init_execution = m_compilatrice.interface_kuri->decl_init_execution_kuri;
            assert(init_execution);
            if (!init_execution->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(init_execution);
            }
            auto corps = init_execution->corps;
            if (!corps->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(corps);
            }
            auto ajoute_init = noeud->comme_ajoute_init();
            corps->bloc->expressions->ajoute_au_début(ajoute_init->expression);
            ajoute_init->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
            break;
        }
        case GenreNoeud::DIRECTIVE_PRE_EXECUTABLE:
        {
            auto pre_executable = noeud->comme_pre_executable();
            auto fichier = m_compilatrice.fichier(pre_executable->lexeme->fichier);
            auto module = fichier->module;
            if (module->directive_pré_exécutable) {
                m_espace
                    ->rapporte_erreur(
                        noeud, "Le module possède déjà une directive d'exécution pré-exécutable")
                    .ajoute_message("La première directive fut déclarée ici :")
                    .ajoute_site(module->directive_pré_exécutable);
                return CodeRetourValidation::Erreur;
            }
            module->directive_pré_exécutable = pre_executable;
            /* NOTE : le métaprogramme ne sera exécuté qu'à la fin de la génération de code. */
            crée_métaprogramme_pour_directive(pre_executable);
            pre_executable->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
            break;
        }
        case GenreNoeud::INSTRUCTION_CHARGE:
        {
            const auto inst = noeud->comme_charge();
            const auto lexeme = inst->expression->lexeme;
            const auto fichier = m_compilatrice.fichier(inst->lexeme->fichier);
            const auto temps = dls::chrono::compte_seconde();
            m_compilatrice.ajoute_fichier_a_la_compilation(
                m_espace, lexeme->chaine, fichier->module, inst->expression);
            noeud->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
            m_temps_chargement += temps.temps();
            break;
        }
        case GenreNoeud::INSTRUCTION_IMPORTE:
        {
            return valide_instruction_importe(noeud->comme_importe());
        }
        case GenreNoeud::DECLARATION_BIBLIOTHEQUE:
        {
            noeud->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
            noeud->bloc_parent->ajoute_membre(noeud->comme_declaration_bibliotheque());
            break;
        }
        case GenreNoeud::DECLARATION_ENTETE_FONCTION:
        {
            return valide_entête_fonction(noeud->comme_entete_fonction());
        }
        case GenreNoeud::DECLARATION_OPERATEUR_POUR:
        {
            return valide_entête_opérateur_pour(noeud->comme_operateur_pour());
        }
        case GenreNoeud::DECLARATION_CORPS_FONCTION:
        {
            auto decl = noeud->comme_corps_fonction();

            if (!decl->entete->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(decl->entete);
            }

            if (decl->entete->est_operateur) {
                return valide_opérateur(decl);
            }

            return valide_fonction(decl);
        }
        case GenreNoeud::EXPRESSION_APPEL:
        {
            auto expr = noeud->comme_appel();
            return valide_appel_fonction(m_compilatrice, *m_espace, *this, expr);
        }
        case GenreNoeud::DIRECTIVE_CUISINE:
        {
            return valide_cuisine(noeud->comme_cuisine());
        }
        case GenreNoeud::DIRECTIVE_EXECUTE:
        {
            auto noeud_directive = noeud->comme_execute();
            auto expression = noeud_directive->expression;
            auto type_expression = expression->type;

            if (noeud_directive->ident == ID::assert_) {
                if (type_expression != TypeBase::BOOL) {
                    m_espace->rapporte_erreur(expression, "Expression non booléenne pour #assert")
                        .ajoute_message("L'expression d'une directive #assert doit être de type "
                                        "booléen, hors le type de l'expression est : ",
                                        chaine_type(type_expression));
                    return CodeRetourValidation::Erreur;
                }
            }

            if (noeud_directive->ident != ID::test) {
                if (auto expr_variable = trouve_expression_non_constante(expression)) {
                    m_espace
                        ->rapporte_erreur(noeud_directive,
                                          "L'expression de la directive n'est pas constante et ne "
                                          "peut donc être évaluée.")
                        .ajoute_message("L'expression non-constante est :\n")
                        .ajoute_site(expr_variable);
                    return CodeRetourValidation::Erreur;
                }
            }

            auto metaprogramme = crée_métaprogramme_pour_directive(noeud_directive);

            m_compilatrice.gestionnaire_code->requiers_compilation_metaprogramme(m_espace,
                                                                                 metaprogramme);

            noeud->type = expression->type;

            if (racine_validation() != noeud) {
                /* avance l'index car il est inutile de revalider ce noeud */
                m_arbre_courant->index_courant += 1;
                return Attente::sur_metaprogramme(metaprogramme);
            }

            noeud->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
            break;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
        {
            return valide_référence_déclaration(noeud->comme_reference_declaration(),
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
            return valide_accès_membre(inst);
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
        {
            auto inst = noeud->comme_assignation_variable();
            return valide_assignation(inst);
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_MULTIPLE:
        {
            auto inst = noeud->comme_assignation_multiple();
            return valide_assignation_multiple(inst);
        }
        case GenreNoeud::DECLARATION_VARIABLE:
        {
            return valide_déclaration_variable(noeud->comme_declaration_variable());
        }
        case GenreNoeud::DECLARATION_VARIABLE_MULTIPLE:
        {
            return valide_déclaration_variable_multiple(
                noeud->comme_declaration_variable_multiple());
        }
        case GenreNoeud::DECLARATION_CONSTANTE:
        {
            return valide_déclaration_constante(noeud->comme_declaration_constante());
        }
        case GenreNoeud::DECLARATION_OPAQUE:
        {
            return valide_type_opaque(noeud->comme_type_opaque());
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
        {
            noeud->type = TypeBase::R32;
            noeud->comme_litterale_reel()->valeur = noeud->lexeme->valeur_reelle;
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
        {
            noeud->type = TypeBase::ENTIER_CONSTANT;
            noeud->comme_litterale_entier()->valeur = noeud->lexeme->valeur_entiere;
            break;
        }
        case GenreNoeud::OPERATEUR_BINAIRE:
        {
            return valide_opérateur_binaire(noeud->comme_expression_binaire());
        }
        case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
        {
            /* Nous devrions être ici uniquement si nous avions une attente. */
            return valide_opérateur_binaire_chaine(noeud->comme_expression_binaire());
        }
        case GenreNoeud::EXPRESSION_LOGIQUE:
        {
            return valide_expression_logique(noeud->comme_expression_logique());
        }
        case GenreNoeud::OPERATEUR_UNAIRE:
        {
            auto expr = noeud->comme_expression_unaire();

            auto enfant = expr->operande;
            auto type = enfant->type;

            CHRONO_TYPAGE(m_stats_typage.opérateurs_unaire, OPERATEUR_UNAIRE__OPERATEUR_UNAIRE);
            if (type->est_type_reference()) {
                type = type_déréférencé_pour(type);
                crée_transtypage_implicite_au_besoin(
                    expr->operande, TransformationType(TypeTransformation::DEREFERENCE));
            }

            if (type->est_type_entier_constant()) {
                type = TypeBase::Z32;
                crée_transtypage_implicite_au_besoin(
                    expr->operande, {TypeTransformation::CONVERTI_ENTIER_CONSTANT, type});
            }

            auto operateurs = m_compilatrice.opérateurs.verrou_lecture();
            auto op = cherche_opérateur_unaire(*operateurs, type, expr->lexeme->genre);

            if (op == nullptr) {
                return Attente::sur_operateur(noeud);
            }

            expr->type = op->type_résultat;
            expr->op = op;

            break;
        }
        case GenreNoeud::EXPRESSION_PRISE_ADRESSE:
        {
            auto prise_adresse = noeud->comme_prise_adresse();
            auto opérande = prise_adresse->opérande;
            auto type_opérande = opérande->type;

            if (type_opérande == nullptr) {
                m_espace->rapporte_erreur(
                    opérande, "Erreur interne : type nul pour l'opérande de la prise d'adresse !");
                return CodeRetourValidation::Erreur;
            }

            if (type_opérande->est_type_type_de_donnees()) {
                CHRONO_TYPAGE(m_stats_typage.opérateurs_unaire, OPERATEUR_UNAIRE__TYPE);
                auto type_de_donnees = type_opérande->comme_type_type_de_donnees();
                auto type_connu = type_de_donnees->type_connu;

                if (type_connu == nullptr) {
                    type_connu = type_de_donnees;
                }
                {
                    CHRONO_TYPAGE(m_stats_typage.opérateurs_unaire, OPERATEUR_UNAIRE__POINTEUR);
                    type_connu = m_compilatrice.typeuse.type_pointeur_pour(type_connu);
                }

                CHRONO_TYPAGE(m_stats_typage.opérateurs_unaire, OPERATEUR_UNAIRE__TYPE_DE_DONNEES);
                noeud->type = m_compilatrice.typeuse.type_type_de_donnees(type_connu);
                break;
            }

            if (!est_valeur_gauche(opérande->genre_valeur)) {
                rapporte_erreur("Ne peut pas prendre l'adresse d'une valeur-droite.", opérande);
                return CodeRetourValidation::Erreur;
            }

            if (type_opérande->est_type_reference()) {
                /* Les références sont des pointeurs implicites, la prise d'adresse ne doit pas
                 * déréférencer. À FAIRE : ajout d'un transtypage référence -> pointeur */
                type_opérande = type_déréférencé_pour(type_opérande);
            }

            prise_adresse->type = m_compilatrice.typeuse.type_pointeur_pour(type_opérande);
            break;
        }
        case GenreNoeud::EXPRESSION_PRISE_REFERENCE:
        {
            auto prise_référence = noeud->comme_prise_reference();
            auto opérande = prise_référence->opérande;
            auto type_opérande = opérande->type;

            if (type_opérande == nullptr) {
                m_espace->rapporte_erreur(
                    prise_référence,
                    "Erreur interne : type nul pour l'opérande d'une prise de référence !");
                return CodeRetourValidation::Erreur;
            }

            if (type_opérande->est_type_type_de_donnees()) {
                CHRONO_TYPAGE(m_stats_typage.opérateurs_unaire, OPERATEUR_UNAIRE__TYPE);
                auto type_de_donnees = type_opérande->comme_type_type_de_donnees();
                auto type_connu = type_de_donnees->type_connu;

                if (type_connu == nullptr) {
                    type_connu = type_de_donnees;
                }

                {
                    CHRONO_TYPAGE(m_stats_typage.opérateurs_unaire, OPERATEUR_UNAIRE__REFERENCE);
                    type_connu = m_compilatrice.typeuse.type_reference_pour(type_connu);
                }

                CHRONO_TYPAGE(m_stats_typage.opérateurs_unaire, OPERATEUR_UNAIRE__TYPE_DE_DONNEES);
                noeud->type = m_compilatrice.typeuse.type_type_de_donnees(type_connu);
                break;
            }

            if (!est_valeur_gauche(opérande->genre_valeur)) {
                rapporte_erreur("Ne peut pas prendre la référence d'une valeur-droite.", opérande);
                return CodeRetourValidation::Erreur;
            }

            if (type_opérande->est_type_reference()) {
                prise_référence->type = type_opérande;
            }
            else {
                prise_référence->type = m_compilatrice.typeuse.type_reference_pour(type_opérande);
            }

            break;
        }
        case GenreNoeud::EXPRESSION_NEGATION_LOGIQUE:
        {
            auto négation = noeud->comme_negation_logique();
            auto opérande = négation->opérande;
            auto type = opérande->type;

            if (type->est_type_reference()) {
                type = type_déréférencé_pour(type);
                crée_transtypage_implicite_au_besoin(
                    négation->opérande, TransformationType(TypeTransformation::DEREFERENCE));
            }

            if (!est_expression_convertible_en_bool(opérande)) {
                rapporte_erreur("Ne peut pas appliquer l'opérateur « ! » au type de l'expression",
                                opérande);
                return CodeRetourValidation::Erreur;
            }

            négation->type = TypeBase::BOOL;
            break;
        }
        case GenreNoeud::EXPRESSION_INDEXAGE:
        {
            auto expr = noeud->comme_indexage();

            auto enfant1 = expr->operande_gauche;
            auto enfant2 = expr->operande_droite;
            auto type1 = enfant1->type;
            auto type2 = enfant2->type;

            if (type1->est_type_reference()) {
                crée_transtypage_implicite_au_besoin(
                    expr->operande_gauche, TransformationType(TypeTransformation::DEREFERENCE));
                type1 = type_déréférencé_pour(type1);
            }

            // À FAIRE : vérifie qu'aucun opérateur ne soit définie sur le type opaque
            if (type1->est_type_opaque()) {
                type1 = type1->comme_type_opaque()->type_opacifie;
            }

            switch (type1->genre) {
                case GenreNoeud::VARIADIQUE:
                case GenreNoeud::TABLEAU_DYNAMIQUE:
                case GenreNoeud::TYPE_TRANCHE:
                {
                    expr->type = type_déréférencé_pour(type1);
                    break;
                }
                case GenreNoeud::TABLEAU_FIXE:
                {
                    auto type_tabl = type1->comme_type_tableau_fixe();
                    expr->type = type_déréférencé_pour(type1);

                    auto res = evalue_expression(m_compilatrice, enfant2->bloc_parent, enfant2);

                    if (!res.est_errone && res.valeur.est_entiere()) {
                        if (res.valeur.entiere() >= type_tabl->taille) {
                            rapporte_erreur_accès_hors_limites(
                                enfant2, type_tabl, res.valeur.entiere());
                            return CodeRetourValidation::Erreur;
                        }

                        /* nous savons que l'accès est dans les limites,
                         * évite d'émettre le code de vérification */
                        expr->aide_generation_code = IGNORE_VERIFICATION;
                    }

                    break;
                }
                case GenreNoeud::POINTEUR:
                {
                    expr->type = type_déréférencé_pour(type1);
                    break;
                }
                case GenreNoeud::CHAINE:
                {
                    expr->type = TypeBase::Z8;
                    break;
                }
                default:
                {
                    auto résultat = trouve_opérateur_pour_expression(
                        *m_espace, expr, type1, type2, GenreLexème::CROCHET_OUVRANT);

                    if (std::holds_alternative<Attente>(résultat)) {
                        return std::get<Attente>(résultat);
                    }

                    auto candidat = std::get<OpérateurCandidat>(résultat);
                    expr->type = candidat.op->type_résultat;
                    expr->op = candidat.op;
                    expr->permute_operandes = candidat.permute_opérandes;

                    crée_transtypage_implicite_au_besoin(expr->operande_gauche,
                                                         candidat.transformation_type1);
                    crée_transtypage_implicite_au_besoin(expr->operande_droite,
                                                         candidat.transformation_type2);
                    break;
                }
            }

            auto type_cible = TypeBase::Z64;
            auto type_index = enfant2->type;

            if (est_type_implicitement_utilisable_pour_indexage(type_index)) {
                crée_transtypage_implicite_au_besoin(
                    expr->operande_droite,
                    {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_cible});
            }
            else {
                TENTE(crée_transtypage_implicite_si_possible(
                    expr->operande_droite,
                    type_cible,
                    RaisonTranstypageImplicite::POUR_EXPRESSION_INDEXAGE));
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_RETOUR:
        {
            auto inst = noeud->comme_retourne();
            return valide_expression_retour(inst);
        }
        case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
        {
            noeud->type = TypeBase::CHAINE;
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
        {
            noeud->type = TypeBase::BOOL;
            noeud->comme_litterale_bool()->valeur = noeud->lexeme->chaine == "vrai";
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
        {
            noeud->type = TypeBase::Z8;
            noeud->comme_litterale_caractere()->valeur = static_cast<uint32_t>(
                noeud->lexeme->valeur_entiere);
            break;
        }
        case GenreNoeud::INSTRUCTION_SAUFSI:
        case GenreNoeud::INSTRUCTION_SI:
        {
            return valide_instruction_si(noeud->comme_si());
        }
        case GenreNoeud::INSTRUCTION_SI_STATIQUE:
        case GenreNoeud::INSTRUCTION_SAUFSI_STATIQUE:
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

                if (inst->est_saufsi_statique()) {
                    if (condition_est_vraie) {
                        // dis à l'unité de sauter les instructions jusqu'au prochain point
                        m_arbre_courant->index_courant = inst->index_bloc_si_faux;
                    }
                }
                else {
                    if (!condition_est_vraie) {
                        // dis à l'unité de sauter les instructions jusqu'au prochain point
                        m_arbre_courant->index_courant = inst->index_bloc_si_faux;
                    }
                }

                inst->visite = true;
            }
            else {
                // dis à l'unité de sauter les instructions jusqu'au prochain point
                m_arbre_courant->index_courant = inst->index_apres;
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_COMPOSEE:
        {
            auto inst = noeud->comme_bloc();

            auto expressions = inst->expressions.verrou_lecture();

            if (expressions->est_vide()) {
                noeud->type = TypeBase::RIEN;
            }
            else {
                noeud->type = expressions->a(expressions->taille() - 1)->type;
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_POUR:
        {
            return valide_instruction_pour(noeud->comme_pour());
        }
        case GenreNoeud::EXPRESSION_COMME:
        {
            auto expr = noeud->comme_comme();
            return valide_expression_comme(expr);
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NUL:
        {
            noeud->type = TypeBase::PTR_NUL;
            break;
        }
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        {
            auto expr = noeud->comme_taille_de();
            expr->type = TypeBase::N32;

            auto expr_type = expr->expression;
            if (résoud_type_final(expr_type, expr_type->type) == CodeRetourValidation::Erreur) {
                return CodeRetourValidation::Erreur;
            }

            if (!expr_type->type->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                /* ce n'est plus la peine de revenir ici une fois que le type sera validé */
                m_arbre_courant->index_courant += 1;
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
                if (type_debut->est_type_entier_constant() && est_type_entier(type_fin)) {
                    type_debut = type_fin;
                    enfant1->type = type_debut;
                    crée_transtypage_implicite_au_besoin(
                        inst->debut, {TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_debut});
                }
                else if (type_fin->est_type_entier_constant() && est_type_entier(type_debut)) {
                    type_fin = type_debut;
                    enfant2->type = type_fin;
                    crée_transtypage_implicite_au_besoin(
                        inst->fin, {TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_fin});
                }
                else {
                    rapporte_erreur_type_opération(type_debut, type_fin, noeud);
                    return CodeRetourValidation::Erreur;
                }
            }
            else if (type_debut->est_type_entier_constant()) {
                type_debut = TypeBase::Z32;
                crée_transtypage_implicite_au_besoin(
                    inst->debut, {TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_debut});
                crée_transtypage_implicite_au_besoin(
                    inst->fin, {TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_debut});
            }

            if (!type_debut->est_type_entier_naturel() && !type_debut->est_type_entier_relatif() &&
                !type_debut->est_type_reel()) {
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
                !est_opérateur_bool(inst->condition->lexeme->genre)) {
                rapporte_erreur("Attendu un opérateur booléen pour la condition", inst->condition);
                return CodeRetourValidation::Erreur;
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_TANTQUE:
        {
            auto inst = noeud->comme_tantque();

            if (inst->condition->type == nullptr &&
                !est_opérateur_bool(inst->condition->lexeme->genre)) {
                rapporte_erreur("Attendu un opérateur booléen pour la condition", inst->condition);
                return CodeRetourValidation::Erreur;
            }

            if (!inst->condition->type->est_type_bool()) {
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

            auto feuilles = expr->expression->comme_virgule();

            if (feuilles->expressions.est_vide()) {
                return CodeRetourValidation::OK;
            }

            auto premiere_feuille = feuilles->expressions[0];

            auto type_feuille = premiere_feuille->type;

            if (type_feuille->est_type_rien()) {
                m_espace->rapporte_erreur(
                    premiere_feuille,
                    "Impossible d'avoir un élément de type « rien » dans un tableau");
                return CodeRetourValidation::Erreur;
            }

            if (type_feuille->est_type_entier_constant()) {
                type_feuille = TypeBase::Z32;
                crée_transtypage_implicite_au_besoin(
                    feuilles->expressions[0],
                    {TypeTransformation::CONVERTI_ENTIER_CONSTANT, type_feuille});
            }

            for (auto i = 1; i < feuilles->expressions.taille(); ++i) {
                TENTE(crée_transtypage_implicite_si_possible(
                    feuilles->expressions[i],
                    type_feuille,
                    RaisonTranstypageImplicite::POUR_CONSTRUCTION_TABLEAU));
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
            if (résoud_type_final(expr, type) == CodeRetourValidation::Erreur) {
                return CodeRetourValidation::Erreur;
            }

            /* Visite récursivement le type pour s'assurer que tous les types dépendants sont
             * validés, ceci est nécessaire pour garantir que les infos types seront générés avec
             * les bonnes données. À FAIRE : permet l'ajournement des infos-types afin de ne pas
             * avoir à attendre. */
            kuri::ensemblon<Type *, 16> types_utilises;
            types_utilises.insere(type);
            auto attente_possible = attente_sur_type_si_drapeau_manquant(
                types_utilises, DrapeauxNoeud::DECLARATION_FUT_VALIDEE);

            if (attente_possible && attente_possible->est<AttenteSurType>() &&
                attente_possible->type() != racine_validation()) {
                return attente_possible.value();
            }

            expr->type = type;

            auto type_info_type = Type::nul();

            switch (expr->type->genre) {
                case GenreNoeud::POLYMORPHIQUE:
                case GenreNoeud::TUPLE:
                {
                    assert_rappel(false, [&]() {
                        dbg() << "Type illégal pour info type : " << chaine_type(expr->type);
                    });
                    break;
                }
                case GenreNoeud::EINI:
                case GenreNoeud::CHAINE:
                case GenreNoeud::RIEN:
                case GenreNoeud::BOOL:
                case GenreNoeud::OCTET:
                case GenreNoeud::TYPE_DE_DONNEES:
                case GenreNoeud::REEL:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_;
                    break;
                }
                case GenreNoeud::ENTIER_CONSTANT:
                case GenreNoeud::ENTIER_NATUREL:
                case GenreNoeud::ENTIER_RELATIF:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_entier;
                    break;
                }
                case GenreNoeud::REFERENCE:
                case GenreNoeud::POINTEUR:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_pointeur;
                    break;
                }
                case GenreNoeud::DECLARATION_STRUCTURE:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_structure;
                    break;
                }
                case GenreNoeud::DECLARATION_UNION:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_union;
                    break;
                }
                case GenreNoeud::TABLEAU_DYNAMIQUE:
                case GenreNoeud::TABLEAU_FIXE:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_tableau;
                    break;
                }
                case GenreNoeud::TYPE_TRANCHE:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_tranche;
                    break;
                }
                case GenreNoeud::FONCTION:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_fonction;
                    break;
                }
                case GenreNoeud::DECLARATION_ENUM:
                case GenreNoeud::ERREUR:
                case GenreNoeud::ENUM_DRAPEAU:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_enum;
                    break;
                }
                case GenreNoeud::DECLARATION_OPAQUE:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_opaque;
                    break;
                }
                case GenreNoeud::VARIADIQUE:
                {
                    type_info_type = m_compilatrice.typeuse.type_info_type_variadique;
                    break;
                }
                CAS_POUR_NOEUDS_HORS_TYPES:
                {
                    assert_rappel(false, [&]() {
                        dbg() << "Noeud non-géré pour type : " << expr->type->genre;
                    });
                    break;
                }
            }

            assert_rappel(type_info_type, [&]() {
                dbg() << "InfoType nul pour type : " << chaine_type(expr->type);
            });

            noeud->type = m_compilatrice.typeuse.type_pointeur_pour(type_info_type);

            break;
        }
        case GenreNoeud::EXPRESSION_INIT_DE:
        {
            auto init_de = noeud->comme_init_de();
            Type *type = nullptr;

            if (résoud_type_final(init_de->expression, type) == CodeRetourValidation::Erreur) {
                rapporte_erreur("impossible de définir le type de init_de", noeud);
                return CodeRetourValidation::Erreur;
            }

            /* À FAIRE : remplace ceci par une attente dans le gestionnaire. */
            m_compilatrice.gestionnaire_code->requiers_initialisation_type(m_espace, type);
            crée_entête_pour_initialisation_type(
                type, m_compilatrice, m_tacheronne->assembleuse, m_compilatrice.typeuse);

            auto types_entrees = kuri::tablet<Type *, 6>(1);
            types_entrees[0] = m_compilatrice.typeuse.type_pointeur_pour(type);

            auto type_fonction = m_compilatrice.typeuse.type_fonction(types_entrees,
                                                                      TypeBase::RIEN);
            noeud->type = type_fonction;
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

            if (expr_type->type->est_type_type_de_donnees()) {
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

            if (!type->est_type_pointeur()) {
                rapporte_erreur("Un pointeur est requis pour le déréférencement via 'mémoire'",
                                expr->expression,
                                erreur::Genre::TYPE_DIFFERENTS);
                return CodeRetourValidation::Erreur;
            }

            auto type_pointeur = type->comme_type_pointeur();
            noeud->type = type_pointeur->type_pointe;

            break;
        }
        case GenreNoeud::DECLARATION_STRUCTURE:
        {
            auto noeud_struct = noeud->comme_type_structure();
            return valide_structure(noeud_struct);
        }
        case GenreNoeud::DECLARATION_UNION:
        {
            auto noeud_union = noeud->comme_type_union();
            return valide_union(noeud_union);
        }
        case GenreNoeud::DECLARATION_ENUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        {
            return valide_énum(noeud->comme_type_enum());
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

            rapporte_erreur("Les coroutines ne sont plus supportées pour l'instant.", noeud);
            return CodeRetourValidation::Erreur;
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

            assert(m_compilatrice.typeuse.type_contexte);
            assert(m_compilatrice.globale_contexte_programme);

            if (!m_compilatrice.globale_contexte_programme->possède_drapeau(
                    DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(m_compilatrice.globale_contexte_programme);
            }

            if (variable->type != m_compilatrice.typeuse.type_contexte) {
                m_espace
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
                /* Nous avons un type variadique externe. */
                auto type_var = m_compilatrice.typeuse.type_variadique(nullptr);
                expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_var);
                return CodeRetourValidation::OK;
            }

            auto type_expr = expr->expression->type;

            if (type_expr->est_type_type_de_donnees()) {
                auto type_de_donnees = type_expr->comme_type_type_de_donnees();
                auto type_var = m_compilatrice.typeuse.type_variadique(
                    type_de_donnees->type_connu);
                expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_var);
            }
            else {
                if (!dls::outils::est_element(type_expr->genre,
                                              GenreNoeud::TABLEAU_FIXE,
                                              GenreNoeud::TABLEAU_DYNAMIQUE,
                                              GenreNoeud::TYPE_TRANCHE,
                                              GenreNoeud::VARIADIQUE)) {
                    m_espace
                        ->rapporte_erreur(
                            expr,
                            "Type invalide pour l'expansion variadique, je requiers "
                            "un type tableau, un type tranche, ou un type variadique")
                        .ajoute_message("Note : le type de l'expression est ")
                        .ajoute_message(chaine_type(type_expr))
                        .ajoute_message("\n");
                }

                if (type_expr->est_type_tableau_fixe()) {
                    auto type_tableau_fixe = type_expr->comme_type_tableau_fixe();
                    type_expr = m_compilatrice.typeuse.crée_type_tranche(
                        type_tableau_fixe->type_pointe);
                    crée_transtypage_implicite_au_besoin(
                        expr->expression,
                        {TypeTransformation::CONVERTI_TABLEAU_FIXE_VERS_TRANCHE, type_expr});
                }
                else if (type_expr->est_type_tableau_dynamique()) {
                    auto type_tableau_dynamique = type_expr->comme_type_tableau_dynamique();
                    type_expr = m_compilatrice.typeuse.crée_type_tranche(
                        type_tableau_dynamique->type_pointe);
                    crée_transtypage_implicite_au_besoin(
                        expr->expression,
                        {TypeTransformation::CONVERTI_TABLEAU_DYNAMIQUE_VERS_TRANCHE, type_expr});
                }

                expr->type = type_expr;
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_TENTE:
        {
            auto inst = noeud->comme_tente();
            inst->type = inst->expression_appelee->type;

            auto type_de_l_erreur = Type::nul();

            // voir ce que l'on retourne
            // - si aucun type erreur -> erreur ?
            // - si erreur seule -> il faudra vérifier l'erreur
            // - si union -> voir si l'union est sûre et contient une erreur, dépaquete celle-ci
            // dans le génération de code

            if (!inst->type->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_type(inst->type);
            }

            if (inst->type->est_type_erreur()) {
                type_de_l_erreur = inst->type;
            }
            else if (inst->type->est_type_union()) {
                auto type_union = inst->type->comme_type_union();
                auto possède_type_erreur = false;

                POUR (type_union->membres) {
                    if (it.type->est_type_erreur()) {
                        possède_type_erreur = true;
                    }
                }

                if (!possède_type_erreur) {
                    rapporte_erreur(
                        "Utilisation de « tente » sur une fonction qui ne retourne pas d'erreur",
                        inst);
                    return CodeRetourValidation::Erreur;
                }

                if (type_union->membres.taille() == 2) {
                    if (type_union->membres[0].type->est_type_erreur()) {
                        type_de_l_erreur = type_union->membres[0].type;
                        inst->type = type_union->membres[1].type;
                    }
                    else {
                        inst->type = type_union->membres[0].type;
                        type_de_l_erreur = type_union->membres[1].type;
                    }
                }
                else {
                    m_espace
                        ->rapporte_erreur(inst,
                                          "Les instructions tentes ne sont pas encore définies "
                                          "pour les unions n'ayant pas 2 membres uniquement.")
                        .ajoute_message("Le type du l'union est ")
                        .ajoute_message(chaine_type(type_union))
                        .ajoute_message("\n");
                    return CodeRetourValidation::Erreur;
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
                    rapporte_erreur_redéfinition_symbole(var_piege, decl);
                    return CodeRetourValidation::Erreur;
                }

                var_piege->type = type_de_l_erreur;

                auto decl_var_piege = m_tacheronne->assembleuse->crée_declaration_variable(
                    var_piege->lexeme);
                decl_var_piege->bloc_parent = inst->bloc;
                decl_var_piege->type = var_piege->type;
                decl_var_piege->ident = var_piege->ident;
                decl_var_piege->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
                decl_var_piege->genre_valeur = GenreValeur::TRANSCENDANTALE;

                inst->expression_piegee->comme_reference_declaration()->declaration_referee =
                    decl_var_piege;

                // ne l'ajoute pas aux expressions, car nous devons l'initialiser manuellement
                inst->bloc->ajoute_membre_au_debut(decl_var_piege);

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

            break;
        }
        case GenreNoeud::INSTRUCTION_EMPL:
        {
            auto empl = noeud->comme_empl();

            if (!empl->expression->est_declaration_variable()) {
                m_espace->rapporte_erreur(empl->expression,
                                          "Les directives empl ne sont pas supportées sur autre "
                                          "chose que des déclarations de variables.");
                return CodeRetourValidation::Erreur;
            }

            auto decl = empl->expression->comme_declaration_variable();

            empl->type = decl->type;
            decl->drapeaux |= DrapeauxNoeud::EMPLOYE;
            auto type_employe = decl->type;

            // permet le déréférencement de pointeur, mais uniquement sur un niveau
            if (type_employe->est_type_pointeur() || type_employe->est_type_reference()) {
                type_employe = type_déréférencé_pour(type_employe);
            }

            if (!type_employe->est_type_structure()) {
                m_unité->espace
                    ->rapporte_erreur(
                        decl, "Impossible d'employer une variable n'étant pas une structure.")
                    .ajoute_message("Le type de la variable est : ")
                    .ajoute_message(chaine_type(type_employe))
                    .ajoute_message(".\n\n");
                return CodeRetourValidation::Erreur;
            }

            if (!type_employe->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_type(type_employe);
            }

            auto type_structure = type_employe->comme_type_structure();

            // pour les structures, prend le bloc_parent qui sera celui de la structure
            auto bloc_parent = decl->bloc_parent;

            // pour les fonctions, utilisent leurs blocs si le bloc_parent est le bloc_parent de la
            // fonction (ce qui est le cas pour les paramètres...)
            if (fonction_courante() &&
                bloc_parent == fonction_courante()->corps->bloc->bloc_parent) {
                bloc_parent = fonction_courante()->corps->bloc;
            }

            POUR_INDEX (type_structure->membres) {
                if (it.drapeaux & MembreTypeComposé::EST_CONSTANT) {
                    continue;
                }

                auto decl_existante = trouve_dans_bloc(
                    bloc_parent, it.nom, bloc_parent->bloc_parent, fonction_courante());

                if (decl_existante) {
                    m_espace
                        ->rapporte_erreur(decl,
                                          "Impossible d'employer la déclaration car une "
                                          "déclaration avec le même nom qu'un de ses membres "
                                          "existe déjà dans le bloc.")
                        .ajoute_message("La déclaration existante est :\n")
                        .ajoute_site(decl_existante)
                        .ajoute_message("Le membre en conflit est :\n")
                        .ajoute_site(it.decl);
                    return CodeRetourValidation::Erreur;
                }

                auto decl_membre = m_tacheronne->assembleuse->crée_declaration_variable(
                    decl->lexeme);
                decl_membre->ident = it.nom;
                decl_membre->type = it.type;
                decl_membre->bloc_parent = bloc_parent;
                decl_membre->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
                decl_membre->declaration_vient_d_un_emploi = decl;
                decl_membre->index_membre_employe = index_it;
                decl_membre->expression = it.expression_valeur_defaut;
                decl_membre->genre_valeur = GenreValeur::TRANSCENDANTALE;

                bloc_parent->ajoute_membre(decl_membre);
            }
            break;
        }
        case GenreNoeud::DIRECTIVE_INTROSPECTION:
        {
            if (noeud->ident == ID::type_de_cette_fonction) {
                if (!racine_validation()->est_corps_fonction()) {
                    m_espace->rapporte_erreur(
                        noeud,
                        "#type_de_cette_fonction utilisée en dehors du corps d'une fonction.");
                    return CodeRetourValidation::Erreur;
                }

                noeud->type = m_compilatrice.typeuse.type_type_de_donnees(
                    fonction_courante()->type);
                return CodeRetourValidation::OK;
            }

            if (noeud->ident == ID::type_de_cette_structure) {
                auto type = union_ou_structure_courante();
                if (!type) {
                    m_espace->rapporte_erreur(
                        noeud,
                        "#type_de_cette_structure utilisée en dehors du bloc "
                        "d'une structure ou d'une union.");
                    return CodeRetourValidation::Erreur;
                }

                noeud->type = type->type;
                return CodeRetourValidation::OK;
            }

            if (noeud->ident == ID::nom_de_cette_fonction) {
                if (!fonction_courante()) {
                    m_espace->rapporte_erreur(
                        noeud, "#noeud_de_cette_fonction utilisé en dehors d'une fonction");
                    return CodeRetourValidation::Erreur;
                }
            }

            noeud->type = TypeBase::CHAINE;
            break;
        }
        case GenreNoeud::EXPRESSION_TYPE_TABLEAU_FIXE:
        {
            return valide_expression_type_tableau_fixe(
                noeud->comme_expression_type_tableau_fixe());
        }
        case GenreNoeud::EXPRESSION_TYPE_TABLEAU_DYNAMIQUE:
        {
            return valide_expression_type_tableau_dynamique(
                noeud->comme_expression_type_tableau_dynamique());
        }
        case GenreNoeud::EXPRESSION_TYPE_TRANCHE:
        {
            return valide_expression_type_tranche(noeud->comme_expression_type_tranche());
        }
        case GenreNoeud::EXPRESSION_TYPE_FONCTION:
        {
            return valide_expression_type_fonction(noeud->comme_expression_type_fonction());
        }
        CAS_POUR_NOEUDS_TYPES_FONDAMENTAUX:
        {
            assert_rappel(false,
                          [&]() { dbg() << "Noeud non-géré pour validation : " << noeud->genre; });
            break;
        }
    }

    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_accès_membre(NoeudExpressionMembre *expression_membre)
{
    auto structure = expression_membre->accedee;

    if (structure->est_reference_declaration()) {
        auto decl = structure->comme_reference_declaration()->declaration_referee;

        if (decl->est_declaration_module()) {
            auto module_ref = decl->comme_declaration_module()->module;
            /* À FAIRE(gestion) : attente spécifique sur tout le module. */
            if (module_ref->bloc == nullptr) {
                return Attente::sur_symbole(structure->comme_reference_declaration());
            }

            auto déclaration_référée = trouve_dans_bloc(module_ref->bloc,
                                                        expression_membre->ident);
            if (!déclaration_référée) {
                return Attente::sur_symbole(structure->comme_reference_declaration());
            }

            if (!déclaration_référée->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(déclaration_référée);
            }

            expression_membre->genre_valeur = déclaration_référée->genre_valeur;
            expression_membre->déclaration_référée = déclaration_référée;
            expression_membre->type = déclaration_référée->type;
            return CodeRetourValidation::OK;
        }
    }

    auto type = donne_type_accédé_effectif(structure->type);

    // Il est possible d'avoir une chaine de type : Struct1.Struct2.Struct3...
    if (type->est_type_type_de_donnees()) {
        auto type_de_donnees = type->comme_type_type_de_donnees();

        if (type_de_donnees->type_connu != nullptr) {
            type = type_de_donnees->type_connu;
            // change le type de la structure également pour simplifier la génération
            // de la RI (nous nous basons sur le type pour ça)
            structure->type = type;
        }
    }

    if (type->est_type_compose()) {
        if (!type->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
            return Attente::sur_type(type);
        }

        auto type_compose = type->comme_type_compose();
        auto info_membre = donne_membre_pour_nom(type_compose, expression_membre->ident);
        if (!info_membre.has_value()) {
            if (expression_membre->possède_drapeau(DrapeauxNoeud::GAUCHE_EXPRESSION_APPEL)) {
                /* Laisse la validation d'appel gérer ce cas. */
                expression_membre->aide_generation_code = PEUT_ÊTRE_APPEL_UNIFORME;
                return CodeRetourValidation::OK;
            }

            rapporte_erreur_membre_inconnu(expression_membre, expression_membre, type_compose);
            return CodeRetourValidation::Erreur;
        }

        auto const index_membre = info_membre->index_membre;
        auto const membre_est_constant = info_membre->membre.drapeaux &
                                         MembreTypeComposé::EST_CONSTANT;
        auto const membre_est_implicite = info_membre->membre.drapeaux &
                                          MembreTypeComposé::EST_IMPLICITE;

        expression_membre->type = info_membre->membre.type;
        expression_membre->index_membre = index_membre;

        if (type->est_type_enum() || type->est_type_erreur()) {
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
            if (structure->type->est_type_enum() &&
                structure->genre_valeur != GenreValeur::DROITE) {
                if (type->est_type_enum_drapeau()) {
                    if (!membre_est_implicite) {
                        expression_membre->genre_valeur = GenreValeur::TRANSCENDANTALE;
                        expression_membre->drapeaux |= DrapeauxNoeud::ACCES_EST_ENUM_DRAPEAU;
                    }
                }
                else {
                    m_espace->rapporte_erreur(
                        expression_membre,
                        "Impossible d'accéder à une variable de type énumération");
                    return CodeRetourValidation::Erreur;
                }
            }
        }
        else if (membre_est_constant) {
            expression_membre->genre_valeur = GenreValeur::DROITE;
        }
        else if (type->est_type_union()) {
            expression_membre->genre = GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION;
        }

        return CodeRetourValidation::OK;
    }

    if (expression_membre->possède_drapeau(DrapeauxNoeud::GAUCHE_EXPRESSION_APPEL)) {
        /* Laisse la validation d'appel gérer ce cas. */
        expression_membre->aide_generation_code = PEUT_ÊTRE_APPEL_UNIFORME;
        return CodeRetourValidation::OK;
    }

    m_espace
        ->rapporte_erreur(structure,
                          "Impossible de référencer un membre d'un type n'étant pas une structure")
        .ajoute_message("Note: le type est « ", chaine_type(type), " »");
    return CodeRetourValidation::Erreur;
}

static bool fonctions_ont_mêmes_définitions(NoeudDeclarationEnteteFonction const &fonction1,
                                            NoeudDeclarationEnteteFonction const &fonction2)
{
    /* À FAIRE(bibliothèque) : stocke les fonctions des bibliothèques dans celles-ci, afin de
     * pouvoir comparer des fonctions externes même si elles sont définies par des modules
     * différents. */
    if (fonction1.possède_drapeau(DrapeauxNoeud::EST_EXTERNE) &&
        fonction2.possède_drapeau(DrapeauxNoeud::EST_EXTERNE) && fonction1.données_externes &&
        fonction2.données_externes &&
        fonction1.données_externes->ident_bibliotheque ==
            fonction2.données_externes->ident_bibliotheque &&
        fonction1.données_externes->nom_symbole == fonction2.données_externes->nom_symbole) {
        return true;
    }

    if (fonction1.ident != fonction2.ident) {
        return false;
    }

    if (fonction1.type != fonction2.type) {
        return false;
    }

    /* Il est valide de redéfinir la fonction principale dans un autre espace. */
    if (fonction1.ident == ID::principale) {
        if (!fonction1.unité || !fonction2.unité) {
            /* S'il manque une unité, nous revérifierons lors de la validation de la deuxième
             * fonction. */
            return false;
        }

        if (fonction1.unité->espace != fonction2.unité->espace) {
            return false;
        }
    }

    return true;
}

RésultatValidation Sémanticienne::valide_entête_fonction(NoeudDeclarationEnteteFonction *decl)
{
    if (decl->est_operateur) {
        if (decl->est_operateur_pour()) {
            return valide_entête_opérateur_pour(decl->comme_operateur_pour());
        }

        return valide_entête_opérateur(decl);
    }

#ifdef STATISTIQUES_DETAILLEES
    auto possède_erreur = true;
    dls::chrono::chrono_rappel_milliseconde chrono_([&](double temps) {
        if (possède_erreur) {
            m_stats_typage.entêtes_fonctions.fusionne_entrée(ENTETE_FONCTION__TENTATIVES_RATEES,
                                                             {"", temps});
        }
    });
#endif

    CHRONO_TYPAGE(m_stats_typage.entêtes_fonctions, ENTETE_FONCTION__ENTETE_FONCTION);

    valide_paramètres_constants_fonction(decl);

    {
        CHRONO_TYPAGE(m_stats_typage.entêtes_fonctions, ENTETE_FONCTION__ARBRE_APLATIS);
        TENTE(valide_arbre_aplatis(decl));
    }

    TENTE(valide_paramètres_fonction(decl))

    if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE)) {
        /* Puisque les types sont polymorphiques, nous n'avons pas besoin de les valider.
         * Ce sera fait lors de la monomorphisation de la fonction. */
        decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
        return CodeRetourValidation::OK;
    }

    TENTE(valide_types_paramètres_fonction(decl));
    TENTE(valide_définition_unique_fonction(decl));
    TENTE(valide_symbole_externe(decl, TypeSymbole::FONCTION));

    decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

    if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE)) {
        /* Marque les paramètres comme étant utilisés afin que les coulisses ne les marquent pas
         * comme inutilisés. */
        for (auto i = 0; i < decl->params.taille(); i++) {
            auto param = decl->parametre_entree(i);
            param->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
        }
    }

#ifdef STATISTIQUES_DETAILLEES
    possède_erreur = false;
#endif

    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_entête_opérateur(NoeudDeclarationEnteteFonction *decl)
{
#ifdef STATISTIQUES_DETAILLEES
    auto possède_erreur = true;
    dls::chrono::chrono_rappel_milliseconde chrono_([&](double temps) {
        if (possède_erreur) {
            m_stats_typage.entêtes_fonctions.fusionne_entrée({"tentatives râtées", temps});
        }
    });
#endif

    CHRONO_TYPAGE(m_stats_typage.entêtes_fonctions, ENTETE_FONCTION__ENTETE_FONCTION);

    {
        CHRONO_TYPAGE(m_stats_typage.entêtes_fonctions, ENTETE_FONCTION__ARBRE_APLATIS);
        TENTE(valide_arbre_aplatis(decl));
    }

    TENTE(valide_paramètres_fonction(decl));
    TENTE(valide_types_paramètres_fonction(decl));

    CHRONO_TYPAGE(m_stats_typage.entêtes_fonctions, ENTETE_FONCTION__TYPES_OPERATEURS);
    auto type_fonc = decl->type->comme_type_fonction();
    auto type_résultat = type_fonc->type_sortie;

    if (type_résultat == TypeBase::RIEN) {
        rapporte_erreur("Un opérateur ne peut retourner 'rien'", decl);
        return CodeRetourValidation::Erreur;
    }

    if (est_opérateur_bool(decl->lexeme->genre) && type_résultat != TypeBase::BOOL) {
        rapporte_erreur("Un opérateur de comparaison doit retourner 'bool'", decl);
        return CodeRetourValidation::Erreur;
    }

    TENTE(valide_définition_unique_opérateur(decl));

    decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

#ifdef STATISTIQUES_DETAILLEES
    possède_erreur = false;
#endif

    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_entête_opérateur_pour(
    NoeudDeclarationOperateurPour *opérateur)
{
    CHRONO_TYPAGE(m_stats_typage.entêtes_fonctions, ENTETE_FONCTION__ENTETE_FONCTION);

    {
        CHRONO_TYPAGE(m_stats_typage.entêtes_fonctions, ENTETE_FONCTION__ARBRE_APLATIS);
        TENTE(valide_arbre_aplatis(opérateur));
    }

    TENTE(valide_paramètres_fonction(opérateur));
    TENTE(valide_types_paramètres_fonction(opérateur));

    if (opérateur->params.taille() == 0) {
        rapporte_erreur("Un opérateur 'pour' doit avoir au moins un paramètre d'entrée",
                        opérateur);
        return CodeRetourValidation::Erreur;
    }

    auto type_itéré = opérateur->params[0]->type;
    auto table_opérateurs = m_compilatrice.opérateurs->donne_ou_crée_table_opérateurs(type_itéré);

    if (table_opérateurs->opérateur_pour != nullptr) {
        rapporte_erreur_redéfinition_fonction(opérateur, table_opérateurs->opérateur_pour);
        return CodeRetourValidation::Erreur;
    }

    table_opérateurs->opérateur_pour = opérateur;

    opérateur->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    return CodeRetourValidation::OK;
}

void Sémanticienne::valide_paramètres_constants_fonction(NoeudDeclarationEnteteFonction *decl)
{
    if (!decl->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE)) {
        /* Seules les fonctions polymorphiques ont des constantes. */
        return;
    }

    POUR (*decl->bloc_constantes->membres.verrou_ecriture()) {
        if (it->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
            /* Les valeurs polymorphiques typées explicitement sont dans les paramètres, et seront
             * donc validées avec les paramètres. */
            if (it->comme_declaration_constante()->expression_type) {
                continue;
            }
        }

        auto type_poly = m_compilatrice.typeuse.crée_polymorphique(it->ident);
        it->type = m_compilatrice.typeuse.type_type_de_donnees(type_poly);
        it->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    }

    if (!decl->monomorphisations) {
        decl->monomorphisations =
            m_tacheronne->allocatrice_noeud.crée_monomorphisations_fonction();
    }
}

RésultatValidation Sémanticienne::valide_paramètres_fonction(NoeudDeclarationEnteteFonction *decl)
{
    CHRONO_TYPAGE(m_stats_typage.entêtes_fonctions, ENTETE_FONCTION__PARAMETRES);
    auto noms = kuri::ensemblon<IdentifiantCode *, 16>();
    auto dernier_est_variadic = false;

    for (auto i = 0; i < decl->params.taille(); ++i) {
        if (!decl->params[i]->est_declaration_variable() && !decl->params[i]->est_empl() &&
            !decl->params[i]->est_declaration_constante()) {
            m_unité->espace->rapporte_erreur(
                decl->params[i], "Le paramètre n'est ni une déclaration, ni un emploi");
            return CodeRetourValidation::Erreur;
        }

        auto param = decl->parametre_entree(i);
        auto expression = param->expression;

        if (noms.possède(param->ident)) {
            rapporte_erreur("Redéfinition de l'argument", param, erreur::Genre::ARGUMENT_REDEFINI);
            return CodeRetourValidation::Erreur;
        }

        if (dernier_est_variadic) {
            rapporte_erreur("Argument déclaré après un argument variadic", param);
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

        noms.insere(param->ident);

        if (param->type->est_type_variadique()) {
            param->drapeaux |= DrapeauxNoeud::EST_VARIADIQUE;
            decl->drapeaux_fonction |= DrapeauxNoeudFonction::EST_VARIADIQUE;
            dernier_est_variadic = true;

            auto type_var = param->type->comme_type_variadique();

            if (!decl->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE) &&
                type_var->type_pointe == nullptr) {
                rapporte_erreur("La déclaration de fonction variadique sans type n'est"
                                " implémentée que pour les fonctions externes",
                                param);
                return CodeRetourValidation::Erreur;
            }
        }
    }

    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_types_paramètres_fonction(
    NoeudDeclarationEnteteFonction *decl)
{
    CHRONO_TYPAGE(m_stats_typage.entêtes_fonctions, ENTETE_FONCTION__TYPES_PARAMETRES);

    kuri::tablet<Type *, 6> types_entrees;
    types_entrees.réserve(decl->params.taille());

    POUR (decl->params) {
        types_entrees.ajoute(it->type);
    }

    Type *type_sortie = nullptr;

    if (decl->params_sorties.taille() == 1) {
        if (résoud_type_final(
                decl->params_sorties[0]->comme_declaration_variable()->expression_type,
                type_sortie) == CodeRetourValidation::Erreur) {
            return CodeRetourValidation::Erreur;
        }
    }
    else {
        kuri::tablet<MembreTypeComposé, 6> membres;
        membres.réserve(decl->params_sorties.taille());

        for (auto &expr : decl->params_sorties) {
            auto type_declare = expr->comme_declaration_variable();
            if (résoud_type_final(type_declare->expression_type, type_sortie) ==
                CodeRetourValidation::Erreur) {
                return CodeRetourValidation::Erreur;
            }

            membres.ajoute({nullptr, type_sortie});
        }

        type_sortie = m_compilatrice.typeuse.crée_tuple(membres);
    }

    decl->param_sortie->type = type_sortie;

    if (decl->ident == ID::principale) {
        if (decl->params.taille() != 0) {
            m_espace->rapporte_erreur(
                decl->params[0],
                "La fonction principale ne doit pas prendre de paramètres d'entrée !");
            return CodeRetourValidation::Erreur;
        }

        if (decl->param_sortie->type->est_type_tuple()) {
            m_espace->rapporte_erreur(
                decl->param_sortie,
                "La fonction principale ne peut retourner qu'une seule valeur !");
            return CodeRetourValidation::Erreur;
        }

        if (decl->param_sortie->type != TypeBase::Z32) {
            m_espace->rapporte_erreur(decl->param_sortie,
                                      "La fonction principale doit retourner un z32 !");
            return CodeRetourValidation::Erreur;
        }
    }

    CHRONO_TYPAGE(m_stats_typage.entêtes_fonctions, ENTETE_FONCTION__TYPES_FONCTION);
    decl->type = m_compilatrice.typeuse.type_fonction(types_entrees, type_sortie);

    return CodeRetourValidation::OK;
}

static bool est_point_entrée_sortie(const NoeudDeclarationEnteteFonction *decl)
{
    return dls::outils::est_element(decl->ident,
                                    ID::__point_d_entree_systeme,
                                    ID::__point_d_entree_dynamique,
                                    ID::__point_de_sortie_dynamique);
}

RésultatValidation Sémanticienne::valide_définition_unique_fonction(
    NoeudDeclarationEnteteFonction *decl)
{
    if (est_point_entrée_sortie(decl)) {
        /* Les points d'entrée et de sortie sont copiés pour chaque espace, donc il est possible
         * qu'ils existent plusieurs fois dans le bloc. */
        return CodeRetourValidation::OK;
    }

    CHRONO_TYPAGE(m_stats_typage.entêtes_fonctions, ENTETE_FONCTION__REDEFINITION);
    auto decl_existante = decl->bloc_parent->declaration_pour_ident(decl->ident);

    if (!decl_existante || !decl_existante->est_entete_fonction()) {
        return CodeRetourValidation::OK;
    }

    auto entete_existante = decl_existante->comme_entete_fonction();
    POUR (*entete_existante->ensemble_de_surchages.verrou_lecture()) {
        if (it == decl || !it->est_entete_fonction()) {
            continue;
        }

        if (fonctions_ont_mêmes_définitions(*decl, *(it->comme_entete_fonction()))) {
            rapporte_erreur_redéfinition_fonction(decl, it);
            return CodeRetourValidation::Erreur;
        }
    }

    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_définition_unique_opérateur(
    NoeudDeclarationEnteteFonction *decl)
{
    CHRONO_TYPAGE(m_stats_typage.entêtes_fonctions, ENTETE_FONCTION__REDEFINITION_OPERATEUR);
    auto operateurs = m_compilatrice.opérateurs.verrou_ecriture();
    auto type_fonc = decl->type->comme_type_fonction();
    auto type_résultat = type_fonc->type_sortie;

    if (decl->params.taille() == 1) {
        auto &iter_op = operateurs->trouve_unaire(decl->lexeme->genre);
        auto type1 = type_fonc->types_entrees[0];

        for (auto i = 0; i < iter_op.taille(); ++i) {
            auto op = &iter_op[i];

            if (op->type_opérande == type1) {
                if (op->est_basique) {
                    rapporte_erreur("redéfinition de l'opérateur basique", decl);
                    return CodeRetourValidation::Erreur;
                }

                m_espace->rapporte_erreur(decl, "Redéfinition de l'opérateur")
                    .ajoute_message("L'opérateur fut déjà défini ici :\n")
                    .ajoute_site(op->déclaration);
                return CodeRetourValidation::Erreur;
            }
        }

        operateurs->ajoute_perso_unaire(decl->lexeme->genre, type1, type_résultat, decl);
        return CodeRetourValidation::OK;
    }

    auto type1 = type_fonc->types_entrees[0];
    auto type2 = type_fonc->types_entrees[1];

    if (type1->table_opérateurs) {
        for (auto &op : type1->table_opérateurs->opérateurs(decl->lexeme->genre).plage()) {
            if (op->type2 == type2) {
                if (op->est_basique) {
                    rapporte_erreur("redéfinition de l'opérateur basique", decl);
                    return CodeRetourValidation::Erreur;
                }

                m_espace->rapporte_erreur(decl, "Redéfinition de l'opérateur")
                    .ajoute_message("L'opérateur fut déjà défini ici :\n")
                    .ajoute_site(op->decl);
                return CodeRetourValidation::Erreur;
            }
        }
    }

    operateurs->ajoute_perso(decl->lexeme->genre, type1, type2, type_résultat, decl);
    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_symbole_externe(NoeudDeclarationSymbole *decl,
                                                         TypeSymbole type_symbole)
{
    // À FAIRE: n'utilise externe que pour les fonctions vraiment externes...
    if (!decl->données_externes || !decl->données_externes->ident_bibliotheque) {
        return CodeRetourValidation::OK;
    }

    auto données_externes = decl->données_externes;

    auto bibliotheque = m_compilatrice.gestionnaire_bibliothèques->trouve_bibliotheque(
        données_externes->ident_bibliotheque);

    if (!bibliotheque) {
        m_espace
            ->rapporte_erreur(decl, "Impossible de définir la bibliothèque où trouver le symbole")
            .ajoute_message("« ",
                            données_externes->ident_bibliotheque->nom,
                            " » ne réfère à aucune bibliothèque !");
        return CodeRetourValidation::Erreur;
    }

    données_externes->symbole = bibliotheque->crée_symbole(données_externes->nom_symbole,
                                                           type_symbole);
    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_arbre_aplatis(NoeudExpression *declaration)
{
    aplatis_arbre(declaration, m_arbre_courant);

    for (; m_arbre_courant->index_courant < m_arbre_courant->noeuds.taille();
         ++m_arbre_courant->index_courant) {
        auto noeud_enfant = m_arbre_courant->noeuds[m_arbre_courant->index_courant];

        if (noeud_enfant->est_declaration_type() && noeud_enfant != racine_validation()) {
            /* Les types ont leurs propres unités de compilation. */
            if (!noeud_enfant->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(noeud_enfant->comme_declaration_type());
            }

            continue;
        }

        if (noeud_enfant->est_entete_fonction() && noeud_enfant != fonction_courante()) {
            /* Les fonctions nichées dans d'autres fonctions ont leurs propres unités de
             * compilation. */
            if (!noeud_enfant->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(noeud_enfant->comme_entete_fonction());
            }

            continue;
        }

        auto résultat = valide_sémantique_noeud(noeud_enfant);
        if (est_erreur(résultat)) {
            m_arbres_aplatis.ajoute(m_arbre_courant);
            return résultat;
        }

        if (est_attente(résultat)) {
            return résultat;
        }
    }

    m_unité->arbre_aplatis = nullptr;
    m_arbres_aplatis.ajoute(m_arbre_courant);

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
                expressions.ajoute({assignation->assignée->ident, assignation->expression});
            }
            else {
                expressions.ajoute({nullptr, it});
            }
        }
    }
    else {
        if (expr->est_assignation_variable()) {
            auto assignation = expr->comme_assignation_variable();
            expressions.ajoute({assignation->assignée->ident, assignation->expression});
        }
        else {
            expressions.ajoute({nullptr, expr});
        }
    }
}

RésultatValidation Sémanticienne::valide_expression_retour(NoeudRetour *inst)
{
    auto fonction = fonction_courante();
    auto type_sortie = Type::nul();
    auto est_coroutine = false;
    auto est_corps_texte = false;

    if (fonction) {
        auto type_fonc = fonction_courante()->type->comme_type_fonction();
        type_sortie = type_fonc->type_sortie;
        est_corps_texte = fonction_courante()->corps->est_corps_texte;
        est_coroutine = fonction_courante()->est_coroutine;
    }
    else {
        /* Nous pouvons être dans le bloc d'un #test, auquel cas la fonction n'a pas encore été
         * créée car la validation du bloc se fait avant le noeud. Vérifions si tel est le cas. */
        if (!(racine_validation()->est_execute() && racine_validation()->ident == ID::test)) {
            m_espace->rapporte_erreur(inst,
                                      "Utilisation de « retourne » en dehors d'une fonction, d'un "
                                      "opérateur, ou d'un #test");
            return CodeRetourValidation::Erreur;
        }

        /* Un #test ne doit rien retourner. */
        type_sortie = TypeBase::RIEN;
    }

    auto const bloc_parent = inst->bloc_parent;
    if (bloc_est_dans_differe(bloc_parent)) {
        rapporte_erreur("« retourne » utilisée dans un bloc « diffère »", inst);
        return CodeRetourValidation::Erreur;
    }

    if (inst->expression == nullptr) {
        inst->type = TypeBase::RIEN;

        /* Vérifie si le type de sortie est une union, auquel cas nous pouvons retourner une valeur
         * du type ayant le membre « rien » actif. */
        if (type_sortie->est_type_union() && !type_sortie->comme_type_union()->est_nonsure) {
            if (peut_construire_union_via_rien(type_sortie->comme_type_union())) {
                inst->aide_generation_code = RETOURNE_UNE_UNION_VIA_RIEN;
                return CodeRetourValidation::OK;
            }
        }

        if ((!est_coroutine && type_sortie != inst->type) || est_corps_texte) {
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

        if (!expr->type->est_type_chaine()) {
            rapporte_erreur("Attendu un type chaine pour le retour de #corps_texte",
                            inst->expression);
            return CodeRetourValidation::Erreur;
        }

        inst->type = TypeBase::CHAINE;

        DonneesAssignations donnees;
        donnees.expression = inst->expression;
        donnees.variables.ajoute(fonction_courante()->params_sorties[0]);
        donnees.transformations.ajoute({});

        inst->donnees_exprs.ajoute(std::move(donnees));
        return CodeRetourValidation::OK;
    }

    if (type_sortie->est_type_rien()) {
        m_espace->rapporte_erreur(inst->expression,
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

            if (expr->type->est_type_tuple()) {
                m_espace->rapporte_erreur(
                    it.expression,
                    "Impossible de nommer les variables de retours si l'expression retourne "
                    "plusieurs valeurs");
                return CodeRetourValidation::Erreur;
            }

            for (auto i = 0; i < fonction_courante()->params_sorties.taille(); ++i) {
                if (it.ident == fonction_courante()->params_sorties[i]->ident) {
                    if (expressions[i] != nullptr) {
                        m_espace->rapporte_erreur(
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
                m_espace->rapporte_erreur(
                    it.expression,
                    "L'expressoin doit avoir un nom si elle suit une autre ayant déjà un nom");
                return CodeRetourValidation::Erreur;
            }

            if (expressions[index_courant] != nullptr) {
                m_espace->rapporte_erreur(
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
                                          Type *type_de_l_expression) -> RésultatValidation {
        auto résultat = cherche_transformation(type_de_l_expression, variable->type);

        if (std::holds_alternative<Attente>(résultat)) {
            return std::get<Attente>(résultat);
        }

        auto transformation = std::get<TransformationType>(résultat);

        if (transformation.type == TypeTransformation::IMPOSSIBLE) {
            rapporte_erreur_assignation_type_différents(
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

        if (it->type->est_type_rien()) {
            rapporte_erreur(
                "impossible de retourner une expression de type « rien » à une variable",
                it,
                erreur::Genre::ASSIGNATION_RIEN);
            return CodeRetourValidation::Erreur;
        }
        else if (it->type->est_type_tuple()) {
            auto type_tuple = it->type->comme_type_tuple();

            donnees.multiple_retour = true;

            for (auto &membre : type_tuple->membres) {
                if (variables.est_vide()) {
                    m_espace->rapporte_erreur(it, "Trop d'expressions de retour");
                    return CodeRetourValidation::Erreur;
                }

                TENTE(valide_typage_et_ajoute(donnees, variables.defile(), it, membre.type));
            }
        }
        else {
            if (variables.est_vide()) {
                m_espace->rapporte_erreur(it, "Trop d'expressions de retour");
                return CodeRetourValidation::Erreur;
            }

            TENTE(valide_typage_et_ajoute(donnees, variables.defile(), it, it->type));
        }

        donnees_retour.ajoute(std::move(donnees));
    }

    // À FAIRE : valeur par défaut des expressions
    if (!variables.est_vide()) {
        m_espace->rapporte_erreur(inst, "Expressions de retour manquante");
        return CodeRetourValidation::Erreur;
    }

    inst->type = type_sortie;

    inst->donnees_exprs.réserve(static_cast<int>(donnees_retour.taille()));
    POUR (donnees_retour) {
        inst->donnees_exprs.ajoute(std::move(it));
    }

    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_cuisine(NoeudDirectiveCuisine *directive)
{
    auto expr = directive->expression;

    if (!expr->est_appel()) {
        m_espace->rapporte_erreur(
            expr, "L'expression d'une directive de cuisson doit être une expression d'appel !");
        return CodeRetourValidation::Erreur;
    }

    if (!expr->type->est_type_fonction()) {
        m_espace->rapporte_erreur(
            expr, "La cuisson d'autre chose qu'une fonction n'est pas encore supportée !");
        return CodeRetourValidation::Erreur;
    }

    if (auto expr_variable = trouve_expression_non_constante(expr)) {
        m_espace
            ->rapporte_erreur(directive,
                              "L'expression de la directive n'est pas constante et ne "
                              "peut donc être évaluée.")
            .ajoute_message("L'expression non-constante est :\n")
            .ajoute_site(expr_variable);
        return CodeRetourValidation::Erreur;
    }

    directive->type = expr->type;
    return CodeRetourValidation::OK;
}

/* ------------------------------------------------------------------------- */
/** \name Valide référence déclaration.
 * \{ */

/* Retourne vrai si la déclaration se situe après la référence à celle-ci. Ceci n'est destiné que
 * pour les déclarations de variables locales à une fonction. */
static bool déclaration_est_postérieure_à_la_référence(NoeudDeclaration const *déclaration,
                                                       NoeudExpressionReference const *référence)
{
    assert(déclaration->bloc_parent);

    if (déclaration->bloc_parent != référence->bloc_parent) {
        /* La déclaration et la référence sont dans deux blocs sémantiques différents. */
        return false;
    }

    if (!déclaration->est_declaration_variable()) {
        return false;
    }

    if (déclaration->possède_drapeau(DrapeauxNoeud::EST_GLOBALE)) {
        return false;
    }

    if (référence->possède_drapeau(DrapeauxNoeud::IDENTIFIANT_EST_ACCENTUÉ_GRAVE)) {
        return false;
    }

    return déclaration->lexeme->ligne > référence->lexeme->ligne;
}

/* Retourne vrai s'il est possible de référencer la déclaration selon son contexte d'utilisation
 * (basé sur sa position par rapport à la déclaration ou ses drapeaux de position dans l'arbre).
 */
static bool est_référence_déclaration_valide(EspaceDeTravail *espace,
                                             NoeudExpressionReference const *expr,
                                             NoeudDeclaration const *decl)
{
    if (déclaration_est_postérieure_à_la_référence(decl, expr)) {
        espace->rapporte_erreur(expr, "Utilisation d'une variable avant sa déclaration.")
            .ajoute_message("Le symbole fut déclaré ici :\n\n")
            .ajoute_site(decl);
        return false;
    }

    if (est_déclaration_polymorphique(decl) &&
        !expr->possède_drapeau(DrapeauxNoeud::GAUCHE_EXPRESSION_APPEL)) {
        espace
            ->rapporte_erreur(
                expr,
                "Référence d'une déclaration polymorphique en dehors d'une expression d'appel.")
            .ajoute_message("Le polymorphe fut déclaré ici :\n\n")
            .ajoute_site(decl);
        return false;
    }

    if (decl->est_entete_fonction()) {
        auto entête = decl->comme_entete_fonction();
        if (entête->possède_drapeau(DrapeauxNoeudFonction::EST_INTRINSÈQUE) &&
            !expr->possède_drapeau(DrapeauxNoeud::GAUCHE_EXPRESSION_APPEL)) {
            espace
                ->rapporte_erreur(
                    expr,
                    "Utilisation d'une fonction intrinsèque en dehors d'une expression d'appel.")
                .ajoute_message(
                    "NOTE : Les fonctions intrinsèques ne peuvent être prises par adresse.");
            return false;
        }
    }

    return true;
}

RésultatValidation Sémanticienne::valide_référence_déclaration(NoeudExpressionReference *expr,
                                                               NoeudBloc *bloc_recherche)
{
    CHRONO_TYPAGE(m_stats_typage.ref_decl, REFERENCE_DECLARATION__VALIDATION);

    assert_rappel(bloc_recherche != nullptr,
                  [&]() { dbg() << erreur::imprime_site(*m_espace, expr); });

    /* Les membres des énums sont des déclarations mais n'ont pas de type, et ne sont pas validées.
     * Pour de telles déclarations, la logique ici nous forcerait à attendre sur ces déclarations
     * jusqu'à leur validation, mais étant déclarées sans types, ceci résulterait en une erreur de
     * compilation. Ce drapeau sers à quitter la fonction dès que possible pour éviter d'attendre
     * sur quoi que soit.
     */
    auto recherche_est_pour_expression_discrimination_énum = false;
    auto bloc_recherche_original = NoeudBloc::nul();

    if (expr->possède_drapeau(DrapeauxNoeud::EXPRESSION_TEST_DISCRIMINATION)) {
        auto const noeud_discr = expr->bloc_parent->appartiens_à_discr;
        assert(noeud_discr);

        auto const expression_discriminée = noeud_discr->expression_discriminee;
        auto const type_discriminée = expression_discriminée->type;
        assert(type_discriminée);

        if (type_discriminée->est_type_enum()) {
            auto type_énum = type_discriminée->comme_type_enum();
            if (!type_énum->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(type_énum);
            }

            bloc_recherche_original = bloc_recherche;
            bloc_recherche = type_énum->bloc;
            recherche_est_pour_expression_discrimination_énum = true;
        }
        else if (type_discriminée->est_type_union()) {
            auto type_union = type_discriminée->comme_type_union();
            if (!type_union->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_declaration(type_union);
            }

            /* Les unions anonymes n'ont pas de bloc. */
            if (type_union->bloc) {
                bloc_recherche_original = bloc_recherche;
                bloc_recherche = type_union->bloc;
            }
        }
    }
    else if (expr->possède_drapeau(DrapeauxNoeud::IDENTIFIANT_EST_ACCENTUÉ_GRAVE)) {
        auto fonction = fonction_courante();
        if (!fonction) {
            m_espace->rapporte_erreur(
                expr, "Identifiant préfixé d'un accent grave en dehors d'une fonction.");
            return CodeRetourValidation::Erreur;
        }

        if (!fonction->est_operateur_pour()) {
            m_espace->rapporte_erreur(
                expr, "Identifiant préfixé d'un accent grave en dehors d'un opérateur « pour ».");
            return CodeRetourValidation::Erreur;
        }

        /* L'entête de l'opérateur est partagé entre toutes les copies du corps. Pour accéder au
         * bon corps, nous devons utiliser la racine de validation qui doit être le corps. */
        assert(racine_validation()->est_corps_fonction());

        auto const corps_operateur_pour = racine_validation()->comme_corps_fonction();
        if (!corps_operateur_pour->est_macro_boucle_pour) {
            m_espace->rapporte_erreur(expr,
                                      "Validation d'un identifiant accentué grave alors qu'aucune "
                                      "boucle « pour » ne requiers l'opérateur");
            return CodeRetourValidation::Erreur;
        }

        auto const noeud_pour = corps_operateur_pour->est_macro_boucle_pour;

        /* « it » et « index_it » peuvent être nommés par le programme, donc les variables
         * correspondantes en dehors du macro peuvent avoir un nom différent. Renseigne directement
         * les déclaration référées. */
        if (expr->ident == ID::it) {
            expr->declaration_referee = noeud_pour->decl_it;
            expr->genre_valeur = noeud_pour->decl_it->genre_valeur;
            expr->type = noeud_pour->decl_it->type;
            return CodeRetourValidation::OK;
        }

        if (expr->ident == ID::index_it) {
            expr->declaration_referee = noeud_pour->decl_index_it;
            expr->type = noeud_pour->decl_index_it->type;
            expr->genre_valeur = noeud_pour->decl_index_it->genre_valeur;
            return CodeRetourValidation::OK;
        }

        /* Les identifiants doivent être soit dans le bloc de la boucle, ou dans un bloc parent de
         * celui-ci. */
        bloc_recherche = noeud_pour->bloc;
    }

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
    /* Nous n'utilisons pas bloc->appartiens_à_fonction pour déterminer la fonction courante car
     * ceci échouerait si la fonction pointée par le bloc est la déclaration d'un type fonction. */
    auto decl = trouve_dans_bloc_ou_module(
        bloc_recherche, expr->ident, fichier, fonction_courante());

    if (decl == nullptr) {
        if (bloc_recherche_original) {
            decl = trouve_dans_bloc_ou_module(
                bloc_recherche_original, expr->ident, fichier, fonction_courante());
        }
        if (decl == nullptr && fonction_courante() &&
            fonction_courante()->possède_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION)) {
            auto site_monomorphisation = fonction_courante()->site_monomorphisation;

            fichier = m_compilatrice.fichier(site_monomorphisation->lexeme->fichier);
            decl = trouve_dans_bloc_ou_module(
                site_monomorphisation->bloc_parent, expr->ident, fichier, fonction_courante());
        }
        if (decl == nullptr) {
            return Attente::sur_symbole(expr);
        }
    }
#endif

    if (recherche_est_pour_expression_discrimination_énum) {
        return CodeRetourValidation::OK;
    }

    if (!est_référence_déclaration_valide(m_espace, expr, decl)) {
        /* Une erreur dû être rapportée. */
        return CodeRetourValidation::Erreur;
    }

    if (decl->est_declaration_type()) {
        if (decl->est_type_opaque() &&
            !decl->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
            return Attente::sur_declaration(decl);
        }

        /* Ne vérifions pas seulement le drapeau DrapeauxNoeud::DECLARATION_FUT_VALIDEE, car la
         * référence peut être vers le type en validation (p.e. un pointeur vers une autre instance
         * de la structure). */
        if (!decl->type) {
            CHRONO_TYPAGE(m_stats_typage.ref_decl, REFERENCE_DECLARATION__TYPE_DE_DONNES);
            expr->type = m_compilatrice.typeuse.type_type_de_donnees(
                decl->comme_declaration_type());
        }
        else {
            assert_rappel(decl->type->est_type_type_de_donnees(), [&]() {
                dbg() << "Le type de " << nom_humainement_lisible(decl)
                      << " n'est pas un type de données.\n"
                      << erreur::imprime_site(*m_espace, decl);
            });

            expr->type = decl->type;
        }
        expr->declaration_referee = decl;
    }
    else {
        if (!decl->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
            // À FAIRE : curseur := curseur.curseurs[0] -> il faut pouvoir déterminer si la
            // référence est celle de la variable que l'on valide, ceci ne fonctionnera pas pour
            // les déclarations multiples, ou les types étant référencés dans les expressions de
            // leurs membres
            if (decl == m_unité->noeud) {
                m_espace->rapporte_erreur(expr,
                                          "Utilisation d'une variable dans sa définition !\n");
                return CodeRetourValidation::Erreur;
            }
            return Attente::sur_declaration(decl);
        }

        // les fonctions peuvent ne pas avoir de type au moment si elles sont des appels
        // polymorphiques
        assert_rappel(decl->type || decl->est_entete_fonction() ||
                          decl->est_declaration_module() ||
                          decl->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE),
                      [&]() { dbg() << erreur::imprime_site(*m_espace, expr); });
        expr->declaration_referee = decl;
        expr->type = decl->type;

        /* si nous avons une valeur polymorphique, crée un type de données
         * temporaire pour que la validation soit contente, elle sera
         * remplacée par une constante appropriée lors de la validation
         * de l'appel */
        if (decl->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
            expr->type = m_compilatrice.typeuse.type_type_de_donnees(expr->type);
        }
    }

    if (decl->est_declaration_constante()) {
        auto valeur = decl->comme_declaration_constante()->valeur_expression;
        /* Remplace tout de suite les constantes de fonctions par les fonctions, pour ne pas
         * avoir à s'en soucier plus tard. */
        if (valeur.est_fonction()) {
            decl->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
            decl = valeur.fonction();
            expr->declaration_referee = decl;
        }
    }

    expr->genre_valeur = decl->genre_valeur;

    decl->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
    if (decl->est_declaration_variable()) {
        auto decl_var = decl->comme_declaration_variable();
        if (decl_var->declaration_vient_d_un_emploi) {
            decl_var->declaration_vient_d_un_emploi->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
        }
    }

    if (decl->possède_drapeau(DrapeauxNoeud::EST_MARQUÉE_INUTILISÉE)) {
        m_espace->rapporte_erreur(expr, "Utilisation d'une déclaration marquée comme inutilisée.")
            .ajoute_message("La déclaration fut déclarée ici :\n")
            .ajoute_site(decl);
        return CodeRetourValidation::Erreur;
    }

    return CodeRetourValidation::OK;
}

/** \} */

RésultatValidation Sémanticienne::valide_type_opaque(NoeudDeclarationTypeOpaque *decl)
{
    auto type_opacifie = Type::nul();

    if (!decl->expression_type->possède_drapeau(DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE)) {
        if (résoud_type_final(decl->expression_type, type_opacifie) ==
            CodeRetourValidation::Erreur) {
            return CodeRetourValidation::Erreur;
        }

        if (!type_opacifie->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
            return Attente::sur_type(type_opacifie);
        }
    }
    else {
        type_opacifie = m_compilatrice.typeuse.crée_polymorphique(decl->expression_type->ident);
    }

    decl->type = m_compilatrice.typeuse.type_type_de_donnees(decl);
    decl->type_opacifie = type_opacifie;
    decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

    if (type_opacifie->est_type_polymorphique()) {
        decl->drapeaux_type |= DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
    }
    else {
        decl->alignement = type_opacifie->alignement;
        decl->taille_octet = type_opacifie->taille_octet;
        m_compilatrice.graphe_dépendance->connecte_type_type(decl, type_opacifie);
    }

    return CodeRetourValidation::OK;
}

MetaProgramme *Sémanticienne::crée_métaprogramme_corps_texte(NoeudBloc *bloc_corps_texte,
                                                             NoeudBloc *bloc_parent,
                                                             const Lexème *lexème)
{
    auto fonction = m_tacheronne->assembleuse->crée_entete_fonction(lexème);
    auto nouveau_corps = fonction->corps;

    assert(m_tacheronne->assembleuse->bloc_courant() == nullptr);
    m_tacheronne->assembleuse->bloc_courant(bloc_parent);

    fonction->bloc_constantes = m_tacheronne->assembleuse->empile_bloc(lexème, fonction);
    fonction->bloc_parametres = m_tacheronne->assembleuse->empile_bloc(lexème, fonction);

    fonction->bloc_parent = bloc_parent;
    nouveau_corps->bloc_parent = fonction->bloc_parametres;
    /* Le corps de la fonction pour les #corps_texte des structures est celui de la déclaration. */
    nouveau_corps->bloc = bloc_corps_texte;

    /* mise en place du type de la fonction : () -> chaine */
    fonction->drapeaux_fonction |= (DrapeauxNoeudFonction::EST_MÉTAPROGRAMME |
                                    DrapeauxNoeudFonction::FUT_GÉNÉRÉE_PAR_LA_COMPILATRICE);

    auto decl_sortie = m_tacheronne->assembleuse->crée_declaration_variable(lexème);
    decl_sortie->ident = m_compilatrice.table_identifiants->identifiant_pour_chaine("__ret0");
    decl_sortie->type = TypeBase::CHAINE;
    decl_sortie->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

    fonction->params_sorties.ajoute(decl_sortie);
    fonction->param_sortie = decl_sortie;

    auto types_entrees = kuri::tablet<Type *, 6>(0);

    auto type_sortie = TypeBase::CHAINE;

    fonction->type = m_compilatrice.typeuse.type_fonction(types_entrees, type_sortie);
    fonction->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

    auto metaprogramme = m_compilatrice.crée_metaprogramme(m_espace);
    metaprogramme->corps_texte = bloc_corps_texte;
    metaprogramme->fonction = fonction;

    m_tacheronne->assembleuse->dépile_bloc();
    m_tacheronne->assembleuse->dépile_bloc();
    m_tacheronne->assembleuse->dépile_bloc();
    assert(m_tacheronne->assembleuse->bloc_courant() == nullptr);

    return metaprogramme;
}

NoeudExpression *Sémanticienne::racine_validation() const
{
    assert(m_unité->noeud);
    return m_unité->noeud;
}

NoeudDeclarationEnteteFonction *Sémanticienne::fonction_courante() const
{
    if (racine_validation()->est_entete_fonction()) {
        return racine_validation()->comme_entete_fonction();
    }

    if (racine_validation()->est_corps_fonction()) {
        return racine_validation()->comme_corps_fonction()->entete;
    }

    return nullptr;
}

Type *Sémanticienne::union_ou_structure_courante() const
{
    if (racine_validation()->est_type_structure()) {
        return racine_validation()->comme_type_structure();
    }

    if (racine_validation()->est_type_union()) {
        return racine_validation()->comme_type_union();
    }

    return nullptr;
}

static void avertis_declarations_inutilisees(EspaceDeTravail const &espace,
                                             NoeudDeclarationEnteteFonction const &entete)
{
    if (entete.possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE)) {
        return;
    }

    /* Les paramètres de sortie sont toujours utilisés.
     * À FAIRE : nous avons les paramètres de sorties des fonctions nichées dans le bloc de cette
     * fonction ?
     */
    POUR (entete.params_sorties) {
        it->comme_declaration_variable()->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
    }
    entete.param_sortie->drapeaux |= DrapeauxNoeud::EST_UTILISEE;

    for (int i = 0; i < entete.params.taille(); ++i) {
        auto decl_param = entete.parametre_entree(i);
        if (decl_param->possède_drapeau(DrapeauxNoeud::EST_MARQUÉE_INUTILISÉE)) {
            continue;
        }

        if (decl_param->possède_drapeau(DrapeauxNoeud::EST_UTILISEE)) {
            continue;
        }

        if (entete.est_operateur && !entete.est_operateur_pour()) {
            espace.rapporte_erreur(decl_param, "Paramètre d'opérateur inutilisé.");
        }
        else {
            espace.rapporte_avertissement(decl_param, "Paramètre inutilisé.");
        }
    }

    auto const &corps = *entete.corps;

    visite_noeud(corps.bloc,
                 PreferenceVisiteNoeud::ORIGINAL,
                 true,
                 [&espace, entete](const NoeudExpression *noeud) {
                     if (noeud->est_type_structure()) {
                         return DecisionVisiteNoeud::IGNORE_ENFANTS;
                     }

                     if (!noeud->est_declaration()) {
                         return DecisionVisiteNoeud::CONTINUE;
                     }

                     /* Ignore les variables implicites des boucles « pour ». */
                     if (noeud->ident == ID::it || noeud->ident == ID::index_it) {
                         return DecisionVisiteNoeud::CONTINUE;
                     }

                     /* '_' sers à définir une variable qui ne sera pas utilisée. */
                     if (noeud->ident == ID::_) {
                         return DecisionVisiteNoeud::CONTINUE;
                     }

                     if (noeud->est_declaration_variable()) {
                         auto const decl_var = noeud->comme_declaration_variable();
                         if (possède_annotation(decl_var, "inutilisée")) {
                             return DecisionVisiteNoeud::CONTINUE;
                         }
                     }

                     if (noeud->est_declaration_variable_multiple()) {
                         /* Les déclarations multiples comme « a, b := ... » ont les déclarations
                          * ajoutées séparément aux membres du bloc. */
                         return DecisionVisiteNoeud::CONTINUE;
                     }

                     /* Les corps fonctions sont des déclarations et sont visités, mais ne sont pas
                      * marqués comme utilisés car seules les entêtes le sont. Évitons d'émettre un
                      * avertissement pour rien. */
                     if (noeud->est_corps_fonction()) {
                         return DecisionVisiteNoeud::CONTINUE;
                     }

                     if (!noeud->possède_drapeau(DrapeauxNoeud::EST_UTILISEE)) {
                         if (noeud->est_entete_fonction()) {
                             auto message = enchaine(
                                 "Dans la fonction ",
                                 entete.ident->nom,
                                 " : fonction « ",
                                 (noeud->ident ? noeud->ident->nom : kuri::chaine_statique("")),
                                 " » inutilisée");
                             espace.rapporte_avertissement(noeud, message);

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

                     /* Ne traversons pas les structures et énumérations non plus. */
                     if (noeud->est_declaration_type()) {
                         return DecisionVisiteNoeud::IGNORE_ENFANTS;
                     }

                     return DecisionVisiteNoeud::CONTINUE;
                 });
}

static void échange_corps_entêtes(NoeudDeclarationEnteteFonction *ancienne_fonction,
                                  NoeudDeclarationEnteteFonction *nouvelle_fonction)
{
    auto nouveau_corps = nouvelle_fonction->corps;
    auto ancien_corps = ancienne_fonction->corps;

    /* Échange les corps. */
    nouvelle_fonction->corps = ancien_corps;
    ancien_corps->entete = nouvelle_fonction;
    ancien_corps->bloc_parent = nouvelle_fonction->bloc_parent;
    ancien_corps->bloc->bloc_parent = nouvelle_fonction->bloc_parametres;

    ancienne_fonction->corps = nouveau_corps;
    nouveau_corps->entete = ancienne_fonction;
    nouveau_corps->bloc_parent = ancienne_fonction->bloc_parent;
    nouveau_corps->bloc->bloc_parent = ancienne_fonction->bloc_parametres;

    /* Remplace les références à ancienne_fonction dans nouvelle_fonction->corps par
     * nouvelle_fonction. */
    visite_noeud(nouvelle_fonction->corps,
                 PreferenceVisiteNoeud::ORIGINAL,
                 false,
                 [&](NoeudExpression const *noeud) -> DecisionVisiteNoeud {
                     if (noeud->est_bloc()) {
                         auto bloc = noeud->comme_bloc();
                         assert(bloc->appartiens_à_fonction == ancienne_fonction);
                         const_cast<NoeudBloc *>(bloc)->appartiens_à_fonction = nouvelle_fonction;
                     }
                     return DecisionVisiteNoeud::CONTINUE;
                 });
}

RésultatValidation Sémanticienne::valide_fonction(NoeudDeclarationCorpsFonction *decl)
{
    auto entete = decl->entete;

    for (int i = 0; i < entete->params.taille(); ++i) {
        auto decl_param = entete->parametre_entree(i);
        if (possède_annotation(decl_param, "inutilisée")) {
            decl_param->drapeaux |= DrapeauxNoeud::EST_MARQUÉE_INUTILISÉE;
        }
    }

    if (entete->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE) &&
        !entete->possède_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION)) {
        // nous ferons l'analyse sémantique plus tard
        decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
        return CodeRetourValidation::OK;
    }

    decl->type = entete->type;

    auto est_corps_texte = decl->est_corps_texte;

    if (est_corps_texte &&
        !decl->possède_drapeau(DrapeauxNoeud::METAPROGRAMME_CORPS_TEXTE_FUT_CREE)) {
        auto metaprogramme = crée_métaprogramme_corps_texte(
            decl->bloc, entete->bloc_parent, decl->lexeme);
        metaprogramme->corps_texte_pour_fonction = entete;

        auto fonction = metaprogramme->fonction;
        échange_corps_entêtes(entete, fonction);

        if (entete->possède_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION)) {
            fonction->drapeaux_fonction |= DrapeauxNoeudFonction::EST_MONOMORPHISATION;
        }
        else {
            fonction->drapeaux_fonction &= ~DrapeauxNoeudFonction::EST_MONOMORPHISATION;
        }
        fonction->site_monomorphisation = entete->site_monomorphisation;

        // préserve les constantes polymorphiques
        if (fonction->possède_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION)) {
            POUR (*entete->bloc_constantes->membres.verrou_lecture()) {
                fonction->bloc_constantes->ajoute_membre(it);
            }
        }

        decl->drapeaux |= DrapeauxNoeud::METAPROGRAMME_CORPS_TEXTE_FUT_CREE;

        /* Puisque nous validons le #corps_texte, l'entête pour la fonction courante doit être
         * celle de la fonction de métaprogramme. */
        entete = fonction;
    }

    CHRONO_TYPAGE(m_stats_typage.corps_fonctions, CORPS_FONCTION__VALIDATION);

    TENTE(valide_arbre_aplatis(decl));

    auto bloc = decl->bloc;
    auto inst_ret = derniere_instruction(bloc);

    /* si aucune instruction de retour -> vérifie qu'aucun type n'a été spécifié */
    if (inst_ret == nullptr) {
        auto type_fonc = entete->type->comme_type_fonction();
        auto type_sortie = type_fonc->type_sortie;

        if (type_sortie->est_type_union() && !type_sortie->comme_type_union()->est_nonsure) {
            if (peut_construire_union_via_rien(type_sortie->comme_type_union())) {
                decl->aide_generation_code = REQUIERS_RETOUR_UNION_VIA_RIEN;
            }
        }
        else {
            if ((!type_fonc->type_sortie->est_type_rien() && !entete->est_coroutine) ||
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

    simplifie_arbre(m_unité->espace, m_tacheronne->assembleuse, m_compilatrice.typeuse, entete);

    if (est_corps_texte) {
        /* Puisque la validation du #corps_texte peut être interrompue, nous devons retrouver le
         * métaprogramme : nous ne pouvons pas prendre l'adresse du métaprogramme créé ci-dessus.
         * À FAIRE : considère réusiner la gestion des métaprogrammes dans le GestionnaireCode afin
         * de pouvoir requérir la compilation du métaprogramme dès sa création, mais d'attendre que
         * la fonction soit validée afin de le compiler.
         */
        auto metaprogramme = m_compilatrice.metaprogramme_pour_fonction(entete);
        m_compilatrice.gestionnaire_code->requiers_compilation_metaprogramme(m_espace,
                                                                             metaprogramme);
    }

    decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

    avertis_declarations_inutilisees(*m_espace, *entete);

    if (entete->possède_drapeau(DrapeauxNoeudFonction::CLICHÉ_ASA_FUT_REQUIS)) {
        imprime_arbre(entete, std::cerr, 0);
    }
    if (entete->possède_drapeau(DrapeauxNoeudFonction::CLICHÉ_ASA_CANONIQUE_FUT_REQUIS)) {
        imprime_arbre_substitue(entete, std::cerr, 0);
    }

    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_opérateur(NoeudDeclarationCorpsFonction *decl)
{
    auto entete = decl->entete;
    decl->type = entete->type;

    TENTE(valide_arbre_aplatis(decl));

    auto inst_ret = derniere_instruction(decl->bloc);

    if (inst_ret == nullptr && !entete->est_operateur_pour()) {
        rapporte_erreur("Instruction de retour manquante", decl, erreur::Genre::TYPE_DIFFERENTS);
        return CodeRetourValidation::Erreur;
    }

    /* La simplification des corps des opérateurs « pour » se fera lors de la simplification de la
     * boucle « pour » utilisant ledit corps. */
    if (!entete->est_operateur_pour()) {
        simplifie_arbre(
            m_unité->espace, m_tacheronne->assembleuse, m_compilatrice.typeuse, entete);
    }

    avertis_declarations_inutilisees(*m_espace, *entete);

    decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    return CodeRetourValidation::OK;
}

enum {
    VALIDE_ENUM_ERREUR,
    VALIDE_ENUM_DRAPEAU,
    VALIDE_ENUM_NORMAL,
};

template <int N>
RésultatValidation Sémanticienne::valide_énum_impl(NoeudEnum *decl)
{
    decl->type = m_compilatrice.typeuse.type_type_de_donnees(decl);
    decl->taille_octet = decl->type_sous_jacent->taille_octet;
    decl->alignement = decl->type_sous_jacent->alignement;

    m_compilatrice.opérateurs->ajoute_opérateur_basique_enum(decl);

    auto noms_rencontres = kuri::ensemblon<IdentifiantCode *, 32>();

    auto derniere_valeur = ValeurExpression();
    assert(!derniere_valeur.est_valide());

    auto &membres = decl->membres;
    membres.réserve(decl->bloc->expressions->taille());
    decl->bloc->réserve_membres(decl->bloc->expressions->taille());

    int64_t valeur_enum_min = std::numeric_limits<int64_t>::max();
    int64_t valeur_enum_max = std::numeric_limits<int64_t>::min();
    int64_t valeurs_legales = 0;

    POUR (*decl->bloc->expressions.verrou_ecriture()) {
        if (!it->est_declaration_constante()) {
            rapporte_erreur("Type d'expression inattendu dans l'énum", it);
            return CodeRetourValidation::Erreur;
        }

        auto decl_expr = it->comme_declaration_constante();
        decl_expr->type = decl;

        decl->bloc->ajoute_membre(decl_expr);

        if (decl_expr->expression_type != nullptr) {
            rapporte_erreur("Expression d'énumération déclarée avec un type", it);
            return CodeRetourValidation::Erreur;
        }

        if (noms_rencontres.possède(decl_expr->ident)) {
            rapporte_erreur("Redéfinition du membre", decl_expr);
            return CodeRetourValidation::Erreur;
        }

        noms_rencontres.insere(decl_expr->ident);

        auto expr = decl_expr->expression;

        auto valeur = ValeurExpression();
        assert(!valeur.est_valide());

        if (expr != nullptr) {
            auto res = evalue_expression(m_compilatrice, decl->bloc, expr);

            if (res.est_errone) {
                m_espace->rapporte_erreur(res.noeud_erreur, res.message_erreur);
                return CodeRetourValidation::Erreur;
            }

            if (N == VALIDE_ENUM_ERREUR) {
                if (res.valeur.entiere() == 0) {
                    m_espace->rapporte_erreur(
                        expr,
                        "L'expression d'une enumération erreur ne peut s'évaluer à 0 (cette "
                        "valeur est réservée par la compilatrice).");
                    return CodeRetourValidation::Erreur;
                }
            }

            if (!res.valeur.est_entiere()) {
                m_espace->rapporte_erreur(
                    expr, "L'expression d'une énumération doit être de type entier");
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
                        m_espace->rapporte_erreur(decl_expr,
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

        if (est_hors_des_limites(valeur.entiere(), decl->type_sous_jacent)) {
            auto e = m_espace->rapporte_erreur(
                decl_expr, "Valeur hors des limites pour le type de l'énumération");
            e.ajoute_message("Le type des données de l'énumération est « ",
                             chaine_type(decl->type_sous_jacent),
                             " ».");
            e.ajoute_message("Les valeurs légales pour un tel type se trouvent entre ",
                             valeur_min(decl->type_sous_jacent),
                             " et ",
                             valeur_max(decl->type_sous_jacent),
                             ".\n");
            e.ajoute_message("Or, la valeur courante est de ", valeur.entiere(), ".\n");
            return CodeRetourValidation::Erreur;
        }

        valeur_enum_min = std::min(valeur.entiere(), valeur_enum_min);
        valeur_enum_max = std::max(valeur.entiere(), valeur_enum_max);

        if (N == VALIDE_ENUM_DRAPEAU) {
            valeurs_legales |= valeur.entiere();
        }

        membres.ajoute({nullptr, decl, it->ident, 0, static_cast<int>(valeur.entiere())});

        derniere_valeur = valeur;
    }

    membres.ajoute({nullptr,
                    TypeBase::Z32,
                    ID::nombre_elements,
                    0,
                    membres.taille(),
                    nullptr,
                    MembreTypeComposé::EST_IMPLICITE});
    membres.ajoute({nullptr,
                    decl,
                    ID::min,
                    0,
                    static_cast<int>(valeur_enum_min),
                    nullptr,
                    MembreTypeComposé::EST_IMPLICITE});
    membres.ajoute({nullptr,
                    decl,
                    ID::max,
                    0,
                    static_cast<int>(valeur_enum_max),
                    nullptr,
                    MembreTypeComposé::EST_IMPLICITE});

    if (N == VALIDE_ENUM_DRAPEAU) {
        membres.ajoute({nullptr,
                        decl,
                        ID::valeurs_legales,
                        0,
                        static_cast<int>(valeurs_legales),
                        nullptr,
                        MembreTypeComposé::EST_IMPLICITE});
        membres.ajoute({nullptr,
                        decl,
                        ID::valeurs_illegales,
                        0,
                        static_cast<int>(~valeurs_legales),
                        nullptr,
                        MembreTypeComposé::EST_IMPLICITE});
        membres.ajoute({nullptr, decl, ID::zero, 0, 0, nullptr, MembreTypeComposé::EST_IMPLICITE});
    }

    decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_énum(NoeudEnum *decl)
{
    CHRONO_TYPAGE(m_stats_typage.énumérations, ENUMERATION__VALIDATION);

    if (decl->est_type_erreur()) {
        decl->type_sous_jacent = TypeBase::Z32;
    }
    else if (decl->expression_type != nullptr) {
        TENTE(valide_sémantique_noeud(decl->expression_type));

        if (résoud_type_final(decl->expression_type, decl->type_sous_jacent) ==
            CodeRetourValidation::Erreur) {
            return CodeRetourValidation::Erreur;
        }

        /* les énum_drapeaux doivent être des types naturels pour éviter les problèmes
         * d'arithmétiques binaire */
        if (decl->est_type_enum_drapeau() && !decl->type_sous_jacent->est_type_entier_naturel()) {
            m_espace
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

        if (!est_type_entier(decl->type_sous_jacent)) {
            m_espace
                ->rapporte_erreur(decl->expression_type,
                                  "Le type de données d'une énumération doit être de type entier.")
                .ajoute_message("NOTE : le type est ", chaine_type(decl->type_sous_jacent), ".\n");
            return CodeRetourValidation::Erreur;
        }
    }
    else if (decl->est_type_enum_drapeau()) {
        decl->type_sous_jacent = TypeBase::N32;
    }
    else {
        decl->type_sous_jacent = TypeBase::Z32;
    }

    if (decl->est_type_erreur()) {
        return valide_énum_impl<VALIDE_ENUM_ERREUR>(decl);
    }

    if (decl->est_type_enum_drapeau()) {
        return valide_énum_impl<VALIDE_ENUM_DRAPEAU>(decl);
    }

    return valide_énum_impl<VALIDE_ENUM_NORMAL>(decl);
}

/* ------------------------------------------------------------------------- */
/** \name Validation des déclarations de structures et d'unions.
 * \{ */

/* Structure auxiliaire pour créer la liste des membres d'une structure ou d'une union.
 * Elle tient également compte du nombre de memrbes non-constants de la structure.
 * Cette structure ne performe aucune validation, ne se contentant que d'ajouter les
 * membres à la liste.
 */
struct ConstructriceMembresTypeComposé {
  private:
    TypeCompose &m_type_composé;
    int m_membres_non_constant = 0;

    /* Les membres n'apparaissant pas dans le code machine sont ajoutés à la fin. */
    kuri::tablet<MembreTypeComposé, 6> m_membres_extras{};

  public:
    ConstructriceMembresTypeComposé(TypeCompose &type_composé, NoeudBloc const *bloc)
        : m_type_composé(type_composé)
    {
        // @réinitialise en cas d'erreurs passées
        type_composé.membres.efface();
        type_composé.membres.réserve(bloc->nombre_de_membres());
    }

    void finalise()
    {
        m_type_composé.nombre_de_membres_réels = m_type_composé.membres.taille();

        POUR (m_membres_extras) {
            m_type_composé.membres.ajoute(it);
        }
    }

    void ajoute_type_de_données(NoeudDeclarationType *déclaration, Typeuse &typeuse)
    {
        // utilisation d'un type de données afin de pouvoir automatiquement déterminer un type
        assert(déclaration->type->est_type_type_de_donnees());
        m_membres_extras.ajoute({nullptr,
                                 déclaration->type,
                                 déclaration->ident,
                                 0,
                                 0,
                                 nullptr,
                                 MembreTypeComposé::EST_CONSTANT});
    }

    void ajoute_constante(NoeudDeclarationConstante *déclaration)
    {
        m_membres_extras.ajoute({déclaration,
                                 déclaration->type,
                                 déclaration->ident,
                                 0,
                                 0,
                                 déclaration->expression,
                                 MembreTypeComposé::EST_CONSTANT});
    }

    void ajoute_membre_employé(NoeudDeclaration *déclaration)
    {
        m_membres_non_constant += 1;
        m_type_composé.membres.ajoute({déclaration->comme_declaration_variable(),
                                       déclaration->type,
                                       déclaration->ident,
                                       0,
                                       0,
                                       nullptr,
                                       MembreTypeComposé::EST_UN_EMPLOI});
    }

    void ajoute_membre_provenant_d_un_emploi(NoeudDeclarationVariable *déclaration)
    {
        m_membres_non_constant += 1;
        m_membres_extras.ajoute({déclaration,
                                 déclaration->type,
                                 déclaration->ident,
                                 0,
                                 0,
                                 déclaration->expression,
                                 MembreTypeComposé::PROVIENT_D_UN_EMPOI});
    }

    void ajoute_membre_simple(NoeudExpression *membre, NoeudExpression *initialisateur)
    {
        m_membres_non_constant += 1;
        auto decl_var_enfant = NoeudDeclarationVariable::nul();
        if (membre->est_declaration_variable()) {
            decl_var_enfant = membre->comme_declaration_variable();
        }
        else if (membre->est_reference_declaration()) {
            auto ref = membre->comme_reference_declaration();
            if (ref->declaration_referee->est_declaration_variable()) {
                decl_var_enfant = ref->declaration_referee->comme_declaration_variable();
            }
        }

        m_type_composé.membres.ajoute(
            {decl_var_enfant, membre->type, membre->ident, 0, 0, initialisateur, 0});
    }

    void ajoute_membre_invisible()
    {
        m_membres_non_constant += 1;
        /* Ajoute un membre, d'un octet de taille. */
        m_type_composé.membres.ajoute({nullptr, TypeBase::BOOL, ID::chaine_vide, 0, 0, nullptr});
    }

    int donne_compte_membres_non_constant() const
    {
        return m_membres_non_constant;
    }
};

static bool le_membre_référence_le_type_par_valeur(TypeCompose const *type_composé,
                                                   NoeudExpression *expression_membre)
{
    if (type_composé == expression_membre->type) {
        return true;
    }

    auto type_membre = expression_membre->type;
    if (type_membre->est_type_tableau_fixe() &&
        type_membre->comme_type_tableau_fixe()->type_pointe == type_composé) {
        return true;
    }

    // À FAIRE : type opaque, tableaux fixe multi-dimensionnel

    return false;
}

static void rapporte_erreur_type_membre_invalide(EspaceDeTravail *espace,
                                                 TypeCompose const *type_composé,
                                                 NoeudExpression *membre)
{
    auto nom_classe = kuri::chaine_statique();

    if (type_composé->est_type_structure()) {
        nom_classe = "la structure";
    }
    else {
        assert(type_composé->est_type_union());
        nom_classe = "l'union";
    }

    auto message = enchaine("Impossible d'utiliser le type « ",
                            chaine_type(membre->type),
                            " » comme membre de ",
                            nom_classe);

    espace->rapporte_erreur(membre, message);
}

static void rapporte_erreur_inclusion_récursive_type(EspaceDeTravail *espace,
                                                     TypeCompose const *type_composé,
                                                     NoeudExpression *expression_membre)
{
    auto message = kuri::chaine_statique();
    if (type_composé->est_type_structure()) {
        message = "Utilisation du type de la structure comme type d'un membre par valeur.";
    }
    else {
        assert(type_composé->est_type_union());
        message = "Utilisation du type de l'union comme type d'un membre par valeur.";
    }

    auto e = espace->rapporte_erreur(expression_membre, message);

    if (expression_membre->est_declaration_variable()) {
        auto déclaration_variable = expression_membre->comme_declaration_variable();
        if (déclaration_variable->declaration_vient_d_un_emploi) {
            e.ajoute_message("Le membre fut inclus via l'emploi suivant :\n")
                .ajoute_site(déclaration_variable->declaration_vient_d_un_emploi);
        }
    }
}

static RésultatValidation valide_types_pour_calcule_taille_type(EspaceDeTravail *espace,
                                                                TypeCompose const *type_composé)
{
    POUR (type_composé->membres) {
        if (it.type->est_type_rien()) {
            continue;
        }

        if (!it.type->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
            return Attente::sur_type(it.type);
        }

        if (it.type->alignement == 0) {
            espace->rapporte_erreur(it.decl, "impossible de définir l'alignement du type")
                .ajoute_message("Le type est « ", chaine_type(it.type), " »\n");
            return CodeRetourValidation::Erreur;
        }

        if (it.type->taille_octet == 0) {
            espace->rapporte_erreur(it.decl, "impossible de définir la taille du type");
            return CodeRetourValidation::Erreur;
        }
    }

    return CodeRetourValidation::OK;
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
RésultatValidation Sémanticienne::valide_structure(NoeudStruct *decl)
{
    if (!decl->type) {
        decl->type = m_compilatrice.typeuse.type_type_de_donnees(decl);
    }

    if (decl->est_externe && decl->bloc == nullptr) {
        decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
        /* INITIALISATION_TYPE_FUT_CREEE est à cause de attente_sur_type_si_drapeau_manquant */
        decl->drapeaux_type |= (DrapeauxTypes::TYPE_NE_REQUIERS_PAS_D_INITIALISATION |
                                DrapeauxTypes::INITIALISATION_TYPE_FUT_CREEE);
        return CodeRetourValidation::OK;
    }

    if (decl->est_polymorphe) {
        TENTE(valide_arbre_aplatis(decl));

        if (!decl->monomorphisations) {
            decl->monomorphisations =
                m_tacheronne->allocatrice_noeud.crée_monomorphisations_struct();
        }

        // nous validerons les membres lors de la monomorphisation
        decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
        decl->drapeaux_type |= DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
        return CodeRetourValidation::OK;
    }

    if (decl->est_corps_texte) {
        /* Nous devons avoir deux passes : une pour créer la fonction du métaprogramme, une autre
         * pour requérir la compilation dudit métaprogramme. */
        if (!decl->metaprogramme_corps_texte) {
            auto metaprogramme = crée_métaprogramme_corps_texte(
                decl->bloc, decl->bloc_parent, decl->lexeme);
            auto fonction = metaprogramme->fonction;
            assert(fonction->corps->bloc);

            decl->metaprogramme_corps_texte = metaprogramme;
            metaprogramme->corps_texte_pour_structure = decl;

            if (decl->est_monomorphisation) {
                decl->bloc_constantes->membres.avec_verrou_ecriture(
                    [fonction](kuri::tableau<NoeudDeclaration *, int> &membres) {
                        POUR (membres) {
                            fonction->bloc_constantes->ajoute_membre(it);
                        }
                    });
            }

            m_compilatrice.gestionnaire_code->requiers_typage(m_espace, fonction->corps);
            return Attente::sur_declaration(fonction->corps);
        }

        auto metaprogramme = decl->metaprogramme_corps_texte;
        auto fichier = m_compilatrice.crée_fichier_pour_metaprogramme(metaprogramme);
        m_compilatrice.gestionnaire_code->requiers_compilation_metaprogramme(m_espace,
                                                                             metaprogramme);
        return Attente::sur_parsage(fichier);
    }

    TENTE(valide_arbre_aplatis(decl));

    CHRONO_TYPAGE(m_stats_typage.structures, STRUCTURE__VALIDATION);

    if (!decl->est_monomorphisation) {
        auto decl_precedente = trouve_dans_bloc(
            decl->bloc_parent, decl, decl->bloc_parent->bloc_parent);

        // la bibliothèque C a des symboles qui peuvent être les mêmes pour les fonctions et les
        // structres (p.e. stat)
        if (decl_precedente != nullptr && decl_precedente->genre == decl->genre) {
            rapporte_erreur_redéfinition_symbole(decl, decl_precedente);
            return CodeRetourValidation::Erreur;
        }
    }

    auto type_compose = decl->comme_type_compose();

    ConstructriceMembresTypeComposé constructrice(*type_compose, decl->bloc);

    POUR (*decl->bloc->membres.verrou_lecture()) {
        if (it->est_declaration_type()) {
            constructrice.ajoute_type_de_données(it->comme_declaration_type(),
                                                 m_compilatrice.typeuse);
            continue;
        }

        if (it->possède_drapeau(DrapeauxNoeud::EMPLOYE)) {
            if (!it->type->est_type_structure()) {
                m_espace->rapporte_erreur(
                    it, "Ne peut pas employer un type n'étant pas une structure");
                return CodeRetourValidation::Erreur;
            }

            constructrice.ajoute_membre_employé(it);
            continue;
        }

        if (it->est_declaration_constante()) {
            constructrice.ajoute_constante(it->comme_declaration_constante());
            continue;
        }

        if (it->est_declaration_variable()) {
            auto decl_var = it->comme_declaration_variable();

            // À FAIRE(emploi) : préserve l'emploi dans les données types
            if (decl_var->declaration_vient_d_un_emploi) {
                if (le_membre_référence_le_type_par_valeur(type_compose, decl_var)) {
                    rapporte_erreur_inclusion_récursive_type(m_espace, type_compose, decl_var);
                    return CodeRetourValidation::Erreur;
                }

                constructrice.ajoute_membre_provenant_d_un_emploi(decl_var);
                continue;
            }

            if (!est_type_valide_pour_membre(decl_var->type)) {
                rapporte_erreur_type_membre_invalide(m_espace, type_compose, decl_var);
                return CodeRetourValidation::Erreur;
            }

            if (le_membre_référence_le_type_par_valeur(type_compose, decl_var)) {
                rapporte_erreur_inclusion_récursive_type(m_espace, type_compose, decl_var);
                return CodeRetourValidation::Erreur;
            }

            constructrice.ajoute_membre_simple(decl_var, decl_var->expression);
            continue;
        }

        if (!it->est_declaration_variable_multiple()) {
            rapporte_erreur("Déclaration inattendu dans le bloc de la structure", it);
            return CodeRetourValidation::Erreur;
        }

        auto decl_var = it->comme_declaration_variable_multiple();
        for (auto &donnees : decl_var->donnees_decl.plage()) {
            for (auto i = 0; i < donnees.variables.taille(); ++i) {
                auto var = donnees.variables[i];

                if (!est_type_valide_pour_membre(var->type)) {
                    rapporte_erreur_type_membre_invalide(m_espace, type_compose, var);
                    return CodeRetourValidation::Erreur;
                }

                if (var->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
                    rapporte_erreur(
                        "Expression invalide dans la déclaration du membre de la structure", var);
                    return CodeRetourValidation::Erreur;
                }

                if (le_membre_référence_le_type_par_valeur(type_compose, var)) {
                    rapporte_erreur_inclusion_récursive_type(m_espace, type_compose, var);
                    return CodeRetourValidation::Erreur;
                }

                /* l'arbre syntaxique des expressions par défaut doivent contenir
                 * la transformation puisque nous n'utilisons pas la déclaration
                 * pour générer la RI */
                auto expression = donnees.expression;
                crée_transtypage_implicite_au_besoin(expression, donnees.transformations[i]);

                constructrice.ajoute_membre_simple(var, expression);
            }
        }
    }

    if (constructrice.donne_compte_membres_non_constant() == 0) {
        if (!decl->est_externe) {
            constructrice.ajoute_membre_invisible();
        }
    }

    constructrice.finalise();

    POUR (*decl->bloc->expressions.verrou_ecriture()) {
        if (!it->est_assignation_variable()) {
            continue;
        }

        auto expr_assign = it->comme_assignation_variable();
        auto variable = expr_assign->assignée;

        for (auto &membre : type_compose->membres) {
            if (membre.nom != variable->ident) {
                continue;
            }

            membre.drapeaux |= MembreTypeComposé::POSSÈDE_EXPRESSION_SPÉCIALE;
            membre.expression_valeur_defaut = expr_assign->expression;
            break;
        }
    }

    /* Valide les types avant le calcul de la taille des types. */
    TENTE(valide_types_pour_calcule_taille_type(m_espace, type_compose));

    if (constructrice.donne_compte_membres_non_constant() == 0) {
        assert(decl->est_externe);
    }
    else {
        calcule_taille_type_composé(type_compose, decl->est_compacte, decl->alignement_desire);
    }

    auto type_struct = decl;

#undef AVERTIS_SUR_REMBOURRAGE_SUPERFLUX
#undef AVERTIS_SUR_FRANCHISSEMENT_LIGNE_DE_CACHE

#ifdef AVERTIS_SUR_REMBOURRAGE_SUPERFLUX
    auto rembourrage_total = 0u;
#endif
    POUR (type_struct->membres) {
        if (it.possède_drapeau(MembreTypeComposé::EST_UN_EMPLOI)) {
            type_struct->types_employés.ajoute(&it);
        }

#ifdef AVERTIS_SUR_FRANCHISSEMENT_LIGNE_DE_CACHE
        auto reste_décalage = it.decalage % 64;
        if (reste_décalage && (reste_décalage + it.type->taille_octet) > 64) {
            espace->rapporte_avertissement(it.decl, "Le membre franchis une ligne de cache");
        }
#endif

#ifdef AVERTIS_SUR_REMBOURRAGE_SUPERFLUX
        /* Revise cette logique ? */
        if (it.rembourrage && it.type->taille_octet < rembourrage_total) {
            espace->rapporte_avertissement(
                it.decl, "Le membre pourrait être stocké dans du rembourrage existant");
        }

        rembourrage_total += it.rembourrage;
#endif
    }

    decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

    simplifie_arbre(m_unité->espace, m_tacheronne->assembleuse, m_compilatrice.typeuse, decl);
    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_union(NoeudUnion *decl)
{
    if (!decl->type) {
        decl->type = m_compilatrice.typeuse.type_type_de_donnees(decl);
    }

    if (decl->est_externe && decl->bloc == nullptr) {
        decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
        /* INITIALISATION_TYPE_FUT_CREEE est à cause de attente_sur_type_si_drapeau_manquant */
        decl->drapeaux_type |= (DrapeauxTypes::TYPE_NE_REQUIERS_PAS_D_INITIALISATION |
                                DrapeauxTypes::INITIALISATION_TYPE_FUT_CREEE);
        return CodeRetourValidation::OK;
    }

    if (decl->est_polymorphe) {
        TENTE(valide_arbre_aplatis(decl));

        if (!decl->monomorphisations) {
            decl->monomorphisations =
                m_tacheronne->allocatrice_noeud.crée_monomorphisations_union();
        }

        // nous validerons les membres lors de la monomorphisation
        decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
        decl->drapeaux_type |= DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
        return CodeRetourValidation::OK;
    }

    if (decl->est_corps_texte) {
        /* Nous devons avoir deux passes : une pour créer la fonction du métaprogramme, une autre
         * pour requérir la compilation dudit métaprogramme. */
        if (!decl->metaprogramme_corps_texte) {
            auto metaprogramme = crée_métaprogramme_corps_texte(
                decl->bloc, decl->bloc_parent, decl->lexeme);
            auto fonction = metaprogramme->fonction;
            assert(fonction->corps->bloc);

            decl->metaprogramme_corps_texte = metaprogramme;
            metaprogramme->corps_texte_pour_structure = decl;

            if (decl->est_monomorphisation) {
                decl->bloc_constantes->membres.avec_verrou_ecriture(
                    [fonction](kuri::tableau<NoeudDeclaration *, int> &membres) {
                        POUR (membres) {
                            fonction->bloc_constantes->ajoute_membre(it);
                        }
                    });
            }

            m_compilatrice.gestionnaire_code->requiers_typage(m_espace, fonction->corps);
            return Attente::sur_declaration(fonction->corps);
        }

        auto metaprogramme = decl->metaprogramme_corps_texte;
        auto fichier = m_compilatrice.crée_fichier_pour_metaprogramme(metaprogramme);
        m_compilatrice.gestionnaire_code->requiers_compilation_metaprogramme(m_espace,
                                                                             metaprogramme);
        return Attente::sur_parsage(fichier);
    }

    TENTE(valide_arbre_aplatis(decl));

    CHRONO_TYPAGE(m_stats_typage.structures, STRUCTURE__VALIDATION);

    if (!decl->est_monomorphisation) {
        auto decl_precedente = trouve_dans_bloc(
            decl->bloc_parent, decl, decl->bloc_parent->bloc_parent);

        // la bibliothèque C a des symboles qui peuvent être les mêmes pour les fonctions et les
        // structres (p.e. stat)
        if (decl_precedente != nullptr && decl_precedente->genre == decl->genre) {
            rapporte_erreur_redéfinition_symbole(decl, decl_precedente);
            return CodeRetourValidation::Erreur;
        }
    }

    auto type_compose = decl->comme_type_compose();
    auto constructrice = ConstructriceMembresTypeComposé(*type_compose, decl->bloc);

    auto type_union = decl;
    type_union->est_nonsure = decl->est_nonsure;

    POUR (*decl->bloc->membres.verrou_ecriture()) {
        if (it->est_declaration_type()) {
            constructrice.ajoute_type_de_données(it->comme_declaration_type(),
                                                 m_compilatrice.typeuse);
            continue;
        }

        if (it->est_declaration_constante()) {
            constructrice.ajoute_constante(it->comme_declaration_constante());
            continue;
        }

        if (it->est_declaration_variable()) {
            auto decl_var = it->comme_declaration_variable();

            // À FAIRE(emploi) : préserve l'emploi dans les données types
            if (decl_var->declaration_vient_d_un_emploi) {
                if (le_membre_référence_le_type_par_valeur(type_compose, decl_var)) {
                    rapporte_erreur_inclusion_récursive_type(m_espace, type_compose, decl_var);
                    return CodeRetourValidation::Erreur;
                }

                constructrice.ajoute_membre_provenant_d_un_emploi(decl_var);
                continue;
            }

            if (decl_var->type->est_type_rien() && decl->est_nonsure) {
                rapporte_erreur("Ne peut avoir un type « rien » dans une union nonsûre",
                                decl_var,
                                erreur::Genre::TYPE_DIFFERENTS);
                return CodeRetourValidation::Erreur;
            }

            if (decl_var->type->est_type_variadique()) {
                rapporte_erreur_type_membre_invalide(m_espace, type_compose, decl_var);
                return CodeRetourValidation::Erreur;
            }

            if (le_membre_référence_le_type_par_valeur(type_compose, decl_var)) {
                rapporte_erreur_inclusion_récursive_type(m_espace, type_compose, decl_var);
                return CodeRetourValidation::Erreur;
            }

            constructrice.ajoute_membre_simple(decl_var, decl_var->expression);
            continue;
        }

        if (!it->est_declaration_variable_multiple()) {
            rapporte_erreur("Déclaration inattendu dans le bloc de la structure", it);
            return CodeRetourValidation::Erreur;
        }

        auto decl_var = it->comme_declaration_variable_multiple();

        for (auto &donnees : decl_var->donnees_decl.plage()) {
            for (auto i = 0; i < donnees.variables.taille(); ++i) {
                auto var = donnees.variables[i];

                if (var->type->est_type_rien() && decl->est_nonsure) {
                    rapporte_erreur("Ne peut avoir un type « rien » dans une union nonsûre",
                                    decl_var,
                                    erreur::Genre::TYPE_DIFFERENTS);
                    return CodeRetourValidation::Erreur;
                }

                if (var->type->est_type_variadique()) {
                    rapporte_erreur_type_membre_invalide(m_espace, type_compose, var);
                    return CodeRetourValidation::Erreur;
                }

                if (var->genre != GenreNoeud::EXPRESSION_REFERENCE_DECLARATION) {
                    rapporte_erreur("Expression invalide dans la déclaration du membre de l'union",
                                    var);
                    return CodeRetourValidation::Erreur;
                }

                if (le_membre_référence_le_type_par_valeur(type_union, var)) {
                    rapporte_erreur_inclusion_récursive_type(m_espace, type_compose, var);
                    return CodeRetourValidation::Erreur;
                }

                /* l'arbre syntaxique des expressions par défaut doivent contenir
                 * la transformation puisque nous n'utilisons pas la déclaration
                 * pour générer la RI */
                auto expression = donnees.expression;
                crée_transtypage_implicite_au_besoin(expression, donnees.transformations[i]);

                constructrice.ajoute_membre_simple(var, expression);
            }
        }
    }

    constructrice.finalise();

    /* Valide les types avant le calcul de la taille des types. */
    TENTE(valide_types_pour_calcule_taille_type(m_espace, type_compose));

    calcule_taille_type_composé(type_union, false, 0);

    if (!decl->est_nonsure) {
        crée_type_structure(m_compilatrice.typeuse, type_union, type_union->decalage_index);
    }

    decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

    return CodeRetourValidation::OK;
}

/** \} */

RésultatValidation Sémanticienne::valide_déclaration_variable(NoeudDeclarationVariable *decl)
{
    auto bloc_final = NoeudBloc::nul();
    if (decl->possède_drapeau(DrapeauxNoeud::EST_PARAMETRE) ||
        decl->possède_drapeau(DrapeauxNoeud::EST_MEMBRE_STRUCTURE)) {
        bloc_final = decl->bloc_parent->bloc_parent;
    }

    {
        CHRONO_TYPAGE(m_stats_typage.validation_decl, DECLARATION_VARIABLES__REDEFINITION);
        if (decl->ident && decl->ident != ID::_) {
            auto decl_prec = trouve_dans_bloc(
                decl->bloc_parent, decl, bloc_final, fonction_courante());

            if (decl_prec != nullptr && decl_prec->genre == decl->genre) {
                if (decl->lexeme->ligne > decl_prec->lexeme->ligne) {
                    rapporte_erreur_redéfinition_symbole(decl, decl_prec);
                    return CodeRetourValidation::Erreur;
                }
            }
        }
    }

    {
        CHRONO_TYPAGE(m_stats_typage.validation_decl, DECLARATION_VARIABLES__RESOLUTION_TYPE);
        if (résoud_type_final(decl->expression_type, decl->type) == CodeRetourValidation::Erreur) {
            return CodeRetourValidation::Erreur;
        }
    }

    auto expression = decl->expression;
    if (expression) {
        if (expression->est_non_initialisation() && !decl->expression_type) {
            m_espace->rapporte_erreur(expression,
                                      "Utilisation d'une non-initialisation alors que la variable "
                                      "est déclarée sans type.");
            return CodeRetourValidation::Erreur;
        }

        if (!expression->est_non_initialisation()) {
            auto type_de_l_expression = expression->type;

            if (!type_de_l_expression) {
                m_espace->rapporte_erreur(expression, "Expression sans type.");
                return CodeRetourValidation::Erreur;
            }

            if (type_de_l_expression->est_type_tuple() || expression->est_virgule()) {
                m_espace->rapporte_erreur(
                    expression,
                    "Trop de valeurs pour initialiser une déclaration de variable singulière.");
                return CodeRetourValidation::Erreur;
            }

            if (decl->type == nullptr) {
                if (type_de_l_expression->est_type_entier_constant()) {
                    decl->type = TypeBase::Z32;
                    crée_transtypage_implicite_au_besoin(
                        decl->expression,
                        {TypeTransformation::CONVERTI_ENTIER_CONSTANT, decl->type});
                }
                else if (type_de_l_expression->est_type_reference()) {
                    decl->type = type_de_l_expression->comme_type_reference()->type_pointe;
                    crée_transtypage_implicite_au_besoin(
                        decl->expression, TransformationType(TypeTransformation::DEREFERENCE));
                }
                else {
                    decl->type = type_de_l_expression;
                }
            }
            else {
                auto résultat = cherche_transformation(type_de_l_expression, decl->type);

                if (std::holds_alternative<Attente>(résultat)) {
                    return std::get<Attente>(résultat);
                }

                auto transformation = std::get<TransformationType>(résultat);
                if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                    rapporte_erreur_assignation_type_différents(
                        decl->type, type_de_l_expression, expression);
                    return CodeRetourValidation::Erreur;
                }

                crée_transtypage_implicite_au_besoin(decl->expression, transformation);
            }
        }
    }

    {
        CHRONO_TYPAGE(m_stats_typage.validation_decl, DECLARATION_VARIABLES__VALIDATION_FINALE);

        if (!decl->possède_drapeau(DrapeauxNoeud::EST_GLOBALE)) {
            /* Les globales et les valeurs polymorphiques sont ajoutées au bloc parent par la
             * syntaxeuse. */
            if (!decl->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
                auto bloc_parent = decl->bloc_parent;
                bloc_parent->ajoute_membre(decl);
            }
        }

        decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
        decl->genre_valeur = GenreValeur::TRANSCENDANTALE;
    }

    if (!fonction_courante()) {
        simplifie_arbre(m_unité->espace, m_tacheronne->assembleuse, m_compilatrice.typeuse, decl);

        TENTE(valide_symbole_externe(decl, TypeSymbole::VARIABLE_GLOBALE))

        /* Pour la génération de RI pour les globales, nous devons attendre que le type fut validé.
         */
        if (!decl->type->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
            /* Ne revalide pas ce noeud. */
            m_arbre_courant->index_courant += 1;
            return Attente::sur_type(decl->type);
        }
    }

    decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    decl->genre_valeur = GenreValeur::TRANSCENDANTALE;
    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_déclaration_variable_multiple(
    NoeudDeclarationVariableMultiple *decl)
{
    auto &ctx = m_contexte_validation_déclaration;
    ctx.variables.efface();
    ctx.données_temp.efface();
    ctx.decls_et_refs.efface();
    ctx.feuilles_variables.efface();
    ctx.feuilles_expressions.efface();
    ctx.données_assignations.efface();

    auto &feuilles_variables = ctx.feuilles_variables;
    {
        CHRONO_TYPAGE(m_stats_typage.validation_decl, DECLARATION_VARIABLES__RASSEMBLE_VARIABLES);
        rassemble_expressions(decl->valeur, feuilles_variables);
    }

    /* Rassemble les variables, et crée des déclarations si nécessaire. */
    auto &decls_et_refs = ctx.decls_et_refs;
    decls_et_refs.redimensionne(feuilles_variables.taille());
    {
        CHRONO_TYPAGE(m_stats_typage.validation_decl, DECLARATION_VARIABLES__PREPARATION);

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

    POUR (decls_et_refs) {
        auto bloc_final = NoeudBloc::nul();
        if (it.decl->possède_drapeau(DrapeauxNoeud::EST_PARAMETRE) ||
            it.decl->possède_drapeau(DrapeauxNoeud::EST_MEMBRE_STRUCTURE)) {
            bloc_final = it.decl->bloc_parent->bloc_parent;
        }

        {
            CHRONO_TYPAGE(m_stats_typage.validation_decl, DECLARATION_VARIABLES__REDEFINITION);
            if (it.decl->ident && it.decl->ident != ID::_) {
                auto decl_prec = trouve_dans_bloc(
                    it.decl->bloc_parent, it.decl, bloc_final, fonction_courante());

                if (decl_prec != nullptr && decl_prec->genre == decl->genre) {
                    if (decl->lexeme->ligne > decl_prec->lexeme->ligne) {
                        rapporte_erreur_redéfinition_symbole(it.ref_decl, decl_prec);
                        return CodeRetourValidation::Erreur;
                    }
                }
            }
        }

        CHRONO_TYPAGE(m_stats_typage.validation_decl, DECLARATION_VARIABLES__RESOLUTION_TYPE);
        if (résoud_type_final(it.decl->expression_type, it.ref_decl->type) ==
            CodeRetourValidation::Erreur) {
            return CodeRetourValidation::Erreur;
        }
    }

    auto &feuilles_expressions = ctx.feuilles_expressions;
    {
        CHRONO_TYPAGE(m_stats_typage.validation_decl,
                      DECLARATION_VARIABLES__RASSEMBLE_EXPRESSIONS);
        rassemble_expressions(decl->expression, feuilles_expressions);
    }

    // pour chaque expression, associe les variables qui doivent recevoir leurs résultats
    // si une variable n'a pas de valeur, prend la valeur de la dernière expression

    auto &variables = ctx.variables;
    {
        CHRONO_TYPAGE(m_stats_typage.validation_decl, DECLARATION_VARIABLES__ENFILE_VARIABLES);

        POUR (feuilles_variables) {
            variables.enfile(it);
        }
    }

    auto &donnees_assignations = ctx.données_assignations;

    auto ajoute_variable = [this](DonneesAssignations &donnees,
                                  NoeudExpression *variable,
                                  NoeudExpression *expression,
                                  Type *type_de_l_expression) -> RésultatValidation {
        if (variable->type == nullptr) {
            if (type_de_l_expression->est_type_entier_constant()) {
                variable->type = TypeBase::Z32;
                donnees.variables.ajoute(variable);
                donnees.transformations.ajoute(
                    {TypeTransformation::CONVERTI_ENTIER_CONSTANT, variable->type});
            }
            else {
                if (type_de_l_expression->est_type_reference()) {
                    variable->type = type_de_l_expression->comme_type_reference()->type_pointe;
                    donnees.variables.ajoute(variable);
                    donnees.transformations.ajoute(
                        TransformationType(TypeTransformation::DEREFERENCE));
                }
                else {
                    variable->type = type_de_l_expression;
                    donnees.variables.ajoute(variable);
                    donnees.transformations.ajoute(
                        TransformationType{TypeTransformation::INUTILE});
                }
            }
        }
        else {
            auto résultat = cherche_transformation(type_de_l_expression, variable->type);

            if (std::holds_alternative<Attente>(résultat)) {
                return std::get<Attente>(résultat);
            }

            auto transformation = std::get<TransformationType>(résultat);
            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_assignation_type_différents(
                    variable->type, type_de_l_expression, expression);
                return CodeRetourValidation::Erreur;
            }

            donnees.variables.ajoute(variable);
            donnees.transformations.ajoute(transformation);
        }

        return CodeRetourValidation::OK;
    };

    {
        CHRONO_TYPAGE(m_stats_typage.validation_decl,
                      DECLARATION_VARIABLES__ASSIGNATION_EXPRESSIONS);

        POUR (feuilles_expressions) {
            auto &donnees = ctx.données_temp;
            donnees.expression = it;

            // il est possible d'ignorer les variables
            if (variables.est_vide()) {
                m_espace->rapporte_erreur(decl,
                                          "Trop d'expressions ou de types pour l'assignation");
                return CodeRetourValidation::Erreur;
            }

            if (decl->possède_drapeau(DrapeauxNoeud::EST_EXTERNE)) {
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
                donnees.transformations.ajoute(TransformationType{TypeTransformation::INUTILE});
            }
            else if (it->type->est_type_tuple()) {
                auto type_tuple = it->type->comme_type_tuple();

                donnees.multiple_retour = true;

                for (auto &membre : type_tuple->membres) {
                    if (variables.est_vide()) {
                        break;
                    }

                    TENTE(ajoute_variable(donnees, variables.defile(), it, membre.type));
                }
            }
            else if (it->type->est_type_rien()) {
                rapporte_erreur(
                    "impossible d'assigner une expression de type « rien » à une variable",
                    it,
                    erreur::Genre::ASSIGNATION_RIEN);
                return CodeRetourValidation::Erreur;
            }
            else {
                TENTE(ajoute_variable(donnees, variables.defile(), it, it->type));
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

                if (var->type->est_type_entier_constant()) {
                    var->type = TypeBase::Z32;
                    transformation = {TypeTransformation::CONVERTI_ENTIER_CONSTANT, var->type};
                }
            }

            donnees->variables.ajoute(var);
            donnees->transformations.ajoute(transformation);
        }
    }

    {
        CHRONO_TYPAGE(m_stats_typage.validation_decl, DECLARATION_VARIABLES__VALIDATION_FINALE);

        POUR (decls_et_refs) {
            auto decl_var = it.decl;
            auto variable = it.ref_decl;

            if (variable->type == nullptr) {
                rapporte_erreur("variable déclarée sans type", variable);
                return CodeRetourValidation::Erreur;
            }

            decl_var->type = variable->type;

            if (!decl_var->possède_drapeau(DrapeauxNoeud::EST_GLOBALE)) {
                /* Les globales et les valeurs polymorphiques sont ajoutées au bloc parent par la
                 * syntaxeuse. */
                if (!decl_var->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
                    auto bloc_parent = decl_var->bloc_parent;
                    bloc_parent->ajoute_membre(decl_var);
                }
            }

            decl_var->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
            decl_var->genre_valeur = GenreValeur::TRANSCENDANTALE;
            variable->genre_valeur = GenreValeur::TRANSCENDANTALE;
        }

        decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    }

    {
        CHRONO_TYPAGE(m_stats_typage.validation_decl, DECLARATION_VARIABLES__COPIE_DONNEES);

        decl->donnees_decl.réserve(static_cast<int>(donnees_assignations.taille()));

        POUR (donnees_assignations) {
            decl->donnees_decl.ajoute(std::move(it));
        }
    }

    if (!fonction_courante()) {
        simplifie_arbre(m_unité->espace, m_tacheronne->assembleuse, m_compilatrice.typeuse, decl);

        POUR (decls_et_refs) {
            TENTE(valide_symbole_externe(it.decl, TypeSymbole::VARIABLE_GLOBALE))
            /* Pour la génération de RI pour les globales, nous devons attendre que le type fut
             * validé.
             */
            if (!it.decl->type->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                /* Ne revalide pas ce noeud. */
                m_arbre_courant->index_courant += 1;
                return Attente::sur_type(it.decl->type);
            }
        }
    }

    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_déclaration_constante(NoeudDeclarationConstante *decl)
{
    if (résoud_type_final(decl->expression_type, decl->type) == CodeRetourValidation::Erreur) {
        return CodeRetourValidation::Erreur;
    }

    auto expression = decl->expression;
    if (!expression) {
        if (decl->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
            decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
            return CodeRetourValidation::OK;
        }
        if (decl->possède_drapeau(DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE)) {
            decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
            return CodeRetourValidation::OK;
        }
        rapporte_erreur("Impossible de ne pas initialiser une constante.", decl);
        return CodeRetourValidation::Erreur;
    }

    /* Utilise la subsitution si existante (p.e. pour #exécute). */
    if (expression->substitution) {
        expression = expression->substitution;
    }

    if (expression->est_non_initialisation()) {
        rapporte_erreur("Impossible de ne pas initialiser une constante.", expression);
        return CodeRetourValidation::Erreur;
    }

    if (expression->est_virgule()) {
        rapporte_erreur("Trop de valeurs pour l'initialisation de la constante.", expression);
        return CodeRetourValidation::Erreur;
    }

    if (expression->type->est_type_tuple()) {
        rapporte_erreur("Ne peut initialisation une constante depuis un tuple.", expression);
        return CodeRetourValidation::Erreur;
    }

    if (decl->type) {
        auto résultat = cherche_transformation(expression->type, decl->type);

        if (std::holds_alternative<Attente>(résultat)) {
            return std::get<Attente>(résultat);
        }

        auto transformation = std::get<TransformationType>(résultat);
        if (transformation.type == TypeTransformation::IMPOSSIBLE) {
            rapporte_erreur_assignation_type_différents(decl->type, expression->type, expression);
            return CodeRetourValidation::Erreur;
        }

        crée_transtypage_implicite_au_besoin(decl->expression, transformation);
    }
    else {
        if (expression->type->est_type_entier_constant()) {
            decl->type = TypeBase::Z32;
        }
        else {
            decl->type = expression->type;
        }
    }

    if (!expression->type->est_type_type_de_donnees()) {
        if (!peut_etre_type_constante(expression->type)) {
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

    decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

    if (!decl->possède_drapeau(DrapeauxNoeud::EST_GLOBALE)) {
        decl->bloc_parent->ajoute_membre(decl);
    }

    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_assignation(NoeudAssignation *inst)
{
    CHRONO_TYPAGE(m_stats_typage.assignations, ASSIGNATION__VALIDATION);
    auto variable = inst->assignée;

    if (!est_valeur_gauche(variable->genre_valeur)) {
        rapporte_erreur("Impossible d'assigner une expression à une valeur-droite !",
                        inst,
                        erreur::Genre::ASSIGNATION_INVALIDE);
        return CodeRetourValidation::Erreur;
    }

    auto expression = inst->expression;
    if (expression->est_virgule() || expression->type->est_type_tuple()) {
        m_espace->rapporte_erreur(expression, "Trop de valeurs pour l'assignation à la variable.");
        return CodeRetourValidation::Erreur;
    }

    if (expression->est_non_initialisation()) {
        rapporte_erreur("Impossible d'utiliser '---' dans une expression d'assignation",
                        inst->expression);
        return CodeRetourValidation::Erreur;
    }

    if (expression->type->est_type_rien()) {
        rapporte_erreur("Impossible d'assigner une expression de type 'rien' à une variable !",
                        inst,
                        erreur::Genre::ASSIGNATION_RIEN);
        return CodeRetourValidation::Erreur;
    }

    auto type_de_la_variable = variable->type;
    auto var_est_reference = type_de_la_variable->est_type_reference();
    auto type_de_l_expression = expression->type;
    auto expr_est_reference = type_de_l_expression->est_type_reference();

    auto transformation = TransformationType();

    if (variable->possède_drapeau(DrapeauxNoeud::ACCES_EST_ENUM_DRAPEAU)) {
        if (!expression->type->est_type_bool()) {
            m_espace
                ->rapporte_erreur(expression,
                                  "L'assignation d'une valeur d'une énum_drapeau doit être "
                                  "une valeur booléenne")
                .ajoute_message(
                    "Le type de l'expression est ", chaine_type(expression->type), "\n");
            return CodeRetourValidation::Erreur;
        }

        return CodeRetourValidation::OK;
    }

    if (var_est_reference && expr_est_reference) {
        // déréférence les deux côtés
        auto résultat = cherche_transformation(type_de_l_expression, type_de_la_variable);

        if (std::holds_alternative<Attente>(résultat)) {
            return std::get<Attente>(résultat);
        }

        transformation = std::get<TransformationType>(résultat);
        if (transformation.type == TypeTransformation::IMPOSSIBLE) {
            rapporte_erreur_assignation_type_différents(
                type_de_la_variable, type_de_l_expression, expression);
            return CodeRetourValidation::Erreur;
        }

        crée_transtypage_implicite_au_besoin(inst->assignée,
                                             TransformationType(TypeTransformation::DEREFERENCE));
        transformation = TransformationType(TypeTransformation::DEREFERENCE);
    }
    else if (var_est_reference) {
        // déréférence var
        type_de_la_variable = type_de_la_variable->comme_type_reference()->type_pointe;

        auto résultat = cherche_transformation(type_de_l_expression, type_de_la_variable);

        if (std::holds_alternative<Attente>(résultat)) {
            return std::get<Attente>(résultat);
        }

        transformation = std::get<TransformationType>(résultat);
        if (transformation.type == TypeTransformation::IMPOSSIBLE) {
            rapporte_erreur_assignation_type_différents(
                type_de_la_variable, type_de_l_expression, expression);
            return CodeRetourValidation::Erreur;
        }

        crée_transtypage_implicite_au_besoin(inst->assignée,
                                             TransformationType(TypeTransformation::DEREFERENCE));
    }
    else if (expr_est_reference) {
        // déréférence expr
        auto résultat = cherche_transformation(type_de_l_expression, type_de_la_variable);

        if (std::holds_alternative<Attente>(résultat)) {
            return std::get<Attente>(résultat);
        }

        transformation = std::get<TransformationType>(résultat);
        if (transformation.type == TypeTransformation::IMPOSSIBLE) {
            rapporte_erreur_assignation_type_différents(
                type_de_la_variable, type_de_l_expression, expression);
            return CodeRetourValidation::Erreur;
        }
    }
    else {
        auto résultat = cherche_transformation(type_de_l_expression, type_de_la_variable);

        if (std::holds_alternative<Attente>(résultat)) {
            return std::get<Attente>(résultat);
        }

        transformation = std::get<TransformationType>(résultat);
        if (transformation.type == TypeTransformation::IMPOSSIBLE) {
            rapporte_erreur_assignation_type_différents(
                type_de_la_variable, type_de_l_expression, expression);
            return CodeRetourValidation::Erreur;
        }
    }

    if (transformation.type != TypeTransformation::INUTILE) {
        crée_transtypage_implicite_au_besoin(inst->expression, transformation);
    }

    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_assignation_multiple(NoeudAssignationMultiple *inst)
{
    CHRONO_TYPAGE(m_stats_typage.assignations, ASSIGNATION__VALIDATION);
    auto variable = inst->assignées;

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
                                  Type *type_de_l_expression) -> RésultatValidation {
        auto type_de_la_variable = var->type;
        auto var_est_reference = type_de_la_variable->est_type_reference();
        auto expr_est_reference = type_de_l_expression->est_type_reference();

        auto transformation = TransformationType();

        if (var->possède_drapeau(DrapeauxNoeud::ACCES_EST_ENUM_DRAPEAU)) {
            if (!expression->type->est_type_bool()) {
                m_espace
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
            auto résultat = cherche_transformation(type_de_l_expression, var->type);

            if (std::holds_alternative<Attente>(résultat)) {
                return std::get<Attente>(résultat);
            }

            transformation = std::get<TransformationType>(résultat);
            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_assignation_type_différents(
                    var->type, type_de_l_expression, expression);
                return CodeRetourValidation::Erreur;
            }

            crée_transtypage_implicite_au_besoin(
                var, TransformationType(TypeTransformation::DEREFERENCE));
            transformation = TransformationType(TypeTransformation::DEREFERENCE);
        }
        else if (var_est_reference) {
            // déréférence var
            type_de_la_variable = type_de_la_variable->comme_type_reference()->type_pointe;

            auto résultat = cherche_transformation(type_de_l_expression, type_de_la_variable);

            if (std::holds_alternative<Attente>(résultat)) {
                return std::get<Attente>(résultat);
            }

            transformation = std::get<TransformationType>(résultat);
            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_assignation_type_différents(
                    var->type, type_de_l_expression, expression);
                return CodeRetourValidation::Erreur;
            }

            crée_transtypage_implicite_au_besoin(
                var, TransformationType(TypeTransformation::DEREFERENCE));
        }
        else if (expr_est_reference) {
            // déréférence expr
            auto résultat = cherche_transformation(type_de_l_expression, var->type);

            if (std::holds_alternative<Attente>(résultat)) {
                return std::get<Attente>(résultat);
            }

            transformation = std::get<TransformationType>(résultat);
            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_assignation_type_différents(
                    var->type, type_de_l_expression, expression);
                return CodeRetourValidation::Erreur;
            }
        }
        else {
            auto résultat = cherche_transformation(type_de_l_expression, var->type);

            if (std::holds_alternative<Attente>(résultat)) {
                return std::get<Attente>(résultat);
            }

            transformation = std::get<TransformationType>(résultat);
            if (transformation.type == TypeTransformation::IMPOSSIBLE) {
                rapporte_erreur_assignation_type_différents(
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

        if (it->type->est_type_rien()) {
            rapporte_erreur("Impossible d'assigner une expression de type 'rien' à une variable !",
                            inst,
                            erreur::Genre::ASSIGNATION_RIEN);
            return CodeRetourValidation::Erreur;
        }

        auto donnees = DonneesAssignations();
        donnees.expression = it;

        if (it->type->est_type_tuple()) {
            auto type_tuple = it->type->comme_type_tuple();

            donnees.multiple_retour = true;

            for (auto &membre : type_tuple->membres) {
                if (variables.est_vide()) {
                    break;
                }

                TENTE(ajoute_variable(donnees, variables.defile(), it, membre.type));
            }
        }
        else {
            TENTE(ajoute_variable(donnees, variables.defile(), it, it->type));
        }

        donnees_assignations.ajoute(std::move(donnees));
    }

    // a, b = c
    auto donnees = &donnees_assignations.back();
    while (!variables.est_vide()) {
        TENTE(ajoute_variable(
            *donnees, variables.defile(), donnees->expression, donnees->expression->type));
    }

    inst->données_exprs.réserve(static_cast<int>(donnees_assignations.taille()));
    POUR (donnees_assignations) {
        inst->données_exprs.ajoute(std::move(it));
    }

    return CodeRetourValidation::OK;
}

template <typename TypeControleBoucle>
CodeRetourValidation Sémanticienne::valide_controle_boucle(TypeControleBoucle *inst)
{
    auto chaine_var = inst->expression == nullptr ? nullptr : inst->expression->ident;
    auto boucle = bloc_est_dans_boucle(inst->bloc_parent, chaine_var);

    if (!boucle) {
        if (!chaine_var) {
            m_espace->rapporte_erreur(
                inst, "« continue » en dehors d'une boucle", erreur::Genre::CONTROLE_INVALIDE);
            return CodeRetourValidation::Erreur;
        }

        m_espace->rapporte_erreur(inst->expression,
                                  "La variable ne réfère à aucune boucle",
                                  erreur::Genre::VARIABLE_INCONNUE);
        return CodeRetourValidation::Erreur;
    }

    inst->boucle_controlee = boucle;
    return CodeRetourValidation::OK;
}

/* ************************************************************************** */

CodeRetourValidation Sémanticienne::résoud_type_final(NoeudExpression *expression_type,
                                                      Type *&type_final)
{
    if (expression_type == nullptr) {
        type_final = nullptr;
        return CodeRetourValidation::OK;
    }

    auto type_var = expression_type->type;

    if (type_var == nullptr) {
        m_espace->rapporte_erreur(expression_type,
                                  "Erreur interne, le type de l'expression est nul !");
        return CodeRetourValidation::Erreur;
    }

    if (!type_var->est_type_type_de_donnees()) {
        m_espace
            ->rapporte_erreur(expression_type,
                              "L'expression du type n'est pas un type de données.")
            .ajoute_message("L'expression est de type : ", chaine_type(type_var), "\n");
        return CodeRetourValidation::Erreur;
    }

    auto type_de_donnees = type_var->comme_type_type_de_donnees();

    if (type_de_donnees->type_connu == nullptr) {
        rapporte_erreur("impossible de définir le type selon l'expression", expression_type);
        return CodeRetourValidation::Erreur;
    }

    type_final = type_de_donnees->type_connu;
    return CodeRetourValidation::OK;
}

void Sémanticienne::rapporte_erreur(const char *message, const NoeudExpression *noeud)
{
    erreur::lance_erreur(message, *m_espace, noeud);
}

void Sémanticienne::rapporte_erreur(const char *message,
                                    const NoeudExpression *noeud,
                                    erreur::Genre genre)
{
    erreur::lance_erreur(message, *m_espace, noeud, genre);
}

void Sémanticienne::rapporte_erreur_redéfinition_symbole(NoeudExpression *decl,
                                                         NoeudDeclaration *decl_prec)
{
    erreur::redefinition_symbole(*m_espace, decl, decl_prec);
}

void Sémanticienne::rapporte_erreur_redéfinition_fonction(NoeudDeclarationEnteteFonction *decl,
                                                          NoeudDeclaration *decl_prec)
{
    erreur::redefinition_fonction(*m_espace, decl_prec, decl);
}

void Sémanticienne::rapporte_erreur_type_arguments(NoeudExpression *type_arg,
                                                   NoeudExpression *type_enf)
{
    erreur::lance_erreur_transtypage_impossible(
        type_arg->type, type_enf->type, *m_espace, type_enf, type_arg);
}

void Sémanticienne::rapporte_erreur_assignation_type_différents(const Type *type_gauche,
                                                                const Type *type_droite,
                                                                NoeudExpression *noeud)
{
    erreur::lance_erreur_assignation_type_differents(type_gauche, type_droite, *m_espace, noeud);
}

void Sémanticienne::rapporte_erreur_type_opération(const Type *type_gauche,
                                                   const Type *type_droite,
                                                   NoeudExpression *noeud)
{
    erreur::lance_erreur_type_operation(type_gauche, type_droite, *m_espace, noeud);
}

void Sémanticienne::rapporte_erreur_accès_hors_limites(NoeudExpression *b,
                                                       TypeTableauFixe *type_tableau,
                                                       int64_t index_acces)
{
    erreur::lance_erreur_acces_hors_limites(
        *m_espace, b, type_tableau->taille, type_tableau, index_acces);
}

void Sémanticienne::rapporte_erreur_membre_inconnu(NoeudExpression *acces,
                                                   NoeudExpression *membre,
                                                   TypeCompose *type)
{
    erreur::membre_inconnu(*m_espace, acces, membre, type);
}

void Sémanticienne::rapporte_erreur_valeur_manquante_discr(
    NoeudExpression *expression, kuri::ensemble<kuri::chaine_statique> const &valeurs_manquantes)
{
    erreur::valeur_manquante_discr(*m_espace, expression, valeurs_manquantes);
}

void Sémanticienne::rapporte_erreur_fonction_nulctx(const NoeudExpression *appl_fonc,
                                                    const NoeudExpression *decl_fonc,
                                                    const NoeudExpression *decl_appel)
{
    erreur::lance_erreur_fonction_nulctx(*m_espace, appl_fonc, decl_fonc, decl_appel);
}

RésultatValidation Sémanticienne::crée_transtypage_implicite_si_possible(
    NoeudExpression *&expression, Type *type_cible, const RaisonTranstypageImplicite raison)
{
    auto résultat = cherche_transformation(expression->type, type_cible);

    if (std::holds_alternative<Attente>(résultat)) {
        return std::get<Attente>(résultat);
    }

    auto transformation = std::get<TransformationType>(résultat);
    if (transformation.type == TypeTransformation::IMPOSSIBLE) {
        kuri::chaine_statique message_principal;
        kuri::chaine_statique message_type_désiré;
        kuri::chaine_statique message_type_obtenu;

        switch (raison) {
            case RaisonTranstypageImplicite::POUR_TEST_DISCRIMINATION:
            {
                message_principal =
                    "Type incompatible pour l'expression de test de discrimination.";
                message_type_désiré = "Le type de l'expression discriminée est : ";
                message_type_obtenu = "Le type de l'expression test est        : ";
                break;
            }
            case RaisonTranstypageImplicite::POUR_EXPRESSION_INDEXAGE:
            {
                message_principal =
                    "Type incompatible pour la valeur d'index de l'expression d'indexage.";
                message_type_désiré = "Le type désiré est          : ";
                message_type_obtenu = "Le type de l'expression est : ";
                break;
            }
            case RaisonTranstypageImplicite::POUR_CONSTRUCTION_TABLEAU:
            {
                message_principal =
                    "Type incompatible pour la valeur utilisée dans la construction de tableau.";
                message_type_désiré = "Le type de valeur des éléments du tableau est : ";
                message_type_obtenu = "Le type de l'expression est                   : ";
                break;
            }
        }

        m_espace
            ->rapporte_erreur(
                expression, message_principal, erreur::Genre::ASSIGNATION_MAUVAIS_TYPE)
            .ajoute_message(message_type_désiré, chaine_type(type_cible), "\n")
            .ajoute_message(message_type_obtenu, chaine_type(expression->type), "\n");
        return CodeRetourValidation::Erreur;
    }

    crée_transtypage_implicite_au_besoin(expression, transformation);
    return CodeRetourValidation::OK;
}

void Sémanticienne::crée_transtypage_implicite_au_besoin(NoeudExpression *&expression,
                                                         TransformationType const &transformation)
{
    if (transformation.type == TypeTransformation::INUTILE) {
        return;
    }

    if (transformation.type == TypeTransformation::CONVERTI_ENTIER_CONSTANT) {
        expression->type = const_cast<Type *>(transformation.type_cible);
        /* Assigne récusirvement le type à tous les entiers constants.
         * Nous pourrions avoir une expression complexe (parenthèse + opérateurs, etc.). */
        visite_noeud(
            expression, PreferenceVisiteNoeud::ORIGINAL, true, [&](NoeudExpression const *noeud) {
                if (noeud->type->est_type_entier_constant()) {
                    const_cast<NoeudExpression *>(noeud)->type = const_cast<Type *>(
                        transformation.type_cible);
                }
                return DecisionVisiteNoeud::CONTINUE;
            });
        return;
    }

    auto type_cible = transformation.type_cible;

    if (type_cible == nullptr) {
        if (transformation.type == TypeTransformation::CONSTRUIT_EINI) {
            type_cible = TypeBase::EINI;
        }
        else if (transformation.type == TypeTransformation::CONVERTI_VERS_PTR_RIEN) {
            type_cible = TypeBase::PTR_RIEN;
        }
        else if (transformation.type == TypeTransformation::PREND_REFERENCE) {
            type_cible = m_compilatrice.typeuse.type_reference_pour(expression->type);
        }
        else if (transformation.type == TypeTransformation::DEREFERENCE) {
            type_cible = type_déréférencé_pour(expression->type);
        }
        else if (transformation.type == TypeTransformation::CONSTRUIT_TABL_OCTET) {
            type_cible = TypeBase::TABL_OCTET;
        }
        else if (transformation.type == TypeTransformation::CONVERTI_TABLEAU_FIXE_VERS_TRANCHE) {
            auto type_tableau_fixe = expression->type->comme_type_tableau_fixe();
            type_cible = m_compilatrice.typeuse.type_tableau_dynamique(
                type_tableau_fixe->type_pointe);
        }
        else {
            assert_rappel(false, [&]() {
                dbg() << "Type Transformation non géré : " << transformation.type;
            });
        }
    }

    auto tfm = transformation;

    if (transformation.type == TypeTransformation::PREND_REFERENCE_ET_CONVERTIS_VERS_BASE) {
        auto noeud_comme = m_tacheronne->assembleuse->crée_comme(expression->lexeme);
        noeud_comme->bloc_parent = expression->bloc_parent;
        noeud_comme->type = m_compilatrice.typeuse.type_reference_pour(expression->type);
        noeud_comme->expression = expression;
        noeud_comme->transformation = TransformationType(TypeTransformation::PREND_REFERENCE);
        noeud_comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;

        expression = noeud_comme;
        tfm.type = TypeTransformation::CONVERTI_VERS_BASE;
    }

    auto noeud_comme = m_tacheronne->assembleuse->crée_comme(expression->lexeme);
    noeud_comme->bloc_parent = expression->bloc_parent;
    noeud_comme->type = const_cast<Type *>(type_cible);
    noeud_comme->expression = expression;
    noeud_comme->transformation = tfm;
    noeud_comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;

    expression = noeud_comme;
}

static bool est_accès_énum_drapeau(NoeudExpression const *expression)
{
    while (expression->est_parenthese()) {
        expression = expression->comme_parenthese()->expression;
    }
    return expression->possède_drapeau(DrapeauxNoeud::ACCES_EST_ENUM_DRAPEAU);
}

RésultatValidation Sémanticienne::valide_opérateur_binaire(NoeudExpressionBinaire *expr)
{
    CHRONO_TYPAGE(m_stats_typage.opérateurs_binaire, OPERATEUR_BINAIRE__VALIDATION);

    auto enfant1 = expr->operande_gauche;
    auto enfant2 = expr->operande_droite;
    auto type1 = enfant1->type;

    if (type1->est_type_type_de_donnees()) {
        return valide_opérateur_binaire_type(expr);
    }

    auto type_op = expr->lexeme->genre;

    /* détecte a comp b comp c */
    if (est_opérateur_comparaison(type_op) && est_opérateur_comparaison(enfant1->lexeme->genre)) {
        return valide_opérateur_binaire_chaine(expr);
    }

    if (est_accès_énum_drapeau(enfant1) && enfant2->est_litterale_bool()) {
        return valide_comparaison_énum_drapeau_bool(
            expr, enfant1, enfant2->comme_litterale_bool());
    }

    if (est_accès_énum_drapeau(enfant2) && enfant1->est_litterale_bool()) {
        return valide_comparaison_énum_drapeau_bool(
            expr, enfant2, enfant1->comme_litterale_bool());
    }

    return valide_opérateur_binaire_générique(expr);
}

/* Retourne vrai si les opérations sont compatibles pour une expression de comparaisons enchainées.
 * Les opérations sont compatibles si :
 * - elles sont similaires ('<' et '<'), ou
 * - elles sont similaires mais l'une d'entre elles utilise '='
 *
 * Tout autre cas est considérer comme malformé, par exemple : 'a < b > c'. Même si nous pourrions
 * généré du code pour ces expressions, elle ne sont pas vraiment sûres puisque, dans l'exemple,
 * 'a' pourrait également être supérieur à 'c', et peut-être que le programmeur eu l'intention de
 * garantir que 'a' est _inférieur_ à 'c'.
 */
static bool sont_opérations_compatibles_pour_comparaison_chainée(
    GenreLexème const opération_droite, GenreLexème const opération_gauche)
{
    if (opération_droite == opération_gauche) {
        /* Si les opérations sont les mêmes, vérifions qu'elles ne sont pas une différence ("!="),
         * car ceci est également ambigüe.) */
        return opération_droite != GenreLexème::DIFFERENCE;
    }

    const GenreLexème opérations_compatibles[][2] = {
        {GenreLexème::INFERIEUR, GenreLexème::INFERIEUR_EGAL},
        {GenreLexème::SUPERIEUR, GenreLexème::SUPERIEUR_EGAL},
    };

    POUR (opérations_compatibles) {
        if (opération_droite == it[0] && opération_gauche == it[1]) {
            return true;
        }

        if (opération_gauche == it[0] && opération_droite == it[1]) {
            return true;
        }
    }

    return false;
}

RésultatValidation Sémanticienne::valide_opérateur_binaire_chaine(NoeudExpressionBinaire *expr)
{
    auto const type_op = expr->lexeme->genre;

    auto const expression_binaire_gauche = expr->operande_gauche->comme_expression_binaire();
    auto const opération_gauche = expression_binaire_gauche->lexeme->genre;

    if (!sont_opérations_compatibles_pour_comparaison_chainée(type_op, opération_gauche)) {
        auto e = m_espace->rapporte_erreur(expr, "Enchainement de comparaison invalide.");
        e.ajoute_message("L'enchainement de comparaison n'est pas valide car les comparaisons "
                         "peuvent être ambigües.\n");
        e.ajoute_message("Les enchainements valides sont :\n");
        e.ajoute_message("    ('<' ou '<=') et ('<' ou '<=')\n");
        e.ajoute_message("    ('>' ou '>=') et ('>' ou '>=')\n");
        e.ajoute_message("    '==' et '=='\n");
        return CodeRetourValidation::Erreur;
    }

    auto const type_gauche = expression_binaire_gauche->operande_droite->type;

    auto const expression_comparée = expr->operande_droite;
    auto const type_droite = expression_comparée->type;

    auto résultat = trouve_opérateur_pour_expression(
        *m_espace, expr, type_gauche, type_droite, type_op);

    if (std::holds_alternative<Attente>(résultat)) {
        return std::get<Attente>(résultat);
    }

    auto candidat = std::get<OpérateurCandidat>(résultat);

    expr->genre = GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE;
    expr->type = TypeBase::BOOL;
    expr->op = candidat.op;
    crée_transtypage_implicite_au_besoin(expr->operande_gauche, candidat.transformation_type1);
    crée_transtypage_implicite_au_besoin(expr->operande_droite, candidat.transformation_type2);
    return CodeRetourValidation::OK;
}

RésultatValidation Sémanticienne::valide_opérateur_binaire_type(NoeudExpressionBinaire *expr)
{
    auto enfant1 = expr->operande_gauche;
    auto enfant2 = expr->operande_droite;
    auto type1 = enfant1->type;
    auto type2 = enfant2->type;

    if (!type2->est_type_type_de_donnees()) {
        rapporte_erreur("Opération impossible entre un type et autre chose", expr);
        return CodeRetourValidation::Erreur;
    }

    auto type_type1 = type1->comme_type_type_de_donnees();
    auto type_type2 = type2->comme_type_type_de_donnees();

    switch (expr->lexeme->genre) {
        default:
        {
            rapporte_erreur("Opérateur inapplicable sur des types", expr);
            return CodeRetourValidation::Erreur;
        }
        case GenreLexème::BARRE:
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

            if (!type_type1->type_connu->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_type(type_type1->type_connu);
            }

            if (!type_type2->type_connu->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                return Attente::sur_type(type_type2->type_connu);
            }

            auto membres = kuri::tablet<MembreTypeComposé, 6>(2);
            membres[0] = {nullptr, type_type1->type_connu, ID::_0};
            membres[1] = {nullptr, type_type2->type_connu, ID::_1};

            auto type_union = m_compilatrice.typeuse.union_anonyme(
                expr->lexeme, expr->bloc_parent, membres);
            expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_union);

            return CodeRetourValidation::OK;
        }
        case GenreLexème::EGALITE:
        {
            // XXX - aucune raison de prendre un verrou ici
            auto op = m_compilatrice.opérateurs->op_comp_égal_types;
            expr->type = op->type_résultat;
            expr->op = op;
            return CodeRetourValidation::OK;
        }
        case GenreLexème::DIFFERENCE:
        {
            // XXX - aucune raison de prendre un verrou ici
            auto op = m_compilatrice.opérateurs->op_comp_diff_types;
            expr->type = op->type_résultat;
            expr->op = op;
            return CodeRetourValidation::OK;
        }
    }
}

static bool est_decalage_bits(GenreLexème genre)
{
    return dls::outils::est_element(genre,
                                    GenreLexème::DECALAGE_DROITE,
                                    GenreLexème::DECALAGE_GAUCHE,
                                    GenreLexème::DEC_DROITE_EGAL,
                                    GenreLexème::DEC_GAUCHE_EGAL);
}

RésultatValidation Sémanticienne::valide_opérateur_binaire_générique(NoeudExpressionBinaire *expr)
{
    auto type_op = expr->lexeme->genre;
    auto assignation_composee = est_assignation_composée(type_op);
    auto enfant1 = expr->operande_gauche;
    auto enfant2 = expr->operande_droite;
    auto type1 = enfant1->type;
    auto type2 = enfant2->type;

    bool type_gauche_est_reference = false;
    if (assignation_composee) {
        type_op = operateur_pour_assignation_composee(type_op);

        if (type1->est_type_reference()) {
            type_gauche_est_reference = true;
            type1 = type1->comme_type_reference()->type_pointe;
            crée_transtypage_implicite_au_besoin(
                expr->operande_gauche, TransformationType(TypeTransformation::DEREFERENCE));
        }
    }

    auto résultat = trouve_opérateur_pour_expression(*m_espace, expr, type1, type2, type_op);

    if (std::holds_alternative<Attente>(résultat)) {
        return std::get<Attente>(résultat);
    }

    auto candidat = std::get<OpérateurCandidat>(résultat);

    expr->type = candidat.op->type_résultat;
    expr->op = candidat.op;
    expr->permute_operandes = candidat.permute_opérandes;

    if (type_gauche_est_reference &&
        candidat.transformation_type1.type != TypeTransformation::INUTILE) {
        m_espace->rapporte_erreur(expr->operande_gauche,
                                  "Impossible de transtyper la valeur à gauche pour une "
                                  "assignation composée.");
        return CodeRetourValidation::Erreur;
    }

    crée_transtypage_implicite_au_besoin(expr->operande_gauche, candidat.transformation_type1);
    crée_transtypage_implicite_au_besoin(expr->operande_droite, candidat.transformation_type2);

    if (assignation_composee) {
        expr->drapeaux |= DrapeauxNoeud::EST_ASSIGNATION_COMPOSEE;

        auto résultat_tfm = cherche_transformation(expr->type, type1);

        if (std::holds_alternative<Attente>(résultat_tfm)) {
            return std::get<Attente>(résultat_tfm);
        }

        auto transformation = std::get<TransformationType>(résultat_tfm);

        if (transformation.type == TypeTransformation::IMPOSSIBLE) {
            rapporte_erreur_assignation_type_différents(type1, expr->type, enfant2);
            return CodeRetourValidation::Erreur;
        }
    }

    if (est_decalage_bits(expr->lexeme->genre)) {
        auto résultat_decalage = evalue_expression(
            m_compilatrice, expr->bloc_parent, expr->operande_droite);
        /* Un résultat erroné veut dire que l'expression n'est pas constante.
         * À FAIRE : granularise pour différencier les expressions non-constantes des erreurs
         * réelles. */
        if (!résultat_decalage.est_errone) {
            auto const bits_max = nombre_de_bits_pour_type(type1);
            auto const decalage = résultat_decalage.valeur.entiere();
            if (résultat_decalage.valeur.entiere() >= bits_max) {
                m_espace->rapporte_erreur(expr, "Décalage binaire trop grand pour le type")
                    .ajoute_message("Le nombre de bits de décalage est de ", decalage, "\n")
                    .ajoute_message("Alors que le nombre maximum de bits de décalage est de ",
                                    bits_max - 1,
                                    " pour le type ",
                                    chaine_type(type1));
                return CodeRetourValidation::Erreur;
            }
        }
    }

    return CodeRetourValidation::OK;
}

/* Note : l'expr_acces_enum n'est pas un NoeudExpressionMembre car nous pouvons avoir des
 * parenthèses. */
RésultatValidation Sémanticienne::valide_comparaison_énum_drapeau_bool(
    NoeudExpressionBinaire *expr,
    NoeudExpression * /*expr_acces_enum*/,
    NoeudExpressionLitteraleBool *expr_bool)
{
    auto type_op = expr->lexeme->genre;

    if (type_op != GenreLexème::EGALITE && type_op != GenreLexème::DIFFERENCE) {
        m_espace->rapporte_erreur(expr,
                                  "Une comparaison entre une valeur d'énumération drapeau et une "
                                  "littérale booléenne doit se faire via « == » ou « != »");
        return CodeRetourValidation::Erreur;
    }

    auto type_bool = expr_bool->type;
    auto résultat = trouve_opérateur_pour_expression(
        *m_espace, expr, type_bool, type_bool, type_op);

    if (std::holds_alternative<Attente>(résultat)) {
        return std::get<Attente>(résultat);
    }

    auto candidat = std::get<OpérateurCandidat>(résultat);
    expr->op = candidat.op;
    expr->type = type_bool;
    return CodeRetourValidation::OK;
}

/* ------------------------------------------------------------------------- */
/** \name Expression logique.
 * \{ */

RésultatValidation Sémanticienne::valide_expression_logique(NoeudExpressionLogique *logique)
{
    auto opérande_gauche = logique->opérande_gauche;
    auto opérande_droite = logique->opérande_droite;

    if (!est_expression_convertible_en_bool(opérande_gauche)) {
        m_espace->rapporte_erreur(
            opérande_gauche, "Expression non conditionnable à gauche de l'opérateur logique !");
        return CodeRetourValidation::Erreur;
    }

    if (!est_expression_convertible_en_bool(opérande_droite)) {
        m_espace->rapporte_erreur(
            opérande_droite, "Expression non conditionnable à droite de l'opérateur logique !");
        return CodeRetourValidation::Erreur;
    }

    /* Les expressions de types a && b || c ou a || b && c ne sont pas valides
     * car nous ne pouvons déterminer le bon ordre d'exécution. */
    if (logique->lexeme->genre == GenreLexème::BARRE_BARRE) {
        if (opérande_gauche->lexeme->genre == GenreLexème::ESP_ESP) {
            m_espace
                ->rapporte_erreur(opérande_gauche,
                                  "Utilisation ambigüe de l'opérateur « && » à gauche de « || » !")
                .ajoute_message("Veuillez utiliser des parenthèses pour clarifier "
                                "l'ordre des comparaisons.");
            return CodeRetourValidation::Erreur;
        }

        if (opérande_droite->lexeme->genre == GenreLexème::ESP_ESP) {
            m_espace
                ->rapporte_erreur(opérande_droite,
                                  "Utilisation ambigüe de l'opérateur « && » à droite de « || » !")
                .ajoute_message("Veuillez utiliser des parenthèses pour clarifier "
                                "l'ordre des comparaisons.");
            return CodeRetourValidation::Erreur;
        }
    }

    logique->type = TypeBase::BOOL;
    return CodeRetourValidation::OK;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Boucle « pour ».
 * \{ */

struct TypageItérandeBouclePour {
    /* Pour définir aide_génération_code. */
    int genre_de_boucle = -1;
    /* Type à utiliser pour « it ». */
    Type *type_variable = nullptr;
    /* Type à utiliser pour « index_it ». */
    Type *type_index = nullptr;
};

using RésultatTypeItérande = std::variant<TypageItérandeBouclePour, Attente>;

static bool est_appel_coroutine(const NoeudExpression *itérand)
{
    if (!itérand->est_appel()) {
        return false;
    }

    auto const appel = itérand->comme_appel();
    auto const fonction_appelee = appel->noeud_fonction_appelee;

    if (!fonction_appelee->est_entete_fonction()) {
        return false;
    }

    auto const entete = fonction_appelee->comme_entete_fonction();
    return entete->est_coroutine;
}

/**
 * Détermine le genre de boucle et le type de « it » et « index_it » selon le noeud itéré.
 *
 * Retourne soit :
 * - une attente si nous itérons un type utilisant un opérateur pour
 * - une instance de #TypageItérandeBouclePour remplis convenablement.
 */
static RésultatTypeItérande détermine_typage_itérande(const NoeudExpression *itéré,
                                                      Typeuse &typeuse)
{
    auto type_variable_itérée = itéré->type;
    while (type_variable_itérée->est_type_opaque()) {
        type_variable_itérée = type_variable_itérée->comme_type_opaque()->type_opacifie;
    }

    /* NOTE : nous testons le type des noeuds d'abord pour ne pas que le
     * type de retour d'une coroutine n'interfère avec le type d'une
     * variable (par exemple quand nous retournons une chaine). */
    if (itéré->est_plage()) {
        return TypageItérandeBouclePour{
            GENERE_BOUCLE_PLAGE, type_variable_itérée, type_variable_itérée};
    }

    if (type_variable_itérée->est_type_tableau_dynamique() ||
        type_variable_itérée->est_type_tableau_fixe() ||
        type_variable_itérée->est_type_variadique() || type_variable_itérée->est_type_tranche()) {
        auto type_itérateur = type_déréférencé_pour(type_variable_itérée);
        auto type_index = TypeBase::Z64;
        return TypageItérandeBouclePour{GENERE_BOUCLE_TABLEAU, type_itérateur, type_index};
    }

    if (type_variable_itérée->est_type_chaine()) {
        auto type_itérateur = TypeBase::Z8;
        auto type_index = TypeBase::Z64;
        return TypageItérandeBouclePour{GENERE_BOUCLE_TABLEAU, type_itérateur, type_index};
    }

    if (est_type_entier(type_variable_itérée) ||
        type_variable_itérée->est_type_entier_constant()) {
        auto type_itérateur = type_variable_itérée;
        if (type_variable_itérée->est_type_entier_constant()) {
            type_itérateur = TypeBase::Z32;
        }

        return TypageItérandeBouclePour{
            GENERE_BOUCLE_PLAGE_IMPLICITE, type_itérateur, type_itérateur};
    }

    /* N'accèdons pas à la table via le registre pour éviter de la créer. */
    auto table_opérateurs = type_variable_itérée->table_opérateurs;
    if (table_opérateurs == nullptr || table_opérateurs->opérateur_pour == nullptr) {
        return Attente::sur_opérateur_pour(type_variable_itérée);
    }

    /* Utilisons le registre pour obtenir la table afin de ne pas avoir à revérifier si le type
     * possède une table d'opérateurs. */
    table_opérateurs = typeuse.operateurs_->donne_ou_crée_table_opérateurs(type_variable_itérée);
    auto const opérateur_pour = table_opérateurs->opérateur_pour;
    auto type_itérateur = opérateur_pour->param_sortie->type;
    /* À FAIRE : typage correct de l'index. */
    auto type_index = TypeBase::Z64;
    return TypageItérandeBouclePour{BOUCLE_POUR_OPÉRATEUR, type_itérateur, type_index};
}

static bool variables_ne_redéfinissent_rien(EspaceDeTravail *espace,
                                            kuri::tableau<NoeudExpression *, int> const &variables,
                                            NoeudBloc const *bloc)
{
    POUR (variables) {
        if (it->ident == ID::_) {
            /* Ignore explicitement la variable. Utile pour les boucles imbriquées. */
            continue;
        }

        auto decl = trouve_dans_bloc(bloc, it->ident);
        if (decl == nullptr) {
            continue;
        }

        if (decl->lexeme->ligne > it->lexeme->ligne) {
            continue;
        }

        erreur::redefinition_symbole(*espace, it, decl);
        return false;
    }

    return true;
}

static NoeudDeclarationVariable *crée_déclaration_pour_variable(AssembleuseArbre *assembleuse,
                                                                NoeudExpression *variable,
                                                                Type *type,
                                                                bool const doit_initialiser)
{
    auto init = NoeudExpression::nul();
    if (doit_initialiser) {
        init = assembleuse->crée_non_initialisation(variable->lexeme);
    }
    auto decl = assembleuse->crée_declaration_variable(variable->comme_reference_declaration(),
                                                       init);
    decl->type = type;
    decl->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    decl->genre_valeur = GenreValeur::TRANSCENDANTALE;
    variable->genre_valeur = GenreValeur::TRANSCENDANTALE;
    return decl;
}

RésultatValidation Sémanticienne::valide_instruction_pour(NoeudPour *inst)
{
    if (est_appel_coroutine(inst->expression)) {
        m_espace->rapporte_erreur(inst->expression,
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
        return CodeRetourValidation::Erreur;
    }

    auto variables = inst->variable->comme_virgule();
    auto const nombre_de_variables = variables->expressions.taille();
    if (nombre_de_variables > 2) {
        rapporte_erreur("Les boucles « pour » ne peuvent avoir que 2 variables maximum : la "
                        "valeur et l'index.",
                        variables);
        return CodeRetourValidation::Erreur;
    }

    auto expression = inst->expression;
    auto const résultat_typage_itérande = détermine_typage_itérande(expression,
                                                                    m_compilatrice.typeuse);
    if (std::holds_alternative<Attente>(résultat_typage_itérande)) {
        return std::get<Attente>(résultat_typage_itérande);
    }

    auto const typage_itérande = std::get<TypageItérandeBouclePour>(résultat_typage_itérande);
    auto const aide_génération_code = static_cast<char>(typage_itérande.genre_de_boucle);

    if (aide_génération_code == BOUCLE_POUR_OPÉRATEUR &&
        (inst->prend_reference || inst->prend_pointeur ||
         inst->lexeme_op != GenreLexème::INFERIEUR)) {
        if (inst->prend_pointeur) {
            m_espace->rapporte_erreur(
                inst,
                "Il est impossible de prendre une référence vers la variable itérée d'une "
                "boucle sur un type non standard.");
        }
        else if (inst->prend_reference) {
            m_espace->rapporte_erreur(
                inst,
                "Il est impossible de prendre l'adresse de la variable itérée d'une "
                "boucle sur un type non standard.");
        }
        else {
            m_espace->rapporte_erreur(inst,
                                      "Il est impossible de spécifier la direction d'une "
                                      "boucle sur un type non standard.");
        }
        return CodeRetourValidation::Erreur;
    }

    /* Le type ne doit plus être un entier_constant après determine_itérande,
     * donc nous pouvons directement l'assigner à enfant2->type.
     * Ceci est nécessaire car la simplification du code accède aux opérateurs
     * selon le type de enfant2. */
    if (expression->type->est_type_entier_constant()) {
        assert(!typage_itérande.type_variable->est_type_entier_constant());
        expression->type = typage_itérande.type_variable;
    }

    /* Le type de l'itérateur, à savoir le type de « it ». */
    auto type_itérateur = typage_itérande.type_variable;

    /* il faut attendre de vérifier que le type est itérable avant de prendre cette
     * indication en compte */
    if (inst->prend_reference) {
        type_itérateur = m_compilatrice.typeuse.type_reference_pour(type_itérateur);
    }
    else if (inst->prend_pointeur) {
        type_itérateur = m_compilatrice.typeuse.type_pointeur_pour(type_itérateur);
    }

    inst->aide_generation_code = aide_génération_code;

    /* Gère les redéfinitions après la détermination du typage de l'expression afin de ne pas avoir
     * à rechercher dans le bloc en cas d'attente. */
    if (!variables_ne_redéfinissent_rien(m_espace, variables->expressions, inst->bloc_parent)) {
        return CodeRetourValidation::Erreur;
    }

    auto bloc = inst->bloc;
    bloc->réserve_membres(nombre_de_variables);

    auto const possède_index = nombre_de_variables == 2;

    auto variable = variables->expressions[0];
    /* Copie l'ident pour les instructions de controle (continue, arrête, reprends). */
    inst->ident = variable->ident;

    /* Transforme les références en déclarations, nous faisons ça ici et non lors
     * du syntaxage ou de la simplification de l'arbre afin de prendre en compte
     * les cas où nous avons une fonction polymorphique : les données des déclarations
     * ne sont pas copiées.
     * Afin de ne pas faire de travail inutile, toutes les variables, saufs les
     * variables d'indexage ne sont pas initialisées. Les variables d'indexages doivent
     * l'être puisqu'elles sont directement testées avec la condition de fin de la
     * boucle.
     */
    auto assembleuse = m_tacheronne->assembleuse;
    assert(m_tacheronne->assembleuse->bloc_courant() == nullptr);
    assembleuse->bloc_courant(inst->bloc_parent);

    inst->decl_it = crée_déclaration_pour_variable(assembleuse, variable, type_itérateur, true);
    variables->expressions[0] = inst->decl_it;
    bloc->ajoute_membre(inst->decl_it);

    if (possède_index) {
        inst->decl_index_it = crée_déclaration_pour_variable(
            assembleuse, variables->expressions[1], typage_itérande.type_index, false);
        variables->expressions[1] = inst->decl_index_it;
        bloc->ajoute_membre(inst->decl_index_it);
    }
    else {
        /* Crée une déclaration pour « index_it » si ni le programme, ni la syntaxeuse n'en
         * a défini une. Ceci est requis pour la simplification du code.
         * Nous ne l'ajoutons pas aux membres du bloc pour éviter de potentiels conflits
         * avec des boucles externes, préservant ainsi le comportement des scripts
         * existants. À FAIRE : ajoute toujours ceci aux blocs ? */
        auto ref = assembleuse->crée_reference_declaration(inst->lexeme);
        ref->ident = ID::index_it;
        inst->decl_index_it = crée_déclaration_pour_variable(
            assembleuse, ref, typage_itérande.type_index, false);
    }
    assembleuse->dépile_bloc();

    if (aide_génération_code != BOUCLE_POUR_OPÉRATEUR) {
        return CodeRetourValidation::OK;
    }

    auto type_variable_itérée = expression->type;
    if (type_variable_itérée->est_type_opaque()) {
        type_variable_itérée = type_variable_itérée->comme_type_opaque()->type_opacifie;
        expression->type = type_variable_itérée;
    }

    auto table_opérateurs = type_variable_itérée->table_opérateurs;
    /* Si nous sommes ici, la table due être créée. */
    assert(table_opérateurs);
    auto opérateur_pour = table_opérateurs->opérateur_pour;

    /* Copie le macro. */
    auto copie_macro = copie_noeud(m_tacheronne->assembleuse,
                                   opérateur_pour,
                                   opérateur_pour->bloc_parent,
                                   OptionsCopieNoeud::PRÉSERVE_DRAPEAUX_VALIDATION |
                                       OptionsCopieNoeud::COPIE_PARAMÈTRES_DANS_MEMBRES);

    auto entête_copie_macro = copie_macro->comme_operateur_pour();
    auto corps_copie_macro = entête_copie_macro->corps;

    /* Installe les pointeurs de contexte. */
    corps_copie_macro->est_macro_boucle_pour = inst;
    inst->corps_operateur_pour = corps_copie_macro;

    /* Inutile de revenir ici, la validation peut reprendre au noeud suivant. */
    m_arbre_courant->index_courant += 1;

    /* Attend sur la validation sémantique du macro. */
    return Attente::sur_declaration(corps_copie_macro);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Instruction si/saufsi.
 * \{ */

/* Rassemble tous les blocs des branches de l'instruction si utilisée comme expression pour
 * initialiser ou assigner une variable.
 * Retourne faux si une erreur est rapportée.
 */
static bool rassemble_blocs_pour_expression_si(NoeudSi const *inst,
                                               EspaceDeTravail *espace,
                                               kuri::tablet<NoeudBloc *, 6> &blocs)
{
    while (true) {
        if (!inst->bloc_si_faux) {
            espace->rapporte_erreur(inst, "Branche « sinon » manquante dans l'expression « si »");
            return false;
        }

        blocs.ajoute(inst->bloc_si_vrai->comme_bloc());

        if (inst->bloc_si_faux->est_bloc()) {
            blocs.ajoute(inst->bloc_si_faux->comme_bloc());
            break;
        }

        if (inst->bloc_si_faux->est_si() || inst->bloc_si_faux->est_saufsi()) {
            inst = inst->bloc_si_faux->comme_si();
            continue;
        }

        /* Nous pouvons avoir d'autres instructions (pour, tantque, etc.).
         * Rapporte une erreur dans ces cas.
         */
        espace->rapporte_erreur(inst->bloc_si_faux, "Branche invalide dans l'expression « si »")
            .ajoute_message(
                "Seules les branches « sinon », « sinon si », et « sinon saufsi » sont "
                "possibles dans une expression d'assignation conditionnelle « si »");
        return false;
    }

    return true;
}

static bool expression_est_valide_pour_assignation_via_si(NoeudExpression const *expr)
{
    return (expr->genre_valeur & GenreValeur::DROITE) != 0;
}

/* Vérifie si l'expression est d'un type valide pour assignation via « si ». Sinon, rapporte une
 * erreur et retourne faux. */
static bool type_est_valide_pour_assignation_via_si(NoeudExpression const *expr,
                                                    EspaceDeTravail *espace)
{
    auto type = expr->type;
    if (!type) {
        espace->rapporte_erreur(
            expr, "Impossible d'assigner l'expression via « si » car l'expression n'a aucun type");
        return false;
    }

    if (type->est_type_rien()) {
        espace->rapporte_erreur(
            expr,
            "Impossible d'assigner l'expression via « si » car l'expression est de type « rien »");
        return false;
    }

    return true;
}

RésultatValidation Sémanticienne::valide_instruction_si(NoeudSi *inst)
{
    auto type_condition = inst->condition->type;

    if (type_condition == nullptr && !est_opérateur_bool(inst->condition->lexeme->genre)) {
        rapporte_erreur("Attendu un opérateur booléen pour la condition", inst->condition);
        return CodeRetourValidation::Erreur;
    }

    if (!est_expression_convertible_en_bool(inst->condition)) {
        m_espace
            ->rapporte_erreur(inst->condition,
                              "Impossible de convertir implicitement l'expression vers "
                              "une expression booléenne",
                              erreur::Genre::TYPE_DIFFERENTS)
            .ajoute_message("Le type de l'expression est ", chaine_type(type_condition), "\n");
        return CodeRetourValidation::Erreur;
    }

    if (!inst->possède_drapeau(DrapeauxNoeud::DROITE_ASSIGNATION)) {
        return CodeRetourValidation::OK;
    }

    /* Pour les expressions x = si y { z } sinon { w }. */

    kuri::tablet<NoeudBloc *, 6> blocs;
    if (!rassemble_blocs_pour_expression_si(inst, m_espace, blocs)) {
        return CodeRetourValidation::Erreur;
    }

    kuri::tablet<NoeudExpression *, 6> expressions_finales;

    POUR (blocs) {
        if (it->expressions->est_vide()) {
            m_espace->rapporte_erreur(it, "Bloc vide pour l'expression « si »");
            return CodeRetourValidation::Erreur;
        }

        auto dernière_expression = it->expressions->dernier_élément();
        if (dernière_expression->est_retourne() || dernière_expression->est_retiens()) {
            continue;
        }

        if (!expression_est_valide_pour_assignation_via_si(dernière_expression)) {
            m_espace
                ->rapporte_erreur(dernière_expression,
                                  "Expression invalide l'assignation via « si »")
                .ajoute_message(
                    "Une expression valide est une expression qui produit une valeur.");
            return CodeRetourValidation::Erreur;
        }

        expressions_finales.ajoute(dernière_expression);
    }

    if (expressions_finales.est_vide()) {
        m_espace->rapporte_erreur(inst, "Aucune expression trouvée pour l'assignation via « si »");
        return CodeRetourValidation::Erreur;
    }

    /* Détermine le type de l'expression selon les dernières expressions des blocs. */

    if (!type_est_valide_pour_assignation_via_si(expressions_finales[0], m_espace)) {
        return CodeRetourValidation::Erreur;
    }

    /* À FAIRE : pour l'instant nous requérons que tous les blocs ont le même type. Il faudra
     * peut-être relaxer cette condition en permettant certaines transformations. */
    auto type_inféré = expressions_finales[0]->type;
    if (type_inféré->est_type_entier_constant()) {
        type_inféré = TypeBase::Z32;
    }

    for (auto i = 1; i < expressions_finales.taille(); i++) {
        auto expr = expressions_finales[i];
        if (!type_est_valide_pour_assignation_via_si(expr, m_espace)) {
            return CodeRetourValidation::Erreur;
        }

        auto const résultat_compatibilité = vérifie_compatibilité(type_inféré, expr->type);

        if (std::holds_alternative<Attente>(résultat_compatibilité)) {
            return std::get<Attente>(résultat_compatibilité);
        }

        auto const poids_transformation = std::get<PoidsTransformation>(résultat_compatibilité);
        auto const transformation = poids_transformation.transformation;

        if (transformation.type == TypeTransformation::IMPOSSIBLE) {
            m_espace
                ->rapporte_erreur(expr, "Expression incompatible pour l'assignation via « si »")
                .ajoute_message("Le type inféré jusqu'ici est « ",
                                chaine_type(type_inféré),
                                " », mais l'expression est de type « ",
                                chaine_type(expr->type),
                                " »");

            return CodeRetourValidation::Erreur;
        }

        crée_transtypage_implicite_au_besoin(expr, transformation);

        if (transformation.type != TypeTransformation::INUTILE) {
            auto bloc = expr->bloc_parent;
            assert_rappel(bloc, [&]() { dbg() << erreur::imprime_site(*m_espace, expr); });
            /* Remplace l'expression. */
            bloc->expressions->supprime_dernier();
            bloc->expressions->ajoute(expr);
        }
    }

    inst->type = type_inféré;

    return CodeRetourValidation::OK;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Validation dépendance bibliothèque.
 * \{ */

RésultatValidation Sémanticienne::valide_dépendance_bibliothèque(
    NoeudDirectiveDependanceBibliotheque *noeud)
{
    auto &gestionnaire_bibliotheques = m_compilatrice.gestionnaire_bibliothèques;

    auto ident_bibliothèque_dépendante = noeud->bibliothèque_dépendante->ident;
    auto ident_bibliothèque_dépendue = noeud->bibliothèque_dépendue->ident;

    if (ident_bibliothèque_dépendante == ident_bibliothèque_dépendue) {
        m_espace->rapporte_erreur(noeud->bibliothèque_dépendue,
                                  "Une bibliothèque ne peut pas dépendre sur elle-même : les deux "
                                  "identifiants sont similaires.");
        return CodeRetourValidation::Erreur;
    }

    auto contexte_recherche_symbole = ContexteRechecheSymbole{};
    contexte_recherche_symbole.bloc_racine = noeud->bloc_parent;
    contexte_recherche_symbole.fichier = m_compilatrice.fichier(noeud->lexeme->fichier);
    contexte_recherche_symbole.fonction_courante = fonction_courante();

    /* Note : libc est spécial car nous la requérons tout le temps et est donc connue de la
     * compilatrice. */

    if (ident_bibliothèque_dépendante != ID::libc) {
        auto noeud_bib_dépendante = trouve_dans_bloc_ou_module(contexte_recherche_symbole,
                                                               ident_bibliothèque_dépendante);

        if (!noeud_bib_dépendante) {
            return Attente::sur_symbole(noeud->bibliothèque_dépendante);
        }

        if (!noeud_bib_dépendante->est_declaration_bibliotheque()) {
            m_espace
                ->rapporte_erreur(
                    noeud->bibliothèque_dépendante,
                    "#dépendance_bibliothèque doit prendre une référence à une bibliothèque, or "
                    "le symbole ne semble pas référencer une bibliothèque.")
                .ajoute_message(
                    "\nNote la déclaration référencée a été résolue comme étant celle de :\n")
                .ajoute_site(noeud_bib_dépendante);
            return CodeRetourValidation::Erreur;
        }
    }

    if (ident_bibliothèque_dépendue != ID::libc) {
        auto noeud_bib_dépendue = trouve_dans_bloc_ou_module(contexte_recherche_symbole,
                                                             ident_bibliothèque_dépendue);

        if (!noeud_bib_dépendue) {
            return Attente::sur_symbole(noeud->bibliothèque_dépendue);
        }

        if (!noeud_bib_dépendue->est_declaration_bibliotheque()) {
            m_espace
                ->rapporte_erreur(
                    noeud->bibliothèque_dépendue,
                    "#dépendance_bibliothèque doit prendre une référence à une bibliothèque, or "
                    "le symbole ne semble pas référencer une bibliothèque.")
                .ajoute_message(
                    "\nNote la déclaration référencée a été résolue comme étant celle de :\n")
                .ajoute_site(noeud_bib_dépendue);
            return CodeRetourValidation::Erreur;
        }
    }

    auto bib_dependante = gestionnaire_bibliotheques->trouve_ou_crée_bibliotheque(
        *m_espace, ident_bibliothèque_dépendante);
    auto bib_dependue = gestionnaire_bibliotheques->trouve_ou_crée_bibliotheque(
        *m_espace, ident_bibliothèque_dépendue);
    bib_dependante->dependances.ajoute(bib_dependue);
    /* Ce n'est pas une déclaration mais #GestionnaireCode.typage_termine le requiers. */
    noeud->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    return CodeRetourValidation::OK;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Instruction importe.
 * \{ */

static Module *donne_module_existant_pour_importe(NoeudInstructionImporte *inst,
                                                  Fichier *fichier,
                                                  Module *module_du_fichier)
{
    auto const expression = inst->expression;
    if (expression->lexeme->genre != GenreLexème::CHAINE_CARACTERE) {
        /* L'expression est un chemin relatif. */
        return nullptr;
    }

    /* À FAIRE : meilleure mise en cache. */
    auto module = static_cast<Module *>(nullptr);
    POUR (module_du_fichier->fichiers) {
        if (it == fichier) {
            continue;
        }
        pour_chaque_element(it->modules_importés, [&](Module *module_) {
            if (module_->nom() == expression->ident) {
                module = module_;
                return kuri::DécisionItération::Arrête;
            }

            return kuri::DécisionItération::Continue;
        });
    }

    return module;
}

RésultatValidation Sémanticienne::valide_instruction_importe(NoeudInstructionImporte *inst)
{
    const auto fichier = m_compilatrice.fichier(inst->lexeme->fichier);
    auto const module_du_fichier = fichier->module;

    auto module = donne_module_existant_pour_importe(inst, fichier, module_du_fichier);
    if (!module) {
        const auto lexeme = inst->expression->lexeme;
        const auto temps = dls::chrono::compte_seconde();
        module = m_compilatrice.importe_module(m_espace, lexeme->chaine, inst->expression);
        m_temps_chargement += temps.temps();
        if (!module) {
            return CodeRetourValidation::Erreur;
        }
    }

    if (module_du_fichier == module) {
        m_espace->rapporte_erreur(inst, "Importation d'un module dans lui-même !\n");
        return CodeRetourValidation::Erreur;
    }

    // @concurrence critique
    if (fichier->importe_module(module->nom())) {
        m_espace->rapporte_avertissement(inst, "Importation superflux du module");
    }
    else {
        fichier->modules_importés.insere(module);
        auto noeud_module = m_tacheronne->assembleuse
                                ->crée_noeud<GenreNoeud::DECLARATION_MODULE>(inst->lexeme)
                                ->comme_declaration_module();
        noeud_module->module = module;
        noeud_module->ident = module->nom();
        noeud_module->bloc_parent = inst->bloc_parent;
        noeud_module->bloc_parent->ajoute_membre(noeud_module);
        noeud_module->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    }

    inst->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    return CodeRetourValidation::OK;
}

/** \} */

ArbreAplatis *Sémanticienne::donne_un_arbre_aplatis()
{
    ArbreAplatis *résultat;
    if (m_arbres_aplatis.est_vide()) {
        résultat = memoire::loge<ArbreAplatis>("ArbreAplatis");
    }
    else {
        résultat = m_arbres_aplatis.dernier_élément();
        m_arbres_aplatis.supprime_dernier();
        résultat->réinitialise();
    }
    return résultat;
}

/* ------------------------------------------------------------------------- */
/** \name Validation expression comme.
 * \{ */

RésultatValidation Sémanticienne::valide_expression_comme(NoeudComme *expr)
{
    if (résoud_type_final(expr->expression_type, expr->type) == CodeRetourValidation::Erreur) {
        return CodeRetourValidation::Erreur;
    }

    if (expr->type == nullptr) {
        rapporte_erreur(
            "Ne peut transtyper vers un type invalide", expr, erreur::Genre::TYPE_INCONNU);
        return CodeRetourValidation::Erreur;
    }

    auto enfant = expr->expression;
    if (enfant->type == nullptr) {
        rapporte_erreur("Ne peut calculer le type d'origine", enfant, erreur::Genre::TYPE_INCONNU);
        return CodeRetourValidation::Erreur;
    }

    auto résultat = cherche_transformation_pour_transtypage(expr->expression->type, expr->type);
    if (std::holds_alternative<Attente>(résultat)) {
        return std::get<Attente>(résultat);
    }

    auto transformation = std::get<TransformationType>(résultat);
    if (transformation.type == TypeTransformation::IMPOSSIBLE) {
        if (!enfant->type->est_type_reference()) {
            rapporte_erreur_type_arguments(expr, expr->expression);
            return CodeRetourValidation::Erreur;
        }

        /* Si nous avons une référence, essayons de trouver une transformation avec le type
         * déréférencé. */

        /* Préserve l'expression pour le message d'erreur au besoin. */
        auto ancienne_expression = expr->expression;

        crée_transtypage_implicite_au_besoin(expr->expression,
                                             TransformationType(TypeTransformation::DEREFERENCE));
        résultat = cherche_transformation_pour_transtypage(expr->expression->type, expr->type);
        if (std::holds_alternative<Attente>(résultat)) {
            return std::get<Attente>(résultat);
        }

        transformation = std::get<TransformationType>(résultat);

        if (transformation.type == TypeTransformation::IMPOSSIBLE) {
            rapporte_erreur_type_arguments(expr, ancienne_expression);
            return CodeRetourValidation::Erreur;
        }
    }

    if (transformation.type == TypeTransformation::INUTILE) {
        /* À FAIRE : ne rapporte pas d'avertissements si le transtypage se fait vers le
         * type monomorphé. */
        if (fonction_courante() &&
            !fonction_courante()->possède_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION)) {
            m_espace->rapporte_avertissement(expr, "Instruction de transtypage inutile.");
        }
    }

    expr->transformation = transformation;
    return CodeRetourValidation::OK;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Validation expression type tableau fixe.
 * \{ */

RésultatValidation Sémanticienne::valide_expression_type_tableau_fixe(
    NoeudExpressionTypeTableauFixe *expr)
{
    auto type_expression_type = expr->expression_type->type;
    if (!type_expression_type->est_type_type_de_donnees()) {
        m_espace->rapporte_erreur(
            expr->expression_type,
            "Attendu un type de données pour l'expression du type tableau fixe.");
        return CodeRetourValidation::Erreur;
    }

    auto expression_taille = expr->expression_taille;
    if (expression_taille->type->est_type_type_de_donnees()) {
        auto type_de_données = expression_taille->type->comme_type_type_de_donnees();

        if (type_de_données->type_connu &&
            !type_de_données->type_connu->est_type_polymorphique()) {
            m_espace->rapporte_erreur(expression_taille,
                                      "Type invalide pour la taille du tableau fixe.");
            return CodeRetourValidation::Erreur;
        }

        auto type_de_donnees = type_expression_type->comme_type_type_de_donnees();
        auto type_connu = type_de_donnees->type_connu ? type_de_donnees->type_connu :
                                                        type_de_donnees;

        auto type_tableau = m_compilatrice.typeuse.type_tableau_fixe(type_de_données->type_connu,
                                                                     type_connu);
        expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_tableau);
        return CodeRetourValidation::OK;
    }

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
        m_espace->rapporte_erreur(expression_taille,
                                  "Impossible de définir un tableau fixe de taille 0 !\n");
        return CodeRetourValidation::Erreur;
    }

    auto taille_tableau = res.valeur.entiere();

    auto type_de_donnees = type_expression_type->comme_type_type_de_donnees();
    auto type_connu = type_de_donnees->type_connu ? type_de_donnees->type_connu : type_de_donnees;

    // À FAIRE: détermine proprement que nous avons un type s'utilisant par valeur
    // via un membre
    if (!type_connu->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        return Attente::sur_type(type_connu);
    }

    auto type_tableau = m_compilatrice.typeuse.type_tableau_fixe(type_connu,
                                                                 int32_t(taille_tableau));
    expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_tableau);

    return CodeRetourValidation::OK;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Validation expression type tableau dynamique.
 * \{ */

RésultatValidation Sémanticienne::valide_expression_type_tableau_dynamique(
    NoeudExpressionTypeTableauDynamique *expr)
{
    auto type_expression_type = expr->expression_type->type;
    if (!type_expression_type->est_type_type_de_donnees()) {
        m_espace->rapporte_erreur(
            expr->expression_type,
            "Attendu un type de données pour l'expression du type tableau fixe.");
        return CodeRetourValidation::Erreur;
    }

    auto type_de_donnees = type_expression_type->comme_type_type_de_donnees();
    auto type_connu = type_de_donnees->type_connu ? type_de_donnees->type_connu : type_de_donnees;
    auto type_tableau = m_compilatrice.typeuse.type_tableau_dynamique(type_connu);
    expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_tableau);
    return CodeRetourValidation::OK;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Validation expression type tableau dynamique.
 * \{ */

RésultatValidation Sémanticienne::valide_expression_type_tranche(NoeudExpressionTypeTranche *expr)
{
    auto type_expression_type = expr->expression_type->type;
    if (!type_expression_type->est_type_type_de_donnees()) {
        m_espace->rapporte_erreur(
            expr->expression_type,
            "Attendu un type de données pour l'expression du type tableau fixe.");
        return CodeRetourValidation::Erreur;
    }

    auto type_de_donnees = type_expression_type->comme_type_type_de_donnees();
    auto type_connu = type_de_donnees->type_connu ? type_de_donnees->type_connu : type_de_donnees;
    auto type_tableau = m_compilatrice.typeuse.crée_type_tranche(type_connu);
    expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_tableau);
    return CodeRetourValidation::OK;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Validation expression type fonction.
 * \{ */

RésultatValidation Sémanticienne::valide_expression_type_fonction(
    NoeudExpressionTypeFonction *expr)
{
    auto types_entrees = kuri::tablet<Type *, 6>(expr->types_entrée.taille());

    for (auto i = 0; i < expr->types_entrée.taille(); ++i) {
        NoeudExpression *type_entree = expr->types_entrée[i];

        if (résoud_type_final(type_entree, types_entrees[i]) == CodeRetourValidation::Erreur) {
            return CodeRetourValidation::Erreur;
        }
    }

    Type *type_sortie = nullptr;

    if (expr->types_sortie.taille() == 1) {
        if (résoud_type_final(expr->types_sortie[0], type_sortie) ==
            CodeRetourValidation::Erreur) {
            return CodeRetourValidation::Erreur;
        }
    }
    else {
        kuri::tablet<MembreTypeComposé, 6> membres;
        membres.réserve(expr->types_sortie.taille());

        for (auto &type_declare : expr->types_sortie) {
            if (résoud_type_final(type_declare, type_sortie) == CodeRetourValidation::Erreur) {
                return CodeRetourValidation::Erreur;
            }

            membres.ajoute({nullptr, type_sortie});
        }

        type_sortie = m_compilatrice.typeuse.crée_tuple(membres);
    }

    auto type_fonction = m_compilatrice.typeuse.type_fonction(types_entrees, type_sortie);
    expr->type = m_compilatrice.typeuse.type_type_de_donnees(type_fonction);
    return CodeRetourValidation::OK;
}

/** \} */
