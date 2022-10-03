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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

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

/**
 * Évalue l'expression dont « b » est la racine. L'expression doit être
 * constante, c'est à dire ne contenir que des noeuds dont la valeur est connue
 * lors de la compilation.
 */
ResultatExpression evalue_expression(Compilatrice &compilatrice,
                                     NoeudBloc *bloc,
                                     NoeudExpression *b)
{
    switch (b->genre) {
        default:
        {
            auto res = ResultatExpression();
            res.est_errone = true;
            res.noeud_erreur = b;
            res.message_erreur = "L'expression n'est pas constante et ne peut être calculée !";

            return res;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
        {
            auto res = ResultatExpression();

            auto fichier = compilatrice.fichier(b->lexeme->fichier);
            auto decl = trouve_dans_bloc_ou_module(bloc, b->ident, fichier);

            if (decl == nullptr) {
                res.est_errone = true;
                res.noeud_erreur = b;
                res.message_erreur = "La variable n'existe pas !";

                return res;
            }

            if (decl->est_entete_fonction()) {
                res.est_errone = false;
                res.valeur = decl->comme_entete_fonction();
                return res;
            }

            if (decl->genre != GenreNoeud::DECLARATION_VARIABLE) {
                res.est_errone = true;
                res.noeud_erreur = b;
                res.message_erreur = "La référence n'est pas celle d'une variable !";

                return res;
            }

            if (!decl->possede_drapeau(EST_CONSTANTE)) {
                res.est_errone = true;
                res.noeud_erreur = b;
                res.message_erreur = "La référence n'est pas celle d'une variable constante !";

                return res;
            }

            auto decl_var = static_cast<NoeudDeclarationVariable *>(decl);

            if (decl_var->valeur_expression.est_valide()) {
                res.est_errone = false;
                res.valeur = decl_var->valeur_expression;
                return res;
            }

            if (decl_var->expression == nullptr) {
                if (decl_var->type->est_enum()) {
                    auto type_enum = static_cast<TypeEnum *>(decl_var->type);

                    POUR (type_enum->membres) {
                        if (it.nom == decl_var->ident) {
                            res.valeur = it.valeur;
                            res.est_errone = false;
                            return res;
                        }
                    }
                }

                res.est_errone = true;
                res.noeud_erreur = b;
                res.message_erreur = "La déclaration de la variable n'a pas d'expression !";

                return res;
            }

            return evalue_expression(compilatrice, decl->bloc_parent, decl_var->expression);
        }
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        {
            auto expr_taille_de = b->comme_taille_de();
            auto type = expr_taille_de->expression->type;

            auto res = ResultatExpression();
            res.valeur = type->taille_octet;
            res.est_errone = false;

            return res;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
        {
            auto res = ResultatExpression();
            res.valeur = b->lexeme->chaine == "vrai";
            res.est_errone = false;

            return res;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
        {
            auto res = ResultatExpression();
            res.est_errone = false;

            /* Si le noeud provient d'un résultat, le lexème ne peut être utilisé pour extraire la
             * valeur car ce n'est pas un lexème de code source. */
            if (b->possede_drapeau(NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE)) {
                res.valeur = static_cast<long>(b->comme_litterale_entier()->valeur);
            }
            else {
                res.valeur = static_cast<long>(b->lexeme->valeur_entiere);
            }

            return res;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
        {
            auto res = ResultatExpression();
            res.est_errone = false;

            /* Si le noeud provient d'un résultat, le lexème ne peut être utilisé pour extraire la
             * valeur car ce n'est pas un lexème de code source. */
            if (b->possede_drapeau(NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE)) {
                res.valeur = static_cast<long>(b->comme_litterale_entier()->valeur);
            }
            else {
                res.valeur = static_cast<long>(b->lexeme->valeur_entiere);
            }

            return res;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
        {
            auto res = ResultatExpression();
            res.est_errone = false;

            /* Si le noeud provient d'un résultat, le lexème ne peut être utilisé pour extraire la
             * valeur car ce n'est pas un lexème de code source. */
            if (b->possede_drapeau(NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE)) {
                res.valeur = b->comme_litterale_reel()->valeur;
            }
            else {
                res.valeur = b->lexeme->valeur_reelle;
            }

            return res;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
        {
            auto res = ResultatExpression();
            res.valeur = b->comme_litterale_chaine();
            res.est_errone = false;
            return res;
        }
        case GenreNoeud::INSTRUCTION_SAUFSI:
        case GenreNoeud::INSTRUCTION_SI:
        {
            auto inst = static_cast<NoeudSi *>(b);

            auto res = evalue_expression(compilatrice, bloc, inst->condition);

            if (res.est_errone) {
                return res;
            }

            if (!res.valeur.est_booleenne()) {
                res.est_errone = true;
                res.noeud_erreur = b;
                res.message_erreur = "L'expression n'est pas de type booléen !";
                return res;
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

            auto res = ResultatExpression();
            res.est_errone = false;

            if (est_operateur_bool(inst->lexeme->genre)) {
                if (res1.valeur.est_reelle()) {
                    res.valeur = applique_operateur_binaire_comp(
                        inst->lexeme->genre, res1.valeur.reelle(), res2.valeur.reelle());
                }
                else if (res1.valeur.est_booleenne()) {
                    res.valeur = applique_operateur_binaire_comp(
                        inst->lexeme->genre, res1.valeur.booleenne(), res2.valeur.booleenne());
                }
                else {
                    res.valeur = applique_operateur_binaire_comp(
                        inst->lexeme->genre, res1.valeur.entiere(), res2.valeur.entiere());
                }
            }
            else {
                if (res1.valeur.est_reelle()) {
                    res.valeur = applique_operateur_binaire(
                        inst->lexeme->genre, res1.valeur.reelle(), res2.valeur.reelle());
                }
                else {
                    res.valeur = applique_operateur_binaire(
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
                auto res = ResultatExpression();
                res.est_errone = false;
                res.valeur = valeur_enum;
                return res;
            }

            if (type_accede->est_tableau_fixe()) {
                if (!ref_membre->membre->est_reference_declaration()) {
                    auto res = ResultatExpression();
                    res.est_errone = true;
                    res.noeud_erreur = b;
                    res.message_erreur =
                        "L'expression n'est pas constante et ne peut être calculée !";

                    return res;
                }

                auto ref_decl_membre = ref_membre->membre->comme_reference_declaration();

                if (ref_decl_membre->ident->nom == "taille") {
                    auto res = ResultatExpression();
                    res.est_errone = false;
                    res.valeur = type_accede->comme_tableau_fixe()->taille;
                    return res;
                }
            }

            auto res = ResultatExpression();
            res.est_errone = true;
            res.noeud_erreur = b;
            res.message_erreur = "L'expression n'est pas constante et ne peut être calculée !";

            return res;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
        {
            auto res = ResultatExpression();
            res.est_errone = false;
            res.valeur = b->comme_construction_tableau();
            return res;
        }
        case GenreNoeud::DIRECTIVE_CUISINE:
        {
            auto cuisine = b->comme_cuisine();
            auto expr = cuisine->expression;
            auto appel = expr->comme_appel();
            auto fonction = appel->expression->comme_entete_fonction();

            auto res = ResultatExpression();
            res.est_errone = false;
            res.valeur = fonction;
            return res;
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
    else {
        os << "invalide";
    }
    return os;
}
