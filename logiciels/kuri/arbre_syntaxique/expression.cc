/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include "expression.hh"

#include "parsage/identifiant.hh"
#include "parsage/outils_lexemes.hh"

#include "compilation/compilatrice.hh"
#include "compilation/portee.hh"

#include "noeud_expression.hh"

#include "log.hh"

/* À FAIRE: les expressions littérales des énumérations ne sont pas validées donc les valeurs sont
 * toujours sur les lexèmes */

#include "plateforme/windows.h"

/* ************************************************************************** */

template <typename T>
static bool applique_négation_logique(T a)
{
    return !a;
}

template <typename T>
static auto applique_opérateur_unaire(GenreLexème id, T a)
{
    switch (id) {
        case GenreLexème::TILDE:
        {
            return ~a;
        }
        case GenreLexème::PLUS_UNAIRE:
        {
            return a;
        }
        case GenreLexème::MOINS_UNAIRE:
        {
            return -a;
        }
        default:
        {
            return T(0);
        }
    }
}

static auto applique_opérateur_unaire(GenreLexème id, double a)
{
    switch (id) {
        case GenreLexème::PLUS_UNAIRE:
        {
            return a;
        }
        case GenreLexème::MOINS_UNAIRE:
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
static auto applique_opérateur_binaire(GenreLexème id, T a, T b)
{
    switch (id) {
        case GenreLexème::PLUS:
        case GenreLexème::PLUS_EGAL:
        {
            return a + b;
        }
        case GenreLexème::MOINS:
        case GenreLexème::MOINS_EGAL:
        {
            return a - b;
        }
        case GenreLexème::FOIS:
        case GenreLexème::MULTIPLIE_EGAL:
        {
            return a * b;
        }
        case GenreLexème::DIVISE:
        case GenreLexème::DIVISE_EGAL:
        {
            return a / b;
        }
        case GenreLexème::POURCENT:
        case GenreLexème::MODULO_EGAL:
        {
            return a % b;
        }
        case GenreLexème::ESPERLUETTE:
        case GenreLexème::ET_EGAL:
        {
            return a & b;
        }
        case GenreLexème::OU_EGAL:
        case GenreLexème::BARRE:
        {
            return a | b;
        }
        case GenreLexème::CHAPEAU:
        case GenreLexème::OUX_EGAL:
        {
            return a ^ b;
        }
        case GenreLexème::DECALAGE_DROITE:
        case GenreLexème::DEC_DROITE_EGAL:
        {
            return a >> b;
        }
        case GenreLexème::DECALAGE_GAUCHE:
        case GenreLexème::DEC_GAUCHE_EGAL:
        {
            return a << b;
        }
        default:
        {
            return T(0);
        }
    }
}

static auto applique_opérateur_binaire(GenreLexème id, double a, double b)
{
    switch (id) {
        case GenreLexème::PLUS:
        case GenreLexème::PLUS_EGAL:
        {
            return a + b;
        }
        case GenreLexème::MOINS:
        case GenreLexème::MOINS_EGAL:
        {
            return a - b;
        }
        case GenreLexème::FOIS:
        case GenreLexème::MULTIPLIE_EGAL:
        {
            return a * b;
        }
        case GenreLexème::DIVISE:
        case GenreLexème::DIVISE_EGAL:
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
static auto applique_opérateur_binaire_comp(GenreLexème id, T a, T b)
{
    switch (id) {
        case GenreLexème::INFERIEUR:
        {
            return a < b;
        }
        case GenreLexème::INFERIEUR_EGAL:
        {
            return a <= b;
        }
        case GenreLexème::SUPERIEUR:
        {
            return a > b;
        }
        case GenreLexème::SUPERIEUR_EGAL:
        {
            return a >= b;
        }
        case GenreLexème::DIFFÉRENCE:
        {
            return a != b;
        }
        case GenreLexème::EGALITE:
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

    if (type->est_type_réel()) {
        return true;
    }

    if (type->est_type_bool()) {
        return true;
    }

    if (type->est_type_entier_constant()) {
        return true;
    }

    if (type->est_type_énum() || type->est_type_erreur()) {
        return true;
    }

    return false;
}

static RésultatExpression erreur_évaluation(const NoeudExpression *b, const char *message)
{
    auto res = RésultatExpression();
    res.est_erroné = true;
    res.noeud_erreur = b;
    res.message_erreur = message;
    return res;
}

RésultatExpression évalue_expression(const Compilatrice &compilatrice, const NoeudExpression *b)
{
    return évalue_expression(compilatrice, b->bloc_parent, b);
}

/**
 * Évalue l'expression dont « b » est la racine. L'expression doit être
 * constante, c'est à dire ne contenir que des noeuds dont la valeur est connue
 * lors de la compilation.
 */
RésultatExpression évalue_expression(const Compilatrice &compilatrice,
                                     NoeudBloc *bloc,
                                     const NoeudExpression *b)
{
    switch (b->genre) {
        default:
        {
            return erreur_évaluation(
                b, "L'expression n'est pas constante et ne peut être calculée !");
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_TYPE:
        case GenreNoeud::EXPRESSION_TYPE_DE:
        {
            return ValeurExpression(b->type->comme_type_type_de_données()->type_connu);
        }
        case GenreNoeud::DÉCLARATION_ENTÊTE_FONCTION:
        {
            return ValeurExpression(
                const_cast<NoeudDéclarationEntêteFonction *>(b->comme_entête_fonction()));
        }
        case GenreNoeud::DÉCLARATION_CONSTANTE:
        {
            auto decl = b->comme_déclaration_constante();

            if (decl->valeur_expression.est_valide()) {
                return decl->valeur_expression;
            }

            if (decl->expression == nullptr) {
                if (decl->type->est_type_énum()) {
                    auto type_enum = static_cast<TypeEnum *>(decl->type);

                    auto info_rubrique = donne_rubrique_pour_nom(type_enum, decl->ident);
                    if (info_rubrique.has_value()) {
                        return ValeurExpression(info_rubrique->rubrique.valeur);
                    }
                }

                if (decl->type->est_type_type_de_données()) {
                    auto type_de_données = decl->type->comme_type_type_de_données();
                    if (type_de_données->type_connu == nullptr) {
                        return erreur_évaluation(
                            b, "La déclaration n'a pas de type de données connu !");
                    }

                    return ValeurExpression(type_de_données->type_connu);
                }

                return erreur_évaluation(b,
                                         "La déclaration de la variable n'a pas d'expression !");
            }

            return évalue_expression(compilatrice, decl->bloc_parent, decl->expression);
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_DÉCLARATION:
        {
            auto fichier = compilatrice.fichier(b->lexème->fichier);
            auto decl = trouve_dans_bloc_ou_module(
                bloc, b->ident, fichier, bloc->appartiens_à_fonction);

            if (decl == nullptr) {
                return erreur_évaluation(b, "La variable n'existe pas !");
            }

            if (decl->est_entête_fonction()) {
                return ValeurExpression(decl->comme_entête_fonction());
            }

            if (decl->est_déclaration_type()) {
                return ValeurExpression(decl->comme_déclaration_type());
            }

            if (!decl->est_déclaration_constante()) {
                return erreur_évaluation(
                    b, "La référence n'est pas celle d'une variable constante !");
            }

            return évalue_expression(compilatrice, decl->bloc_parent, decl);
        }
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        {
            auto expr_taille_de = b->comme_taille_de();
            auto type = expr_taille_de->expression->type;
            type = type->comme_type_type_de_données()->type_connu;
            return ValeurExpression(type->taille_octet);
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_BOOLÉEN:
        {
            return ValeurExpression(b->lexème->chaine == "vrai");
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_ENTIER:
        {
            /* Si le noeud provient d'un résultat, le lexème ne peut être utilisé pour extraire la
             * valeur car ce n'est pas un lexème de code source. */
            if (b->possède_drapeau(DrapeauxNoeud::NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE)) {
                return ValeurExpression(static_cast<int64_t>(b->comme_littérale_entier()->valeur));
            }

            return ValeurExpression(static_cast<int64_t>(b->lexème->valeur_entiere));
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_CARACTÈRE:
        {
            /* Si le noeud provient d'un résultat, le lexème ne peut être utilisé pour extraire la
             * valeur car ce n'est pas un lexème de code source. */
            if (b->possède_drapeau(DrapeauxNoeud::NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE)) {
                return ValeurExpression(static_cast<int64_t>(b->comme_littérale_entier()->valeur));
            }

            return ValeurExpression(static_cast<int64_t>(b->lexème->valeur_entiere));
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_RÉEL:
        {
            /* Si le noeud provient d'un résultat, le lexème ne peut être utilisé pour extraire la
             * valeur car ce n'est pas un lexème de code source. */
            if (b->possède_drapeau(DrapeauxNoeud::NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE)) {
                return ValeurExpression(b->comme_littérale_réel()->valeur);
            }

            return ValeurExpression(b->lexème->valeur_reelle);
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_CHAINE:
        {
            return ValeurExpression(b->comme_littérale_chaine());
        }
        case GenreNoeud::INSTRUCTION_SAUFSI:
        case GenreNoeud::INSTRUCTION_SI:
        {
            auto inst = static_cast<const NoeudSi *>(b);
            auto res = évalue_expression(compilatrice, bloc, inst->condition);

            if (res.est_erroné) {
                return res;
            }

            if (!res.valeur.est_booléenne()) {
                return erreur_évaluation(b, "L'expression n'est pas de type booléen !");
            }

            if (res.valeur.booléenne() == (b->genre == GenreNoeud::INSTRUCTION_SI)) {
                res = évalue_expression(compilatrice, bloc, inst->bloc_si_vrai);
            }
            else {
                if (inst->bloc_si_faux) {
                    res = évalue_expression(compilatrice, bloc, inst->bloc_si_faux);
                }
            }

            return res;
        }
        case GenreNoeud::OPÉRATEUR_UNAIRE:
        {
            auto inst = b->comme_expression_unaire();
            auto res = évalue_expression(compilatrice, bloc, inst->opérande);

            if (res.est_erroné) {
                return res;
            }

            if (res.valeur.est_réelle()) {
                res.valeur = applique_opérateur_unaire(inst->lexème->genre, res.valeur.réelle());
            }
            else if (res.valeur.est_entière()) {
                res.valeur = applique_opérateur_unaire(inst->lexème->genre, res.valeur.entière());
            }

            return res;
        }
        case GenreNoeud::EXPRESSION_NÉGATION_LOGIQUE:
        {
            auto négation = b->comme_négation_logique();
            auto res = évalue_expression(compilatrice, bloc, négation->opérande);
            if (res.est_erroné) {
                return res;
            }

            if (res.valeur.est_réelle()) {
                res.valeur = applique_négation_logique(res.valeur.réelle());
            }
            else if (res.valeur.est_booléenne()) {
                res.valeur = applique_négation_logique(res.valeur.booléenne());
            }
            else if (res.valeur.est_entière()) {
                res.valeur = applique_négation_logique(res.valeur.entière());
            }
            else if (res.valeur.est_chaine()) {
                auto chaine = res.valeur.chaine();
                res.valeur = chaine->lexème->chaine.taille() == 0;
            }
            else {
                return erreur_évaluation(b,
                                         "L'expression n'est pas évaluable car l'opération "
                                         "n'est pas applicable sur le type de l'opérande.");
            }

            return res;
        }
        case GenreNoeud::OPÉRATEUR_BINAIRE:
        {
            auto inst = b->comme_expression_binaire();
            auto res1 = évalue_expression(compilatrice, bloc, inst->opérande_gauche);

            if (res1.est_erroné) {
                return res1;
            }

            auto res2 = évalue_expression(compilatrice, bloc, inst->opérande_droite);

            if (res2.est_erroné) {
                return res2;
            }

            ValeurExpression res = ValeurExpression();

            if (est_opérateur_bool(inst->lexème->genre)) {
                if (res1.valeur.est_réelle() && res2.valeur.est_réelle()) {
                    res = applique_opérateur_binaire_comp(
                        inst->lexème->genre, res1.valeur.réelle(), res2.valeur.réelle());
                }
                else if (res1.valeur.est_booléenne() && res2.valeur.est_booléenne()) {
                    res = applique_opérateur_binaire_comp(
                        inst->lexème->genre, res1.valeur.booléenne(), res2.valeur.booléenne());
                }
                else if (res1.valeur.est_entière() && res2.valeur.est_entière()) {
                    res = applique_opérateur_binaire_comp(
                        inst->lexème->genre, res1.valeur.entière(), res2.valeur.entière());
                }
                else if (res1.valeur.est_type() && res2.valeur.est_type()) {
                    res = applique_opérateur_binaire_comp(
                        inst->lexème->genre, res1.valeur.type(), res2.valeur.type());
                }
                else {
                    return erreur_évaluation(b,
                                             "L'expression n'est pas évaluable car l'opération "
                                             "n'est pas applicable sur les types données.");
                }
            }
            else {
                if (res1.valeur.est_réelle() && res2.valeur.est_réelle()) {
                    res = applique_opérateur_binaire(
                        inst->lexème->genre, res1.valeur.réelle(), res2.valeur.réelle());
                }
                else if (res1.valeur.est_entière() && res2.valeur.est_entière()) {
                    res = applique_opérateur_binaire(
                        inst->lexème->genre, res1.valeur.entière(), res2.valeur.entière());
                }
                else {
                    return erreur_évaluation(b,
                                             "L'expression n'est pas évaluable car l'opération "
                                             "n'est pas applicable sur les types données.");
                }
            }

            return res;
        }
        case GenreNoeud::EXPRESSION_LOGIQUE:
        {
            auto logique = b->comme_expression_logique();

            auto res1 = évalue_expression(compilatrice, bloc, logique->opérande_gauche);
            if (res1.est_erroné) {
                return res1;
            }
            if (!res1.valeur.est_booléenne()) {
                return erreur_évaluation(
                    logique->opérande_gauche,
                    "L'expression n'est pas évaluable car elle n'est pas de type booléen.");
            }

            auto res2 = évalue_expression(compilatrice, bloc, logique->opérande_droite);
            if (res2.est_erroné) {
                return res2;
            }
            if (!res2.valeur.est_booléenne()) {
                return erreur_évaluation(
                    logique->opérande_droite,
                    "L'expression n'est pas évaluable car elle n'est pas de type booléen.");
            }

            ValeurExpression res = ValeurExpression();
            if (logique->lexème->genre == GenreLexème::ESP_ESP) {
                res = res1.valeur.booléenne() && res2.valeur.booléenne();
            }
            else {
                res = res1.valeur.booléenne() || res2.valeur.booléenne();
            }
            return res;
        }
        case GenreNoeud::EXPRESSION_PARENTHÈSE:
        {
            auto inst = b->comme_parenthèse();
            return évalue_expression(compilatrice, bloc, inst->expression);
        }
        case GenreNoeud::EXPRESSION_COMME:
        {
            auto inst = b->comme_comme();
            auto résultat = évalue_expression(compilatrice, bloc, inst->expression);

            if (!inst->type) {
                // À FAIRE : les expressions 'comme' (et les autres) ne sont pas validées dans les
                // énumérations... (voir aussi commentaire en haut du fichier).
                return résultat;
            }

            if (est_type_entier(inst->type) || inst->type->est_type_pointeur()) {
                if (résultat.valeur.est_entière()) {
                    /* OK. */
                }
                else if (résultat.valeur.est_booléenne()) {
                    résultat.valeur = ValeurExpression(int(résultat.valeur.booléenne()));
                }
                else if (résultat.valeur.est_réelle()) {
                    résultat.valeur = ValeurExpression(int64_t(résultat.valeur.réelle()));
                }
                else {
                    return erreur_évaluation(
                        inst, "Impossible de convertir la valeur vers le type cible.");
                }
            }
            else if (inst->type->est_type_réel()) {
                if (résultat.valeur.est_entière()) {
                    résultat.valeur = ValeurExpression(double(résultat.valeur.entière()));
                }
                else if (résultat.valeur.est_booléenne()) {
                    résultat.valeur = ValeurExpression(double(résultat.valeur.booléenne()));
                }
                else if (résultat.valeur.est_réelle()) {
                    /* OK. */
                }
                else {
                    return erreur_évaluation(
                        inst, "Impossible de convertir la valeur vers le type cible.");
                }
            }
            else {
                return erreur_évaluation(inst,
                                         "Impossible de convertir la valeur vers le type cible.");
            }

            return résultat;
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_RUBRIQUE:
        {
            auto ref_rubrique = b->comme_référence_rubrique();

            if (ref_rubrique->déclaration_référée) {
                auto accédée = ref_rubrique->accédée->comme_référence_déclaration();
                auto déclaration_module = accédée->déclaration_référée->comme_déclaration_module();
                auto module = déclaration_module->module;
                auto fichier = compilatrice.fichier(ref_rubrique->lexème->fichier);
                auto déclarations = kuri::tablet<NoeudDéclaration *, 10>();
                trouve_déclarations_dans_module(
                    déclarations, module, ref_rubrique->ident, fichier);

                assert(déclarations.taille() > 0);

                if (déclarations.taille() > 1) {
                    return erreur_évaluation(b,
                                             "L'expression ne peut être évaluée car le module "
                                             "contient plusieurs symboles de ce nom.");
                }

                return évalue_expression(compilatrice, module->bloc, déclarations[0]);
            }

            auto type_accede = ref_rubrique->accédée->type;
            type_accede = donne_type_accédé_effectif(type_accede);

            if (type_accede->est_type_type_de_données()) {
                type_accede = type_accede->comme_type_type_de_données()->type_connu;
                if (!type_accede) {
                    return erreur_évaluation(b,
                                             "L'expression de référence de rubrique n'est pas une "
                                             "référence d'un type de données connu.");
                }
            }

            if (!type_accede->est_type_composé()) {
                return erreur_évaluation(
                    b, "L'expression ne référence pas la rubrique d'un type composé.");
            }

            auto type_composé = type_accede->comme_type_composé();
            auto &rubrique = type_composé->rubriques[ref_rubrique->indice_rubrique];

            if (rubrique.est_constant()) {
                auto &rubrique = type_composé->rubriques[ref_rubrique->indice_rubrique];
                auto decl = rubrique.decl;
                // À FAIRE : nettoye tout ça, déduplique avec canonicalisation
                if (decl) {
                    if (!decl->est_déclaration_constante()) {
                        return erreur_évaluation(b,
                                                 "La rubrique n'a pas de déclaration constante.");
                    }

                    if (decl->comme_déclaration_constante()->valeur_expression.est_valide()) {
                        return decl->comme_déclaration_constante()->valeur_expression;
                    }
                }
                return ValeurExpression(rubrique.valeur);
            }

            return erreur_évaluation(b, "La rubrique n'est pas une constante.");
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
            auto fonction = appel->expression->comme_entête_fonction();

            return ValeurExpression(fonction);
        }
        case GenreNoeud::EXPRESSION_APPEL:
        {
            auto expression_appel = b->comme_appel();

            if (expression_appel->aide_génération_code != CONSTRUIT_OPAQUE) {
                return erreur_évaluation(b,
                                         "Impossible d'utiliser une expression d'appel qui n'est "
                                         "pas la construction d'un type opaque");
            }

            auto type_opacifié = b->type->comme_type_opaque()->type_opacifié;

            if (!est_type_opaque_utilisable_pour_constante(type_opacifié)) {
                return erreur_évaluation(b,
                                         "Impossible de construire une expression constante "
                                         "depuis un type opacifié n'étant pas un type entier, "
                                         "booléen, réel, énumération, ou type erreur");
            }

            auto const nombre_de_paramètres = expression_appel->paramètres_résolus.taille();
            if (nombre_de_paramètres == 0) {
                return erreur_évaluation(b,
                                         "Impossible de construire une expression constante "
                                         "depuis un type opacifié sans paramètre");
            }

            if (nombre_de_paramètres > 1) {
                return erreur_évaluation(b,
                                         "Impossible de construire une expression constante "
                                         "depuis un type opacifié avec plusieurs paramètres");
            }

            /* L'assignation du type opaque à la valeur se fera par quiconque nous a appelé. */
            return évalue_expression(compilatrice, bloc, expression_appel->paramètres_résolus[0]);
        }
    }
}

std::ostream &operator<<(std::ostream &os, ValeurExpression valeur)
{
    if (valeur.est_booléenne()) {
        os << valeur.booléenne();
    }
    else if (valeur.est_entière()) {
        os << valeur.entière();
    }
    else if (valeur.est_réelle()) {
        os << valeur.réelle();
    }
    else if (valeur.est_chaine()) {
        auto chaine = valeur.chaine();
        os << chaine->lexème->chaine;
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
