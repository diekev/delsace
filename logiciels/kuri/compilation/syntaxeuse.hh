/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "biblinternes/structures/tableau_page.hh"
#include "parsage/base_syntaxeuse.hh"
#include "structures/pile.hh"

struct Annotation;
struct Compilatrice;
struct NoeudBloc;
struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationSymbole;
struct NoeudDeclarationVariable;
struct NoeudExpression;
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
    UniteCompilation const *m_unité = nullptr;

    NoeudExpressionVirgule *m_noeud_expression_virgule = nullptr;

    bool m_est_declaration_type_opaque = false;

    /* Bloc courant recevant les constantes polymorphiques. */
    kuri::pile<NoeudBloc *> bloc_constantes_polymorphiques{};

    kuri::pile<NoeudDeclarationEnteteFonction *> fonctions_courantes{};

  public:
    Syntaxeuse(Tacheronne &tacheronne, UniteCompilation const *unite);

    EMPECHE_COPIE(Syntaxeuse);

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

    NoeudBloc *analyse_bloc(bool accolade_requise = true);

    NoeudExpression *analyse_appel_fonction(NoeudExpression *gauche);

    NoeudExpression *analyse_declaration_enum(NoeudExpression *gauche);
    NoeudDeclarationEnteteFonction *analyse_declaration_fonction(Lexeme const *lexeme);
    NoeudExpression *analyse_declaration_operateur();
    void analyse_expression_retour_type(NoeudDeclarationEnteteFonction *noeud,
                                        bool pour_operateur);

    /* Structures et unions. */
    NoeudExpression *analyse_declaration_structure(NoeudExpression *gauche);
    NoeudExpression *analyse_declaration_union(NoeudExpression *gauche);
    void analyse_directives_structure_ou_union(NoeudStruct *noeud);
    void analyse_paramètres_polymorphiques_structure_ou_union(NoeudStruct *noeud);
    void analyse_membres_structure_ou_union(NoeudStruct *decl_struct);
    NoeudBloc *analyse_bloc_membres_structure_ou_union(NoeudStruct *decl_struct);

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

    /* Analyse une série d'expressions séparées par des virgules. */
    NoeudExpression *analyse_expression_avec_virgule(GenreLexeme lexème_racine);

    void analyse_annotations(kuri::tableau<Annotation, int> &annotations);

    void gere_erreur_rapportee(const kuri::chaine &message_erreur) override;

    void requiers_typage(NoeudExpression *noeud);

    bool ignore_point_virgule_implicite();

    void analyse_directive_déclaration_variable(NoeudDeclarationVariable *déclaration);
    void analyse_directive_symbole_externe(NoeudDeclarationSymbole *déclaration_symbole);
};
