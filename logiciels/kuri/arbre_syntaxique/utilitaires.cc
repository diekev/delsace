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
#include "utilitaires/log.hh"

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
    if (drapeaux == DrapeauxNoeud(0)) {
        os << "AUCUN";
        return os;
    }

#define SI_DRAPEAU_UTILISE(drapeau)                                                               \
    if ((drapeaux & DrapeauxNoeud::drapeau) != DrapeauxNoeud(0)) {                                \
        identifiants.ajoute(#drapeau);                                                            \
    }

    kuri::tablet<kuri::chaine_statique, 32> identifiants;

    SI_DRAPEAU_UTILISE(EMPLOYE)
    SI_DRAPEAU_UTILISE(EST_EXTERNE)
    SI_DRAPEAU_UTILISE(EST_MEMBRE_STRUCTURE)
    SI_DRAPEAU_UTILISE(EST_ASSIGNATION_COMPOSEE)
    SI_DRAPEAU_UTILISE(EST_VARIADIQUE)
    SI_DRAPEAU_UTILISE(EST_IMPLICITE)
    SI_DRAPEAU_UTILISE(EST_GLOBALE)
    SI_DRAPEAU_UTILISE(DECLARATION_TYPE_POLYMORPHIQUE)
    SI_DRAPEAU_UTILISE(DECLARATION_FUT_VALIDEE)
    SI_DRAPEAU_UTILISE(RI_FUT_GENEREE)
    SI_DRAPEAU_UTILISE(CODE_BINAIRE_FUT_GENERE)
    SI_DRAPEAU_UTILISE(TRANSTYPAGE_IMPLICITE)
    SI_DRAPEAU_UTILISE(EST_PARAMETRE)
    SI_DRAPEAU_UTILISE(EST_VALEUR_POLYMORPHIQUE)
    SI_DRAPEAU_UTILISE(POUR_CUISSON)
    SI_DRAPEAU_UTILISE(ACCES_EST_ENUM_DRAPEAU)
    SI_DRAPEAU_UTILISE(EST_UTILISEE)
    SI_DRAPEAU_UTILISE(METAPROGRAMME_CORPS_TEXTE_FUT_CREE)
    SI_DRAPEAU_UTILISE(NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE)
    SI_DRAPEAU_UTILISE(DÉPENDANCES_FURENT_RÉSOLUES)
    SI_DRAPEAU_UTILISE(IDENTIFIANT_EST_ACCENTUÉ_GRAVE)
    SI_DRAPEAU_UTILISE(LEXÈME_EST_RÉUTILISÉ_POUR_SUBSTITUTION)
    SI_DRAPEAU_UTILISE(DROITE_ASSIGNATION)
    SI_DRAPEAU_UTILISE(DROITE_CONDITION)
    SI_DRAPEAU_UTILISE(GAUCHE_EXPRESSION_APPEL)
    SI_DRAPEAU_UTILISE(EXPRESSION_BLOC_SI)
    SI_DRAPEAU_UTILISE(EXPRESSION_TEST_DISCRIMINATION)
    SI_DRAPEAU_UTILISE(EST_LOCALE)
    SI_DRAPEAU_UTILISE(EST_DÉCLARATION_EXPRESSION_VIRGULE)

    auto virgule = "";

    POUR (identifiants) {
        os << virgule << it;
        virgule = " | ";
    }

#undef SI_DRAPEAU_UTILISE

    return os;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DrapeauxNoeudFonction
 * \{ */

std::ostream &operator<<(std::ostream &os, DrapeauxNoeudFonction const drapeaux)
{
    if (drapeaux == DrapeauxNoeudFonction(0)) {
        os << "AUCUN";
        return os;
    }

#define SI_DRAPEAU_UTILISE(drapeau)                                                               \
    if ((drapeaux & DrapeauxNoeudFonction::drapeau) != DrapeauxNoeudFonction(0)) {                \
        identifiants.ajoute(#drapeau);                                                            \
    }

    kuri::tablet<kuri::chaine_statique, 32> identifiants;

    SI_DRAPEAU_UTILISE(FORCE_ENLIGNE)
    SI_DRAPEAU_UTILISE(FORCE_HORSLIGNE)
    SI_DRAPEAU_UTILISE(FORCE_SANSTRACE)
    SI_DRAPEAU_UTILISE(FORCE_SANSBROYAGE)
    SI_DRAPEAU_UTILISE(EST_EXTERNE)
    SI_DRAPEAU_UTILISE(EST_IPA_COMPILATRICE)
    SI_DRAPEAU_UTILISE(EST_RACINE)
    SI_DRAPEAU_UTILISE(EST_INTRINSÈQUE)
    SI_DRAPEAU_UTILISE(EST_INITIALISATION_TYPE)
    SI_DRAPEAU_UTILISE(EST_MÉTAPROGRAMME)
    SI_DRAPEAU_UTILISE(EST_VARIADIQUE)
    SI_DRAPEAU_UTILISE(EST_POLYMORPHIQUE)
    SI_DRAPEAU_UTILISE(EST_MONOMORPHISATION)
    SI_DRAPEAU_UTILISE(FUT_GÉNÉRÉE_PAR_LA_COMPILATRICE)
    SI_DRAPEAU_UTILISE(CLICHÉ_ASA_FUT_REQUIS)
    SI_DRAPEAU_UTILISE(CLICHÉ_ASA_CANONIQUE_FUT_REQUIS)
    SI_DRAPEAU_UTILISE(CLICHÉ_RI_FUT_REQUIS)
    SI_DRAPEAU_UTILISE(CLICHÉ_RI_FINALE_FUT_REQUIS)
    SI_DRAPEAU_UTILISE(CLICHÉ_CODE_BINAIRE_FUT_REQUIS)

    auto virgule = "";

    POUR (identifiants) {
        os << virgule << it;
        virgule = " | ";
    }

#undef SI_DRAPEAU_UTILISE
    return os;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Visibilité symbole.
 * \{ */

std::ostream &operator<<(std::ostream &os, VisibilitéSymbole visibilité)
{
    switch (visibilité) {
        case VisibilitéSymbole::INTERNE:
        {
            os << "INTERNE";
            break;
        }
        case VisibilitéSymbole::EXPORTÉ:
        {
            os << "EXPORTÉ";
            break;
        }
    }
    return os << "VISIBILITÉ INCONNUE";
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name DrapeauxTypes
 * \{ */

std::ostream &operator<<(std::ostream &os, DrapeauxTypes const drapeaux)
{
    if (drapeaux == DrapeauxTypes(0)) {
        os << "AUCUN";
        return os;
    }

#define SI_DRAPEAU_UTILISE(drapeau)                                                               \
    if ((drapeaux & DrapeauxTypes::drapeau) != DrapeauxTypes(0)) {                                \
        identifiants.ajoute(#drapeau);                                                            \
    }

    kuri::tablet<kuri::chaine_statique, 32> identifiants;

    SI_DRAPEAU_UTILISE(TYPE_NE_REQUIERS_PAS_D_INITIALISATION)
    SI_DRAPEAU_UTILISE(TYPE_EST_POLYMORPHIQUE)
    SI_DRAPEAU_UTILISE(INITIALISATION_TYPE_FUT_CREEE)
    SI_DRAPEAU_UTILISE(POSSEDE_TYPE_POINTEUR)
    SI_DRAPEAU_UTILISE(POSSEDE_TYPE_REFERENCE)
    SI_DRAPEAU_UTILISE(POSSEDE_TYPE_TABLEAU_FIXE)
    SI_DRAPEAU_UTILISE(POSSEDE_TYPE_TABLEAU_DYNAMIQUE)
    SI_DRAPEAU_UTILISE(POSSEDE_TYPE_TYPE_DE_DONNEES)
    SI_DRAPEAU_UTILISE(TYPE_POSSEDE_OPERATEURS_DE_BASE)
    SI_DRAPEAU_UTILISE(UNITE_POUR_INITIALISATION_FUT_CREE)

    auto virgule = "";

    POUR (identifiants) {
        os << virgule << it;
        virgule = " | ";
    }

#undef SI_DRAPEAU_UTILISE
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
        if (!it->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
            continue;
        }
        aplatis_arbre(it, arbre_aplatis, {});
    }

    POUR (entête->params) {
        if (it->possède_drapeau(DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE)) {
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
            auto bloc = racine->comme_bloc();

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
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        {
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::DECLARATION_STRUCTURE:
        case GenreNoeud::DECLARATION_UNION:
        {
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::DECLARATION_OPAQUE:
        {
            auto opaque = racine->comme_type_opaque();
            /* Évite les déclarations de types polymorphiques car cela gène la validation puisque
             * la déclaration n'est dans aucun bloc. */
            if (!opaque->expression_type->possède_drapeau(
                    DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE)) {
                aplatis_arbre(opaque->expression_type, arbre_aplatis, drapeau);
            }
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::DECLARATION_VARIABLE:
        {
            auto expr = racine->comme_declaration_variable();

            // N'aplatis pas expr->valeur car ça ne sers à rien dans ce cas.
            aplatis_arbre(
                expr->expression, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
            aplatis_arbre(
                expr->expression_type, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);

            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::DECLARATION_CONSTANTE:
        {
            auto constante = racine->comme_declaration_constante();

            // N'aplatis pas expr->valeur car ça ne sers à rien dans ce cas.
            aplatis_arbre(
                constante->expression, arbre_aplatis, drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
            aplatis_arbre(constante->expression_type,
                          arbre_aplatis,
                          drapeau | DrapeauxNoeud::DROITE_ASSIGNATION);
            arbre_aplatis.ajoute(constante);
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
        case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
        {
            auto expr = racine->comme_expression_binaire();
            expr->drapeaux |= drapeau;

            aplatis_arbre(expr->operande_gauche, arbre_aplatis, drapeau);
            aplatis_arbre(expr->operande_droite, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::EXPRESSION_PLAGE:
        {
            auto expr = racine->comme_plage();
            expr->drapeaux |= drapeau;
            aplatis_arbre(expr->debut, arbre_aplatis, drapeau);
            aplatis_arbre(expr->fin, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(expr);
            break;
        }
        case GenreNoeud::OPERATEUR_BINAIRE:
        {
            auto expr = racine->comme_expression_binaire();
            expr->drapeaux |= drapeau;

            if (est_assignation_composée(expr->lexeme->genre)) {
                drapeau |= DrapeauxNoeud::DROITE_ASSIGNATION;
            }

            aplatis_arbre(expr->operande_gauche, arbre_aplatis, drapeau);
            aplatis_arbre(expr->operande_droite, arbre_aplatis, drapeau);

            if (expr->lexeme->genre != GenreLexeme::VIRGULE) {
                arbre_aplatis.ajoute(expr);
            }

            break;
        }
        case GenreNoeud::EXPRESSION_LOGIQUE:
        {
            auto logique = racine->comme_expression_logique();
            logique->drapeaux |= drapeau;
            aplatis_arbre(logique->opérande_gauche, arbre_aplatis, drapeau);
            aplatis_arbre(logique->opérande_droite, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(logique);
            break;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
        {
            auto expr = racine->comme_reference_membre();
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
            auto expr = racine->comme_appel();
            expr->drapeaux |= drapeau;

            aplatis_arbre(
                expr->expression, arbre_aplatis, drapeau | DrapeauxNoeud::GAUCHE_EXPRESSION_APPEL);

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
        case GenreNoeud::EXPRESSION_PRISE_ADRESSE:
        {
            auto prise_adresse = racine->comme_prise_adresse();
            prise_adresse->drapeaux |= drapeau;
            aplatis_arbre(prise_adresse->opérande, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(prise_adresse);
            break;
        }
        case GenreNoeud::EXPRESSION_PRISE_REFERENCE:
        {
            auto prise_référence = racine->comme_prise_reference();
            prise_référence->drapeaux |= drapeau;
            aplatis_arbre(prise_référence->opérande, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(prise_référence);
            break;
        }
        case GenreNoeud::EXPRESSION_NEGATION_LOGIQUE:
        {
            auto négation = racine->comme_negation_logique();
            négation->drapeaux |= drapeau;
            aplatis_arbre(négation->opérande, arbre_aplatis, drapeau);
            arbre_aplatis.ajoute(négation);
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
            auto expr = racine->comme_execute();
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
            auto expr = racine->comme_boucle();

            aplatis_arbre(expr->condition,
                          arbre_aplatis,
                          DrapeauxNoeud::DROITE_ASSIGNATION | DrapeauxNoeud::DROITE_CONDITION);
            arbre_aplatis.ajoute(expr);
            aplatis_arbre(expr->bloc, arbre_aplatis, DrapeauxNoeud::AUCUN);

            break;
        }
        case GenreNoeud::INSTRUCTION_POUR:
        {
            auto expr = racine->comme_pour();

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
            auto expr = racine->comme_discr();

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
            auto expr = racine->comme_si();

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
            auto expr = racine->comme_pousse_contexte();

            aplatis_arbre(expr->expression, arbre_aplatis, DrapeauxNoeud::DROITE_ASSIGNATION);
            arbre_aplatis.ajoute(expr);
            aplatis_arbre(expr->bloc, arbre_aplatis, DrapeauxNoeud::AUCUN);

            break;
        }
        case GenreNoeud::INSTRUCTION_TENTE:
        {
            auto inst = racine->comme_tente();
            inst->drapeaux |= drapeau;

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
            break;
        }
        default:
        {
            assert_rappel(false, [&]() {
                std::cerr << "Genre de noeud non-géré : " << racine->genre << '\n';
            });
            break;
        }
    }
}

void aplatis_arbre(NoeudExpression *declaration, ArbreAplatis *arbre_aplatis)
{
    if (declaration->est_entete_fonction()) {
        auto entete = declaration->comme_entete_fonction();
        if (arbre_aplatis->noeuds.taille() == 0) {
            aplatis_entête_fonction(entete, arbre_aplatis->noeuds);
        }
        return;
    }

    if (declaration->est_corps_fonction()) {
        auto corps = declaration->comme_corps_fonction();
        if (arbre_aplatis->noeuds.taille() == 0) {
            aplatis_arbre(corps->bloc, arbre_aplatis->noeuds, {});
        }
        return;
    }

    if (declaration->est_declaration_classe()) {
        auto structure = declaration->comme_declaration_classe();

        if (arbre_aplatis->noeuds.taille() == 0) {
            if (structure->est_polymorphe) {
                POUR (*structure->bloc_constantes->membres.verrou_lecture()) {
                    aplatis_arbre(it, arbre_aplatis->noeuds, {});
                }
            }
            else {
                aplatis_arbre(structure->bloc, arbre_aplatis->noeuds, {});
            }
        }
        return;
    }

    if (declaration->est_execute()) {
        auto execute = declaration->comme_execute();
        if (arbre_aplatis->noeuds.taille() == 0) {
            aplatis_arbre(execute, arbre_aplatis->noeuds, {});
        }
        return;
    }

    if (declaration->est_declaration_variable()) {
        auto declaration_variable = declaration->comme_declaration_variable();
        if (arbre_aplatis->noeuds.taille() == 0) {
            aplatis_arbre(declaration_variable, arbre_aplatis->noeuds, {});
        }
        return;
    }

    if (declaration->est_declaration_constante()) {
        auto declaration_variable = declaration->comme_declaration_constante();
        if (arbre_aplatis->noeuds.taille() == 0) {
            aplatis_arbre(declaration_variable, arbre_aplatis->noeuds, {});
        }
        return;
    }

    if (declaration->est_ajoute_fini()) {
        auto ajoute_fini = declaration->comme_ajoute_fini();
        if (arbre_aplatis->noeuds.taille() == 0) {
            aplatis_arbre(ajoute_fini, arbre_aplatis->noeuds, {});
        }
        return;
    }

    if (declaration->est_ajoute_init()) {
        auto ajoute_init = declaration->comme_ajoute_init();
        if (arbre_aplatis->noeuds.taille() == 0) {
            aplatis_arbre(ajoute_init, arbre_aplatis->noeuds, {});
        }
        return;
    }

    if (declaration->est_type_opaque()) {
        auto opaque = declaration->comme_type_opaque();
        if (arbre_aplatis->noeuds.taille() == 0) {
            aplatis_arbre(opaque, arbre_aplatis->noeuds, {});
        }
        return;
    }

    assert_rappel(false, [&]() {
        dbg() << "Noeud non-géré pour l'aplatissement de l'arbre : " << declaration->genre;
    });
}

/* ------------------------------------------------------------------------- */
/** \name Détection des expressions constantes.
 * Ceci est utilisé pour détecter si l'exécution d'un métaprogramme est possible.
 * \{ */

NoeudExpression const *trouve_expression_non_constante(NoeudExpression const *expression)
{
    if (!expression) {
        /* Nous pouvons avoir des sous-expressions nulles (par exemple dans la construction de
         * structures dont certains membres ne sont pas initialisés). */
        return nullptr;
    }

    switch (expression->genre) {
        default:
        {
            return expression;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NUL:
        case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
        case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
        case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
        case GenreNoeud::EXPRESSION_TYPE_DE:
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        case GenreNoeud::EXPRESSION_INFO_DE:
        case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
        case GenreNoeud::DIRECTIVE_INTROSPECTION:
        {
            return nullptr;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
        {
            auto référence_déclaration = expression->comme_reference_declaration();
            if (!référence_déclaration->declaration_referee) {
                return référence_déclaration;
            }

            auto déclaration_référée = référence_déclaration->declaration_referee;
            if (déclaration_référée->est_declaration_type()) {
                return nullptr;
            }

            if (déclaration_référée->est_declaration_module()) {
                return nullptr;
            }

            if (déclaration_référée->est_entete_fonction()) {
                return nullptr;
            }

            if (déclaration_référée->est_declaration_constante()) {
                return nullptr;
            }

            return référence_déclaration;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
        {
            auto référence_membre = expression->comme_reference_membre();
            auto accédé = référence_membre->accedee;
            if (auto expr_variable = trouve_expression_non_constante(accédé)) {
                return expr_variable;
            }

            if (accédé->est_reference_declaration()) {
                if (accédé->comme_reference_declaration()
                        ->declaration_referee->est_declaration_module()) {
                    assert(référence_membre->déclaration_référée);
                    return trouve_expression_non_constante(référence_membre->déclaration_référée);
                }
            }

            /* À FAIRE : vérifie proprement que nous avons un type InfoType. */
            if (accédé->est_info_de()) {
                return nullptr;
            }

            auto type_accédé = donne_type_accédé_effectif(accédé->type);

            if (type_accédé->est_type_tableau_fixe()) {
                /* Seul l'accès à la taille est correcte, sinon nous ne serions pas ici. */
                return nullptr;
            }
            if (type_accédé->est_type_enum() || type_accédé->est_type_erreur()) {
                return nullptr;
            }
            if (type_accédé->est_type_type_de_donnees() &&
                référence_membre->genre_valeur == GenreValeur::DROITE) {
                /* Nous accédons à une valeur constante. */
                return nullptr;
            }
            auto type_compose = static_cast<TypeCompose *>(type_accédé);
            auto &membre = type_compose->membres[référence_membre->index_membre];

            if (membre.drapeaux == MembreTypeComposé::EST_CONSTANT) {
                return nullptr;
            }

            return référence_membre;
        }
        case GenreNoeud::EXPRESSION_PARENTHESE:
        {
            auto op = expression->comme_parenthese();
            return trouve_expression_non_constante(op->expression);
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
        {
            auto op = expression->comme_construction_tableau();
            return trouve_expression_non_constante(op->expression);
        }
        case GenreNoeud::OPERATEUR_UNAIRE:
        {
            auto op = expression->comme_expression_unaire();
            return trouve_expression_non_constante(op->operande);
        }
        case GenreNoeud::OPERATEUR_BINAIRE:
        case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
        case GenreNoeud::EXPRESSION_INDEXAGE:
        {
            auto op = expression->comme_expression_binaire();

            if (auto expr_variable = trouve_expression_non_constante(op->operande_gauche)) {
                return expr_variable;
            }

            if (auto expr_variable = trouve_expression_non_constante(op->operande_droite)) {
                return expr_variable;
            }

            return nullptr;
        }
        case GenreNoeud::EXPRESSION_LOGIQUE:
        {
            auto logique = expression->comme_expression_logique();
            if (auto expr_variable = trouve_expression_non_constante(logique->opérande_gauche)) {
                return expr_variable;
            }
            if (auto expr_variable = trouve_expression_non_constante(logique->opérande_droite)) {
                return expr_variable;
            }
            return nullptr;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
        case GenreNoeud::EXPRESSION_APPEL:
        {
            auto appel = expression->comme_appel();

            POUR (appel->parametres_resolus) {
                if (auto expr_variable = trouve_expression_non_constante(it)) {
                    return expr_variable;
                }
            }

            return nullptr;
        }
        case GenreNoeud::EXPRESSION_VIRGULE:
        {
            auto op = expression->comme_virgule();

            POUR (op->expressions) {
                if (auto expr_variable = trouve_expression_non_constante(it)) {
                    return expr_variable;
                }
            }

            return nullptr;
        }
        case GenreNoeud::EXPRESSION_COMME:
        {
            auto expression_comme = expression->comme_comme();
            return trouve_expression_non_constante(expression_comme->expression);
        }
        case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
        {
            auto tableau_args = expression->comme_args_variadiques();
            POUR (tableau_args->expressions) {
                if (auto expr_variable = trouve_expression_non_constante(it)) {
                    return expr_variable;
                }
            }

            return nullptr;
        }
    }

    return expression;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Détection de valeurs constantes pour les globales.
 * \{ */

bool peut_être_utilisée_pour_initialisation_constante_globale(NoeudExpression const *expression)
{
    if (!expression) {
        /* Membre non définis d'une construction de structure. */
        return true;
    }

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
        case GenreNoeud::EXPRESSION_INFO_DE:
        case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
        case GenreNoeud::DIRECTIVE_INTROSPECTION:
        {
            return true;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
        {
            auto référence_déclaration = expression->comme_reference_declaration();
            if (!référence_déclaration->declaration_referee) {
                return false;
            }

            auto déclaration_référée = référence_déclaration->declaration_referee;
            if (déclaration_référée->est_declaration_type()) {
                return true;
            }

            if (déclaration_référée->est_entete_fonction()) {
                return true;
            }

            if (déclaration_référée->possède_drapeau(DrapeauxNoeud::EST_GLOBALE)) {
                return true;
            }

            if (déclaration_référée->est_declaration_constante()) {
                return true;
            }

            return false;
        }
        case GenreNoeud::EXPRESSION_PRISE_ADRESSE:
        {
            auto prise_adresse = expression->comme_prise_adresse();
            return peut_être_utilisée_pour_initialisation_constante_globale(
                prise_adresse->opérande);
        }
        case GenreNoeud::EXPRESSION_PARENTHESE:
        {
            auto op = expression->comme_parenthese();
            return peut_être_utilisée_pour_initialisation_constante_globale(op->expression);
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
        {
            auto op = expression->comme_construction_tableau();
            return peut_être_utilisée_pour_initialisation_constante_globale(op->expression);
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
        {
            auto appel = expression->comme_construction_structure();

            POUR (appel->parametres_resolus) {
                if (!peut_être_utilisée_pour_initialisation_constante_globale(it)) {
                    return false;
                }
            }

            return true;
        }
        case GenreNoeud::EXPRESSION_VIRGULE:
        {
            auto op = expression->comme_virgule();

            POUR (op->expressions) {
                if (!peut_être_utilisée_pour_initialisation_constante_globale(it)) {
                    return false;
                }
            }

            return true;
        }
    }
}

/** \} */

// -----------------------------------------------------------------------------
// Implémentation des méthodes supplémentaires de l'arbre syntaxique

kuri::tablet<IdentifiantCode *, 6> donne_les_noms_de_la_hiérarchie(NoeudBloc *bloc)
{
    kuri::tablet<IdentifiantCode *, 6> noms;

    while (bloc) {
        if (bloc->ident) {
            noms.ajoute(bloc->ident);
        }

        bloc = bloc->bloc_parent;
    }

    return noms;
}

kuri::chaine_statique NoeudDeclarationEnteteFonction::donne_nom_broyé(Broyeuse &broyeuse)
{
    if (nom_broye_ != "") {
        return nom_broye_;
    }

    if (ident != ID::principale && !possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE |
                                                    DrapeauxNoeudFonction::FORCE_SANSBROYAGE)) {
        auto noms = donne_les_noms_de_la_hiérarchie(bloc_parent);
        nom_broye_ = broyeuse.broye_nom_fonction(this, noms);
    }
    else {
        nom_broye_ = lexeme->chaine;
    }

    return nom_broye_;
}

Type *NoeudDeclarationEnteteFonction::type_initialisé() const
{
    assert(possède_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE));
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
    if (table_membres.possède(decl->ident)) {
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

static void ajoute_à_ensemble_de_surcharge(NoeudDeclaration *decl, NoeudDeclaration *à_ajouter)
{
    if (decl->est_entete_fonction()) {
        auto entête_existante = decl->comme_entete_fonction();
        entête_existante->ensemble_de_surchages->ajoute(à_ajouter->comme_declaration_symbole());
        return;
    }

    if (decl->est_declaration_type()) {
        auto type_existant = decl->comme_declaration_type();
        type_existant->ensemble_de_surchages->ajoute(à_ajouter->comme_declaration_symbole());
        return;
    }

    assert_rappel(false, [&]() { dbg() << "Pas d'ensemble de surcharges pour " << decl->genre; });
}

void NoeudBloc::ajoute_membre(NoeudDeclaration *decl)
{
    if (decl->ident == ID::_ || decl->ident == nullptr) {
        /* Inutile d'avoir les variables ignorées ou les temporaires créées lors de la
         * canonicalisation ou la génération des initialisations des types comme membres du bloc.
         */
        return;
    }

    if (decl->est_declaration_symbole()) {
        auto decl_existante = declaration_pour_ident(decl->ident);
        if (decl_existante && decl_existante->est_declaration_symbole()) {
            ajoute_à_ensemble_de_surcharge(decl_existante, decl);
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
        auto résultat = table_membres.valeur_ou(expr->ident, nullptr);
        if (résultat != expr) {
            return résultat;
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

kuri::tableau_statique<const MembreTypeComposé> NoeudDeclarationTypeCompose::
    donne_membres_pour_code_machine() const
{
    return {membres.begin(), nombre_de_membres_réels};
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

static void remplis_membre_info_type(InfoTypeMembreStructure *info_type_membre,
                                     MembreTypeComposé const &membre)
{
    info_type_membre->decalage = membre.decalage;
    info_type_membre->nom = membre.nom->nom;
    info_type_membre->drapeaux = membre.drapeaux;

    if (membre.decl) {
        if (membre.decl->est_declaration_variable()) {
            copie_annotations(membre.decl->comme_declaration_variable()->annotations,
                              info_type_membre->annotations);
        }
        else if (membre.decl->est_declaration_constante()) {
            copie_annotations(membre.decl->comme_declaration_constante()->annotations,
                              info_type_membre->annotations);
        }
    }
}

InfoType *ConvertisseuseNoeudCode::crée_info_type_pour(Typeuse &typeuse, Type *type)
{
    auto crée_info_type_entier = [this](uint32_t taille_en_octet, bool est_signe) {
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
        case GenreNoeud::POLYMORPHIQUE:
        case GenreNoeud::TUPLE:
        {
            return nullptr;
        }
        case GenreNoeud::OCTET:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::OCTET;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::BOOL:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::BOOLEEN;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::CHAINE:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::CHAINE;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::EINI:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::EINI;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        {
            auto type_tableau = type->comme_type_tableau_dynamique();

            auto info_type = allocatrice_infos_types.infos_types_tableaux.ajoute_element();
            info_type->genre = GenreInfoType::TABLEAU;
            info_type->taille_en_octet = type->taille_octet;
            info_type->est_tableau_fixe = false;
            info_type->taille_fixe = 0;
            info_type->type_pointe = crée_info_type_pour(typeuse, type_tableau->type_pointe);

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::VARIADIQUE:
        {
            auto type_variadique = type->comme_type_variadique();

            auto info_type = allocatrice_infos_types.infos_types_variadiques.ajoute_element();
            info_type->genre = GenreInfoType::VARIADIQUE;
            info_type->taille_en_octet = type->taille_octet;

            // type nul pour les types variadiques des fonctions externes (p.e. printf(const char
            // *, ...))
            if (type_variadique->type_pointe) {
                info_type->type_élément = crée_info_type_pour(typeuse,
                                                              type_variadique->type_pointe);
            }

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_type_tableau_fixe();

            auto info_type = allocatrice_infos_types.infos_types_tableaux.ajoute_element();
            info_type->genre = GenreInfoType::TABLEAU;
            info_type->taille_en_octet = type->taille_octet;
            info_type->est_tableau_fixe = true;
            info_type->taille_fixe = type_tableau->taille;
            info_type->type_pointe = crée_info_type_pour(typeuse, type_tableau->type_pointe);

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::ENTIER_CONSTANT:
        {
            type->info_type = crée_info_type_entier(4, true);
            break;
        }
        case GenreNoeud::ENTIER_NATUREL:
        {
            type->info_type = crée_info_type_entier(type->taille_octet, false);
            break;
        }
        case GenreNoeud::ENTIER_RELATIF:
        {
            type->info_type = crée_info_type_entier(type->taille_octet, true);
            break;
        }
        case GenreNoeud::REEL:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::REEL;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::RIEN:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::RIEN;
            info_type->taille_en_octet = 0;

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::TYPE_DE_DONNEES:
        {
            auto info_type = allocatrice_infos_types.infos_types.ajoute_element();
            info_type->genre = GenreInfoType::TYPE_DE_DONNEES;
            info_type->taille_en_octet = type->taille_octet;

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::POINTEUR:
        case GenreNoeud::REFERENCE:
        {
            auto info_type = allocatrice_infos_types.infos_types_pointeurs.ajoute_element();
            info_type->genre = GenreInfoType::POINTEUR;
            info_type->type_pointe = crée_info_type_pour(typeuse, type_dereference_pour(type));
            info_type->taille_en_octet = type->taille_octet;
            info_type->est_reference = type->est_type_reference();

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::DECLARATION_STRUCTURE:
        {
            auto type_struct = type->comme_type_structure();

            auto info_type = allocatrice_infos_types.infos_types_structures.ajoute_element();
            type->info_type = info_type;

            info_type->genre = GenreInfoType::STRUCTURE;
            info_type->taille_en_octet = type->taille_octet;
            info_type->nom = donne_nom_hiérarchique(type_struct);

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
                info_type_membre->info = crée_info_type_pour(typeuse, it.type);
                remplis_membre_info_type(info_type_membre, it);
                info_type->membres.ajoute(info_type_membre);
            }

            copie_annotations(type_struct->annotations, info_type->annotations);

            info_type->structs_employees.reserve(type_struct->types_employés.taille());
            POUR (type_struct->types_employés) {
                auto info_struct_employe = crée_info_type_pour(typeuse, it->type);
                info_type->structs_employees.ajoute(
                    static_cast<InfoTypeStructure *>(info_struct_employe));
            }

            break;
        }
        case GenreNoeud::DECLARATION_UNION:
        {
            auto type_union = type->comme_type_union();

            auto info_type = allocatrice_infos_types.infos_types_unions.ajoute_element();
            info_type->genre = GenreInfoType::UNION;
            info_type->est_sure = !type_union->est_nonsure;
            info_type->type_le_plus_grand = crée_info_type_pour(typeuse,
                                                                type_union->type_le_plus_grand);
            info_type->decalage_index = type_union->decalage_index;
            info_type->taille_en_octet = type_union->taille_octet;
            info_type->nom = donne_nom_hiérarchique(type_union);

            info_type->membres.reserve(type_union->membres.taille());

            POUR (type_union->membres) {
                auto info_type_membre =
                    allocatrice_infos_types.infos_types_membres_structures.ajoute_element();
                info_type_membre->info = crée_info_type_pour(typeuse, it.type);
                remplis_membre_info_type(info_type_membre, it);
                info_type->membres.ajoute(info_type_membre);
            }

            copie_annotations(type_union->annotations, info_type->annotations);

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::DECLARATION_ENUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        {
            auto type_enum = static_cast<TypeEnum *>(type);

            auto info_type = allocatrice_infos_types.infos_types_enums.ajoute_element();
            info_type->genre = GenreInfoType::ENUM;
            info_type->nom = donne_nom_hiérarchique(type_enum);
            info_type->est_drapeau = type_enum->est_type_enum_drapeau();
            info_type->taille_en_octet = type_enum->taille_octet;
            info_type->type_sous_jacent = static_cast<InfoTypeEntier *>(
                crée_info_type_pour(typeuse, type_enum->type_sous_jacent));

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
        case GenreNoeud::FONCTION:
        {
            auto type_fonction = type->comme_type_fonction();

            auto info_type = allocatrice_infos_types.infos_types_fonctions.ajoute_element();
            info_type->genre = GenreInfoType::FONCTION;
            info_type->est_coroutine = false;
            info_type->taille_en_octet = type->taille_octet;

            info_type->types_entrees.reserve(type_fonction->types_entrees.taille());

            POUR (type_fonction->types_entrees) {
                info_type->types_entrees.ajoute(crée_info_type_pour(typeuse, it));
            }

            auto type_sortie = type_fonction->type_sortie;

            if (type_sortie->est_type_tuple()) {
                auto tuple = type_sortie->comme_type_tuple();
                info_type->types_sorties.reserve(tuple->membres.taille());

                POUR (tuple->membres) {
                    info_type->types_sorties.ajoute(crée_info_type_pour(typeuse, it.type));
                }
            }
            else {
                info_type->types_sorties.reserve(1);
                info_type->types_sorties.ajoute(crée_info_type_pour(typeuse, type_sortie));
            }

            type->info_type = info_type;
            break;
        }
        case GenreNoeud::DECLARATION_OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();

            auto info_type = allocatrice_infos_types.infos_types_opaques.ajoute_element();
            info_type->genre = GenreInfoType::OPAQUE;
            info_type->nom = donne_nom_hiérarchique(type_opaque);
            info_type->type_opacifie = crée_info_type_pour(typeuse, type_opaque->type_opacifie);

            type->info_type = info_type;
            break;
        }
        default:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud géré pour type : " << type->genre; });
            break;
        }
    }

    typeuse.définis_info_type_pour_type(type->info_type, type);
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
        case GenreInfoType::VARIADIQUE:
        {
            const auto info_type_variadique = static_cast<const InfoTypeVariadique *>(type);
            auto type_élément = convertis_info_type(typeuse, info_type_variadique->type_élément);
            return typeuse.type_variadique(type_élément);
        }
    }

    return nullptr;
}

/* ------------------------------------------------------------------------- */
/** \name Implémentation des fonctions supplémentaires de l'AssembleuseArbre
 * \{ */

NoeudExpressionBinaire *AssembleuseArbre::crée_expression_binaire(const Lexeme *lexeme,
                                                                  const OpérateurBinaire *op,
                                                                  NoeudExpression *expr1,
                                                                  NoeudExpression *expr2)
{
    assert(op);
    auto op_bin = crée_expression_binaire(lexeme);
    op_bin->operande_gauche = expr1;
    op_bin->operande_droite = expr2;
    op_bin->op = op;
    op_bin->type = op->type_résultat;
    return op_bin;
}

NoeudExpressionReference *AssembleuseArbre::crée_reference_declaration(const Lexeme *lexeme,
                                                                       NoeudDeclaration *decl)
{
    auto ref = crée_reference_declaration(lexeme);
    ref->declaration_referee = decl;
    ref->type = decl->type;
    ref->ident = decl->ident;
    return ref;
}

NoeudSi *AssembleuseArbre::crée_si(const Lexeme *lexeme, GenreNoeud genre_noeud)
{
    if (genre_noeud == GenreNoeud::INSTRUCTION_SI) {
        return crée_noeud<GenreNoeud::INSTRUCTION_SI>(lexeme)->comme_si();
    }

    return crée_noeud<GenreNoeud::INSTRUCTION_SAUFSI>(lexeme)->comme_saufsi();
}

NoeudBloc *AssembleuseArbre::crée_bloc_seul(const Lexeme *lexeme, NoeudBloc *bloc_parent)
{
    auto bloc = crée_noeud<GenreNoeud::INSTRUCTION_COMPOSEE>(lexeme)->comme_bloc();
    bloc->bloc_parent = bloc_parent;
    return bloc;
}

NoeudAssignation *AssembleuseArbre::crée_assignation_variable(const Lexeme *lexeme,
                                                              NoeudExpression *assignee,
                                                              NoeudExpression *expression)
{
    auto assignation = crée_noeud<GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE>(lexeme)
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

NoeudDeclarationVariable *AssembleuseArbre::crée_declaration_variable(const Lexeme *lexeme,
                                                                      Type *type,
                                                                      IdentifiantCode *ident,
                                                                      NoeudExpression *expression)
{
    auto ref = crée_reference_declaration(lexeme);
    ref->ident = ident;
    ref->type = type;
    return crée_declaration_variable(ref, expression);
}

NoeudDeclarationVariable *AssembleuseArbre::crée_declaration_variable(
    NoeudExpressionReference *ref, NoeudExpression *expression)
{
    auto declaration = crée_declaration_variable(ref->lexeme);
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

NoeudDeclarationVariable *AssembleuseArbre::crée_declaration_variable(
    NoeudExpressionReference *ref)
{
    auto decl = crée_declaration_variable(ref->lexeme);
    decl->valeur = ref;
    decl->ident = ref->ident;
    ref->declaration_referee = decl;
    return decl;
}

NoeudExpressionMembre *AssembleuseArbre::crée_reference_membre(const Lexeme *lexeme,
                                                               NoeudExpression *accede,
                                                               Type *type,
                                                               int index)
{
    auto acces = crée_reference_membre(lexeme);
    acces->accedee = accede;
    acces->type = type;
    acces->index_membre = index;
    return acces;
}

NoeudExpressionBinaire *AssembleuseArbre::crée_indexage(const Lexeme *lexeme,
                                                        NoeudExpression *expr1,
                                                        NoeudExpression *expr2,
                                                        bool ignore_verification)
{
    auto indexage = crée_noeud<GenreNoeud::EXPRESSION_INDEXAGE>(lexeme)->comme_indexage();
    indexage->operande_gauche = expr1;
    indexage->operande_droite = expr2;
    indexage->type = type_dereference_pour(expr1->type);
    if (ignore_verification) {
        indexage->aide_generation_code = IGNORE_VERIFICATION;
    }
    return indexage;
}

NoeudExpressionAppel *AssembleuseArbre::crée_appel(const Lexeme *lexeme,
                                                   NoeudExpression *appelee,
                                                   Type *type)
{
    auto appel = crée_appel(lexeme);
    appel->noeud_fonction_appelee = appelee;
    appel->type = type;

    if (appelee->est_entete_fonction()) {
        appel->expression = crée_reference_declaration(lexeme, appelee->comme_entete_fonction());
    }
    else {
        appel->expression = appelee;
    }

    return appel;
}

NoeudExpressionAppel *AssembleuseArbre::crée_construction_structure(const Lexeme *lexeme,
                                                                    TypeCompose *type)
{
    auto structure = crée_appel(lexeme);
    structure->genre = GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE;
    structure->parametres_resolus.reserve(type->membres.taille());
    structure->expression = type;
    structure->noeud_fonction_appelee = type;
    structure->type = type;
    return structure;
}

NoeudExpressionLitteraleEntier *AssembleuseArbre::crée_litterale_entier(Lexeme const *lexeme,
                                                                        Type *type,
                                                                        uint64_t valeur)
{
    auto lit = crée_litterale_entier(lexeme);
    lit->type = type;
    lit->valeur = valeur;
    return lit;
}

NoeudExpressionLitteraleBool *AssembleuseArbre::crée_litterale_bool(Lexeme const *lexeme,
                                                                    Type *type,
                                                                    bool valeur)
{
    auto lit = crée_litterale_bool(lexeme);
    lit->type = type;
    lit->valeur = valeur;
    return lit;
}

NoeudExpressionLitteraleReel *AssembleuseArbre::crée_litterale_reel(Lexeme const *lexeme,
                                                                    Type *type,
                                                                    double valeur)
{
    auto lit = crée_litterale_reel(lexeme);
    lit->type = type;
    lit->valeur = valeur;
    return lit;
}

NoeudExpression *AssembleuseArbre::crée_reference_type(Lexeme const *lexeme, Type *type)
{
    auto ref_type = crée_reference_type(lexeme);
    ref_type->type = type;
    return ref_type;
}

NoeudAssignation *AssembleuseArbre::crée_incrementation(const Lexeme *lexeme,
                                                        NoeudExpression *valeur)
{
    auto type = valeur->type;

    auto inc = crée_expression_binaire(lexeme);
    inc->op = type->table_opérateurs->opérateur_ajt;
    assert(inc->op);
    inc->operande_gauche = valeur;
    inc->type = type;

    if (est_type_entier(type)) {
        inc->operande_droite = crée_litterale_entier(valeur->lexeme, type, 1);
    }
    else if (type->est_type_reel()) {
        // À FAIRE(r16)
        inc->operande_droite = crée_litterale_reel(valeur->lexeme, type, 1.0);
    }

    return crée_assignation_variable(valeur->lexeme, valeur, inc);
}

NoeudAssignation *AssembleuseArbre::crée_decrementation(const Lexeme *lexeme,
                                                        NoeudExpression *valeur)
{
    auto type = valeur->type;

    auto inc = crée_expression_binaire(lexeme);
    inc->op = type->table_opérateurs->opérateur_sst;
    assert(inc->op);
    inc->operande_gauche = valeur;
    inc->type = type;

    if (est_type_entier(type)) {
        inc->operande_droite = crée_litterale_entier(valeur->lexeme, type, 1);
    }
    else if (type->est_type_reel()) {
        // À FAIRE(r16)
        inc->operande_droite = crée_litterale_reel(valeur->lexeme, type, 1.0);
    }

    return crée_assignation_variable(valeur->lexeme, valeur, inc);
}

NoeudExpressionPriseAdresse *crée_prise_adresse(AssembleuseArbre *assem,
                                                Lexeme const *lexème,
                                                NoeudExpression *expression,
                                                TypePointeur *type_résultat)
{
    assert(type_résultat->type_pointe == expression->type);

    auto résultat = assem->crée_prise_adresse(lexème);
    résultat->opérande = expression;
    résultat->type = type_résultat;
    return résultat;
}

NoeudDeclarationVariable *crée_retour_défaut_fonction(AssembleuseArbre *assembleuse,
                                                      Lexeme const *lexème)
{
    auto type_declaré = assembleuse->crée_reference_type(lexème);

    auto déclaration_paramètre = assembleuse->crée_declaration_variable(
        lexème, TypeBase::RIEN, ID::__ret0, nullptr);
    déclaration_paramètre->expression_type = type_declaré;
    déclaration_paramètre->drapeaux |= DrapeauxNoeud::EST_PARAMETRE;
    return déclaration_paramètre;
}

/** \} */

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

void imprime_détails_fonction(EspaceDeTravail *espace,
                              NoeudDeclarationEnteteFonction const *entête,
                              std::ostream &os)
{
    os << "Détail pour " << ordre_fonction(entête) << " " << entête->lexeme->chaine << " :\n";
    os << "-- Type                    : " << chaine_type(entête->type) << '\n';
    os << "-- Est polymorphique       : " << std::boolalpha
       << entête->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE) << '\n';
    os << "-- Est #corps_texte        : " << std::boolalpha << entête->corps->est_corps_texte
       << '\n';
    os << "-- Entête fut validée      : " << std::boolalpha
       << entête->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE) << '\n';
    os << "-- Corps fut validé        : " << std::boolalpha
       << entête->corps->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE) << '\n';
    os << "-- Est monomorphisation    : " << std::boolalpha
       << entête->possède_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION) << '\n';
    os << "-- Est initialisation type : " << std::boolalpha
       << entête->possède_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE) << '\n';
    if (entête->possède_drapeau(DrapeauxNoeudFonction::EST_MONOMORPHISATION)) {
        os << "-- Paramètres de monomorphisation :\n";
        POUR ((*entête->bloc_constantes->membres.verrou_lecture())) {
            os << "     " << it->ident->nom << " : " << chaine_type(it->type) << '\n';
        }
    }
    if (espace) {
        os << "-- Site de définition :\n";
        os << erreur::imprime_site(*espace, entête);
    }
}

kuri::chaine nom_humainement_lisible(NoeudExpression const *noeud)
{
    if (noeud->est_entete_fonction()) {
        auto entete = noeud->comme_entete_fonction();

        if (entete->possède_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE)) {
            return enchaine("init_de(", chaine_type(entete->type_initialisé()), ")");
        }

        if (entete->est_operateur) {
            if (entete->params.taille() == 2) {
                auto const type1 = entete->parametre_entree(0)->type;
                auto const type2 = entete->parametre_entree(1)->type;
                return enchaine("opérateur ",
                                entete->lexeme->chaine,
                                " (",
                                chaine_type(type1),
                                ", ",
                                chaine_type(type2),
                                ")");
            }

            if (entete->params.taille() == 1) {
                auto const type1 = entete->parametre_entree(0)->type;
                return enchaine(
                    "opérateur ", entete->lexeme->chaine, " (", chaine_type(type1), ")");
            }

            return enchaine("opérateur ", entete->lexeme->chaine);
        }

        if (entete->ident) {
            Enchaineuse enchaineuse;
            enchaineuse << entete->ident->nom;

            auto virgule = "(";
            for (int i = 0; i < entete->params.taille(); i++) {
                auto const param = entete->parametre_entree(i);
                enchaineuse << virgule << chaine_type(param->type);
                virgule = ", ";
            }
            if (entete->params.taille() == 0) {
                enchaineuse << "(";
            }
            enchaineuse << ") -> " << chaine_type(entete->param_sortie->type);

            return enchaineuse.chaine();
        }

        return "fonction anonyme";
    }

    if (noeud->ident) {
        return noeud->ident->nom;
    }

    return "anonyme";
}

Type *donne_type_accédé_effectif(Type *type_accédé)
{
    /* nous pouvons avoir une référence d'un pointeur, donc déréférence au plus */
    while (type_accédé->est_type_pointeur() || type_accédé->est_type_reference()) {
        type_accédé = type_dereference_pour(type_accédé);
    }

    if (type_accédé->est_type_opaque()) {
        type_accédé = type_accédé->comme_type_opaque()->type_opacifie;
    }

    return type_accédé;
}

/* ------------------------------------------------------------------------- */
/** \name Fonctions d'initialisation des types.
 * \{ */

static Lexeme lexème_sentinel = {};

NoeudDeclarationEnteteFonction *crée_entête_pour_initialisation_type(Type *type,
                                                                     Compilatrice &compilatrice,
                                                                     AssembleuseArbre *assembleuse,
                                                                     Typeuse &typeuse)
{
    if (type->fonction_init) {
        return type->fonction_init;
    }

    if (type->est_type_enum()) {
        /* Les fonctions pour les types de bases durent être créées au début de la compilation. */
        assert(type->comme_type_enum()->type_sous_jacent->fonction_init);
        return type->comme_type_enum()->type_sous_jacent->fonction_init;
    }

    auto type_param = typeuse.type_pointeur_pour(type);
    if (type->est_type_union() && !type->comme_type_union()->est_nonsure) {
        type_param = typeuse.type_pointeur_pour(type, false, false);
    }

    auto types_entrées = kuri::tablet<Type *, 6>();
    types_entrées.ajoute(type_param);

    auto type_fonction = typeuse.type_fonction(types_entrées, TypeBase::RIEN, false);

    static Lexeme lexème_entête = {};
    auto entête = assembleuse->crée_entete_fonction(&lexème_entête);
    entête->drapeaux_fonction |= DrapeauxNoeudFonction::EST_INITIALISATION_TYPE;

    entête->bloc_constantes = assembleuse->crée_bloc_seul(&lexème_sentinel, nullptr);
    entête->bloc_parametres = assembleuse->crée_bloc_seul(&lexème_sentinel,
                                                          entête->bloc_constantes);

    /* Paramètre d'entrée. */
    {
        static Lexeme lexème_déclaration = {};
        auto déclaration_paramètre = assembleuse->crée_declaration_variable(
            &lexème_déclaration, type_param, ID::pointeur, nullptr);
        déclaration_paramètre->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

        if (type->est_type_reference()) {
            /* Les références ne sont pas initialisées. Marquons-les comme tel afin de plaire aux
             * coulisses. */
            déclaration_paramètre->drapeaux |= DrapeauxNoeud::EST_MARQUÉE_INUTILISÉE;
        }
        else {
            déclaration_paramètre->drapeaux |= DrapeauxNoeud::EST_UTILISEE;
        }

        entête->params.ajoute(déclaration_paramètre);
    }

    /* Paramètre de sortie. */
    {
        static const Lexeme lexème_rien = {"rien", {}, GenreLexeme::RIEN, 0, 0, 0};
        auto déclaration_paramètre = crée_retour_défaut_fonction(assembleuse, &lexème_rien);
        déclaration_paramètre->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

        entête->params_sorties.ajoute(déclaration_paramètre);
        entête->param_sortie = déclaration_paramètre;
    }

    entête->type = type_fonction;
    entête->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    entête->drapeaux_fonction |= (DrapeauxNoeudFonction::FORCE_ENLIGNE |
                                  DrapeauxNoeudFonction::FORCE_SANSTRACE |
                                  DrapeauxNoeudFonction::FUT_GÉNÉRÉE_PAR_LA_COMPILATRICE);

    type->fonction_init = entête;

    if (type->est_type_union()) {
        /* Assigne également la fonction au type structure car c'est lui qui est utilisé lors
         * de la génération de RI. */
        auto type_structure = type->comme_type_union()->type_structure;
        if (type_structure) {
            type_structure->fonction_init = entête;
        }
    }

    return type->fonction_init;
}

static void crée_assignation(AssembleuseArbre *assembleuse,
                             NoeudExpression *variable,
                             NoeudExpression *expression)
{
    assert(variable->type);
    assert(expression->type);
    auto bloc = assembleuse->bloc_courant();
    auto assignation = assembleuse->crée_assignation_variable(
        &lexème_sentinel, variable, expression);
    assignation->type = expression->type;
    bloc->ajoute_expression(assignation);
}

static void crée_initialisation_defaut_pour_type(Type *type,
                                                 Compilatrice &compilatrice,
                                                 AssembleuseArbre *assembleuse,
                                                 NoeudExpression *ref_param,
                                                 NoeudExpression *expr_valeur_défaut,
                                                 Typeuse &typeuse)
{
    if (expr_valeur_défaut) {
        crée_assignation(assembleuse, ref_param, expr_valeur_défaut);
        return;
    }

    switch (type->genre) {
        case GenreNoeud::RIEN:
        case GenreNoeud::POLYMORPHIQUE:
        {
            break;
        }
        case GenreNoeud::EINI:
        case GenreNoeud::CHAINE:
        case GenreNoeud::DECLARATION_STRUCTURE:
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        case GenreNoeud::VARIADIQUE:
        case GenreNoeud::DECLARATION_UNION:
        {
            auto prise_adresse = crée_prise_adresse(
                assembleuse, &lexème_sentinel, ref_param, typeuse.type_pointeur_pour(type));
            auto fonction = crée_entête_pour_initialisation_type(
                type, compilatrice, assembleuse, typeuse);
            auto appel = assembleuse->crée_appel(&lexème_sentinel, fonction, TypeBase::RIEN);
            appel->parametres_resolus.ajoute(prise_adresse);
            assembleuse->bloc_courant()->ajoute_expression(appel);
            break;
        }
        case GenreNoeud::BOOL:
        {
            static Lexeme littéral_bool = {};
            littéral_bool.genre = GenreLexeme::FAUX;
            auto valeur_défaut = assembleuse->crée_litterale_bool(&littéral_bool);
            valeur_défaut->type = type;
            crée_assignation(assembleuse, ref_param, valeur_défaut);
            break;
        }
        case GenreNoeud::OCTET:
        case GenreNoeud::ENTIER_CONSTANT:
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::ENTIER_RELATIF:
        case GenreNoeud::TYPE_DE_DONNEES:
        case GenreNoeud::DECLARATION_ENUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        {
            auto valeur_défaut = assembleuse->crée_litterale_entier(&lexème_sentinel, type, 0);
            crée_assignation(assembleuse, ref_param, valeur_défaut);
            break;
        }
        case GenreNoeud::REEL:
        {
            auto valeur_défaut = assembleuse->crée_litterale_reel(&lexème_sentinel, type, 0);
            crée_assignation(assembleuse, ref_param, valeur_défaut);
            break;
        }
        case GenreNoeud::REFERENCE:
        {
            break;
        }
        case GenreNoeud::POINTEUR:
        case GenreNoeud::FONCTION:
        {
            auto valeur_défaut = assembleuse->crée_litterale_nul(&lexème_sentinel);
            valeur_défaut->type = ref_param->type;
            crée_assignation(assembleuse, ref_param, valeur_défaut);
            break;
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_type_tableau_fixe();
            auto type_élément = type_tableau->type_pointe;

            auto type_pointeur_type_pointe = typeuse.type_pointeur_pour(
                type_élément, false, false);

            /* NOTE: pour les tableaux fixes, puisque le déréférencement de pointeur est compliqué
             * avec les indexages, nous passons par une variable locale temporaire et copierons la
             * variable initialisée dans la mémoire pointée par le paramètre. */
            auto valeur_résultat = assembleuse->crée_declaration_variable(
                &lexème_sentinel,
                type_tableau,
                nullptr,
                assembleuse->crée_non_initialisation(&lexème_sentinel));
            assembleuse->bloc_courant()->ajoute_membre(valeur_résultat);
            assembleuse->bloc_courant()->ajoute_expression(valeur_résultat);
            auto ref_résultat = assembleuse->crée_reference_declaration(&lexème_sentinel,
                                                                        valeur_résultat);

            /* Toutes les variables doivent être initialisées (ou nous devons nous assurer que tous
             * les types possibles créés par la compilation ont une fonction d'initalisation). */
            auto init_it = assembleuse->crée_litterale_nul(&lexème_sentinel);
            init_it->type = type_pointeur_type_pointe;

            auto decl_it = assembleuse->crée_declaration_variable(
                &lexème_sentinel, type_pointeur_type_pointe, nullptr, init_it);
            auto ref_it = assembleuse->crée_reference_declaration(&lexème_sentinel, decl_it);

            assembleuse->bloc_courant()->ajoute_membre(decl_it);

            auto variable = assembleuse->crée_virgule(&lexème_sentinel);
            variable->expressions.ajoute(decl_it);

            // il nous faut créer une boucle sur le tableau.
            // pour * tableau { initialise_type(it); }
            auto pour = assembleuse->crée_pour(&lexème_sentinel);
            pour->prend_pointeur = true;
            pour->expression = ref_résultat;
            pour->bloc = assembleuse->crée_bloc(&lexème_sentinel);
            pour->aide_generation_code = GENERE_BOUCLE_TABLEAU;
            pour->variable = variable;
            pour->decl_it = decl_it;
            pour->decl_index_it = assembleuse->crée_declaration_variable(
                &lexème_sentinel, TypeBase::Z64, ID::index_it, nullptr);

            auto fonction = crée_entête_pour_initialisation_type(
                type_élément, compilatrice, assembleuse, typeuse);
            auto appel = assembleuse->crée_appel(&lexème_sentinel, fonction, TypeBase::RIEN);
            appel->parametres_resolus.ajoute(ref_it);

            pour->bloc->ajoute_expression(appel);

            assembleuse->bloc_courant()->ajoute_expression(pour);

            auto assignation_résultat = assembleuse->crée_assignation_variable(
                &lexème_sentinel, ref_param, ref_résultat);
            assembleuse->bloc_courant()->ajoute_expression(assignation_résultat);
            break;
        }
        case GenreNoeud::DECLARATION_OPAQUE:
        {
            auto opaque = type->comme_type_opaque();
            auto type_opacifié = opaque->type_opacifie;

            // Transtype vers le type opacifié, et crée l'initialisation pour le type opacifié.
            auto prise_adresse = crée_prise_adresse(
                assembleuse, &lexème_sentinel, ref_param, typeuse.type_pointeur_pour(type));

            auto comme = assembleuse->crée_comme(&lexème_sentinel);
            comme->expression = prise_adresse;
            comme->type = typeuse.type_pointeur_pour(type_opacifié);
            comme->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, comme->type};

            auto fonc_init = crée_entête_pour_initialisation_type(
                type_opacifié, compilatrice, assembleuse, typeuse);
            auto appel = assembleuse->crée_appel(&lexème_sentinel, fonc_init, TypeBase::RIEN);
            appel->parametres_resolus.ajoute(comme);
            assembleuse->bloc_courant()->ajoute_expression(appel);
            break;
        }
        case GenreNoeud::TUPLE:
        {
            // Les tuples ne sont que pour représenter les sorties des fonctions, ils ne devraient
            // pas avoir d'initialisation.
            break;
        }
        default:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud géré pour type : " << type->genre; });
            break;
        }
    }
}

/* Assigne la fonction d'initialisation de type au type énum en se basant sur son type de données.
 */
static void assigne_fonction_init_énum(Typeuse &typeuse, TypeEnum *type)
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

void crée_noeud_initialisation_type(EspaceDeTravail *espace,
                                    Type *type,
                                    AssembleuseArbre *assembleuse)
{
    auto &typeuse = espace->compilatrice().typeuse;

    if (type->est_type_enum()) {
        assigne_fonction_init_énum(typeuse, type->comme_type_enum());
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

    auto entête = crée_entête_pour_initialisation_type(
        type, espace->compilatrice(), assembleuse, typeuse);

    sauvegarde_fonction_init(typeuse, type, entête);

    auto corps = entête->corps;
    corps->aide_generation_code = REQUIERS_CODE_EXTRA_RETOUR;

    corps->bloc = assembleuse->crée_bloc_seul(&lexème_sentinel, entête->bloc_parametres);

    assert(assembleuse->bloc_courant() == nullptr);
    assembleuse->bloc_courant(corps->bloc);

    auto decl_param = entête->params[0]->comme_declaration_variable();
    auto ref_param = assembleuse->crée_reference_declaration(&lexème_sentinel, decl_param);

    switch (type->genre) {
        case GenreNoeud::RIEN:
        case GenreNoeud::POLYMORPHIQUE:
        {
            break;
        }
        case GenreNoeud::BOOL:
        case GenreNoeud::OCTET:
        case GenreNoeud::ENTIER_CONSTANT:
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::ENTIER_RELATIF:
        case GenreNoeud::TYPE_DE_DONNEES:
        case GenreNoeud::REEL:
        case GenreNoeud::REFERENCE:
        case GenreNoeud::POINTEUR:
        case GenreNoeud::FONCTION:
        case GenreNoeud::TABLEAU_FIXE:
        case GenreNoeud::DECLARATION_ENUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        {
            auto deref = assembleuse->crée_memoire(&lexème_sentinel);
            deref->expression = ref_param;
            deref->type = type;
            crée_initialisation_defaut_pour_type(
                type, espace->compilatrice(), assembleuse, deref, nullptr, typeuse);
            break;
        }
        case GenreNoeud::DECLARATION_OPAQUE:
        {
            auto type_opacifié = type->comme_type_opaque()->type_opacifie;
            auto type_pointeur_opacifié = typeuse.type_pointeur_pour(type_opacifié);

            auto comme_type_opacifie = assembleuse->crée_comme(&lexème_sentinel);
            comme_type_opacifie->expression = ref_param;
            comme_type_opacifie->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                                   type_pointeur_opacifié};
            comme_type_opacifie->type = type_pointeur_opacifié;

            auto deref = assembleuse->crée_memoire(&lexème_sentinel);
            deref->expression = comme_type_opacifie;
            deref->type = type_opacifié;

            crée_initialisation_defaut_pour_type(
                type_opacifié, espace->compilatrice(), assembleuse, deref, nullptr, typeuse);
            break;
        }
        case GenreNoeud::EINI:
        case GenreNoeud::CHAINE:
        case GenreNoeud::DECLARATION_STRUCTURE:
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        case GenreNoeud::VARIADIQUE:
        {
            auto type_composé = static_cast<TypeCompose *>(type);

            if (type_composé->est_type_structure()) {
                auto decl = type_composé->comme_type_structure();
                if (decl && decl->est_polymorphe) {
                    espace->rapporte_erreur_sans_site(
                        "Erreur interne : création d'une fonction d'initialisation pour un type "
                        "polymorphique !");
                }
            }

            POUR_INDEX (type_composé->membres) {
                if (it.ne_doit_pas_être_dans_code_machine() &&
                    !it.expression_initialisation_est_spéciale()) {
                    continue;
                }

                if (it.expression_valeur_defaut &&
                    it.expression_valeur_defaut->est_non_initialisation()) {
                    continue;
                }

                auto ref_membre = assembleuse->crée_reference_membre(
                    &lexème_sentinel, ref_param, it.type, index_it);
                crée_initialisation_defaut_pour_type(it.type,
                                                     espace->compilatrice(),
                                                     assembleuse,
                                                     ref_membre,
                                                     it.expression_valeur_defaut,
                                                     typeuse);
            }

            break;
        }
        case GenreNoeud::DECLARATION_UNION:
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
                auto transtype = assembleuse->crée_comme(&lexème_sentinel);
                transtype->expression = ref_param;
                transtype->transformation = TransformationType{
                    TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                    typeuse.type_pointeur_pour(membre.type)};
                transtype->type = const_cast<Type *>(transtype->transformation.type_cible);

                auto deref = assembleuse->crée_memoire(&lexème_sentinel);
                deref->expression = transtype;
                deref->type = membre.type;

                crée_initialisation_defaut_pour_type(membre.type,
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

                auto param_comme_structure = assembleuse->crée_comme(&lexème_sentinel);
                param_comme_structure->type = type_pointeur_type_structure;
                param_comme_structure->expression = ref_param;
                param_comme_structure->transformation = TransformationType{
                    TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_pointeur_type_structure};

                if (membre.type->est_type_rien()) {
                    /* Seul l'index doit être initialisé. (Support union ne contenant que « rien »
                     * comme types des membres). */
                    auto ref_membre = assembleuse->crée_reference_membre(&lexème_sentinel);
                    ref_membre->accedee = param_comme_structure;
                    ref_membre->index_membre = 0;
                    ref_membre->type = TypeBase::Z32;
                    ref_membre->aide_generation_code = IGNORE_VERIFICATION;
                    crée_initialisation_defaut_pour_type(TypeBase::Z32,
                                                         espace->compilatrice(),
                                                         assembleuse,
                                                         ref_membre,
                                                         nullptr,
                                                         typeuse);
                    break;
                }

                auto ref_membre = assembleuse->crée_reference_membre(&lexème_sentinel);
                ref_membre->accedee = param_comme_structure;
                ref_membre->index_membre = 0;
                ref_membre->type = membre.type;
                ref_membre->aide_generation_code = IGNORE_VERIFICATION;
                crée_initialisation_defaut_pour_type(membre.type,
                                                     espace->compilatrice(),
                                                     assembleuse,
                                                     ref_membre,
                                                     membre.expression_valeur_defaut,
                                                     typeuse);

                ref_membre = assembleuse->crée_reference_membre(&lexème_sentinel);
                ref_membre->accedee = param_comme_structure;
                ref_membre->index_membre = 1;
                ref_membre->type = TypeBase::Z32;
                ref_membre->aide_generation_code = IGNORE_VERIFICATION;
                crée_initialisation_defaut_pour_type(TypeBase::Z32,
                                                     espace->compilatrice(),
                                                     assembleuse,
                                                     ref_membre,
                                                     nullptr,
                                                     typeuse);
            }

            break;
        }
        case GenreNoeud::TUPLE:
        {
            // Les tuples ne sont que pour représenter les sorties des fonctions, ils ne devraient
            // pas avoir d'initialisation.
            break;
        }
        default:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud géré pour type : " << type->genre; });
            break;
        }
    }

    assembleuse->depile_bloc();
    simplifie_arbre(espace, assembleuse, typeuse, entête);
    assigne_fonction_init(type, entête);
    corps->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
}

/** \} */

bool possède_annotation(const BaseDeclarationVariable *decl, kuri::chaine_statique annotation)
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
        return entete->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE);
    }

    if (decl->est_type_structure()) {
        auto const structure = decl->comme_type_structure();
        return structure->est_polymorphe;
    }

    if (decl->est_type_opaque()) {
        auto const opaque = decl->comme_type_opaque();
        return opaque->expression_type->possède_drapeau(
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

void imprime_membres_blocs_récursifs(NoeudBloc const *bloc)
{
    Indentation indentation;

    while (bloc) {
        dbg() << indentation << "bloc " << bloc;
        indentation.incrémente();

        POUR (*bloc->membres.verrou_lecture()) {
            dbg() << indentation << it->ident->nom << " (" << it->ident << ")";
        }
        bloc = bloc->bloc_parent;
    }
}

UniteCompilation **donne_adresse_unité(NoeudExpression *noeud)
{
    if (noeud->est_entete_fonction()) {
        return &noeud->comme_entete_fonction()->unité;
    }
    if (noeud->est_corps_fonction()) {
        return &noeud->comme_corps_fonction()->unité;
    }
    if (noeud->est_importe()) {
        return &noeud->comme_importe()->unité;
    }
    if (noeud->est_charge()) {
        return &noeud->comme_charge()->unité;
    }
    if (noeud->est_declaration_variable()) {
        return &noeud->comme_declaration_variable()->unité;
    }
    if (noeud->est_execute()) {
        return &noeud->comme_execute()->unité;
    }
    if (noeud->est_declaration_classe()) {
        return &noeud->comme_declaration_classe()->unité;
    }
    if (noeud->est_declaration_bibliotheque()) {
        return &noeud->comme_declaration_bibliotheque()->unité;
    }
    if (noeud->est_type_opaque()) {
        return &noeud->comme_type_opaque()->unité;
    }
    if (noeud->est_dependance_bibliotheque()) {
        return &noeud->comme_dependance_bibliotheque()->unité;
    }
    if (noeud->est_type_enum()) {
        return &noeud->comme_type_enum()->unité;
    }
    if (noeud->est_ajoute_fini()) {
        return &noeud->comme_ajoute_fini()->unité;
    }
    if (noeud->est_ajoute_init()) {
        return &noeud->comme_ajoute_init()->unité;
    }
    if (noeud->est_declaration_constante()) {
        return &noeud->comme_declaration_constante()->unité;
    }

    assert_rappel(false, [&]() {
        dbg() << "Noeud non-géré pour l'adresse de l'unité de compilation " << noeud->genre;
    });
    return nullptr;
}
