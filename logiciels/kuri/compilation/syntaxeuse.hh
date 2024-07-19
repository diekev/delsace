/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "arbre_syntaxique/prodeclaration.hh"
#include "arbre_syntaxique/utilitaires.hh"

#include "parsage/base_syntaxeuse.hh"
#include "structures/pile.hh"

struct Annotation;
struct Compilatrice;
struct Tacheronne;
struct UniteCompilation;

enum class GenreNoeud : unsigned char;

enum class Associativité : int {
    GAUCHE,
    DROITE,
};

struct DonnéesPrécédence {
    int précédence = 0;
    Associativité associativité = Associativité::GAUCHE;
};

struct Syntaxeuse : BaseSyntaxeuse {
  private:
    Compilatrice &m_compilatrice;
    Tacheronne &m_tacheronne;
    UniteCompilation const *m_unité = nullptr;

    NoeudExpressionVirgule *m_noeud_expression_virgule = nullptr;

    bool m_est_déclaration_type_opaque = false;

    /* Bloc courant recevant les constantes polymorphiques. */
    kuri::pile<NoeudBloc *> bloc_constantes_polymorphiques{};

    kuri::pile<NoeudDéclarationEntêteFonction *> fonctions_courantes{};

    bool m_fonction_courante_retourne_plusieurs_valeurs = false;

    PortéeSymbole m_portée = PortéeSymbole::EXPORT;

  public:
    Syntaxeuse(Tacheronne &tacheronne, UniteCompilation const *unite);

    EMPECHE_COPIE(Syntaxeuse);

  private:
    void quand_commence() override;
    void quand_termine() override;
    void analyse_une_chose() override;

    bool apparie_commentaire() const;
    bool apparie_expression() const;
    bool apparie_expression_unaire() const;
    bool apparie_expression_secondaire() const;
    bool apparie_instruction() const;

    /* NOTE: lexème_final n'est utilisé que pour éviter de traiter les virgules comme des
     * opérateurs dans les expressions des appels et déclarations de paramètres de fonctions. */
    NoeudExpression *analyse_expression(DonnéesPrécédence const &donnees_precedence,
                                        GenreLexème racine_expression,
                                        GenreLexème lexème_final);
    NoeudExpression *analyse_expression_unaire(GenreLexème lexème_final);
    NoeudExpression *analyse_expression_primaire(GenreLexème racine_expression,
                                                 GenreLexème lexème_final);
    NoeudExpression *analyse_expression_secondaire(NoeudExpression *gauche,
                                                   DonnéesPrécédence const &donnees_precedence,
                                                   GenreLexème racine_expression,
                                                   GenreLexème lexème_final);

    NoeudBloc *analyse_bloc(bool accolade_requise = true);

    NoeudExpression *analyse_appel_fonction(NoeudExpression *gauche);

    NoeudExpression *analyse_déclaration_enum(Lexème const *lexème_nom);
    bool est_déclaration_type_fonction();
    NoeudExpression *analyse_déclaration_fonction(Lexème const *lexeme);
    void analyse_directives_fonction(NoeudDéclarationEntêteFonction *noeud);
    NoeudExpression *analyse_déclaration_type_fonction(Lexème const *lexeme);
    NoeudExpression *analyse_déclaration_opérateur();
    void analyse_directives_opérateur(NoeudDéclarationEntêteFonction *noeud);
    void analyse_expression_retour_type(NoeudDéclarationEntêteFonction *noeud,
                                        bool pour_operateur);

    /* Structures et unions. */
    NoeudExpression *analyse_déclaration_structure(Lexème const *lexème_nom);
    NoeudExpression *analyse_déclaration_union(Lexème const *lexème_nom);
    void analyse_directives_structure(NoeudStruct *noeud);
    void analyse_directives_union(NoeudUnion *noeud);
    void analyse_paramètres_polymorphiques_structure_ou_union(NoeudDéclarationClasse *noeud);
    void analyse_membres_structure_ou_union(NoeudDéclarationClasse *decl_struct);
    NoeudBloc *analyse_bloc_membres_structure_ou_union(NoeudDéclarationClasse *decl_struct);

    NoeudExpression *analyse_instruction();
    NoeudExpression *analyse_instruction_boucle();
    NoeudExpression *analyse_instruction_discr();
    NoeudExpression *analyse_instruction_pour();
    void analyse_specifiants_instruction_pour(NoeudPour *noeud);
    NoeudExpression *analyse_instruction_pousse_contexte();
    NoeudExpression *analyse_instruction_répète();
    NoeudExpression *analyse_instruction_si(GenreNoeud genre_noeud);
    NoeudExpression *analyse_instruction_si_statique(Lexème *lexeme);
    NoeudExpression *analyse_instruction_tantque();

    /* Analyse une série d'expressions séparées par des virgules. */
    NoeudExpression *analyse_expression_avec_virgule(GenreLexème lexème_racine,
                                                     bool force_noeud_virgule);

    bool est_déclaration_type_tableau();

    NoeudExpression *analyse_expression_crochet_ouvrant(Lexème const *lexème,
                                                        GenreLexème racine_expression,
                                                        GenreLexème lexème_final);

    NoeudExpressionTypeTableauFixe *parse_type_tableau_fixe(Lexème const *lexème,
                                                            GenreLexème racine_expression,
                                                            GenreLexème lexème_final);

    NoeudExpressionConstructionTableau *parse_construction_tableau(Lexème const *lexème,
                                                                   GenreLexème racine_expression,
                                                                   GenreLexème lexème_final);

    NoeudExpression *analyse_référence_déclaration(Lexème const *lexème_référence);

    void analyse_annotations(kuri::tableau<Annotation, int> &annotations);

    void gère_erreur_rapportée(kuri::chaine_statique message_erreur) override;

    void rapporte_erreur_avec_site(NoeudExpression const *site, kuri::chaine_statique message);

    void requiers_typage(NoeudExpression *noeud);

    bool ignore_point_virgule_implicite();

    void analyse_directive_déclaration_variable(NoeudDéclarationVariable *déclaration);
    void analyse_directive_symbole_externe(NoeudDéclarationSymbole *déclaration_symbole,
                                           NoeudDirectiveFonction *directive);

    void recycle_référence(NoeudExpressionRéférence *référence);
    void imprime_ligne_source(const Lexème *lexème, kuri::chaine_statique message);
};
