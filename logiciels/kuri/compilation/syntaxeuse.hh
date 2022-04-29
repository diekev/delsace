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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/tableau_page.hh"
#include "parsage/base_syntaxeuse.hh"

struct Annotation;
struct Compilatrice;
struct NoeudBloc;
struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationVariable;
struct NoeudExpression;
struct NoeudExpressionReference;
struct NoeudExpressionVirgule;
struct NoeudPour;
struct NoeudStruct;
struct Tacheronne;
struct UniteCompilation;

enum class GenreNoeud : unsigned char;

enum class Associativite : int {
    GAUCHE,
    DROITE,
};

struct DonneesPrecedence {
    int precedence = 0;
    Associativite associativite = Associativite::GAUCHE;
};

struct Syntaxeuse : BaseSyntaxeuse {
  private:
    Compilatrice &m_compilatrice;
    Tacheronne &m_tacheronne;
    UniteCompilation *m_unite = nullptr;

    NoeudExpressionVirgule *m_noeud_expression_virgule = nullptr;

    bool est_dans_fonction = false;
    bool m_est_declaration_type_opaque = false;
    NoeudDeclarationEnteteFonction *fonction_courante = nullptr;
    NoeudStruct *structure_courante = nullptr;

  public:
    Syntaxeuse(Tacheronne &tacheronne, UniteCompilation *unite);

    COPIE_CONSTRUCT(Syntaxeuse);

  private:
    void quand_commence() override;
    void quand_termine() override;
    void analyse_une_chose() override;

    bool apparie_expression() const;
    bool apparie_expression_unaire() const;
    bool apparie_expression_secondaire() const;
    bool apparie_instruction() const;

    /* NOTE: lexeme_final n'est utilisé que pour éviter de traiter les virgules comme des
     * opérateurs dans les expressions des appels et déclarations de paramètres de fonctions. */
    NoeudExpression *analyse_expression(DonneesPrecedence const &donnees_precedence,
                                        GenreLexeme racine_expression,
                                        GenreLexeme lexeme_final);
    NoeudExpression *analyse_expression_unaire(GenreLexeme lexeme_final);
    NoeudExpression *analyse_expression_primaire(GenreLexeme racine_expression,
                                                 GenreLexeme lexeme_final);
    NoeudExpression *analyse_expression_secondaire(NoeudExpression *gauche,
                                                   DonneesPrecedence const &donnees_precedence,
                                                   GenreLexeme racine_expression,
                                                   GenreLexeme lexeme_final);

    NoeudBloc *analyse_bloc(bool accolade_requise = true, bool pour_pousse_contexte = false);

    NoeudExpression *analyse_appel_fonction(NoeudExpression *gauche);

    NoeudExpression *analyse_declaration_enum(NoeudExpression *gauche);
    NoeudDeclarationEnteteFonction *analyse_declaration_fonction(Lexeme const *lexeme);
    NoeudExpression *analyse_declaration_operateur();
    NoeudExpression *analyse_declaration_structure(NoeudExpression *gauche);

    NoeudExpression *analyse_instruction();
    NoeudExpression *analyse_instruction_boucle();
    NoeudExpression *analyse_instruction_discr();
    NoeudExpression *analyse_instruction_pour();
    void analyse_specifiants_instruction_pour(NoeudPour *noeud);
    NoeudExpression *analyse_instruction_pousse_contexte();
    NoeudExpression *analyse_instruction_repete();
    NoeudExpression *analyse_instruction_si(GenreNoeud genre_noeud);
    NoeudExpression *analyse_instruction_si_statique(Lexeme *lexeme);
    NoeudExpression *analyse_instruction_tantque();

    void analyse_annotations(kuri::tableau<Annotation, int> &annotations);

    void gere_erreur_rapportee(const kuri::chaine &message_erreur) override;
};
