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
 * toujours surl les lexèmes */

/* ************************************************************************** */

template <typename T>
static auto applique_operateur_unaire(GenreLexeme id, T a)
{
    switch (id) {
        case GenreLexeme::EXCLAMATION:
        {
            return T(!a);
        }
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
        case GenreLexeme::INFERIEUR_EGAL:
        {
            return a < b;
        }
        case GenreLexeme::SUPERIEUR:
        case GenreLexeme::SUPERIEUR_EGAL:
        {
            return a > b;
        }
        case GenreLexeme::DIFFERENCE:
        {
            return a != b;
        }
        case GenreLexeme::ESP_ESP:
        {
            return a && b;
        }
        case GenreLexeme::EGALITE:
        {
            return a == b;
        }
        case GenreLexeme::BARRE_BARRE:
        {
            return a || b;
        }
        default:
        {
            return false;
        }
    }
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
        case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
        {
            auto fichier = compilatrice.fichier(b->lexeme->fichier);
            auto decl = trouve_dans_bloc_ou_module(bloc, b->ident, fichier);

            if (decl == nullptr) {
                return erreur_evaluation(b, "La variable n'existe pas !");
            }

            if (decl->est_entete_fonction()) {
                return ValeurExpression(decl->comme_entete_fonction());
            }

            if (decl->genre != GenreNoeud::DECLARATION_VARIABLE) {
                return erreur_evaluation(b, "La référence n'est pas celle d'une variable !");
            }

            if (!decl->possede_drapeau(DrapeauxNoeud::EST_CONSTANTE)) {
                return erreur_evaluation(
                    b, "La référence n'est pas celle d'une variable constante !");
            }

            auto decl_var = static_cast<NoeudDeclarationVariable *>(decl);

            if (decl_var->valeur_expression.est_valide()) {
                return decl_var->valeur_expression;
            }

            if (decl_var->expression == nullptr) {
                if (decl_var->type->est_enum()) {
                    auto type_enum = static_cast<TypeEnum *>(decl_var->type);

                    auto info_membre = type_enum->donne_membre_pour_nom(decl_var->ident);
                    if (info_membre.has_value()) {
                        return ValeurExpression(info_membre->membre.valeur);
                    }
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
            if (b->possede_drapeau(DrapeauxNoeud::NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE)) {
                return ValeurExpression(static_cast<int64_t>(b->comme_litterale_entier()->valeur));
            }

            return ValeurExpression(static_cast<int64_t>(b->lexeme->valeur_entiere));
        }
        case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
        {
            /* Si le noeud provient d'un résultat, le lexème ne peut être utilisé pour extraire la
             * valeur car ce n'est pas un lexème de code source. */
            if (b->possede_drapeau(DrapeauxNoeud::NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE)) {
                return ValeurExpression(static_cast<int64_t>(b->comme_litterale_entier()->valeur));
            }

            return ValeurExpression(static_cast<int64_t>(b->lexeme->valeur_entiere));
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
        {
            /* Si le noeud provient d'un résultat, le lexème ne peut être utilisé pour extraire la
             * valeur car ce n'est pas un lexème de code source. */
            if (b->possede_drapeau(DrapeauxNoeud::NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE)) {
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

            if (est_operateur_bool(inst->lexeme->genre)) {
                if (res1.valeur.est_reelle()) {
                    res = applique_operateur_binaire_comp(
                        inst->lexeme->genre, res1.valeur.reelle(), res2.valeur.reelle());
                }
                else if (res1.valeur.est_booleenne()) {
                    res = applique_operateur_binaire_comp(
                        inst->lexeme->genre, res1.valeur.booleenne(), res2.valeur.booleenne());
                }
                else {
                    res = applique_operateur_binaire_comp(
                        inst->lexeme->genre, res1.valeur.entiere(), res2.valeur.entiere());
                }
            }
            else {
                if (res1.valeur.est_reelle()) {
                    res = applique_operateur_binaire(
                        inst->lexeme->genre, res1.valeur.reelle(), res2.valeur.reelle());
                }
                else {
                    res = applique_operateur_binaire(
                        inst->lexeme->genre, res1.valeur.entiere(), res2.valeur.entiere());
                }
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

            if (type_accede->genre == GenreType::ENUM || type_accede->genre == GenreType::ERREUR) {
                auto type_enum = static_cast<TypeEnum *>(type_accede);
                auto valeur_enum = type_enum->membres[ref_membre->index_membre].valeur;
                return ValeurExpression(valeur_enum);
            }

            if (type_accede->est_tableau_fixe()) {
                if (!ref_membre->membre->est_reference_declaration()) {
                    return erreur_evaluation(
                        b, "L'expression n'est pas constante et ne peut être calculée !");
                }

                auto ref_decl_membre = ref_membre->membre->comme_reference_declaration();

                if (ref_decl_membre->ident->nom == "taille") {
                    return ValeurExpression(type_accede->comme_tableau_fixe()->taille);
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
