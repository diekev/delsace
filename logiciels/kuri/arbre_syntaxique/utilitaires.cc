/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "utilitaires.hh"

#include "biblinternes/outils/assert.hh"

#include "compilation/broyage.hh"
#include "compilation/compilatrice.hh"
#include "compilation/erreur.h"
#include "compilation/espace_de_travail.hh"
#include "compilation/typage.hh"
#include "compilation/validation_semantique.hh"

#include "parsage/identifiant.hh"
#include "parsage/modules.hh"
#include "parsage/outils_lexemes.hh"

#include "utilitaires/log.hh"

#include "assembleuse.hh"
#include "canonicalisation.hh"
#include "cas_genre_noeud.hh"
#include "noeud_expression.hh"

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
    SI_DRAPEAU_UTILISE(EXPRESSION_TYPE_EST_CONTRAINTE_POLY)
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
/** \name PositionCodeNoeud
 * Drapeaux pour définir où se trouve le noeud dans l'arbre syntaxique.
 * \{ */

std::ostream &operator<<(std::ostream &os, PositionCodeNoeud const position)
{
    if (position == PositionCodeNoeud::AUCUNE) {
        os << "AUCUNE";
        return os;
    }

#define SI_DRAPEAU_UTILISE(drapeau)                                                               \
    if ((position & PositionCodeNoeud::drapeau) != PositionCodeNoeud::AUCUNE) {                   \
        identifiants.ajoute(#drapeau);                                                            \
    }

    kuri::tablet<kuri::chaine_statique, 32> identifiants;

    SI_DRAPEAU_UTILISE(DROITE_ASSIGNATION)
    SI_DRAPEAU_UTILISE(DROITE_CONDITION)
    SI_DRAPEAU_UTILISE(GAUCHE_EXPRESSION_APPEL)
    SI_DRAPEAU_UTILISE(EXPRESSION_BLOC_SI)
    SI_DRAPEAU_UTILISE(EXPRESSION_TEST_DISCRIMINATION)
    SI_DRAPEAU_UTILISE(DROITE_CONTRAINTE_POLYMORPHIQUE)

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
/** \name PortéeSymbole
 * Définis la portée d'un symbole (sa visibilité au sein ou hors d'un module).
 * \{ */

std::ostream &operator<<(std::ostream &os, PortéeSymbole const portée)
{
    switch (portée) {
        case PortéeSymbole::EXPORT:
        {
            os << "EXPORT";
            break;
        }
        case PortéeSymbole::MODULE:
        {
            os << "MODULE";
            break;
        }
        case PortéeSymbole::FICHIER:
        {
            os << "FICHIER";
            break;
        }
    }
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
/** \name GenreValeur
 * \{ */

std::ostream &operator<<(std::ostream &os, GenreValeur const genre_valeur)
{
    if (genre_valeur == GenreValeur::INVALIDE) {
        os << "INVALIDE";
        return os;
    }

#define SI_DRAPEAU_UTILISE(drapeau)                                                               \
    if ((genre_valeur & GenreValeur::drapeau) != GenreValeur(0)) {                                \
        identifiants.ajoute(#drapeau);                                                            \
    }

    kuri::tablet<kuri::chaine_statique, 32> identifiants;

    SI_DRAPEAU_UTILISE(GAUCHE)
    SI_DRAPEAU_UTILISE(DROITE)

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
                          PositionCodeNoeud position);

/* Fonction pour aplatir l'arbre d'une entête de fonction. Pour les déclarations de types
 * fonctions (fonc()(rien)) l'arbre aplatis est l'arbre du noeud parent (structure, fonction,
 * etc.), et non celui de l'entête. */
static void aplatis_entête_fonction(NoeudDéclarationEntêteFonction *entête,
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
                          PositionCodeNoeud position)
{
    if (racine == nullptr) {
        return;
    }

    switch (racine->genre) {
        case GenreNoeud::COMMENTAIRE:
        case GenreNoeud::DÉCLARATION_BIBLIOTHÈQUE:
        case GenreNoeud::DIRECTIVE_DÉPENDANCE_BIBLIOTHÈQUE:
        case GenreNoeud::DÉCLARATION_MODULE:
        case GenreNoeud::DIRECTIVE_FONCTION:
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
            aplatis_arbre(ajoute_init->expression, arbre_aplatis, position);
            arbre_aplatis.ajoute(ajoute_init);
            break;
        }
        case GenreNoeud::DIRECTIVE_AJOUTE_FINI:
        {
            auto ajoute_fini = racine->comme_ajoute_fini();
            aplatis_arbre(ajoute_fini->expression, arbre_aplatis, position);
            arbre_aplatis.ajoute(ajoute_fini);
            break;
        }
        case GenreNoeud::DIRECTIVE_PRÉ_EXÉCUTABLE:
        {
            auto pre_executable = racine->comme_pré_exécutable();
            aplatis_arbre(pre_executable->expression, arbre_aplatis, position);
            arbre_aplatis.ajoute(pre_executable);
            break;
        }
        case GenreNoeud::INSTRUCTION_COMPOSÉE:
        {
            auto bloc = racine->comme_bloc();

            auto const &expressions = bloc->expressions.verrou_lecture();

            /* Supprime ce drapeau si nous l'avons hérité, il ne doit pas être utilisé pour des
             * instructions si/saufsi qui ne sont pas des enfants de l'instruction si/saufsi
             * parent. */
            position &= ~PositionCodeNoeud::EXPRESSION_BLOC_SI;

            auto dernière_expression = expressions->taille() ? expressions->dernier_élément() :
                                                               NoeudExpression::nul();
            POUR (*expressions) {
                auto drapeaux = it == dernière_expression ? position : PositionCodeNoeud::AUCUNE;
                aplatis_arbre(it, arbre_aplatis, drapeaux);
            }

            // Il nous faut le bloc pour savoir quoi différer
            arbre_aplatis.ajoute(bloc);

            break;
        }
        case GenreNoeud::DÉCLARATION_ENTÊTE_FONCTION:
        {
            /* L'aplatissement d'une fonction dans une fonction doit déjà avoir été fait. */
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::DÉCLARATION_OPÉRATEUR_POUR:
        case GenreNoeud::DÉCLARATION_CORPS_FONCTION:
        {
            /* L'aplatissement d'une fonction dans une fonction doit déjà avoir été fait */
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        {
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        case GenreNoeud::DÉCLARATION_UNION:
        {
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            auto opaque = racine->comme_type_opaque();
            /* Évite les déclarations de types polymorphiques car cela gène la validation puisque
             * la déclaration n'est dans aucun bloc. */
            if (!opaque->expression_type->possède_drapeau(
                    DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE)) {
                aplatis_arbre(opaque->expression_type, arbre_aplatis, position);
            }
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::DÉCLARATION_VARIABLE:
        {
            auto expr = racine->comme_déclaration_variable();

            aplatis_arbre(
                expr->expression, arbre_aplatis, position | PositionCodeNoeud::DROITE_ASSIGNATION);
            aplatis_arbre(expr->expression_type,
                          arbre_aplatis,
                          position | PositionCodeNoeud::DROITE_ASSIGNATION);

            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::DÉCLARATION_VARIABLE_MULTIPLE:
        {
            auto expr = racine->comme_déclaration_variable_multiple();

            // N'aplatis pas expr->valeur car ça ne sers à rien dans ce cas.
            aplatis_arbre(
                expr->expression, arbre_aplatis, position | PositionCodeNoeud::DROITE_ASSIGNATION);
            aplatis_arbre(expr->expression_type,
                          arbre_aplatis,
                          position | PositionCodeNoeud::DROITE_ASSIGNATION);

            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::DÉCLARATION_CONSTANTE:
        {
            auto constante = racine->comme_déclaration_constante();

            // N'aplatis pas expr->valeur car ça ne sers à rien dans ce cas.
            aplatis_arbre(constante->expression,
                          arbre_aplatis,
                          position | PositionCodeNoeud::DROITE_ASSIGNATION);
            aplatis_arbre(constante->expression_type,
                          arbre_aplatis,
                          position | PositionCodeNoeud::DROITE_ASSIGNATION);
            arbre_aplatis.ajoute(constante);
            break;
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
        {
            auto expr = racine->comme_assignation_variable();
            expr->position |= position;

            aplatis_arbre(expr->assignée, arbre_aplatis, position);
            aplatis_arbre(
                expr->expression, arbre_aplatis, position | PositionCodeNoeud::DROITE_ASSIGNATION);
            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_MULTIPLE:
        {
            auto expr = racine->comme_assignation_multiple();
            expr->position |= position;

            aplatis_arbre(expr->assignées, arbre_aplatis, position);
            aplatis_arbre(
                expr->expression, arbre_aplatis, position | PositionCodeNoeud::DROITE_ASSIGNATION);
            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::EXPRESSION_COMME:
        {
            auto expr = racine->comme_comme();
            expr->position |= position;
            aplatis_arbre(expr->expression, arbre_aplatis, position);
            aplatis_arbre(expr->expression_type,
                          arbre_aplatis,
                          position | PositionCodeNoeud::DROITE_ASSIGNATION);
            arbre_aplatis.ajoute(expr);
            break;
        }
        case GenreNoeud::EXPRESSION_INDEXAGE:
        case GenreNoeud::OPÉRATEUR_COMPARAISON_CHAINÉE:
        {
            auto expr = racine->comme_expression_binaire();
            expr->position |= position;

            aplatis_arbre(expr->opérande_gauche, arbre_aplatis, position);
            aplatis_arbre(expr->opérande_droite, arbre_aplatis, position);
            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::EXPRESSION_PLAGE:
        {
            auto expr = racine->comme_plage();
            expr->position |= position;
            aplatis_arbre(expr->début, arbre_aplatis, position);
            aplatis_arbre(expr->fin, arbre_aplatis, position);
            arbre_aplatis.ajoute(expr);
            break;
        }
        case GenreNoeud::OPÉRATEUR_BINAIRE:
        {
            auto expr = racine->comme_expression_binaire();
            expr->position |= position;

            if (est_assignation_composée(expr->lexème->genre)) {
                position |= PositionCodeNoeud::DROITE_ASSIGNATION;
            }

            aplatis_arbre(expr->opérande_gauche, arbre_aplatis, position);

            if (expr->lexème->genre == GenreLexème::DIVISE) {
                /* Pour les contraintes polymorphiques. Peut être granulariser dans le future (à
                 * savoir, ne gérer que les choses marquées comme polymorphiques). */
                position |= PositionCodeNoeud::DROITE_CONTRAINTE_POLYMORPHIQUE;
            }

            aplatis_arbre(expr->opérande_droite, arbre_aplatis, position);

            if (expr->lexème->genre != GenreLexème::VIRGULE) {
                arbre_aplatis.ajoute(expr);
            }

            break;
        }
        case GenreNoeud::EXPRESSION_LOGIQUE:
        {
            auto logique = racine->comme_expression_logique();
            logique->position |= position;
            aplatis_arbre(logique->opérande_gauche, arbre_aplatis, position);
            aplatis_arbre(logique->opérande_droite, arbre_aplatis, position);
            arbre_aplatis.ajoute(logique);
            break;
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_LOGIQUE:
        {
            auto logique = racine->comme_assignation_logique();
            logique->position |= position;
            aplatis_arbre(logique->opérande_gauche, arbre_aplatis, position);
            position |= PositionCodeNoeud::DROITE_ASSIGNATION;
            aplatis_arbre(logique->opérande_droite, arbre_aplatis, position);
            arbre_aplatis.ajoute(logique);
            break;
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_MEMBRE:
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_MEMBRE_UNION:
        {
            auto expr = racine->comme_référence_membre();
            expr->position |= position;

            aplatis_arbre(expr->accédée, arbre_aplatis, position);
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
            expr->position |= position;

            aplatis_arbre(expr->expression, arbre_aplatis, position);
            expr->expression->position |= PositionCodeNoeud::GAUCHE_EXPRESSION_APPEL;

            POUR (expr->paramètres) {
                if (it->est_assignation_variable()) {
                    // n'aplatis pas le nom du paramètre car cela clasherait avec une variable
                    // locale, ou résulterait en une erreur de compilation « variable inconnue »
                    auto expr_assing = it->comme_assignation_variable();
                    aplatis_arbre(expr_assing->expression,
                                  arbre_aplatis,
                                  position | PositionCodeNoeud::DROITE_ASSIGNATION);
                }
                else {
                    aplatis_arbre(
                        it, arbre_aplatis, position | PositionCodeNoeud::DROITE_ASSIGNATION);
                }
            }

            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::EXPRESSION_INFO_DE:
        {
            auto expression_info_de = racine->comme_info_de();
            expression_info_de->position |= position;
            aplatis_arbre(expression_info_de->expression,
                          arbre_aplatis,
                          position | PositionCodeNoeud::EXPRESSION_INFO_DE);
            arbre_aplatis.ajoute(expression_info_de);
            break;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
        case GenreNoeud::EXPRESSION_MÉMOIRE:
        case GenreNoeud::EXPRESSION_PARENTHÈSE:
        case GenreNoeud::OPÉRATEUR_UNAIRE:
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        case GenreNoeud::EXPANSION_VARIADIQUE:
        case GenreNoeud::EXPRESSION_TYPE_DE:
        case GenreNoeud::INSTRUCTION_EMPL:
        {
            auto expr = static_cast<NoeudExpressionUnaire *>(racine);
            expr->position |= position;
            aplatis_arbre(expr->opérande, arbre_aplatis, position);
            arbre_aplatis.ajoute(expr);
            break;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU_TYPÉ:
        {
            auto tableau = racine->comme_construction_tableau_typé();
            tableau->position |= position;
            aplatis_arbre(tableau->expression_type, arbre_aplatis, position);
            aplatis_arbre(tableau->expression, arbre_aplatis, position);
            arbre_aplatis.ajoute(tableau);
            break;
        }
        case GenreNoeud::EXPRESSION_PRISE_ADRESSE:
        {
            auto prise_adresse = racine->comme_prise_adresse();
            prise_adresse->position |= position;
            aplatis_arbre(prise_adresse->opérande, arbre_aplatis, position);
            arbre_aplatis.ajoute(prise_adresse);
            break;
        }
        case GenreNoeud::EXPRESSION_PRISE_RÉFÉRENCE:
        {
            auto prise_référence = racine->comme_prise_référence();
            prise_référence->position |= position;
            aplatis_arbre(prise_référence->opérande, arbre_aplatis, position);
            arbre_aplatis.ajoute(prise_référence);
            break;
        }
        case GenreNoeud::EXPRESSION_NÉGATION_LOGIQUE:
        {
            auto négation = racine->comme_négation_logique();
            négation->position |= position;
            aplatis_arbre(négation->opérande, arbre_aplatis, position);
            arbre_aplatis.ajoute(négation);
            break;
        }
        case GenreNoeud::INSTRUCTION_ARRÊTE:
        case GenreNoeud::INSTRUCTION_CONTINUE:
        case GenreNoeud::INSTRUCTION_REPRENDS:
        {
            auto inst = racine->comme_controle_boucle();
            inst->position |= position;
            aplatis_arbre(inst->expression, arbre_aplatis, position);
            arbre_aplatis.ajoute(inst);
            break;
        }
        case GenreNoeud::INSTRUCTION_CHARGE:
        case GenreNoeud::INSTRUCTION_IMPORTE:
        {
            racine->position |= position;
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::INSTRUCTION_RETOUR:
        case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
        {
            auto inst = racine->comme_retourne();
            inst->position |= position;
            position |= PositionCodeNoeud::DROITE_ASSIGNATION;
            aplatis_arbre(inst->expression, arbre_aplatis, position);
            arbre_aplatis.ajoute(inst);
            break;
        }
        case GenreNoeud::INSTRUCTION_RETIENS:
        {
            auto inst = racine->comme_retiens();
            inst->position |= position;
            position |= PositionCodeNoeud::DROITE_ASSIGNATION;
            aplatis_arbre(inst->expression, arbre_aplatis, position);
            arbre_aplatis.ajoute(inst);
            break;
        }
        case GenreNoeud::DIRECTIVE_CUISINE:
        {
            auto cuisine = racine->comme_cuisine();
            cuisine->position |= position;

            position |= PositionCodeNoeud::DROITE_ASSIGNATION;
            cuisine->expression->drapeaux |= DrapeauxNoeud::POUR_CUISSON;

            aplatis_arbre(cuisine->expression, arbre_aplatis, position);
            arbre_aplatis.ajoute(cuisine);
            break;
        }
        case GenreNoeud::DIRECTIVE_EXÉCUTE:
        {
            auto expr = racine->comme_exécute();
            expr->position |= position;

            if (expr->ident == ID::assert_ || expr->ident == ID::test) {
                position |= PositionCodeNoeud::DROITE_ASSIGNATION;
            }

            aplatis_arbre(expr->expression, arbre_aplatis, position);
            arbre_aplatis.ajoute(expr);
            break;
        }
        case GenreNoeud::EXPRESSION_INIT_DE:
        {
            auto init_de = racine->comme_init_de();
            aplatis_arbre(init_de->expression, arbre_aplatis, position);
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_BOOLÉEN:
        case GenreNoeud::EXPRESSION_LITTÉRALE_CARACTÈRE:
        case GenreNoeud::EXPRESSION_LITTÉRALE_CHAINE:
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_RÉEL:
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_ENTIER:
        case GenreNoeud::EXPRESSION_LITTÉRALE_NUL:
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_DÉCLARATION:
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_TYPE:
        {
            racine->position |= position;
            arbre_aplatis.ajoute(racine);
            break;
        }
        case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
        {
            // Ceci ne doit pas être dans l'arbre à ce niveau
            break;
        }
        case GenreNoeud::INSTRUCTION_BOUCLE:
        case GenreNoeud::INSTRUCTION_RÉPÈTE:
        case GenreNoeud::INSTRUCTION_TANTQUE:
        {
            auto expr = racine->comme_boucle();

            aplatis_arbre(expr->condition,
                          arbre_aplatis,
                          PositionCodeNoeud::DROITE_ASSIGNATION |
                              PositionCodeNoeud::DROITE_CONDITION);
            arbre_aplatis.ajoute(expr);
            aplatis_arbre(expr->bloc, arbre_aplatis, PositionCodeNoeud::AUCUNE);

            break;
        }
        case GenreNoeud::INSTRUCTION_POUR:
        {
            auto expr = racine->comme_pour();

            // n'ajoute pas la variable, sa déclaration n'a pas de type
            aplatis_arbre(expr->expression, arbre_aplatis, PositionCodeNoeud::DROITE_ASSIGNATION);
            arbre_aplatis.ajoute(expr);

            aplatis_arbre(expr->bloc, arbre_aplatis, PositionCodeNoeud::AUCUNE);
            aplatis_arbre(expr->bloc_sansarrêt, arbre_aplatis, PositionCodeNoeud::AUCUNE);
            aplatis_arbre(expr->bloc_sinon, arbre_aplatis, PositionCodeNoeud::AUCUNE);

            break;
        }
        case GenreNoeud::INSTRUCTION_DISCR:
        case GenreNoeud::INSTRUCTION_DISCR_ÉNUM:
        case GenreNoeud::INSTRUCTION_DISCR_UNION:
        {
            auto expr = racine->comme_discr();

            aplatis_arbre(expr->expression_discriminée,
                          arbre_aplatis,
                          PositionCodeNoeud::DROITE_ASSIGNATION);

            POUR (expr->paires_discr) {
                for (auto expression : it->expression->comme_virgule()->expressions) {
                    if (!expression->est_appel()) {
                        aplatis_arbre(expression,
                                      arbre_aplatis,
                                      PositionCodeNoeud::EXPRESSION_TEST_DISCRIMINATION);
                        continue;
                    }

                    /* Les expressions d'appel sont des expressions d'extraction des valeurs, nous
                     * ne voulons pas valider sémantiquement les « paramètres » car ils ne
                     * références pas des variables mais en crée (les valider résulterait alors en
                     * une erreur de compilation). */
                    auto appel = expression->comme_appel();
                    aplatis_arbre(appel->expression,
                                  arbre_aplatis,
                                  PositionCodeNoeud::EXPRESSION_TEST_DISCRIMINATION);
                }
            }

            arbre_aplatis.ajoute(expr);

            POUR (expr->paires_discr) {
                aplatis_arbre(it->bloc, arbre_aplatis, PositionCodeNoeud::AUCUNE);
            }

            aplatis_arbre(expr->bloc_sinon, arbre_aplatis, PositionCodeNoeud::AUCUNE);

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
            expr->position |= position;

            /* Seul l'expression racine, directement après l'assignation, doit être marquée comme
             * tel. */
            if ((position & PositionCodeNoeud::EXPRESSION_BLOC_SI) ==
                PositionCodeNoeud::EXPRESSION_BLOC_SI) {
                expr->position &= ~PositionCodeNoeud::DROITE_ASSIGNATION;
            }

            aplatis_arbre(expr->condition,
                          arbre_aplatis,
                          PositionCodeNoeud::DROITE_ASSIGNATION |
                              PositionCodeNoeud::DROITE_CONDITION);
            aplatis_arbre(expr->bloc_si_vrai,
                          arbre_aplatis,
                          position | PositionCodeNoeud::EXPRESSION_BLOC_SI);
            aplatis_arbre(expr->bloc_si_faux,
                          arbre_aplatis,
                          position | PositionCodeNoeud::EXPRESSION_BLOC_SI);

            /* mets l'instruction à la fin afin de pouvoir déterminer le type de
             * l'expression selon les blocs si nous sommes à droite d'une expression */
            arbre_aplatis.ajoute(expr);

            break;
        }
        case GenreNoeud::INSTRUCTION_SI_STATIQUE:
        case GenreNoeud::INSTRUCTION_SAUFSI_STATIQUE:
        {
            auto inst = racine->comme_si_statique();
            aplatis_arbre(inst->condition, arbre_aplatis, PositionCodeNoeud::DROITE_ASSIGNATION);
            arbre_aplatis.ajoute(inst);
            aplatis_arbre(inst->bloc_si_vrai, arbre_aplatis, PositionCodeNoeud::AUCUNE);
            arbre_aplatis.ajoute(inst);  // insère une deuxième fois pour pouvoir sauter le code du
                                         // bloc_si_faux si la condition évalue à « vrai »
            inst->index_bloc_si_faux = arbre_aplatis.taille() - 1;
            aplatis_arbre(inst->bloc_si_faux, arbre_aplatis, PositionCodeNoeud::AUCUNE);
            inst->index_après = arbre_aplatis.taille() - 1;
            break;
        }
        case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
        {
            auto expr = racine->comme_pousse_contexte();

            aplatis_arbre(expr->expression, arbre_aplatis, PositionCodeNoeud::DROITE_ASSIGNATION);
            arbre_aplatis.ajoute(expr);
            aplatis_arbre(expr->bloc, arbre_aplatis, PositionCodeNoeud::AUCUNE);

            break;
        }
        case GenreNoeud::INSTRUCTION_TENTE:
        {
            auto inst = racine->comme_tente();
            inst->position |= position;

            if (inst->expression_piégée) {
                position |= PositionCodeNoeud::DROITE_ASSIGNATION;
            }

            aplatis_arbre(inst->expression_appelée, arbre_aplatis, position);
            arbre_aplatis.ajoute(inst);
            aplatis_arbre(inst->bloc, arbre_aplatis, PositionCodeNoeud::AUCUNE);

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
                aplatis_arbre(it, arbre_aplatis, position);
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_DIFFÈRE:
        {
            auto inst = racine->comme_diffère();
            aplatis_arbre(inst->expression, arbre_aplatis, position);
            arbre_aplatis.ajoute(inst);
            break;
        }
        case GenreNoeud::EXPRESSION_TYPE_TABLEAU_FIXE:
        {
            auto expr = racine->comme_expression_type_tableau_fixe();
            aplatis_arbre(expr->expression_taille, arbre_aplatis, position);
            aplatis_arbre(expr->expression_type, arbre_aplatis, position);
            arbre_aplatis.ajoute(expr);
            break;
        }
        case GenreNoeud::EXPRESSION_TYPE_TABLEAU_DYNAMIQUE:
        {
            auto expr = racine->comme_expression_type_tableau_dynamique();
            aplatis_arbre(expr->expression_type, arbre_aplatis, position);
            arbre_aplatis.ajoute(expr);
            break;
        }
        case GenreNoeud::EXPRESSION_TYPE_TRANCHE:
        {
            auto expr = racine->comme_expression_type_tranche();
            aplatis_arbre(expr->expression_type, arbre_aplatis, position);
            arbre_aplatis.ajoute(expr);
            break;
        }
        case GenreNoeud::EXPRESSION_TYPE_FONCTION:
        {
            auto expr = racine->comme_expression_type_fonction();
            POUR (expr->types_entrée) {
                aplatis_arbre(it, arbre_aplatis, position);
            }
            POUR (expr->types_sortie) {
                aplatis_arbre(it, arbre_aplatis, position);
            }
            arbre_aplatis.ajoute(expr);
            break;
        }
        case GenreNoeud::EXPRESSION_SÉLECTION:
        {
            auto sélection = racine->comme_sélection();
            arbre_aplatis.ajoute(sélection->condition);
            arbre_aplatis.ajoute(sélection->si_vrai);
            arbre_aplatis.ajoute(sélection->si_faux);
            arbre_aplatis.ajoute(sélection);
            break;
        }
        CAS_POUR_NOEUDS_TYPES_FONDAMENTAUX:
        {
            assert_rappel(false,
                          [&]() { dbg() << "Genre de noeud non-géré : " << racine->genre; });
            break;
        }
    }
}

void aplatis_arbre(NoeudExpression *declaration, ArbreAplatis *arbre_aplatis)
{
    if (declaration->est_entête_fonction()) {
        auto entete = declaration->comme_entête_fonction();
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

    if (declaration->est_déclaration_classe()) {
        auto structure = declaration->comme_déclaration_classe();

        if (arbre_aplatis->noeuds.taille() == 0) {
            if (structure->est_polymorphe) {
                POUR (*structure->bloc_constantes->membres.verrou_lecture()) {
                    aplatis_arbre(it, arbre_aplatis->noeuds, {});
                }

                if (structure->est_corps_texte) {
                    return;
                }
            }

            aplatis_arbre(structure->bloc, arbre_aplatis->noeuds, {});
        }
        return;
    }

    if (declaration->est_exécute()) {
        auto exécute = declaration->comme_exécute();
        if (arbre_aplatis->noeuds.taille() == 0) {
            aplatis_arbre(exécute, arbre_aplatis->noeuds, {});
        }
        return;
    }

    if (declaration->est_déclaration_variable()) {
        auto declaration_variable = declaration->comme_déclaration_variable();
        if (arbre_aplatis->noeuds.taille() == 0) {
            aplatis_arbre(declaration_variable, arbre_aplatis->noeuds, {});
        }
        return;
    }

    if (declaration->est_déclaration_variable_multiple()) {
        auto declaration_variable = declaration->comme_déclaration_variable_multiple();
        if (arbre_aplatis->noeuds.taille() == 0) {
            aplatis_arbre(declaration_variable, arbre_aplatis->noeuds, {});
        }
        return;
    }

    if (declaration->est_déclaration_constante()) {
        auto declaration_variable = declaration->comme_déclaration_constante();
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
        case GenreNoeud::EXPRESSION_LITTÉRALE_NUL:
        case GenreNoeud::EXPRESSION_LITTÉRALE_CHAINE:
        case GenreNoeud::EXPRESSION_LITTÉRALE_CARACTÈRE:
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_RÉEL:
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_ENTIER:
        case GenreNoeud::EXPRESSION_LITTÉRALE_BOOLÉEN:
        case GenreNoeud::EXPRESSION_TYPE_DE:
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        case GenreNoeud::EXPRESSION_INFO_DE:
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_TYPE:
        case GenreNoeud::DIRECTIVE_INTROSPECTION:
        {
            return nullptr;
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_DÉCLARATION:
        {
            auto référence_déclaration = expression->comme_référence_déclaration();
            if (!référence_déclaration->déclaration_référée) {
                return référence_déclaration;
            }

            auto déclaration_référée = référence_déclaration->déclaration_référée;
            if (déclaration_référée->est_déclaration_type()) {
                return nullptr;
            }

            if (déclaration_référée->est_déclaration_module()) {
                return nullptr;
            }

            if (déclaration_référée->est_entête_fonction()) {
                return nullptr;
            }

            if (déclaration_référée->est_déclaration_constante()) {
                return nullptr;
            }

            return référence_déclaration;
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_MEMBRE:
        {
            auto référence_membre = expression->comme_référence_membre();
            auto accédé = référence_membre->accédée;
            if (auto expr_variable = trouve_expression_non_constante(accédé)) {
                return expr_variable;
            }

            if (accédé->est_référence_déclaration()) {
                if (accédé->comme_référence_déclaration()
                        ->déclaration_référée->est_déclaration_module()) {
                    assert(référence_membre->déclaration_référée);
                    return trouve_expression_non_constante(référence_membre->déclaration_référée);
                }
            }

            /* À FAIRE : vérifie proprement que nous avons un type InfoType. */
            if (accédé->est_info_de()) {
                return nullptr;
            }

            auto type_accédé = donne_type_accédé_effectif(accédé->type);

            if (type_accédé->est_type_type_de_données()) {
                if (référence_membre->genre_valeur == GenreValeur::DROITE) {
                    /* Nous accédons à une valeur constante. */
                    return nullptr;
                }

                type_accédé = type_accédé->comme_type_type_de_données()->type_connu;
                /* Puisque nous accédons à quelque chose, nous ne devrions pas avoir une
                 * type_de_données inconnu. */
                assert(type_accédé);
            }

            if (type_accédé->est_type_tableau_fixe()) {
                /* Seul l'accès à la taille est correcte, sinon nous ne serions pas ici. */
                return nullptr;
            }
            if (type_accédé->est_type_énum() || type_accédé->est_type_erreur()) {
                return nullptr;
            }
            auto type_compose = type_accédé->comme_type_composé();
            auto &membre = type_compose->membres[référence_membre->index_membre];

            if (membre.drapeaux == MembreTypeComposé::EST_CONSTANT) {
                return nullptr;
            }

            return référence_membre;
        }
        case GenreNoeud::EXPRESSION_PARENTHÈSE:
        {
            auto op = expression->comme_parenthèse();
            return trouve_expression_non_constante(op->expression);
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
        {
            auto op = expression->comme_construction_tableau();
            return trouve_expression_non_constante(op->expression);
        }
        case GenreNoeud::OPÉRATEUR_UNAIRE:
        {
            auto op = expression->comme_expression_unaire();
            return trouve_expression_non_constante(op->opérande);
        }
        case GenreNoeud::OPÉRATEUR_BINAIRE:
        case GenreNoeud::OPÉRATEUR_COMPARAISON_CHAINÉE:
        case GenreNoeud::EXPRESSION_INDEXAGE:
        {
            auto op = expression->comme_expression_binaire();

            if (auto expr_variable = trouve_expression_non_constante(op->opérande_gauche)) {
                return expr_variable;
            }

            if (auto expr_variable = trouve_expression_non_constante(op->opérande_droite)) {
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

            POUR (appel->paramètres_résolus) {
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
        case GenreNoeud::EXPRESSION_LITTÉRALE_NUL:
        case GenreNoeud::EXPRESSION_LITTÉRALE_CHAINE:
        case GenreNoeud::EXPRESSION_LITTÉRALE_CARACTÈRE:
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_RÉEL:
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_ENTIER:
        case GenreNoeud::EXPRESSION_LITTÉRALE_BOOLÉEN:
        case GenreNoeud::EXPRESSION_TYPE_DE:
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        case GenreNoeud::EXPRESSION_INFO_DE:
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_TYPE:
        case GenreNoeud::DIRECTIVE_INTROSPECTION:
        {
            return true;
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_DÉCLARATION:
        {
            auto référence_déclaration = expression->comme_référence_déclaration();
            if (!référence_déclaration->déclaration_référée) {
                return false;
            }

            auto déclaration_référée = référence_déclaration->déclaration_référée;
            if (déclaration_référée->est_déclaration_type()) {
                return true;
            }

            if (déclaration_référée->est_entête_fonction()) {
                return true;
            }

            if (déclaration_référée->possède_drapeau(DrapeauxNoeud::EST_GLOBALE)) {
                return true;
            }

            if (déclaration_référée->est_déclaration_constante()) {
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
        case GenreNoeud::EXPRESSION_PARENTHÈSE:
        {
            auto op = expression->comme_parenthèse();
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

            POUR (appel->paramètres_résolus) {
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

/* ------------------------------------------------------------------------- */
/** \name HiérarchieDeNoms
 * \{ */

HiérarchieDeNoms donne_hiérarchie_nom(NoeudDéclarationSymbole const *symbole)
{
    HiérarchieDeNoms résultat;

    kuri::ensemblon<NoeudExpression *, 6> noeuds_visités;

    résultat.hiérarchie.ajoute(symbole);

    if (symbole->est_déclaration_classe() && symbole->comme_déclaration_classe()->est_anonyme) {
        /* Place les unions anonymes dans le contexte globale car sinon nous aurions des
         * dépendances cycliques quand la première définition fut rencontrée dans le type de retour
         * d'une fonction. */
        return résultat;
    }

    auto bloc = symbole->bloc_parent;
    while (bloc) {
        if (bloc->appartiens_à_fonction) {
            if (!noeuds_visités.possède(bloc->appartiens_à_fonction)) {
                résultat.hiérarchie.ajoute(bloc->appartiens_à_fonction);
                noeuds_visités.insère(bloc->appartiens_à_fonction);
            }
        }
        else if (bloc->appartiens_à_type) {
            if (!noeuds_visités.possède(bloc->appartiens_à_type)) {
                résultat.hiérarchie.ajoute(bloc->appartiens_à_type);
                noeuds_visités.insère(bloc->appartiens_à_type);
            }
        }

        if (bloc->bloc_parent == nullptr) {
            /* Bloc du module. */
            résultat.ident_module = bloc->ident;
            break;
        }

        bloc = bloc->bloc_parent;
    }

    return résultat;
}

void imprime_hiérarchie_nom(HiérarchieDeNoms const &hiérarchie)
{
    dbg() << "============ Hiérarchie";

    if (hiérarchie.ident_module && hiérarchie.ident_module != ID::chaine_vide) {
        dbg() << "-- " << hiérarchie.ident_module->nom;
    }

    for (auto i = hiérarchie.hiérarchie.taille() - 1; i >= 0; i -= 1) {
        auto noeud = hiérarchie.hiérarchie[i];
        dbg() << "-- " << nom_humainement_lisible(noeud);
    }
}

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

/** \} */

// -----------------------------------------------------------------------------
// Implémentation des méthodes supplémentaires de l'arbre syntaxique

kuri::chaine_statique NoeudDéclarationEntêteFonction::donne_nom_broyé(Broyeuse &broyeuse)
{
    if (nom_broyé_ != "") {
        return nom_broyé_;
    }

    if (ident != ID::principale && !possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE |
                                                    DrapeauxNoeudFonction::FORCE_SANSBROYAGE)) {
        nom_broyé_ = broyeuse.broye_nom_fonction(this);
    }
    else if (données_externes) {
        nom_broyé_ = données_externes->nom_symbole;
    }
    else if (ident) {
        nom_broyé_ = ident->nom;
    }
    else {
        nom_broyé_ = lexème->chaine;
    }

    return nom_broyé_;
}

Type *NoeudDéclarationEntêteFonction::type_initialisé() const
{
    assert(possède_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE));
    return params[0]->type->comme_type_pointeur()->type_pointé;
}

int NoeudBloc::nombre_de_membres() const
{
    return membres->taille();
}

void NoeudBloc::réserve_membres(int nombre)
{
    membres->réserve(nombre);
}

static constexpr auto TAILLE_MAX_TABLEAU_MEMBRES = 16;

template <typename T>
using PointeurTableauVerrouille = typename kuri::tableau_synchrone<T>::PointeurVerrouille;

using TableMembres = kuri::table_hachage<IdentifiantCode const *, NoeudDéclaration *>;

static void ajoute_membre(TableMembres &table_membres, NoeudDéclaration *decl)
{
    /* Nous devons faire en sorte que seul le premier membre du nom est ajouté, afin que l'ensemble
     * de surcharge lui soit réservé, et que c'est ce membre qui est retourné. */
    if (table_membres.possède(decl->ident)) {
        return;
    }
    table_membres.insère(decl->ident, decl);
}

static void init_table_hachage_membres(PointeurTableauVerrouille<NoeudDéclaration *> &membres,
                                       TableMembres &table_membres)
{
    if (table_membres.taille() != 0) {
        return;
    }

    POUR (*membres) {
        ajoute_membre(table_membres, it);
    }
}

static void ajoute_à_ensemble_de_surcharge(NoeudDéclaration *decl, NoeudDéclaration *à_ajouter)
{
    if (decl->est_entête_fonction()) {
        auto entête_existante = decl->comme_entête_fonction();
        /* Garantis la présence de l'entête dans l'ensemble de surcharge. Ceci nous évitera d'avoir
         * une vérification séparée quand l'ensemble de surcharge est utilisé. */
        if (entête_existante->ensemble_de_surchages->taille() == 0) {
            entête_existante->ensemble_de_surchages->ajoute(entête_existante);
        }
        entête_existante->ensemble_de_surchages->ajoute(à_ajouter->comme_déclaration_symbole());
        return;
    }

    if (decl->est_déclaration_type()) {
        auto type_existant = decl->comme_déclaration_type();
        /* Garantis la présence du type dans l'ensemble de surcharge. Ceci nous évitera d'avoir une
         * vérification séparée quand l'ensemble de surcharge est utilisé. */
        if (type_existant->ensemble_de_surchages->taille() == 0) {
            type_existant->ensemble_de_surchages->ajoute(type_existant);
        }
        type_existant->ensemble_de_surchages->ajoute(à_ajouter->comme_déclaration_symbole());
        return;
    }

    assert_rappel(false, [&]() {
        dbg() << "Pas d'ensemble de surcharges pour " << decl->genre << ", "
              << nom_humainement_lisible(decl);
    });
}

void NoeudBloc::ajoute_membre(NoeudDéclaration *decl)
{
    if (decl->ident == ID::_ || decl->ident == nullptr) {
        /* Inutile d'avoir les variables ignorées ou les temporaires créées lors de la
         * canonicalisation ou la génération des initialisations des types comme membres du bloc.
         */
        return;
    }

    if (decl->est_déclaration_symbole()) {
        auto decl_existante = declaration_pour_ident(decl->ident);
        if (decl_existante && decl_existante->est_déclaration_symbole()) {
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

void NoeudBloc::ajoute_membre_au_debut(NoeudDéclaration *decl)
{
    auto membres_ = membres.verrou_ecriture();
    if (membres_->taille() >= TAILLE_MAX_TABLEAU_MEMBRES) {
        init_table_hachage_membres(membres_, table_membres);
        ::ajoute_membre(table_membres, decl);
    }

    membres_->ajoute_au_début(decl);
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

NoeudDéclaration *NoeudBloc::membre_pour_index(int index) const
{
    return membres->a(index);
}

NoeudDéclaration *NoeudBloc::declaration_pour_ident(IdentifiantCode const *ident_recherche) const
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

NoeudDéclaration *NoeudBloc::declaration_avec_meme_ident_que(NoeudExpression const *expr) const
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

kuri::tableau_statique<const MembreTypeComposé> NoeudDéclarationTypeComposé::
    donne_membres_pour_code_machine() const
{
    return {membres.begin(), nombre_de_membres_réels};
}

/* ------------------------------------------------------------------------- */
/** \name Implémentation des fonctions supplémentaires de l'AssembleuseArbre
 * \{ */

NoeudExpressionBinaire *AssembleuseArbre::crée_expression_binaire(const Lexème *lexeme,
                                                                  const OpérateurBinaire *op,
                                                                  NoeudExpression *expr1,
                                                                  NoeudExpression *expr2)
{
    assert(op);
    auto op_bin = crée_expression_binaire(lexeme, expr1, expr2);
    op_bin->op = op;
    op_bin->type = op->type_résultat;
    return op_bin;
}

NoeudExpressionRéférence *AssembleuseArbre::crée_référence_déclaration(const Lexème *lexeme,
                                                                       NoeudDéclaration *decl)
{
    auto ref = crée_référence_déclaration(lexeme);
    ref->déclaration_référée = decl;
    ref->type = decl->type;
    ref->ident = decl->ident;
    return ref;
}

NoeudSi *AssembleuseArbre::crée_si(const Lexème *lexeme, GenreNoeud genre_noeud)
{
    if (genre_noeud == GenreNoeud::INSTRUCTION_SI) {
        return crée_noeud<GenreNoeud::INSTRUCTION_SI>(lexeme)->comme_si();
    }

    return crée_noeud<GenreNoeud::INSTRUCTION_SAUFSI>(lexeme)->comme_saufsi();
}

NoeudBloc *AssembleuseArbre::crée_bloc_seul(const Lexème *lexeme, NoeudBloc *bloc_parent)
{
    auto bloc = crée_noeud<GenreNoeud::INSTRUCTION_COMPOSÉE>(lexeme)->comme_bloc();
    bloc->bloc_parent = bloc_parent;
    return bloc;
}

NoeudDéclarationVariable *AssembleuseArbre::crée_déclaration_variable(const Lexème *lexeme,
                                                                      Type *type,
                                                                      IdentifiantCode *ident,
                                                                      NoeudExpression *expression)
{
    auto decl = crée_déclaration_variable(lexeme, expression, nullptr);
    decl->ident = ident;
    decl->type = type;
    return decl;
}

NoeudDéclarationVariable *AssembleuseArbre::crée_déclaration_variable(
    NoeudExpressionRéférence *ref, NoeudExpression *expression)
{
    auto declaration = crée_déclaration_variable(ref->lexème, ref->type, ref->ident, expression);
    ref->déclaration_référée = declaration;
    return declaration;
}

NoeudDéclarationVariable *AssembleuseArbre::crée_déclaration_variable(
    NoeudExpressionRéférence *ref)
{
    auto decl = crée_déclaration_variable(ref, nullptr);
    ref->déclaration_référée = decl;
    return decl;
}

NoeudExpressionMembre *AssembleuseArbre::crée_référence_membre(const Lexème *lexeme,
                                                               NoeudExpression *accede,
                                                               Type *type,
                                                               int index)
{
    auto acces = crée_référence_membre(lexeme, accede);
    auto type_accédé = donne_type_accédé_effectif(accede->type);
    if (type_accédé->est_type_composé()) {
        auto type_composé = type_accédé->comme_type_composé();
        auto membre = type_composé->membres[index];
        acces->ident = membre.nom;
    }
    acces->type = type;
    acces->index_membre = index;
    return acces;
}

NoeudExpressionBinaire *AssembleuseArbre::crée_indexage(const Lexème *lexeme,
                                                        NoeudExpression *expr1,
                                                        NoeudExpression *expr2,
                                                        bool ignore_verification)
{
    auto indexage = crée_noeud<GenreNoeud::EXPRESSION_INDEXAGE>(lexeme)->comme_indexage();
    indexage->opérande_gauche = expr1;
    indexage->opérande_droite = expr2;
    indexage->type = type_déréférencé_pour(expr1->type);
    if (ignore_verification) {
        indexage->aide_génération_code = IGNORE_VERIFICATION;
    }
    return indexage;
}

NoeudExpressionAppel *AssembleuseArbre::crée_appel(const Lexème *lexeme,
                                                   NoeudExpression *appelee,
                                                   Type *type)
{
    auto expression = NoeudExpression::nul();
    if (appelee->est_entête_fonction()) {
        expression = crée_référence_déclaration(lexeme, appelee->comme_entête_fonction());
    }
    else {
        expression = appelee;
    }

    auto appel = crée_appel(lexeme, expression);
    appel->noeud_fonction_appelée = appelee;
    appel->type = type;
    return appel;
}

NoeudExpressionAppel *AssembleuseArbre::crée_construction_structure(const Lexème *lexeme,
                                                                    TypeCompose *type)
{
    auto structure = crée_appel(lexeme, type);
    structure->genre = GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE;
    structure->paramètres_résolus.réserve(type->membres.taille());
    structure->noeud_fonction_appelée = type;
    structure->type = type;
    return structure;
}

NoeudExpressionLittéraleEntier *AssembleuseArbre::crée_littérale_entier(Lexème const *lexeme,
                                                                        Type *type,
                                                                        uint64_t valeur)
{
    auto lit = crée_littérale_entier(lexeme);
    lit->type = type;
    lit->valeur = valeur;
    return lit;
}

NoeudExpressionLittéraleBool *AssembleuseArbre::crée_littérale_bool(Lexème const *lexeme,
                                                                    Type *type,
                                                                    bool valeur)
{
    auto lit = crée_littérale_bool(lexeme);
    lit->type = type;
    lit->valeur = valeur;
    return lit;
}

NoeudExpressionLittéraleRéel *AssembleuseArbre::crée_littérale_réel(Lexème const *lexeme,
                                                                    Type *type,
                                                                    double valeur)
{
    auto lit = crée_littérale_réel(lexeme);
    lit->type = type;
    lit->valeur = valeur;
    return lit;
}

NoeudExpression *AssembleuseArbre::crée_référence_type(Lexème const *lexeme, Type *type)
{
    auto ref_type = crée_référence_type(lexeme);
    ref_type->type = type;
    return ref_type;
}

NoeudAssignation *AssembleuseArbre::crée_incrementation(const Lexème *lexeme,
                                                        NoeudExpression *valeur)
{
    auto type = valeur->type;

    auto opérande_droite = NoeudExpression::nul();
    if (est_type_entier(type)) {
        opérande_droite = crée_littérale_entier(valeur->lexème, type, 1);
    }
    else if (type->est_type_réel()) {
        // À FAIRE(r16)
        opérande_droite = crée_littérale_réel(valeur->lexème, type, 1.0);
    }

    auto inc = crée_expression_binaire(lexeme, valeur, opérande_droite);
    inc->op = type->table_opérateurs->opérateur_ajt;
    assert(inc->op);
    inc->type = type;

    return crée_assignation_variable(valeur->lexème, valeur, inc);
}

NoeudAssignation *AssembleuseArbre::crée_decrementation(const Lexème *lexeme,
                                                        NoeudExpression *valeur)
{
    auto type = valeur->type;

    auto opérande_droite = NoeudExpression::nul();
    if (est_type_entier(type)) {
        opérande_droite = crée_littérale_entier(valeur->lexème, type, 1);
    }
    else if (type->est_type_réel()) {
        // À FAIRE(r16)
        opérande_droite = crée_littérale_réel(valeur->lexème, type, 1.0);
    }

    auto inc = crée_expression_binaire(lexeme, valeur, opérande_droite);
    inc->op = type->table_opérateurs->opérateur_sst;
    assert(inc->op);
    inc->type = type;

    return crée_assignation_variable(valeur->lexème, valeur, inc);
}

NoeudExpressionPriseAdresse *crée_prise_adresse(AssembleuseArbre *assem,
                                                Lexème const *lexème,
                                                NoeudExpression *expression,
                                                TypePointeur *type_résultat)
{
    assert(type_résultat->type_pointé == expression->type);

    auto résultat = assem->crée_prise_adresse(lexème, expression);
    résultat->type = type_résultat;
    return résultat;
}

NoeudDéclarationVariable *crée_retour_défaut_fonction(AssembleuseArbre *assembleuse,
                                                      Lexème const *lexème)
{
    auto type_declaré = assembleuse->crée_référence_type(lexème);

    auto déclaration_paramètre = assembleuse->crée_déclaration_variable(
        lexème, TypeBase::RIEN, ID::__ret0, nullptr);
    déclaration_paramètre->expression_type = type_declaré;
    déclaration_paramètre->drapeaux |= DrapeauxNoeud::EST_PARAMETRE;
    return déclaration_paramètre;
}

/** \} */

static const char *ordre_fonction(NoeudDéclarationEntêteFonction const *entete)
{
    if (entete->est_opérateur) {
        return "l'opérateur";
    }

    if (entete->est_coroutine) {
        return "la coroutine";
    }

    return "la fonction";
}

void imprime_détails_fonction(EspaceDeTravail *espace,
                              NoeudDéclarationEntêteFonction const *entête,
                              std::ostream &os)
{
    os << "Détail pour " << ordre_fonction(entête) << " " << entête->lexème->chaine << " :\n";
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
    if (noeud->est_entête_fonction()) {
        auto entete = noeud->comme_entête_fonction();

        if (entete->possède_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE)) {
            return enchaine("init_de(", chaine_type(entete->type_initialisé()), ")");
        }

        if (entete->est_opérateur) {
            if (entete->params.taille() == 2) {
                auto const type1 = entete->parametre_entree(0)->type;
                auto const type2 = entete->parametre_entree(1)->type;
                return enchaine("opérateur ",
                                entete->lexème->chaine,
                                " (",
                                chaine_type(type1),
                                ", ",
                                chaine_type(type2),
                                ")");
            }

            if (entete->params.taille() == 1) {
                auto const type1 = entete->parametre_entree(0)->type;
                return enchaine(
                    "opérateur ", entete->lexème->chaine, " (", chaine_type(type1), ")");
            }

            return enchaine("opérateur ", entete->lexème->chaine);
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

    if (noeud->est_déclaration_type()) {
        return chaine_type(noeud->comme_déclaration_type());
    }

    if (noeud->ident) {
        return noeud->ident->nom;
    }

    return "anonyme";
}

Type *donne_type_accédé_effectif(Type *type_accédé)
{
    /* nous pouvons avoir une référence d'un pointeur, donc déréférence au plus */
    while (type_accédé->est_type_pointeur() || type_accédé->est_type_référence()) {
        type_accédé = type_déréférencé_pour(type_accédé);
    }

    if (type_accédé->est_type_opaque()) {
        type_accédé = type_accédé->comme_type_opaque()->type_opacifié;
    }

    return type_accédé;
}

/* ------------------------------------------------------------------------- */
/** \name Fonctions d'initialisation des types.
 * \{ */

static Lexème lexème_sentinel = {};

NoeudDéclarationEntêteFonction *crée_entête_pour_initialisation_type(Type *type,
                                                                     AssembleuseArbre *assembleuse,
                                                                     Typeuse &typeuse)
{
    if (type->fonction_init) {
        return type->fonction_init;
    }

    if (type->est_type_énum()) {
        /* Les fonctions pour les types de bases durent être créées au début de la compilation. */
        assert(type->comme_type_énum()->type_sous_jacent->fonction_init);
        return type->comme_type_énum()->type_sous_jacent->fonction_init;
    }

    auto type_param = typeuse.type_pointeur_pour(type);
    if (type->est_type_union() && !type->comme_type_union()->est_nonsure) {
        type_param = typeuse.type_pointeur_pour(type, false, false);
    }

    auto types_entrées = kuri::tablet<Type *, 6>();
    types_entrées.ajoute(type_param);

    auto type_fonction = typeuse.type_fonction(types_entrées, TypeBase::RIEN, false);

    static Lexème lexème_entête = {};
    auto entête = assembleuse->crée_entête_fonction(&lexème_entête);
    entête->drapeaux_fonction |= DrapeauxNoeudFonction::EST_INITIALISATION_TYPE;

    entête->bloc_constantes = assembleuse->crée_bloc_seul(&lexème_sentinel, nullptr);
    entête->bloc_paramètres = assembleuse->crée_bloc_seul(&lexème_sentinel,
                                                          entête->bloc_constantes);

    /* Paramètre d'entrée. */
    {
        static Lexème lexème_déclaration = {};
        auto déclaration_paramètre = assembleuse->crée_déclaration_variable(
            &lexème_déclaration, type_param, ID::pointeur, nullptr);
        déclaration_paramètre->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;

        if (type->est_type_référence()) {
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
        static const Lexème lexème_rien = {"rien", {}, GenreLexème::RIEN, 0, 0, 0};
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
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        case GenreNoeud::TYPE_TRANCHE:
        case GenreNoeud::VARIADIQUE:
        case GenreNoeud::DÉCLARATION_UNION:
        {
            auto prise_adresse = crée_prise_adresse(
                assembleuse, &lexème_sentinel, ref_param, typeuse.type_pointeur_pour(type));
            auto fonction = crée_entête_pour_initialisation_type(type, assembleuse, typeuse);
            auto appel = assembleuse->crée_appel(&lexème_sentinel, fonction, TypeBase::RIEN);
            appel->paramètres_résolus.ajoute(prise_adresse);
            assembleuse->bloc_courant()->ajoute_expression(appel);
            break;
        }
        case GenreNoeud::BOOL:
        {
            static Lexème littéral_bool = {};
            littéral_bool.genre = GenreLexème::FAUX;
            auto valeur_défaut = assembleuse->crée_littérale_bool(&littéral_bool);
            valeur_défaut->type = type;
            crée_assignation(assembleuse, ref_param, valeur_défaut);
            break;
        }
        case GenreNoeud::OCTET:
        case GenreNoeud::ENTIER_CONSTANT:
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::ENTIER_RELATIF:
        case GenreNoeud::TYPE_DE_DONNÉES:
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        {
            auto valeur_défaut = assembleuse->crée_littérale_entier(&lexème_sentinel, type, 0);
            crée_assignation(assembleuse, ref_param, valeur_défaut);
            break;
        }
        case GenreNoeud::RÉEL:
        {
            auto valeur_défaut = assembleuse->crée_littérale_réel(&lexème_sentinel, type, 0);
            crée_assignation(assembleuse, ref_param, valeur_défaut);
            break;
        }
        case GenreNoeud::RÉFÉRENCE:
        {
            break;
        }
        case GenreNoeud::POINTEUR:
        case GenreNoeud::FONCTION:
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        {
            auto valeur_défaut = assembleuse->crée_littérale_nul(&lexème_sentinel);
            valeur_défaut->type = ref_param->type;
            crée_assignation(assembleuse, ref_param, valeur_défaut);
            break;
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_type_tableau_fixe();
            auto type_élément = type_tableau->type_pointé;

            auto type_pointeur_type_pointe = typeuse.type_pointeur_pour(
                type_élément, false, false);

            /* NOTE: pour les tableaux fixes, puisque le déréférencement de pointeur est compliqué
             * avec les indexages, nous passons par une variable locale temporaire et copierons la
             * variable initialisée dans la mémoire pointée par le paramètre. */
            auto valeur_résultat = assembleuse->crée_déclaration_variable(
                &lexème_sentinel,
                type_tableau,
                nullptr,
                assembleuse->crée_non_initialisation(&lexème_sentinel));
            assembleuse->bloc_courant()->ajoute_expression(valeur_résultat);
            auto ref_résultat = assembleuse->crée_référence_déclaration(&lexème_sentinel,
                                                                        valeur_résultat);

            /* Toutes les variables doivent être initialisées (ou nous devons nous assurer que tous
             * les types possibles créés par la compilation ont une fonction d'initalisation). */
            auto init_it = assembleuse->crée_littérale_nul(&lexème_sentinel);
            init_it->type = type_pointeur_type_pointe;

            auto decl_it = assembleuse->crée_déclaration_variable(
                &lexème_sentinel, type_pointeur_type_pointe, ID::it, init_it);
            auto ref_it = assembleuse->crée_référence_déclaration(&lexème_sentinel, decl_it);

            auto variable = assembleuse->crée_virgule(&lexème_sentinel);
            variable->expressions.ajoute(decl_it);

            // il nous faut créer une boucle sur le tableau.
            // pour * tableau { initialise_type(it); }
            auto pour = assembleuse->crée_pour(&lexème_sentinel, variable, ref_résultat);
            pour->prend_pointeur = true;
            pour->bloc = assembleuse->crée_bloc(&lexème_sentinel);
            pour->aide_génération_code = GENERE_BOUCLE_TABLEAU;
            pour->decl_it = decl_it;
            pour->decl_index_it = assembleuse->crée_déclaration_variable(
                &lexème_sentinel, TypeBase::Z64, ID::index_it, nullptr);

            auto fonction = crée_entête_pour_initialisation_type(
                type_élément, assembleuse, typeuse);
            auto appel = assembleuse->crée_appel(&lexème_sentinel, fonction, TypeBase::RIEN);
            appel->paramètres_résolus.ajoute(ref_it);

            pour->bloc->ajoute_expression(appel);

            assembleuse->bloc_courant()->ajoute_expression(pour);

            auto assignation_résultat = assembleuse->crée_assignation_variable(
                &lexème_sentinel, ref_param, ref_résultat);
            assembleuse->bloc_courant()->ajoute_expression(assignation_résultat);
            break;
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            auto opaque = type->comme_type_opaque();
            auto type_opacifié = opaque->type_opacifié;

            // Transtype vers le type opacifié, et crée l'initialisation pour le type opacifié.
            auto prise_adresse = crée_prise_adresse(
                assembleuse, &lexème_sentinel, ref_param, typeuse.type_pointeur_pour(type));

            auto comme = assembleuse->crée_comme(&lexème_sentinel, prise_adresse, nullptr);
            comme->type = typeuse.type_pointeur_pour(type_opacifié);
            comme->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, comme->type};

            auto fonc_init = crée_entête_pour_initialisation_type(
                type_opacifié, assembleuse, typeuse);
            auto appel = assembleuse->crée_appel(&lexème_sentinel, fonc_init, TypeBase::RIEN);
            appel->paramètres_résolus.ajoute(comme);
            assembleuse->bloc_courant()->ajoute_expression(appel);
            break;
        }
        case GenreNoeud::TUPLE:
        {
            // Les tuples ne sont que pour représenter les sorties des fonctions, ils ne devraient
            // pas avoir d'initialisation.
            break;
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
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
                                     NoeudDéclarationEntêteFonction *entete)
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

    if (type->est_type_énum()) {
        assigne_fonction_init_énum(typeuse, type->comme_type_énum());
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

    auto entête = crée_entête_pour_initialisation_type(type, assembleuse, typeuse);

    sauvegarde_fonction_init(typeuse, type, entête);

    auto corps = entête->corps;
    corps->aide_génération_code = REQUIERS_CODE_EXTRA_RETOUR;

    corps->bloc = assembleuse->crée_bloc_seul(&lexème_sentinel, entête->bloc_paramètres);

    assert(assembleuse->bloc_courant() == nullptr);
    assembleuse->bloc_courant(corps->bloc);

    auto decl_param = entête->params[0]->comme_déclaration_variable();
    auto ref_param = assembleuse->crée_référence_déclaration(&lexème_sentinel, decl_param);

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
        case GenreNoeud::TYPE_DE_DONNÉES:
        case GenreNoeud::RÉEL:
        case GenreNoeud::RÉFÉRENCE:
        case GenreNoeud::POINTEUR:
        case GenreNoeud::FONCTION:
        case GenreNoeud::TABLEAU_FIXE:
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        {
            auto deref = assembleuse->crée_mémoire(&lexème_sentinel, ref_param);
            deref->type = type;
            crée_initialisation_defaut_pour_type(type, assembleuse, deref, nullptr, typeuse);
            break;
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            auto type_opacifié = type->comme_type_opaque()->type_opacifié;
            auto type_pointeur_opacifié = typeuse.type_pointeur_pour(type_opacifié);

            auto comme_type_opacifie = assembleuse->crée_comme(
                &lexème_sentinel, ref_param, nullptr);
            comme_type_opacifie->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                                                   type_pointeur_opacifié};
            comme_type_opacifie->type = type_pointeur_opacifié;

            auto deref = assembleuse->crée_mémoire(&lexème_sentinel, comme_type_opacifie);
            deref->type = type_opacifié;

            crée_initialisation_defaut_pour_type(
                type_opacifié, assembleuse, deref, nullptr, typeuse);
            break;
        }
        case GenreNoeud::EINI:
        case GenreNoeud::CHAINE:
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        case GenreNoeud::TYPE_TRANCHE:
        case GenreNoeud::VARIADIQUE:
        {
            auto type_composé = type->comme_type_composé();

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

                auto ref_membre = assembleuse->crée_référence_membre(
                    &lexème_sentinel, ref_param, it.type, index_it);
                crée_initialisation_defaut_pour_type(
                    it.type, assembleuse, ref_membre, it.expression_valeur_defaut, typeuse);
            }

            break;
        }
        case GenreNoeud::DÉCLARATION_UNION:
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
                auto transtype = assembleuse->crée_comme(&lexème_sentinel, ref_param, nullptr);
                transtype->transformation = TransformationType{
                    TypeTransformation::CONVERTI_VERS_TYPE_CIBLE,
                    typeuse.type_pointeur_pour(membre.type)};
                transtype->type = const_cast<Type *>(transtype->transformation.type_cible);

                auto deref = assembleuse->crée_mémoire(&lexème_sentinel, transtype);
                deref->type = membre.type;

                crée_initialisation_defaut_pour_type(
                    membre.type, assembleuse, deref, membre.expression_valeur_defaut, typeuse);
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

                auto param_comme_structure = assembleuse->crée_comme(
                    &lexème_sentinel, ref_param, nullptr);
                param_comme_structure->type = type_pointeur_type_structure;
                param_comme_structure->transformation = TransformationType{
                    TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_pointeur_type_structure};

                if (membre.type->est_type_rien()) {
                    /* Seul l'index doit être initialisé. (Support union ne contenant que « rien »
                     * comme types des membres). */
                    auto ref_membre = assembleuse->crée_référence_membre(&lexème_sentinel,
                                                                         param_comme_structure);
                    ref_membre->index_membre = 0;
                    ref_membre->type = TypeBase::Z32;
                    ref_membre->aide_génération_code = IGNORE_VERIFICATION;
                    crée_initialisation_defaut_pour_type(
                        TypeBase::Z32, assembleuse, ref_membre, nullptr, typeuse);
                    break;
                }

                auto ref_membre = assembleuse->crée_référence_membre(&lexème_sentinel,
                                                                     param_comme_structure);
                ref_membre->index_membre = 0;
                ref_membre->type = membre.type;
                ref_membre->aide_génération_code = IGNORE_VERIFICATION;
                crée_initialisation_defaut_pour_type(membre.type,
                                                     assembleuse,
                                                     ref_membre,
                                                     membre.expression_valeur_defaut,
                                                     typeuse);

                ref_membre = assembleuse->crée_référence_membre(&lexème_sentinel,
                                                                param_comme_structure);
                ref_membre->index_membre = 1;
                ref_membre->type = TypeBase::Z32;
                ref_membre->aide_génération_code = IGNORE_VERIFICATION;
                crée_initialisation_defaut_pour_type(
                    TypeBase::Z32, assembleuse, ref_membre, nullptr, typeuse);
            }

            break;
        }
        case GenreNoeud::TUPLE:
        {
            // Les tuples ne sont que pour représenter les sorties des fonctions, ils ne devraient
            // pas avoir d'initialisation.
            break;
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
            break;
        }
    }

    assembleuse->dépile_bloc();
    simplifie_arbre(espace, assembleuse, typeuse, entête);
    assigne_fonction_init(type, entête);
    corps->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
}

/** \} */

bool possède_annotation(const BaseDéclarationVariable *decl, kuri::chaine_statique annotation)
{
    POUR (decl->annotations) {
        if (it.nom == annotation) {
            return true;
        }
    }

    return false;
}

bool est_déclaration_polymorphique(NoeudDéclaration const *decl)
{
    if (decl->est_entête_fonction()) {
        auto const entete = decl->comme_entête_fonction();
        return entete->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE);
    }

    if (decl->est_type_structure()) {
        auto const structure = decl->comme_type_structure();
        return structure->est_polymorphe;
    }

    if (decl->est_type_union()) {
        auto const structure = decl->comme_type_union();
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
    NoeudDéclarationEntêteFonction const *fonction)
{
    if (!fonction) {
        return true;
    }

    if (fonction->bloc_constantes->appartiens_à_fonction != fonction) {
        return false;
    }
    if (fonction->bloc_paramètres->appartiens_à_fonction != fonction) {
        return false;
    }
    if (fonction->bloc_paramètres->bloc_parent != fonction->bloc_constantes) {
        return false;
    }
    if (fonction->corps->bloc->appartiens_à_fonction != fonction) {
        return false;
    }
    if (fonction->corps->bloc->bloc_parent != fonction->bloc_paramètres) {
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
    if (noeud->est_entête_fonction()) {
        return &noeud->comme_entête_fonction()->unité;
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
    if (noeud->est_déclaration_variable()) {
        return &noeud->comme_déclaration_variable()->unité;
    }
    if (noeud->est_déclaration_variable_multiple()) {
        return &noeud->comme_déclaration_variable_multiple()->unité;
    }
    if (noeud->est_exécute()) {
        return &noeud->comme_exécute()->unité;
    }
    if (noeud->est_déclaration_classe()) {
        return &noeud->comme_déclaration_classe()->unité;
    }
    if (noeud->est_déclaration_bibliothèque()) {
        return &noeud->comme_déclaration_bibliothèque()->unité;
    }
    if (noeud->est_type_opaque()) {
        return &noeud->comme_type_opaque()->unité;
    }
    if (noeud->est_dépendance_bibliothèque()) {
        return &noeud->comme_dépendance_bibliothèque()->unité;
    }
    if (noeud->est_type_énum()) {
        return &noeud->comme_type_énum()->unité;
    }
    if (noeud->est_ajoute_fini()) {
        return &noeud->comme_ajoute_fini()->unité;
    }
    if (noeud->est_ajoute_init()) {
        return &noeud->comme_ajoute_init()->unité;
    }
    if (noeud->est_déclaration_constante()) {
        return &noeud->comme_déclaration_constante()->unité;
    }

    assert_rappel(false, [&]() {
        dbg() << "Noeud non-géré pour l'adresse de l'unité de compilation " << noeud->genre;
    });
    return nullptr;
}
