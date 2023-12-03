/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include "expression.hh"

#include "biblinternes/outils/conditions.h"

#include "parsage/identifiant.hh"
#include "parsage/outils_lexemes.hh"

#include "compilation/compilatrice.hh"
#include "compilation/portee.hh"

#include "noeud_expression.hh"

/* À FAIRE: les expressions littérales des énumérations ne sont pas validées donc les valeurs sont
 * toujours sur les lexèmes */

/* ************************************************************************** */

template <typename T>
static bool applique_négation_logique(T a)
{
    return !a;
}

template <typename T>
static auto applique_operateur_unaire(GenreLexeme id, T a)
{
    switch (id) {
        case GenreLexeme::TILDE:
        {
            return ~a;
        }
        case GenreLexeme::PLUS_UNAIRE:
        {
            return a;
        }
        case GenreLexeme::MOINS_UNAIRE:
        {
            return -a;
        }
        default:
        {
            return T(0);
        }
    }
}

static auto applique_operateur_unaire(GenreLexeme id, double a)
{
    switch (id) {
        case GenreLexeme::PLUS_UNAIRE:
        {
            return a;
        }
        case GenreLexeme::MOINS_UNAIRE:
        {
            return -a;
        }
        default:
        {
            return 0.0;
        }
    }
}

template <typename T>
static auto applique_operateur_binaire(GenreLexeme id, T a, T b)
{
    switch (id) {
        case GenreLexeme::PLUS:
        case GenreLexeme::PLUS_EGAL:
        {
            return a + b;
        }
        case GenreLexeme::MOINS:
        case GenreLexeme::MOINS_EGAL:
        {
            return a - b;
        }
        case GenreLexeme::FOIS:
        case GenreLexeme::MULTIPLIE_EGAL:
        {
            return a * b;
        }
        case GenreLexeme::DIVISE:
        case GenreLexeme::DIVISE_EGAL:
        {
            return a / b;
        }
        case GenreLexeme::POURCENT:
        case GenreLexeme::MODULO_EGAL:
        {
            return a % b;
        }
        case GenreLexeme::ESPERLUETTE:
        case GenreLexeme::ET_EGAL:
        {
            return a & b;
        }
        case GenreLexeme::OU_EGAL:
        case GenreLexeme::BARRE:
        {
            return a | b;
        }
        case GenreLexeme::CHAPEAU:
        case GenreLexeme::OUX_EGAL:
        {
            return a ^ b;
        }
        case GenreLexeme::DECALAGE_DROITE:
        case GenreLexeme::DEC_DROITE_EGAL:
        {
            return a >> b;
        }
        case GenreLexeme::DECALAGE_GAUCHE:
        case GenreLexeme::DEC_GAUCHE_EGAL:
        {
            return a << b;
        }
        default:
        {
            return T(0);
        }
    }
}

static auto applique_operateur_binaire(GenreLexeme id, double a, double b)
{
    switch (id) {
        case GenreLexeme::PLUS:
        case GenreLexeme::PLUS_EGAL:
        {
            return a + b;
        }
        case GenreLexeme::MOINS:
        case GenreLexeme::MOINS_EGAL:
        {
            return a - b;
        }
        case GenreLexeme::FOIS:
        case GenreLexeme::MULTIPLIE_EGAL:
        {
            return a * b;
        }
        case GenreLexeme::DIVISE:
        case GenreLexeme::DIVISE_EGAL:
        {
            return a / b;
        }
        default:
        {
            return 0.0;
        }
    }
}

template <typename T>
static auto applique_operateur_binaire_comp(GenreLexeme id, T a, T b)
{
    switch (id) {
        case GenreLexeme::INFERIEUR:
        {
            return a < b;
        }
        case GenreLexeme::INFERIEUR_EGAL:
        {
            return a <= b;
        }
        case GenreLexeme::SUPERIEUR:
        {
            return a > b;
        }
        case GenreLexeme::SUPERIEUR_EGAL:
        {
            return a >= b;
        }
        case GenreLexeme::DIFFERENCE:
        {
            return a != b;
        }
        case GenreLexeme::EGALITE:
        {
            return a == b;
        }
        default:
        {
            return false;
        }
    }
}

static bool est_type_opaque_utilisable_pour_constante(Type const *type)
{
    if (est_type_entier(type)) {
        return true;
    }

    if (type->est_type_reel()) {
        return true;
    }

    if (type->est_type_bool()) {
        return true;
    }

    if (type->est_type_entier_constant()) {
        return true;
    }

    if (type->est_type_enum() || type->est_type_erreur()) {
        return true;
    }

    return false;
}

static ResultatExpression erreur_evaluation(const NoeudExpression *b, const char *message)
{
    auto res = ResultatExpression();
    res.est_errone = true;
    res.noeud_erreur = b;
    res.message_erreur = message;
    return res;
}

/**
 * Évalue l'expression dont « b » est la racine. L'expression doit être
 * constante, c'est à dire ne contenir que des noeuds dont la valeur est connue
 * lors de la compilation.
 */
ResultatExpression evalue_expression(const Compilatrice &compilatrice,
                                     NoeudBloc *bloc,
                                     const NoeudExpression *b)
{
    switch (b->genre) {
        default:
        {
            return erreur_evaluation(
                b, "L'expression n'est pas constante et ne peut être calculée !");
        }
        case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
        case GenreNoeud::EXPRESSION_TYPE_DE:
        {
            return ValeurExpression(b->type->comme_type_type_de_donnees()->type_connu);
        }
        case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
        {
            auto fichier = compilatrice.fichier(b->lexeme->fichier);
            auto decl = trouve_dans_bloc_ou_module(
                bloc, b->ident, fichier, bloc->appartiens_à_fonction);

            if (decl == nullptr) {
                return erreur_evaluation(b, "La variable n'existe pas !");
            }

            if (decl->est_entete_fonction()) {
                return ValeurExpression(decl->comme_entete_fonction());
            }

            if (!decl->est_declaration_constante()) {
                return erreur_evaluation(
                    b, "La référence n'est pas celle d'une variable constante !");
            }

            auto decl_var = decl->comme_declaration_constante();

            if (decl_var->valeur_expression.est_valide()) {
                return decl_var->valeur_expression;
            }

            if (decl_var->expression == nullptr) {
                if (decl_var->type->est_type_enum()) {
                    auto type_enum = static_cast<TypeEnum *>(decl_var->type);

                    auto info_membre = donne_membre_pour_nom(type_enum, decl_var->ident);
                    if (info_membre.has_value()) {
                        return ValeurExpression(info_membre->membre.valeur);
                    }
                }

                if (decl_var->type->est_type_type_de_donnees()) {
                    auto type_de_données = decl_var->type->comme_type_type_de_donnees();
                    if (type_de_données->type_connu == nullptr) {
                        return erreur_evaluation(
                            b, "La déclaration n'a pas de type de données connu !");
                    }

                    return ValeurExpression(type_de_données->type_connu);
                }

                return erreur_evaluation(b,
                                         "La déclaration de la variable n'a pas d'expression !");
            }

            return evalue_expression(compilatrice, decl->bloc_parent, decl_var->expression);
        }
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        {
            auto expr_taille_de = b->comme_taille_de();
            auto type = expr_taille_de->expression->type;
            return ValeurExpression(type->taille_octet);
        }
        case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
        {
            return ValeurExpression(b->lexeme->chaine == "vrai");
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
        {
            /* Si le noeud provient d'un résultat, le lexème ne peut être utilisé pour extraire la
             * valeur car ce n'est pas un lexème de code source. */
            if (b->possède_drapeau(DrapeauxNoeud::NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE)) {
                return ValeurExpression(static_cast<int64_t>(b->comme_litterale_entier()->valeur));
            }

            return ValeurExpression(static_cast<int64_t>(b->lexeme->valeur_entiere));
        }
        case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
        {
            /* Si le noeud provient d'un résultat, le lexème ne peut être utilisé pour extraire la
             * valeur car ce n'est pas un lexème de code source. */
            if (b->possède_drapeau(DrapeauxNoeud::NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE)) {
                return ValeurExpression(static_cast<int64_t>(b->comme_litterale_entier()->valeur));
            }

            return ValeurExpression(static_cast<int64_t>(b->lexeme->valeur_entiere));
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
        {
            /* Si le noeud provient d'un résultat, le lexème ne peut être utilisé pour extraire la
             * valeur car ce n'est pas un lexème de code source. */
            if (b->possède_drapeau(DrapeauxNoeud::NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE)) {
                return ValeurExpression(b->comme_litterale_reel()->valeur);
            }

            return ValeurExpression(b->lexeme->valeur_reelle);
        }
        case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
        {
            return ValeurExpression(b->comme_litterale_chaine());
        }
        case GenreNoeud::INSTRUCTION_SAUFSI:
        case GenreNoeud::INSTRUCTION_SI:
        {
            auto inst = static_cast<const NoeudSi *>(b);
            auto res = evalue_expression(compilatrice, bloc, inst->condition);

            if (res.est_errone) {
                return res;
            }

            if (!res.valeur.est_booleenne()) {
                return erreur_evaluation(b, "L'expression n'est pas de type booléen !");
            }

            if (res.valeur.booleenne() == (b->genre == GenreNoeud::INSTRUCTION_SI)) {
                res = evalue_expression(compilatrice, bloc, inst->bloc_si_vrai);
            }
            else {
                if (inst->bloc_si_faux) {
                    res = evalue_expression(compilatrice, bloc, inst->bloc_si_faux);
                }
            }

            return res;
        }
        case GenreNoeud::OPERATEUR_UNAIRE:
        {
            auto inst = b->comme_expression_unaire();
            auto res = evalue_expression(compilatrice, bloc, inst->operande);

            if (res.est_errone) {
                return res;
            }

            if (res.valeur.est_reelle()) {
                res.valeur = applique_operateur_unaire(inst->lexeme->genre, res.valeur.reelle());
            }
            else if (res.valeur.est_entiere()) {
                res.valeur = applique_operateur_unaire(inst->lexeme->genre, res.valeur.entiere());
            }

            return res;
        }
        case GenreNoeud::EXPRESSION_NEGATION_LOGIQUE:
        {
            auto négation = b->comme_negation_logique();
            auto res = evalue_expression(compilatrice, bloc, négation->opérande);
            if (res.est_errone) {
                return res;
            }

            if (res.valeur.est_reelle()) {
                res.valeur = applique_négation_logique(res.valeur.reelle());
            }
            else if (res.valeur.est_booleenne()) {
                res.valeur = applique_négation_logique(res.valeur.booleenne());
            }
            else if (res.valeur.est_entiere()) {
                res.valeur = applique_négation_logique(res.valeur.entiere());
            }
            else if (res.valeur.est_chaine()) {
                auto chaine = res.valeur.chaine();
                res.valeur = chaine->lexeme->chaine.taille() == 0;
            }
            else {
                return erreur_evaluation(b,
                                         "L'expression n'est pas évaluable car l'opération "
                                         "n'est pas applicable sur le type de l'opérande.");
            }

            return res;
        }
        case GenreNoeud::OPERATEUR_BINAIRE:
        {
            auto inst = b->comme_expression_binaire();
            auto res1 = evalue_expression(compilatrice, bloc, inst->operande_gauche);

            if (res1.est_errone) {
                return res1;
            }

            auto res2 = evalue_expression(compilatrice, bloc, inst->operande_droite);

            if (res2.est_errone) {
                return res2;
            }

            ValeurExpression res = ValeurExpression();

            if (est_opérateur_bool(inst->lexeme->genre)) {
                if (res1.valeur.est_reelle() && res2.valeur.est_reelle()) {
                    res = applique_operateur_binaire_comp(
                        inst->lexeme->genre, res1.valeur.reelle(), res2.valeur.reelle());
                }
                else if (res1.valeur.est_booleenne() && res2.valeur.est_booleenne()) {
                    res = applique_operateur_binaire_comp(
                        inst->lexeme->genre, res1.valeur.booleenne(), res2.valeur.booleenne());
                }
                else if (res1.valeur.est_entiere() && res2.valeur.est_entiere()) {
                    res = applique_operateur_binaire_comp(
                        inst->lexeme->genre, res1.valeur.entiere(), res2.valeur.entiere());
                }
                else if (res1.valeur.est_type() && res2.valeur.est_type()) {
                    res = applique_operateur_binaire_comp(
                        inst->lexeme->genre, res1.valeur.type(), res2.valeur.type());
                }
                else {
                    return erreur_evaluation(b,
                                             "L'expression n'est pas évaluable car l'opération "
                                             "n'est pas applicable sur les types données.");
                }
            }
            else {
                if (res1.valeur.est_reelle() && res2.valeur.est_reelle()) {
                    res = applique_operateur_binaire(
                        inst->lexeme->genre, res1.valeur.reelle(), res2.valeur.reelle());
                }
                else if (res1.valeur.est_entiere() && res2.valeur.est_entiere()) {
                    res = applique_operateur_binaire(
                        inst->lexeme->genre, res1.valeur.entiere(), res2.valeur.entiere());
                }
                else {
                    return erreur_evaluation(b,
                                             "L'expression n'est pas évaluable car l'opération "
                                             "n'est pas applicable sur les types données.");
                }
            }

            return res;
        }
        case GenreNoeud::EXPRESSION_LOGIQUE:
        {
            auto logique = b->comme_expression_logique();

            auto res1 = evalue_expression(compilatrice, bloc, logique->opérande_gauche);
            if (res1.est_errone) {
                return res1;
            }
            if (!res1.valeur.est_booleenne()) {
                return erreur_evaluation(
                    logique->opérande_gauche,
                    "L'expression n'est pas évaluable car elle n'est pas de type booléen.");
            }

            auto res2 = evalue_expression(compilatrice, bloc, logique->opérande_droite);
            if (res2.est_errone) {
                return res2;
            }
            if (!res2.valeur.est_booleenne()) {
                return erreur_evaluation(
                    logique->opérande_droite,
                    "L'expression n'est pas évaluable car elle n'est pas de type booléen.");
            }

            ValeurExpression res = ValeurExpression();
            if (logique->lexeme->genre == GenreLexeme::ESP_ESP) {
                res = res1.valeur.booleenne() && res2.valeur.booleenne();
            }
            else {
                res = res1.valeur.booleenne() || res2.valeur.booleenne();
            }
            return res;
        }
        case GenreNoeud::EXPRESSION_PARENTHESE:
        {
            auto inst = b->comme_parenthese();
            return evalue_expression(compilatrice, bloc, inst->expression);
        }
        case GenreNoeud::EXPRESSION_COMME:
        {
            /* À FAIRE : transtypage de l'expression constante */
            auto inst = b->comme_comme();
            return evalue_expression(compilatrice, bloc, inst->expression);
        }
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
        {
            auto ref_membre = b->comme_reference_membre();
            auto type_accede = ref_membre->accedee->type;

            if (type_accede->est_type_enum() || type_accede->est_type_erreur()) {
                auto type_enum = static_cast<TypeEnum *>(type_accede);
                auto valeur_enum = type_enum->membres[ref_membre->index_membre].valeur;
                return ValeurExpression(valeur_enum);
            }

            if (type_accede->est_type_tableau_fixe()) {
                if (ref_membre->ident == ID::taille) {
                    return ValeurExpression(type_accede->comme_type_tableau_fixe()->taille);
                }
            }

            return erreur_evaluation(
                b, "L'expression n'est pas constante et ne peut être calculée !");
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
        {
            return ValeurExpression(b->comme_construction_tableau());
        }
        case GenreNoeud::DIRECTIVE_CUISINE:
        {
            auto cuisine = b->comme_cuisine();
            auto expr = cuisine->expression;
            auto appel = expr->comme_appel();
            auto fonction = appel->expression->comme_entete_fonction();

            return ValeurExpression(fonction);
        }
        case GenreNoeud::EXPRESSION_APPEL:
        {
            auto expression_appel = b->comme_appel();

            if (expression_appel->aide_generation_code != CONSTRUIT_OPAQUE) {
                return erreur_evaluation(b,
                                         "Impossible d'utiliser une expression d'appel qui n'est "
                                         "pas la construction d'un type opaque");
            }

            auto type_opacifié = b->type->comme_type_opaque()->type_opacifie;

            if (!est_type_opaque_utilisable_pour_constante(type_opacifié)) {
                return erreur_evaluation(b,
                                         "Impossible de construire une expression constante "
                                         "depuis un type opacifié n'étant pas un type entier, "
                                         "booléen, réel, énumération, ou type erreur");
            }

            auto const nombre_de_paramètres = expression_appel->parametres_resolus.taille();
            if (nombre_de_paramètres == 0) {
                return erreur_evaluation(b,
                                         "Impossible de construire une expression constante "
                                         "depuis un type opacifié sans paramètre");
            }

            if (nombre_de_paramètres > 1) {
                return erreur_evaluation(b,
                                         "Impossible de construire une expression constante "
                                         "depuis un type opacifié avec plusieurs paramètres");
            }

            /* L'assignation du type opaque à la valeur se fera par quiconque nous a appelé. */
            return evalue_expression(compilatrice, bloc, expression_appel->parametres_resolus[0]);
        }
    }
}

std::ostream &operator<<(std::ostream &os, ValeurExpression valeur)
{
    if (valeur.est_booleenne()) {
        os << valeur.booleenne();
    }
    else if (valeur.est_entiere()) {
        os << valeur.entiere();
    }
    else if (valeur.est_reelle()) {
        os << valeur.reelle();
    }
    else if (valeur.est_chaine()) {
        auto chaine = valeur.chaine();
        os << chaine->lexeme->chaine;
    }
    else if (valeur.est_tableau_fixe()) {
        os << "[...]";
    }
    else if (valeur.est_fonction()) {
        auto const fonction = valeur.fonction();
        os << (fonction->ident ? fonction->ident->nom : "");
    }
    else {
        os << "invalide";
    }
    return os;
}
