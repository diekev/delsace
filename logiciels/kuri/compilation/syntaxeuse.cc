/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "syntaxeuse.hh"

#include <array>
#include <iostream>

#include "biblinternes/outils/assert.hh"

#include "parsage/modules.hh"
#include "parsage/outils_lexemes.hh"
#include "parsage/site_source.hh"

#include "arbre_syntaxique/assembleuse.hh"
#include "arbre_syntaxique/noeud_expression.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "ipa.hh"
#include "numerique.hh"
#include "typage.hh"

enum {
    OPERATEUR_EST_SURCHARGEABLE = (1 << 0),
    EST_EXPRESSION = (1 << 1),
    EST_EXPRESSION_UNAIRE = (1 << 2),
    EST_EXPRESSION_SECONDAIRE = (1 << 3),
    EST_LEXEME_TYPE = (1 << 4),
    EST_INSTRUCTION = (1 << 5),
};

static constexpr auto table_drapeaux_lexemes = [] {
    std::array<u_char, 256> t{};

    for (auto i = 0u; i < 256; ++i) {
        t[i] = 0;

        switch (static_cast<GenreLexeme>(i)) {
            default:
            {
                break;
            }
            case GenreLexeme::INFERIEUR:
            case GenreLexeme::INFERIEUR_EGAL:
            case GenreLexeme::SUPERIEUR:
            case GenreLexeme::SUPERIEUR_EGAL:
            case GenreLexeme::DIFFERENCE:
            case GenreLexeme::EGALITE:
            case GenreLexeme::PLUS:
            case GenreLexeme::PLUS_UNAIRE:
            case GenreLexeme::MOINS:
            case GenreLexeme::MOINS_UNAIRE:
            case GenreLexeme::FOIS:
            case GenreLexeme::FOIS_UNAIRE:
            case GenreLexeme::DIVISE:
            case GenreLexeme::DECALAGE_DROITE:
            case GenreLexeme::DECALAGE_GAUCHE:
            case GenreLexeme::POURCENT:
            case GenreLexeme::ESPERLUETTE:
            case GenreLexeme::BARRE:
            case GenreLexeme::TILDE:
            case GenreLexeme::EXCLAMATION:
            case GenreLexeme::CHAPEAU:
            case GenreLexeme::CROCHET_OUVRANT:
            case GenreLexeme::POUR:
            {
                t[i] |= OPERATEUR_EST_SURCHARGEABLE;
                break;
            }
        }

        // expression unaire
        switch (static_cast<GenreLexeme>(i)) {
            case GenreLexeme::EXCLAMATION:
            case GenreLexeme::MOINS:
            case GenreLexeme::MOINS_UNAIRE:
            case GenreLexeme::PLUS:
            case GenreLexeme::PLUS_UNAIRE:
            case GenreLexeme::TILDE:
            case GenreLexeme::FOIS:
            case GenreLexeme::FOIS_UNAIRE:
            case GenreLexeme::ESP_UNAIRE:
            case GenreLexeme::ESPERLUETTE:
            case GenreLexeme::TROIS_POINTS:
            case GenreLexeme::EXPANSION_VARIADIQUE:
            case GenreLexeme::ACCENT_GRAVE:
            {
                t[i] |= EST_EXPRESSION_UNAIRE;
                t[i] |= EST_EXPRESSION;
                break;
            }
            default:
            {
                break;
            }
        }

        // expresssion
        switch (static_cast<GenreLexeme>(i)) {
            case GenreLexeme::CARACTERE:
            case GenreLexeme::CHAINE_CARACTERE:
            case GenreLexeme::CHAINE_LITTERALE:
            case GenreLexeme::COROUT:
            case GenreLexeme::CROCHET_OUVRANT:  // construit tableau
            case GenreLexeme::DIRECTIVE:
            case GenreLexeme::DOLLAR:
            case GenreLexeme::EMPL:
            case GenreLexeme::ENUM:
            case GenreLexeme::ENUM_DRAPEAU:
            case GenreLexeme::ERREUR:
            case GenreLexeme::FAUX:
            case GenreLexeme::FONC:
            case GenreLexeme::INFO_DE:
            case GenreLexeme::INIT_DE:
            case GenreLexeme::MEMOIRE:
            case GenreLexeme::NOMBRE_ENTIER:
            case GenreLexeme::NOMBRE_REEL:
            case GenreLexeme::NON_INITIALISATION:
            case GenreLexeme::NUL:
            case GenreLexeme::OPERATEUR:
            case GenreLexeme::PARENTHESE_OUVRANTE:  // expression entre parenthèse
            case GenreLexeme::STRUCT:
            case GenreLexeme::TABLEAU:
            case GenreLexeme::TAILLE_DE:
            case GenreLexeme::TYPE_DE:
            case GenreLexeme::TENTE:
            case GenreLexeme::UNION:
            case GenreLexeme::VRAI:
            {
                t[i] |= EST_EXPRESSION;
                break;
            }
            default:
            {
                break;
            }
        }

        // expresssion type
        switch (static_cast<GenreLexeme>(i)) {
            case GenreLexeme::N8:
            case GenreLexeme::N16:
            case GenreLexeme::N32:
            case GenreLexeme::N64:
            case GenreLexeme::R16:
            case GenreLexeme::R32:
            case GenreLexeme::R64:
            case GenreLexeme::Z8:
            case GenreLexeme::Z16:
            case GenreLexeme::Z32:
            case GenreLexeme::Z64:
            case GenreLexeme::BOOL:
            case GenreLexeme::RIEN:
            case GenreLexeme::EINI:
            case GenreLexeme::CHAINE:
            case GenreLexeme::CHAINE_CARACTERE:
            case GenreLexeme::OCTET:
            case GenreLexeme::TYPE_DE_DONNEES:
            {
                t[i] |= EST_LEXEME_TYPE;
                t[i] |= EST_EXPRESSION;
                break;
            }
            default:
                break;
        }

        // expresssion secondaire
        switch (static_cast<GenreLexeme>(i)) {
            case GenreLexeme::BARRE:
            case GenreLexeme::BARRE_BARRE:
            case GenreLexeme::CHAPEAU:
            case GenreLexeme::CROCHET_OUVRANT:
            case GenreLexeme::DECALAGE_DROITE:
            case GenreLexeme::DECALAGE_GAUCHE:
            case GenreLexeme::DECLARATION_CONSTANTE:
            case GenreLexeme::DECLARATION_VARIABLE:
            case GenreLexeme::DEC_DROITE_EGAL:
            case GenreLexeme::DEC_GAUCHE_EGAL:
            case GenreLexeme::DIFFERENCE:
            case GenreLexeme::DIVISE:
            case GenreLexeme::DIVISE_EGAL:
            case GenreLexeme::EGAL:
            case GenreLexeme::EGALITE:
            case GenreLexeme::ESPERLUETTE:
            case GenreLexeme::ESP_ESP:
            case GenreLexeme::ET_EGAL:
            case GenreLexeme::FOIS:
            case GenreLexeme::INFERIEUR:
            case GenreLexeme::INFERIEUR_EGAL:
            case GenreLexeme::MODULO_EGAL:
            case GenreLexeme::MOINS:
            case GenreLexeme::MOINS_EGAL:
            case GenreLexeme::MULTIPLIE_EGAL:
            case GenreLexeme::OUX_EGAL:
            case GenreLexeme::OU_EGAL:
            case GenreLexeme::PARENTHESE_OUVRANTE:
            case GenreLexeme::PLUS:
            case GenreLexeme::PLUS_EGAL:
            case GenreLexeme::POINT:
            case GenreLexeme::POURCENT:
            case GenreLexeme::SUPERIEUR:
            case GenreLexeme::SUPERIEUR_EGAL:
            case GenreLexeme::TROIS_POINTS:
            case GenreLexeme::VIRGULE:
            case GenreLexeme::COMME:
            case GenreLexeme::DOUBLE_POINTS:
            {
                t[i] |= EST_EXPRESSION_SECONDAIRE;
                break;
            }
            default:
            {
                break;
            }
        }

        switch (static_cast<GenreLexeme>(i)) {
            case GenreLexeme::ACCOLADE_OUVRANTE:
            case GenreLexeme::ARRETE:
            case GenreLexeme::BOUCLE:
            case GenreLexeme::CONTINUE:
            case GenreLexeme::DIFFERE:
            case GenreLexeme::DISCR:
            case GenreLexeme::NONSUR:
            case GenreLexeme::POUR:
            case GenreLexeme::POUSSE_CONTEXTE:
            case GenreLexeme::REPETE:
            case GenreLexeme::REPRENDS:
            case GenreLexeme::RETIENS:
            case GenreLexeme::RETOURNE:
            case GenreLexeme::SAUFSI:
            case GenreLexeme::SI:
            case GenreLexeme::TANTQUE:
            {
                t[i] |= EST_INSTRUCTION;
                break;
            }
            default:
            {
                break;
            }
        }
    }

    return t;
}();

static constexpr auto table_associativite_lexemes = [] {
    std::array<Associativite, 256> t{};

    for (auto i = 0u; i < 256; ++i) {
        t[i] = static_cast<Associativite>(-1);

        switch (static_cast<GenreLexeme>(i)) {
            case GenreLexeme::VIRGULE:
            case GenreLexeme::TROIS_POINTS:
            case GenreLexeme::EGAL:
            case GenreLexeme::DECLARATION_VARIABLE:
            case GenreLexeme::DECLARATION_CONSTANTE:
            case GenreLexeme::PLUS_EGAL:
            case GenreLexeme::MOINS_EGAL:
            case GenreLexeme::DIVISE_EGAL:
            case GenreLexeme::MULTIPLIE_EGAL:
            case GenreLexeme::MODULO_EGAL:
            case GenreLexeme::ET_EGAL:
            case GenreLexeme::OU_EGAL:
            case GenreLexeme::OUX_EGAL:
            case GenreLexeme::DEC_DROITE_EGAL:
            case GenreLexeme::DEC_GAUCHE_EGAL:
            case GenreLexeme::BARRE_BARRE:
            case GenreLexeme::ESP_ESP:
            case GenreLexeme::BARRE:
            case GenreLexeme::CHAPEAU:
            case GenreLexeme::ESPERLUETTE:
            case GenreLexeme::DIFFERENCE:
            case GenreLexeme::EGALITE:
            case GenreLexeme::INFERIEUR:
            case GenreLexeme::INFERIEUR_EGAL:
            case GenreLexeme::SUPERIEUR:
            case GenreLexeme::SUPERIEUR_EGAL:
            case GenreLexeme::DECALAGE_GAUCHE:
            case GenreLexeme::DECALAGE_DROITE:
            case GenreLexeme::PLUS:
            case GenreLexeme::MOINS:
            case GenreLexeme::FOIS:
            case GenreLexeme::DIVISE:
            case GenreLexeme::POURCENT:
            case GenreLexeme::POINT:
            case GenreLexeme::CROCHET_OUVRANT:
            case GenreLexeme::PARENTHESE_OUVRANTE:
            case GenreLexeme::COMME:
            case GenreLexeme::DOUBLE_POINTS:
            {
                t[i] = Associativite::GAUCHE;
                break;
            }
            case GenreLexeme::EXCLAMATION:
            case GenreLexeme::TILDE:
            case GenreLexeme::PLUS_UNAIRE:
            case GenreLexeme::FOIS_UNAIRE:
            case GenreLexeme::ESP_UNAIRE:
            case GenreLexeme::MOINS_UNAIRE:
            case GenreLexeme::EXPANSION_VARIADIQUE:
            case GenreLexeme::ACCENT_GRAVE:
            {
                t[i] = Associativite::DROITE;
                break;
            }
            default:
            {
                break;
            }
        }
    }

    return t;
}();

static constexpr int PRECEDENCE_VIRGULE = 3;
static constexpr int PRECEDENCE_TYPE = 4;

static constexpr auto table_precedence_lexemes = [] {
    std::array<char, 256> t{};

    for (auto i = 0u; i < 256; ++i) {
        t[i] = -1;

        switch (static_cast<GenreLexeme>(i)) {
            case GenreLexeme::TROIS_POINTS:
            {
                t[i] = 1;
                break;
            }
            case GenreLexeme::EGAL:
            case GenreLexeme::DECLARATION_VARIABLE:
            case GenreLexeme::DECLARATION_CONSTANTE:
            case GenreLexeme::PLUS_EGAL:
            case GenreLexeme::MOINS_EGAL:
            case GenreLexeme::DIVISE_EGAL:
            case GenreLexeme::MULTIPLIE_EGAL:
            case GenreLexeme::MODULO_EGAL:
            case GenreLexeme::ET_EGAL:
            case GenreLexeme::OU_EGAL:
            case GenreLexeme::OUX_EGAL:
            case GenreLexeme::DEC_DROITE_EGAL:
            case GenreLexeme::DEC_GAUCHE_EGAL:
            {
                t[i] = 2;
                break;
            }
            case GenreLexeme::VIRGULE:
            {
                t[i] = PRECEDENCE_VIRGULE;
                break;
            }
            case GenreLexeme::DOUBLE_POINTS:
            {
                t[i] = PRECEDENCE_TYPE;
                break;
            }
            case GenreLexeme::BARRE_BARRE:
            {
                t[i] = 5;
                break;
            }
            case GenreLexeme::ESP_ESP:
            {
                t[i] = 6;
                break;
            }
            case GenreLexeme::BARRE:
            {
                t[i] = 7;
                break;
            }
            case GenreLexeme::CHAPEAU:
            {
                t[i] = 8;
                break;
            }
            case GenreLexeme::ESPERLUETTE:
            {
                t[i] = 9;
                break;
            }
            case GenreLexeme::DIFFERENCE:
            case GenreLexeme::EGALITE:
            {
                t[i] = 10;
                break;
            }
            case GenreLexeme::INFERIEUR:
            case GenreLexeme::INFERIEUR_EGAL:
            case GenreLexeme::SUPERIEUR:
            case GenreLexeme::SUPERIEUR_EGAL:
            {
                t[i] = 11;
                break;
            }
            case GenreLexeme::DECALAGE_GAUCHE:
            case GenreLexeme::DECALAGE_DROITE:
            {
                t[i] = 12;
                break;
            }
            case GenreLexeme::PLUS:
            case GenreLexeme::MOINS:
            {
                t[i] = 13;
                break;
            }
            case GenreLexeme::FOIS:
            case GenreLexeme::DIVISE:
            case GenreLexeme::POURCENT:
            {
                t[i] = 14;
                break;
            }
            case GenreLexeme::COMME:
            {
                t[i] = 15;
                break;
            }
            case GenreLexeme::EXCLAMATION:
            case GenreLexeme::TILDE:
            case GenreLexeme::PLUS_UNAIRE:
            case GenreLexeme::MOINS_UNAIRE:
            case GenreLexeme::FOIS_UNAIRE:
            case GenreLexeme::ESP_UNAIRE:
            case GenreLexeme::EXPANSION_VARIADIQUE:
            case GenreLexeme::ACCENT_GRAVE:
            {
                t[i] = 16;
                break;
            }
            case GenreLexeme::PARENTHESE_OUVRANTE:
            case GenreLexeme::POINT:
            case GenreLexeme::CROCHET_OUVRANT:
            {
                t[i] = 17;
                break;
            }
            default:
            {

                break;
            }
        }
    }

    return t;
}();

static inline bool est_operateur_surchargeable(GenreLexeme genre)
{
    return (table_drapeaux_lexemes[static_cast<size_t>(genre)] & OPERATEUR_EST_SURCHARGEABLE) != 0;
}

static inline int precedence_pour_operateur(GenreLexeme genre_operateur)
{
    int precedence = table_precedence_lexemes[static_cast<size_t>(genre_operateur)];
    assert_rappel(precedence != -1, [&]() {
        std::cerr << "Aucune précédence pour l'opérateur : " << chaine_du_lexème(genre_operateur)
                  << '\n';
    });
    return precedence;
}

static inline Associativite associativite_pour_operateur(GenreLexeme genre_operateur)
{
    auto associativite = table_associativite_lexemes[static_cast<size_t>(genre_operateur)];
    assert_rappel(associativite != static_cast<Associativite>(-1), [&]() {
        std::cerr << "Aucune précédence pour l'opérateur : " << chaine_du_lexème(genre_operateur)
                  << '\n';
    });
    return associativite;
}

Syntaxeuse::Syntaxeuse(Tacheronne &tacheronne, UniteCompilation *unite)
    : BaseSyntaxeuse(unite->fichier), m_compilatrice(tacheronne.compilatrice),
      m_tacheronne(tacheronne), m_unite(unite)
{
    auto module = m_fichier->module;

    m_tacheronne.assembleuse->depile_tout();

    module->mutex.lock();
    {
        if (module->bloc == nullptr) {
            module->bloc = m_tacheronne.assembleuse->empile_bloc(lexeme_courant(), nullptr);

            if (module->nom() != ID::Kuri) {
                /* Crée un membre pour l'import implicite du module Kuri afin de pouvoir accéder
                 * aux fonctions de ce module via une expression de référence de membre :
                 * « Kuri.fonction(...) ». */
                static Lexeme lexème_ident_kuri = {};
                lexème_ident_kuri.genre = GenreLexeme::CHAINE_CARACTERE;
                lexème_ident_kuri.ident = ID::Kuri;
                auto noeud_module = m_tacheronne.assembleuse
                                        ->crée_noeud<GenreNoeud::DECLARATION_MODULE>(
                                            &lexème_ident_kuri)
                                        ->comme_declaration_module();
                noeud_module->module = m_compilatrice.module_kuri;
                noeud_module->ident = ID::Kuri;
                noeud_module->bloc_parent = module->bloc;
                noeud_module->bloc_parent->ajoute_membre(noeud_module);
                noeud_module->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
            }

            module->bloc->ident = module->nom();
        }
        else {
            m_tacheronne.assembleuse->bloc_courant(module->bloc);
        }
    }
    module->mutex.unlock();
}

void Syntaxeuse::quand_commence()
{
    /* Nous faisons ça ici afin de ne pas trop avoir de méprédictions de branches
     * dans la boucle principale (qui ne sera alors pas exécutée car les lexèmes
     * auront été consommés). */
    if (!m_fichier->métaprogramme_corps_texte) {
        return;
    }

    auto metaprogramme = m_fichier->métaprogramme_corps_texte;

    if (metaprogramme->corps_texte_pour_fonction) {
        auto recipiente = metaprogramme->corps_texte_pour_fonction;
        m_tacheronne.assembleuse->bloc_courant(recipiente->bloc_parametres);

        fonctions_courantes.empile(recipiente);
        recipiente->corps->bloc = analyse_bloc(false);
        recipiente->corps->est_corps_texte = false;
        recipiente->drapeaux_fonction &= ~DrapeauxNoeudFonction::EST_MÉTAPROGRAMME;
        fonctions_courantes.depile();
    }
    else if (metaprogramme->corps_texte_pour_structure) {
        auto recipiente = metaprogramme->corps_texte_pour_structure;
        recipiente->arbre_aplatis.efface();

        auto bloc_parent = (recipiente->bloc_constantes) ? recipiente->bloc_constantes :
                                                           recipiente->bloc_parent;
        m_tacheronne.assembleuse->bloc_courant(bloc_parent);

        recipiente->bloc = analyse_bloc(false);
        recipiente->bloc->fusionne_membres(recipiente->bloc_constantes);
        recipiente->est_corps_texte = false;
    }

    metaprogramme->fut_execute = true;
}

void Syntaxeuse::quand_termine()
{
    m_tacheronne.assembleuse->depile_tout();
}

void Syntaxeuse::analyse_une_chose()
{
    if (ignore_point_virgule_implicite()) {
        return;
    }

    auto lexeme = lexeme_courant();
    auto genre_lexeme = lexeme->genre;

    if (genre_lexeme == GenreLexeme::IMPORTE) {
        consomme();

        auto noeud = m_tacheronne.assembleuse->crée_importe(lexeme);
        noeud->bloc_parent->ajoute_expression(noeud);

        if (!apparie(GenreLexeme::CHAINE_LITTERALE) && !apparie(GenreLexeme::CHAINE_CARACTERE)) {
            rapporte_erreur("Attendu une chaine littérale après 'importe'");
        }

        noeud->expression = m_tacheronne.assembleuse->crée_reference_declaration(lexeme_courant());

        requiers_typage(noeud);
        m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::IMPORTE;

        consomme();
    }
    else if (genre_lexeme == GenreLexeme::CHARGE) {
        consomme();

        auto noeud = m_tacheronne.assembleuse->crée_charge(lexeme);
        noeud->bloc_parent->ajoute_expression(noeud);

        if (!apparie(GenreLexeme::CHAINE_LITTERALE) && !apparie(GenreLexeme::CHAINE_CARACTERE)) {
            rapporte_erreur("Attendu une chaine littérale après 'charge'");
        }

        noeud->expression = m_tacheronne.assembleuse->crée_reference_declaration(lexeme_courant());

        requiers_typage(noeud);
        m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::CHARGE;

        consomme();
    }
    else if (apparie_expression()) {
        auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);

        if (!noeud) {
            /* Ceci peut arriver si nous avons une erreur. */
            return;
        }

        if (noeud->est_declaration()) {
            noeud->drapeaux |= DrapeauxNoeud::EST_GLOBALE;

            if (noeud->est_declaration_variable()) {
                noeud->bloc_parent->ajoute_membre(noeud->comme_declaration_variable());
                if (noeud->ident == ID::__contexte_fil_principal) {
                    m_compilatrice.globale_contexte_programme =
                        noeud->comme_declaration_variable();
                }
                requiers_typage(noeud);
            }
            else if (noeud->est_entete_fonction()) {
                requiers_typage(noeud);
            }
            else if (noeud->est_type_opaque()) {
                requiers_typage(noeud);
            }
            else if (noeud->est_type_structure()) {
                requiers_typage(noeud);
            }
            else if (noeud->est_declaration_bibliotheque()) {
                requiers_typage(noeud);
            }
        }
        else if (noeud->est_execute()) {
            if (noeud->ident != ID::test || m_compilatrice.arguments.active_tests) {
                requiers_typage(noeud);
            }
        }
        else if (noeud->est_ajoute_fini() || noeud->est_ajoute_init()) {
            requiers_typage(noeud);
        }
        else if (noeud->est_dependance_bibliotheque()) {
            requiers_typage(noeud);
        }
        else {
            m_unite->espace->rapporte_erreur(
                noeud,
                "Expression invalide pour le contexte global. Le contexte global doit contenir "
                "des déclarations ou des directives.");
            m_possède_erreur = true;
        }

        noeud->bloc_parent->ajoute_expression(noeud);
    }
    else {
        rapporte_erreur("attendu une expression ou une instruction");
    }
}

bool Syntaxeuse::apparie_expression() const
{
    auto genre = lexeme_courant()->genre;
    return (table_drapeaux_lexemes[static_cast<size_t>(genre)] & EST_EXPRESSION) != 0;
}

bool Syntaxeuse::apparie_expression_unaire() const
{
    auto genre = lexeme_courant()->genre;
    return (table_drapeaux_lexemes[static_cast<size_t>(genre)] & EST_EXPRESSION_UNAIRE) != 0;
}

bool Syntaxeuse::apparie_expression_secondaire() const
{
    auto genre = lexeme_courant()->genre;
    return (table_drapeaux_lexemes[static_cast<size_t>(genre)] & EST_EXPRESSION_SECONDAIRE) != 0;
}

bool Syntaxeuse::apparie_instruction() const
{
    auto genre = lexeme_courant()->genre;
    return (table_drapeaux_lexemes[static_cast<size_t>(genre)] & EST_INSTRUCTION) != 0;
}

NoeudExpression *Syntaxeuse::analyse_expression(DonneesPrecedence const &donnees_precedence,
                                                GenreLexeme racine_expression,
                                                GenreLexeme lexeme_final)
{
    auto expression = analyse_expression_primaire(racine_expression, lexeme_final);

    while (!fini() && apparie_expression_secondaire() && lexeme_courant()->genre != lexeme_final) {
        auto nouvelle_precedence = precedence_pour_operateur(lexeme_courant()->genre);

        if (nouvelle_precedence < donnees_precedence.precedence) {
            break;
        }

        if (nouvelle_precedence == donnees_precedence.precedence &&
            donnees_precedence.associativite == Associativite::GAUCHE) {
            break;
        }

        auto nouvelle_associativite = associativite_pour_operateur(lexeme_courant()->genre);
        expression = analyse_expression_secondaire(expression,
                                                   {nouvelle_precedence, nouvelle_associativite},
                                                   racine_expression,
                                                   lexeme_final);
    }

    if (!expression) {
        rapporte_erreur("Attendu une expression primaire");
    }

    return expression;
}

NoeudExpression *Syntaxeuse::analyse_expression_unaire(GenreLexeme lexeme_final)
{
    auto lexeme = lexeme_courant();
    auto genre_noeud = GenreNoeud::OPERATEUR_UNAIRE;

    switch (lexeme->genre) {
        case GenreLexeme::MOINS:
        {
            lexeme->genre = GenreLexeme::MOINS_UNAIRE;
            break;
        }
        case GenreLexeme::PLUS:
        {
            lexeme->genre = GenreLexeme::PLUS_UNAIRE;
            break;
        }
        case GenreLexeme::EXPANSION_VARIADIQUE:
        case GenreLexeme::TROIS_POINTS:
        {
            lexeme->genre = GenreLexeme::EXPANSION_VARIADIQUE;
            genre_noeud = GenreNoeud::EXPANSION_VARIADIQUE;
            break;
        }
        case GenreLexeme::FOIS:
        {
            lexeme->genre = GenreLexeme::FOIS_UNAIRE;
            break;
        }
        case GenreLexeme::ESPERLUETTE:
        {
            lexeme->genre = GenreLexeme::ESP_UNAIRE;
            break;
        }
        case GenreLexeme::ESP_UNAIRE:
        case GenreLexeme::EXCLAMATION:
        case GenreLexeme::TILDE:
        case GenreLexeme::PLUS_UNAIRE:
        case GenreLexeme::MOINS_UNAIRE:
        case GenreLexeme::FOIS_UNAIRE:
        {
            break;
        }
        case GenreLexeme::ACCENT_GRAVE:
        {
            consomme();
            if (fini()) {
                return nullptr;
            }

            lexeme = lexeme_courant();
            consomme();
            auto noeud = m_tacheronne.assembleuse->crée_reference_declaration(lexeme);
            noeud->drapeaux |= DrapeauxNoeud::IDENTIFIANT_EST_ACCENTUÉ_GRAVE;
            return noeud;
        }
        default:
        {
            assert_rappel(false, [&]() {
                std::cerr << "Lexème inattendu comme opérateur unaire : "
                          << chaine_du_lexème(lexeme->genre) << '\n';
                std::cerr << crée_message_erreur("");
            });
            return nullptr;
        }
    }

    consomme();

    auto precedence = precedence_pour_operateur(lexeme->genre);
    auto associativite = associativite_pour_operateur(lexeme->genre);

    auto noeud = static_cast<NoeudExpressionUnaire *>(
        m_tacheronne.assembleuse->crée_noeud<GenreNoeud::OPERATEUR_UNAIRE>(lexeme));
    noeud->genre = genre_noeud;

    // cette vérification n'est utile que pour les arguments variadiques sans type
    if (apparie_expression()) {
        noeud->operande = analyse_expression(
            {precedence, associativite}, GenreLexeme::INCONNU, lexeme_final);
    }

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_expression_primaire(GenreLexeme racine_expression,
                                                         GenreLexeme lexeme_final)
{
    if (apparie_expression_unaire()) {
        return analyse_expression_unaire(lexeme_final);
    }

    auto lexeme = lexeme_courant();

    switch (lexeme->genre) {
        case GenreLexeme::CARACTERE:
        {
            consomme();
            return m_tacheronne.assembleuse->crée_litterale_caractere(lexeme);
        }
        case GenreLexeme::CHAINE_CARACTERE:
        {
            consomme();
            return m_tacheronne.assembleuse->crée_reference_declaration(lexeme);
        }
        case GenreLexeme::CHAINE_LITTERALE:
        {
            consomme();
            return m_tacheronne.assembleuse->crée_litterale_chaine(lexeme);
        }
        case GenreLexeme::TABLEAU:
        case GenreLexeme::CROCHET_OUVRANT:
        {
            consomme();
            lexeme->genre = GenreLexeme::TABLEAU;

            auto expression_entre_crochets = NoeudExpression::nul();
            if (apparie_expression()) {
                auto ancien_noeud_virgule = m_noeud_expression_virgule;
                m_noeud_expression_virgule = nullptr;
                expression_entre_crochets = analyse_expression(
                    {}, GenreLexeme::CROCHET_OUVRANT, GenreLexeme::INCONNU);
                m_noeud_expression_virgule = ancien_noeud_virgule;
            }

            ignore_point_virgule_implicite();

            consomme(GenreLexeme::CROCHET_FERMANT, "Attendu un crochet fermant");

            if (apparie_expression()) {
                // nous avons l'expression d'un type
                auto noeud = m_tacheronne.assembleuse->crée_expression_binaire(lexeme);
                noeud->operande_gauche = expression_entre_crochets;
                noeud->operande_droite = analyse_expression(
                    {PRECEDENCE_TYPE, Associativite::GAUCHE}, racine_expression, lexeme_final);

                return noeud;
            }

            auto noeud = m_tacheronne.assembleuse->crée_construction_tableau(lexeme);

            /* Le reste de la pipeline suppose que l'expression est une virgule,
             * donc créons une telle expression au cas où nous n'avons qu'un seul
             * élément dans le tableau. */
            if (!expression_entre_crochets->est_virgule()) {
                auto virgule = m_tacheronne.assembleuse->crée_virgule(lexeme);
                virgule->expressions.ajoute(expression_entre_crochets);
                expression_entre_crochets = virgule;
            }

            noeud->expression = expression_entre_crochets;

            return noeud;
        }
        case GenreLexeme::EMPL:
        {
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_empl(lexeme);
            noeud->expression = analyse_expression({}, GenreLexeme::EMPL, lexeme_final);
            return noeud;
        }
        case GenreLexeme::FAUX:
        case GenreLexeme::VRAI:
        {
            consomme();
            return m_tacheronne.assembleuse->crée_litterale_bool(lexeme);
        }
        case GenreLexeme::INFO_DE:
        {
            consomme();
            consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'info_de'");

            auto noeud = m_tacheronne.assembleuse->crée_info_de(lexeme);
            noeud->expression = analyse_expression({}, GenreLexeme::INFO_DE, GenreLexeme::INCONNU);

            consomme(GenreLexeme::PARENTHESE_FERMANTE,
                     "Attendu ')' après l'expression de 'info_de'");

            return noeud;
        }
        case GenreLexeme::INIT_DE:
        {
            consomme();
            consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'init_de'");

            auto noeud = m_tacheronne.assembleuse->crée_init_de(lexeme);
            noeud->expression = analyse_expression({}, GenreLexeme::INIT_DE, GenreLexeme::INCONNU);

            consomme(GenreLexeme::PARENTHESE_FERMANTE,
                     "Attendu ')' après l'expression de 'init_de'");

            return noeud;
        }
        case GenreLexeme::MEMOIRE:
        {
            consomme();
            consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'mémoire'");

            auto noeud = m_tacheronne.assembleuse->crée_memoire(lexeme);
            noeud->expression = analyse_expression({}, GenreLexeme::MEMOIRE, GenreLexeme::INCONNU);

            consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après l'expression");

            return noeud;
        }
        case GenreLexeme::NOMBRE_ENTIER:
        {
            consomme();
            return m_tacheronne.assembleuse->crée_litterale_entier(lexeme);
        }
        case GenreLexeme::NOMBRE_REEL:
        {
            consomme();
            return m_tacheronne.assembleuse->crée_litterale_reel(lexeme);
        }
        case GenreLexeme::NON_INITIALISATION:
        {
            consomme();
            return m_tacheronne.assembleuse->crée_non_initialisation(lexeme);
        }
        case GenreLexeme::NUL:
        {
            consomme();
            return m_tacheronne.assembleuse->crée_litterale_nul(lexeme);
        }
        case GenreLexeme::OPERATEUR:
        {
            return analyse_declaration_operateur();
        }
        case GenreLexeme::PARENTHESE_OUVRANTE:
        {
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_parenthese(lexeme);
            noeud->expression = analyse_expression(
                {}, GenreLexeme::PARENTHESE_OUVRANTE, GenreLexeme::INCONNU);

            consomme(GenreLexeme::PARENTHESE_FERMANTE, "attendu une parenthèse fermante");

            return noeud;
        }
        case GenreLexeme::TAILLE_DE:
        {
            consomme();
            consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'taille_de'");

            auto noeud = m_tacheronne.assembleuse->crée_taille_de(lexeme);
            noeud->expression = analyse_expression(
                {}, GenreLexeme::TAILLE_DE, GenreLexeme::INCONNU);

            consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après le type de 'taille_de'");

            return noeud;
        }
        case GenreLexeme::TYPE_DE:
        {
            consomme();
            consomme(GenreLexeme::PARENTHESE_OUVRANTE, "Attendu '(' après 'type_de'");

            auto noeud = m_tacheronne.assembleuse->crée_type_de(lexeme);
            noeud->expression = analyse_expression({}, GenreLexeme::TYPE_DE, GenreLexeme::INCONNU);

            consomme(GenreLexeme::PARENTHESE_FERMANTE, "Attendu ')' après le type de 'type_de'");

            return noeud;
        }
        case GenreLexeme::TENTE:
        {
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_tente(lexeme);
            noeud->expression_appelee = analyse_expression(
                {}, GenreLexeme::TENTE, GenreLexeme::INCONNU);

            if (apparie(GenreLexeme::PIEGE)) {
                consomme();

                if (apparie(GenreLexeme::NONATTEIGNABLE)) {
                    consomme();
                }
                else {
                    noeud->expression_piegee = analyse_expression(
                        {}, GenreLexeme::PIEGE, GenreLexeme::INCONNU);
                    noeud->bloc = analyse_bloc();
                }
            }

            return noeud;
        }
        case GenreLexeme::DIRECTIVE:
        {
            consomme();

            lexeme = lexeme_courant();
            auto directive = lexeme->ident;

            consomme();

            if (directive == ID::execute || directive == ID::assert_ || directive == ID::test) {
                auto noeud = m_tacheronne.assembleuse->crée_execute(lexeme);
                noeud->ident = directive;

                if (directive == ID::test) {
                    m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::TEST;
                    noeud->expression = analyse_bloc();
                }
                else {
                    if (directive == ID::execute) {
                        m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::EXÉCUTE;
                    }
                    else if (directive == ID::assert_) {
                        m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::ASSERT;
                    }
                    noeud->expression = analyse_expression(
                        {}, GenreLexeme::DIRECTIVE, GenreLexeme::INCONNU);
                }

                return noeud;
            }
            else if (directive == ID::corps_boucle) {
                return m_tacheronne.assembleuse->crée_directive_corps_boucle(lexeme);
            }
            else if (directive == ID::si) {
                m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::SI_STATIQUE;
                return analyse_instruction_si_statique(lexeme);
            }
            else if (directive == ID::saufsi) {
                m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::SI_STATIQUE;
                return analyse_instruction_si_statique(lexeme);
            }
            else if (directive == ID::cuisine) {
                auto noeud = m_tacheronne.assembleuse->crée_cuisine(lexeme);
                noeud->ident = directive;
                noeud->expression = analyse_expression(
                    {}, GenreLexeme::DIRECTIVE, GenreLexeme::INCONNU);
                m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::CUISINE;
                return noeud;
            }
            else if (directive == ID::dependance_bibliotheque) {
                auto noeud = m_tacheronne.assembleuse->crée_dependance_bibliotheque(lexeme);

                auto lexème_bibliothèque_dépendante = lexeme_courant();
                consomme(
                    GenreLexeme::CHAINE_CARACTERE,
                    "Attendue une chaine de caractère pour définir la bibliothèque dépendante");
                noeud->bibliothèque_dépendante =
                    m_tacheronne.assembleuse->crée_reference_declaration(
                        lexème_bibliothèque_dépendante);

                auto lexème_bibliothèque_dépendue = lexeme_courant();
                consomme(GenreLexeme::CHAINE_CARACTERE,
                         "Attendue une chaine de caractère pour définir la bibliothèque dépendue");
                noeud->bibliothèque_dépendue =
                    m_tacheronne.assembleuse->crée_reference_declaration(
                        lexème_bibliothèque_dépendue);
                return noeud;
            }
            else if (directive == ID::ajoute_init) {
                auto noeud = m_tacheronne.assembleuse->crée_ajoute_init(lexeme);
                noeud->ident = directive;
                noeud->expression = analyse_expression(
                    {}, GenreLexeme::DIRECTIVE, GenreLexeme::INCONNU);
                m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::AJOUTE_INIT;
                return noeud;
            }
            else if (directive == ID::ajoute_fini) {
                auto noeud = m_tacheronne.assembleuse->crée_ajoute_fini(lexeme);
                noeud->ident = directive;
                noeud->expression = analyse_expression(
                    {}, GenreLexeme::DIRECTIVE, GenreLexeme::INCONNU);
                m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::AJOUTE_FINI;
                return noeud;
            }
            else if (directive == ID::pre_executable) {
                auto noeud = m_tacheronne.assembleuse->crée_pre_executable(lexeme);
                noeud->ident = directive;
                noeud->expression = analyse_expression(
                    {}, GenreLexeme::DIRECTIVE, GenreLexeme::INCONNU);
                m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::PRÉ_EXÉCUTABLE;
                return noeud;
            }
            else if (directive == ID::nom_de_cette_fonction ||
                     directive == ID::chemin_de_ce_fichier ||
                     directive == ID::chemin_de_ce_module) {
                auto noeud = m_tacheronne.assembleuse->crée_directive_instrospection(lexeme);
                return noeud;
            }
            else {
                /* repositionne le lexème courant afin que les messages d'erreurs pointent au bon
                 * endroit */
                recule();
                rapporte_erreur("Directive d'expression primaire inconnue.");
            }

            return nullptr;
        }
        case GenreLexeme::DOLLAR:
        {
            consomme();
            lexeme = lexeme_courant();

            consomme(GenreLexeme::CHAINE_CARACTERE, "attendu une chaine de caractère après '$'");

            auto noeud = m_tacheronne.assembleuse->crée_reference_declaration(lexeme);

            auto noeud_decl_param = m_tacheronne.assembleuse->crée_declaration_variable(lexeme);
            noeud_decl_param->valeur = noeud;
            noeud->declaration_referee = noeud_decl_param;

            if (!bloc_constantes_polymorphiques.est_vide()) {
                auto bloc_constantes = bloc_constantes_polymorphiques.haut();

                if (bloc_constantes->declaration_pour_ident(noeud->ident) != nullptr) {
                    recule();
                    rapporte_erreur("redéfinition du type polymorphique");
                }

                bloc_constantes->ajoute_membre(noeud_decl_param);
            }
            else if (!m_est_declaration_type_opaque) {
                rapporte_erreur("déclaration d'un type polymorphique hors d'une fonction, d'une "
                                "structure, ou de la déclaration d'un type opaque");
            }

            if (apparie(GenreLexeme::DOUBLE_POINTS)) {
                consomme();
                noeud_decl_param->expression_type = analyse_expression(
                    {}, racine_expression, lexeme_final);
                /* Nous avons une déclaration de valeur polymorphique, retournons-la. */
                noeud_decl_param->drapeaux |= (DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE |
                                               DrapeauxNoeud::EST_CONSTANTE);
                return noeud_decl_param;
            }

            noeud->drapeaux |= DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE;
            noeud_decl_param->drapeaux |= (DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE |
                                           DrapeauxNoeud::EST_CONSTANTE);
            return noeud;
        }
        case GenreLexeme::FONC:
        {
            return analyse_declaration_fonction(lexeme);
        }
        /* Ceux-ci doivent déjà avoir été gérés. */
        case GenreLexeme::COROUT:
        case GenreLexeme::ENUM:
        case GenreLexeme::ENUM_DRAPEAU:
        case GenreLexeme::ERREUR:
        case GenreLexeme::STRUCT:
        case GenreLexeme::UNION:
        {
            auto message_erreur = enchaine(
                "« ",
                chaine_du_lexème(lexeme->genre),
                " » ne peut pas être utilisé comme expression primaire.\n");
            rapporte_erreur(message_erreur);
            return nullptr;
        }
        case GenreLexeme::SI:
        {
            return analyse_instruction_si(GenreNoeud::INSTRUCTION_SI);
        }
        case GenreLexeme::SAUFSI:
        {
            return analyse_instruction_si(GenreNoeud::INSTRUCTION_SAUFSI);
        }
        default:
        {
            if (est_identifiant_type(lexeme->genre)) {
                consomme();
                return m_tacheronne.assembleuse->crée_reference_type(lexeme);
            }

            rapporte_erreur("attendu une expression primaire");
            return nullptr;
        }
    }
}

NoeudExpression *Syntaxeuse::analyse_expression_secondaire(
    NoeudExpression *gauche,
    const DonneesPrecedence &donnees_precedence,
    GenreLexeme racine_expression,
    GenreLexeme lexeme_final)
{
    auto lexeme = lexeme_courant();

    switch (lexeme->genre) {
        case GenreLexeme::BARRE:
        case GenreLexeme::BARRE_BARRE:
        case GenreLexeme::CHAPEAU:
        case GenreLexeme::DECALAGE_DROITE:
        case GenreLexeme::DECALAGE_GAUCHE:
        case GenreLexeme::DEC_DROITE_EGAL:
        case GenreLexeme::DEC_GAUCHE_EGAL:
        case GenreLexeme::DIFFERENCE:
        case GenreLexeme::DIVISE:
        case GenreLexeme::DIVISE_EGAL:
        case GenreLexeme::EGALITE:
        case GenreLexeme::ESPERLUETTE:
        case GenreLexeme::ESP_ESP:
        case GenreLexeme::ET_EGAL:
        case GenreLexeme::FOIS:
        case GenreLexeme::INFERIEUR:
        case GenreLexeme::INFERIEUR_EGAL:
        case GenreLexeme::MODULO_EGAL:
        case GenreLexeme::MOINS:
        case GenreLexeme::MOINS_EGAL:
        case GenreLexeme::MULTIPLIE_EGAL:
        case GenreLexeme::OUX_EGAL:
        case GenreLexeme::OU_EGAL:
        case GenreLexeme::PLUS:
        case GenreLexeme::PLUS_EGAL:
        case GenreLexeme::POURCENT:
        case GenreLexeme::SUPERIEUR:
        case GenreLexeme::SUPERIEUR_EGAL:
        {
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_expression_binaire(lexeme);
            noeud->operande_gauche = gauche;
            noeud->operande_droite = analyse_expression(
                donnees_precedence, racine_expression, lexeme_final);
            return noeud;
        }
        case GenreLexeme::VIRGULE:
        {
            consomme();

            auto noeud_expression_virgule = m_noeud_expression_virgule;

            if (!m_noeud_expression_virgule) {
                m_noeud_expression_virgule = m_tacheronne.assembleuse->crée_virgule(lexeme);
                m_noeud_expression_virgule->expressions.ajoute(gauche);
                noeud_expression_virgule = m_noeud_expression_virgule;
            }

            auto droite = analyse_expression(donnees_precedence, racine_expression, lexeme_final);

            m_noeud_expression_virgule = noeud_expression_virgule;
            m_noeud_expression_virgule->expressions.ajoute(droite);

            return m_noeud_expression_virgule;
        }
        case GenreLexeme::CROCHET_OUVRANT:
        {
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_indexage(lexeme);
            noeud->operande_gauche = gauche;
            noeud->operande_droite = analyse_expression(
                {}, GenreLexeme::CROCHET_OUVRANT, GenreLexeme::INCONNU);

            consomme(GenreLexeme::CROCHET_FERMANT, "attendu un crochet fermant");

            return noeud;
        }
        case GenreLexeme::DECLARATION_CONSTANTE:
        {
            if (!gauche->est_reference_declaration()) {
                rapporte_erreur("Attendu une référence à une déclaration à gauche de « :: »");
                return nullptr;
            }

            consomme();

            m_est_declaration_type_opaque = false;

            switch (lexeme_courant()->genre) {
                default:
                {
                    break;
                }
                case GenreLexeme::COROUT:
                case GenreLexeme::FONC:
                {
                    auto noeud_fonction = analyse_declaration_fonction(gauche->lexeme);

                    if (noeud_fonction->est_declaration_type) {
                        auto noeud = m_tacheronne.assembleuse->crée_declaration_variable(lexeme);
                        noeud->ident = gauche->ident;
                        noeud->valeur = gauche;
                        noeud->expression = noeud_fonction;
                        noeud->drapeaux |= DrapeauxNoeud::EST_CONSTANTE;
                        gauche->drapeaux |= DrapeauxNoeud::EST_CONSTANTE;
                        gauche->comme_reference_declaration()->declaration_referee = noeud;

                        return noeud;
                    }

                    return noeud_fonction;
                }
                case GenreLexeme::STRUCT:
                {
                    return analyse_declaration_structure(gauche);
                }
                case GenreLexeme::UNION:
                {
                    return analyse_declaration_union(gauche);
                }
                case GenreLexeme::ENUM:
                case GenreLexeme::ENUM_DRAPEAU:
                case GenreLexeme::ERREUR:
                {
                    return analyse_declaration_enum(gauche);
                }
                case GenreLexeme::DIRECTIVE:
                {
                    auto position = m_position;

                    consomme();

                    auto directive = lexeme_courant()->ident;

                    if (directive == ID::bibliotheque) {
                        consomme();
                        auto chaine_bib = lexeme_courant()->chaine;
                        consomme(GenreLexeme::CHAINE_LITTERALE,
                                 "Attendu une chaine littérale après la directive");
                        auto noeud = m_tacheronne.assembleuse->crée_declaration_bibliotheque(
                            lexeme);
                        noeud->ident = gauche->ident;
                        noeud->bibliotheque =
                            m_compilatrice.gestionnaire_bibliotheques->crée_bibliotheque(
                                *m_unite->espace, noeud, gauche->ident, chaine_bib);
                        return noeud;
                    }

                    if (directive == ID::opaque) {
                        consomme();
                        auto noeud = m_tacheronne.assembleuse->crée_type_opaque(gauche->lexeme);
                        m_est_declaration_type_opaque = true;
                        noeud->expression_type = analyse_expression(
                            donnees_precedence, racine_expression, lexeme_final);
                        m_est_declaration_type_opaque = false;
                        noeud->bloc_parent->ajoute_membre(noeud);
                        return noeud;
                    }

                    if (directive == ID::cuisine) {
                        recule();
                        break;
                    }

                    if (directive == ID::execute) {
                        recule();
                        break;
                    }

                    m_position = position - 1;
                    consomme();
                    break;
                }
            }

            auto noeud = m_tacheronne.assembleuse->crée_declaration_variable(lexeme);
            noeud->ident = gauche->ident;
            noeud->valeur = gauche;
            noeud->expression = analyse_expression(
                donnees_precedence, racine_expression, lexeme_final);
            noeud->drapeaux |= DrapeauxNoeud::EST_CONSTANTE;
            gauche->drapeaux |= DrapeauxNoeud::EST_CONSTANTE;

            if (gauche->est_reference_declaration()) {
                gauche->comme_reference_declaration()->declaration_referee = noeud;
            }

            return noeud;
        }
        case GenreLexeme::DOUBLE_POINTS:
        {
            consomme();

            /* Deux cas :
             * a, b : z32 = ...
             * a, b : z32 : ...
             * Dans les deux cas, nous vérifions que nous n'avons que des références séparées par
             * des virgules.
             */
            if (m_noeud_expression_virgule) {
                POUR (m_noeud_expression_virgule->expressions) {
                    if (it->est_declaration_variable()) {
                        m_unite->espace->rapporte_erreur(it,
                                                         "Obtenu une déclaration de variable au "
                                                         "sein d'une expression-virgule.");
                    }

                    if (!it->est_reference_declaration()) {
                        m_unite->espace->rapporte_erreur(
                            it, "Expression inattendue dans l'expression virgule.");
                    }
                }

                auto decl = m_tacheronne.assembleuse->crée_declaration_variable(lexeme);
                decl->valeur = m_noeud_expression_virgule;
                decl->expression_type = analyse_expression(
                    donnees_precedence, racine_expression, lexeme_final);

                if (!bloc_constantes_polymorphiques.est_vide()) {
                    decl->drapeaux |= DrapeauxNoeud::EST_LOCALE;
                }
                m_noeud_expression_virgule = nullptr;
                return decl;
            }

            if (gauche->est_reference_declaration()) {
                // nous avons la déclaration d'un type (a: z32)
                auto decl = m_tacheronne.assembleuse->crée_declaration_variable(
                    gauche->comme_reference_declaration());
                decl->expression_type = analyse_expression(
                    donnees_precedence, racine_expression, lexeme_final);
                if (!bloc_constantes_polymorphiques.est_vide()) {
                    decl->drapeaux |= DrapeauxNoeud::EST_LOCALE;
                }
                analyse_directive_déclaration_variable(decl);
                return decl;
            }

            if (gauche->est_declaration_variable()) {
                // nous avons la déclaration d'une constante (a: z32 : 12)
                auto decl = gauche->comme_declaration_variable();
                decl->expression = analyse_expression(
                    donnees_precedence, racine_expression, lexeme_final);
                decl->drapeaux |= DrapeauxNoeud::EST_CONSTANTE;
                return decl;
            }

            m_unite->espace->rapporte_erreur(gauche,
                                             "Expression inattendu à gauche du double-point");
            return nullptr;
        }
        case GenreLexeme::DECLARATION_VARIABLE:
        {
            if (gauche->est_declaration_variable()) {
                m_unite->espace->rapporte_erreur(
                    gauche, "Utilisation de « := » alors qu'un type fut déclaré avec « : »");
            }

            if (gauche->est_virgule()) {
                auto noeud_virgule = gauche->comme_virgule();

                // détecte les expressions du style : a : z32, b := ... , a[0] := ..., etc.
                POUR (noeud_virgule->expressions) {
                    if (it->est_declaration_variable()) {
                        m_unite->espace->rapporte_erreur(
                            it, "Utilisation de « := » alors qu'un type fut déclaré avec « : ».");
                    }

                    if (!it->est_reference_declaration()) {
                        m_unite->espace->rapporte_erreur(
                            it, "Expression inattendue à gauche de « := »");
                    }

                    auto decl = m_tacheronne.assembleuse->crée_declaration_variable(
                        it->comme_reference_declaration());
                    if (!bloc_constantes_polymorphiques.est_vide()) {
                        decl->drapeaux |= DrapeauxNoeud::EST_LOCALE;
                    }
                    decl->drapeaux |= DrapeauxNoeud::EST_DÉCLARATION_EXPRESSION_VIRGULE;
                }
            }
            else if (!gauche->est_reference_declaration()) {
                m_unite->espace->rapporte_erreur(gauche,
                                                 "Expression inattendue à gauche de « := »");
            }

            consomme();

            m_noeud_expression_virgule = nullptr;

            auto noeud = m_tacheronne.assembleuse->crée_declaration_variable(lexeme);
            noeud->ident = gauche->ident;
            noeud->valeur = gauche;
            noeud->expression = analyse_expression(
                donnees_precedence, racine_expression, lexeme_final);

            if (gauche->est_reference_declaration()) {
                gauche->comme_reference_declaration()->declaration_referee = noeud;
            }

            m_noeud_expression_virgule = nullptr;

            return noeud;
        }
        case GenreLexeme::EGAL:
        {
            consomme();

            m_noeud_expression_virgule = nullptr;

            if (gauche->est_declaration_variable()) {
                if (gauche->lexeme->genre == GenreLexeme::DECLARATION_VARIABLE) {
                    /* repositionne le lexème courant afin que les messages d'erreurs pointent au
                     * bon endroit */
                    recule();
                    rapporte_erreur("utilisation de '=' alors que nous somme à droite de ':='");
                }

                auto decl = gauche->comme_declaration_variable();
                decl->expression = analyse_expression(
                    donnees_precedence, racine_expression, lexeme_final);

                m_noeud_expression_virgule = nullptr;

                analyse_directive_déclaration_variable(decl);

                return decl;
            }

            if (gauche->est_virgule()) {
                auto noeud_virgule = gauche->comme_virgule();

                // détecte les expressions du style : a : z32, b = ...
                POUR (noeud_virgule->expressions) {
                    if (it->est_declaration_variable()) {
                        m_unite->espace->rapporte_erreur(
                            it,
                            "Obtenu une déclaration de variable dans l'expression séparée par "
                            "virgule à gauche d'une assignation. Les variables doivent être "
                            "déclarées avant leurs assignations.");
                    }
                }
            }

            auto noeud = m_tacheronne.assembleuse->crée_assignation_variable(lexeme);
            noeud->variable = gauche;
            noeud->expression = analyse_expression(
                donnees_precedence, racine_expression, lexeme_final);

            m_noeud_expression_virgule = nullptr;

            return noeud;
        }
        case GenreLexeme::POINT:
        {
            consomme();

            if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
                rapporte_erreur("Attendu un identifiant après '.'");
            }

            lexeme = lexeme_courant();
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_reference_membre(lexeme);
            noeud->accedee = gauche;
            return noeud;
        }
        case GenreLexeme::TROIS_POINTS:
        {
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_plage(lexeme);
            noeud->debut = gauche;
            noeud->fin = analyse_expression(donnees_precedence, racine_expression, lexeme_final);
            return noeud;
        }
        case GenreLexeme::PARENTHESE_OUVRANTE:
        {
            return analyse_appel_fonction(gauche);
        }
        case GenreLexeme::COMME:
        {
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_comme(lexeme);
            noeud->expression = gauche;
            noeud->expression_type = analyse_expression_primaire(GenreLexeme::COMME,
                                                                 GenreLexeme::INCONNU);
            return noeud;
        }
        default:
        {
            assert_rappel(false, [&]() {
                std::cerr << "Lexème inattendu comme expression secondaire : "
                          << chaine_du_lexème(lexeme->genre) << '\n';
                std::cerr << crée_message_erreur("");
            });
            return nullptr;
        }
    }
}

NoeudExpression *Syntaxeuse::analyse_instruction()
{
    auto lexeme = lexeme_courant();

    switch (lexeme->genre) {
        case GenreLexeme::ACCOLADE_OUVRANTE:
        {
            return analyse_bloc();
        }
        case GenreLexeme::DIFFERE:
        {
            consomme();
            auto inst = m_tacheronne.assembleuse->crée_differe(lexeme);

            if (apparie(GenreLexeme::ACCOLADE_OUVRANTE)) {
                inst->expression = analyse_bloc();
                static_cast<NoeudBloc *>(inst->expression)->appartiens_a_differe = inst;
            }
            else {
                inst->expression = analyse_expression(
                    {}, GenreLexeme::DIFFERE, GenreLexeme::INCONNU);
            }

            return inst;
        }
        case GenreLexeme::NONSUR:
        {
            consomme();
            auto bloc = analyse_bloc();
            bloc->est_nonsur = true;
            return bloc;
        }
        case GenreLexeme::ARRETE:
        {
            auto noeud = m_tacheronne.assembleuse->crée_arrete(lexeme);
            consomme();

            if (apparie(GenreLexeme::CHAINE_CARACTERE)) {
                noeud->expression = m_tacheronne.assembleuse->crée_reference_declaration(
                    lexeme_courant());
                consomme();
            }

            return noeud;
        }
        case GenreLexeme::CONTINUE:
        {
            auto noeud = m_tacheronne.assembleuse->crée_continue(lexeme);
            consomme();

            if (apparie(GenreLexeme::CHAINE_CARACTERE)) {
                noeud->expression = m_tacheronne.assembleuse->crée_reference_declaration(
                    lexeme_courant());
                consomme();
            }

            return noeud;
        }
        case GenreLexeme::REPRENDS:
        {
            auto noeud = m_tacheronne.assembleuse->crée_reprends(lexeme);
            consomme();

            if (apparie(GenreLexeme::CHAINE_CARACTERE)) {
                noeud->expression = m_tacheronne.assembleuse->crée_reference_declaration(
                    lexeme_courant());
                consomme();
            }

            return noeud;
        }
        case GenreLexeme::RETIENS:
        {
            auto noeud = m_tacheronne.assembleuse->crée_retiens(lexeme);
            consomme();

            if (apparie_expression()) {
                noeud->expression = analyse_expression_avec_virgule(GenreLexeme::RETIENS);
            }

            return noeud;
        }
        case GenreLexeme::RETOURNE:
        {
            auto noeud = m_tacheronne.assembleuse->crée_retourne(lexeme);
            consomme();

            if (apparie_expression()) {
                noeud->expression = analyse_expression_avec_virgule(GenreLexeme::RETOURNE);
            }

            return noeud;
        }
        case GenreLexeme::BOUCLE:
        {
            return analyse_instruction_boucle();
        }
        case GenreLexeme::DISCR:
        {
            return analyse_instruction_discr();
        }
        case GenreLexeme::POUR:
        {
            return analyse_instruction_pour();
        }
        case GenreLexeme::POUSSE_CONTEXTE:
        {
            return analyse_instruction_pousse_contexte();
        }
        case GenreLexeme::REPETE:
        {
            return analyse_instruction_repete();
        }
        case GenreLexeme::SAUFSI:
        {
            return analyse_instruction_si(GenreNoeud::INSTRUCTION_SAUFSI);
        }
        case GenreLexeme::SI:
        {
            return analyse_instruction_si(GenreNoeud::INSTRUCTION_SI);
        }
        case GenreLexeme::TANTQUE:
        {
            return analyse_instruction_tantque();
        }
        default:
        {
            assert_rappel(false, [&]() {
                std::cerr << "Lexème inattendu comme instruction : "
                          << chaine_du_lexème(lexeme->genre) << '\n';
                std::cerr << crée_message_erreur("");
            });
            return nullptr;
        }
    }
}

NoeudBloc *Syntaxeuse::analyse_bloc(bool accolade_requise)
{
    /* Pour les instructions de controles de flux, il est plus simple et plus robuste de détecter
     * un point-vigule implicite ici que de le faire pour chaque instruction. */
    ignore_point_virgule_implicite();

    auto lexeme = lexeme_courant();
    empile_etat("dans l'analyse du bloc", lexeme);

    if (accolade_requise) {
        consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{'");
    }

    NoeudDeclarationEnteteFonction *fonction_courante = fonctions_courantes.est_vide() ?
                                                            nullptr :
                                                            fonctions_courantes.haut();
    auto bloc = m_tacheronne.assembleuse->empile_bloc(lexeme, fonction_courante);

    auto expressions = kuri::tablet<NoeudExpression *, 32>();

    while (!fini() && !apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
        if (ignore_point_virgule_implicite()) {
            continue;
        }

        if (apparie_instruction()) {
            auto noeud = analyse_instruction();
            expressions.ajoute(noeud);
        }
        else if (apparie_expression()) {
            auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);
            expressions.ajoute(noeud);
        }
        else {
            rapporte_erreur("attendu une expression ou une instruction");
        }
    }

    copie_tablet_tableau(expressions, *bloc->expressions.verrou_ecriture());
    m_tacheronne.assembleuse->depile_bloc();

    if (accolade_requise) {
        consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu une accolade fermante '}'");
    }

    depile_etat();

    return bloc;
}

NoeudExpression *Syntaxeuse::analyse_appel_fonction(NoeudExpression *gauche)
{
    auto noeud = m_tacheronne.assembleuse->crée_appel(lexeme_courant());
    noeud->expression = gauche;

    consomme(GenreLexeme::PARENTHESE_OUVRANTE, "attendu une parenthèse ouvrante");

    auto params = kuri::tablet<NoeudExpression *, 16>();

    while (!fini() && apparie_expression()) {
        auto expr = analyse_expression({}, GenreLexeme::FONC, GenreLexeme::VIRGULE);
        params.ajoute(expr);

        if (expr->est_declaration_variable()) {
            rapporte_erreur("Obtenu une déclaration de variable dans l'expression d'appel");
        }

        if (ignore_point_virgule_implicite()) {
            if (!apparie(GenreLexeme::PARENTHESE_FERMANTE) && !apparie(GenreLexeme::VIRGULE)) {
                rapporte_erreur(
                    "Attendu une parenthèse fermante ou une virgule après la nouvelle ligne");
            }

            continue;
        }

        if (!apparie(GenreLexeme::VIRGULE)) {
            break;
        }

        consomme();
    }

    copie_tablet_tableau(params, noeud->parametres);

    consomme(GenreLexeme::PARENTHESE_FERMANTE,
             "attendu ')' à la fin des arguments de l'expression d'appel");

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_boucle()
{
    auto noeud = m_tacheronne.assembleuse->crée_boucle(lexeme_courant());
    consomme();
    noeud->bloc = analyse_bloc();
    noeud->bloc->appartiens_a_boucle = noeud;
    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_discr()
{
    auto noeud_discr = m_tacheronne.assembleuse->crée_discr(lexeme_courant());
    consomme();

    noeud_discr->expression_discriminee = analyse_expression(
        {}, GenreLexeme::DISCR, GenreLexeme::INCONNU);

    noeud_discr->bloc = m_tacheronne.assembleuse->empile_bloc(lexeme_courant(),
                                                              fonctions_courantes.haut());
    noeud_discr->bloc->appartiens_à_discr = noeud_discr;

    consomme(GenreLexeme::ACCOLADE_OUVRANTE,
             "Attendu une accolade ouvrante '{' après l'expression de « discr »");

    auto sinon_rencontre = false;

    auto paires_discr = kuri::tablet<NoeudPaireDiscr *, 32>();

    while (!fini() && !apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
        if (apparie(GenreLexeme::SINON)) {
            consomme();

            if (sinon_rencontre) {
                rapporte_erreur("Redéfinition d'un bloc sinon");
            }

            sinon_rencontre = true;

            noeud_discr->bloc_sinon = analyse_bloc();
        }
        else {
            auto expr = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);
            auto bloc = analyse_bloc();

            if (!expr->est_virgule()) {
                auto noeud_virgule = m_tacheronne.assembleuse->crée_virgule(expr->lexeme);
                noeud_virgule->expressions.ajoute(expr);

                expr = noeud_virgule;
            }
            else {
                m_noeud_expression_virgule = nullptr;
            }

            auto noeud_paire = m_tacheronne.assembleuse->crée_paire_discr(expr->lexeme);
            noeud_paire->expression = expr;
            noeud_paire->bloc = bloc;
            paires_discr.ajoute(noeud_paire);
        }
    }

    copie_tablet_tableau(paires_discr, noeud_discr->paires_discr);

    consomme(GenreLexeme::ACCOLADE_FERMANTE,
             "Attendu une accolade fermante '}' à la fin du bloc de « discr »");

    m_tacheronne.assembleuse->depile_bloc();
    return noeud_discr;
}

void Syntaxeuse::analyse_specifiants_instruction_pour(NoeudPour *noeud)
{
    bool eu_direction = false;

    while (!fini()) {
        switch (lexeme_courant()->genre) {
            default:
            {
                return;
            }
            case GenreLexeme::ESPERLUETTE:
            case GenreLexeme::ESP_UNAIRE:
            {
                if (noeud->prend_reference) {
                    rapporte_erreur("redéfinition d'une prise de référence");
                }

                if (noeud->prend_pointeur) {
                    rapporte_erreur("définition d'une prise de référence alors qu'une prise de "
                                    "pointeur fut spécifiée");
                }

                noeud->prend_reference = true;
                consomme();
                break;
            }
            case GenreLexeme::FOIS:
            case GenreLexeme::FOIS_UNAIRE:
            {
                if (noeud->prend_pointeur) {
                    rapporte_erreur("redéfinition d'une prise de pointeur");
                }

                if (noeud->prend_reference) {
                    rapporte_erreur("définition d'une prise de pointeur alors qu'une prise de "
                                    "référence fut spécifiée");
                }

                noeud->prend_pointeur = true;
                consomme();
                break;
            }
            case GenreLexeme::INFERIEUR:
            case GenreLexeme::SUPERIEUR:
            {
                if (eu_direction) {
                    rapporte_erreur(
                        "redéfinition d'une direction alors qu'une autre a déjà été spécifiée");
                }

                noeud->lexeme_op = lexeme_courant()->genre;
                eu_direction = true;
                consomme();
                break;
            }
        }
    }
}

NoeudExpression *Syntaxeuse::analyse_instruction_pour()
{
    auto noeud = m_tacheronne.assembleuse->crée_pour(lexeme_courant());
    consomme();

    analyse_specifiants_instruction_pour(noeud);

    auto expression = analyse_expression({}, GenreLexeme::POUR, GenreLexeme::INCONNU);

    if (apparie(GenreLexeme::DANS)) {
        consomme();

        if (!expression->est_virgule()) {
            static Lexeme lexeme_virgule = {",", {}, GenreLexeme::VIRGULE, 0, 0, 0};
            auto noeud_virgule = m_tacheronne.assembleuse->crée_virgule(&lexeme_virgule);
            noeud_virgule->expressions.ajoute(expression);

            expression = noeud_virgule;
        }
        else {
            m_noeud_expression_virgule = nullptr;
        }

        noeud->variable = expression;
        noeud->expression = analyse_expression({}, GenreLexeme::DANS, GenreLexeme::INCONNU);
    }
    else {
        auto noeud_it = m_tacheronne.assembleuse->crée_reference_declaration(noeud->lexeme);
        noeud_it->ident = ID::it;

        auto noeud_index = m_tacheronne.assembleuse->crée_reference_declaration(noeud->lexeme);
        noeud_index->ident = ID::index_it;

        static Lexeme lexeme_virgule = {",", {}, GenreLexeme::VIRGULE, 0, 0, 0};
        auto noeud_virgule = m_tacheronne.assembleuse->crée_virgule(&lexeme_virgule);
        noeud_virgule->expressions.ajoute(noeud_it);
        noeud_virgule->expressions.ajoute(noeud_index);

        noeud->variable = noeud_virgule;
        noeud->expression = expression;
    }

    noeud->bloc = analyse_bloc();
    noeud->bloc->appartiens_a_boucle = noeud;

    if (apparie(GenreLexeme::SANSARRET)) {
        consomme();
        noeud->bloc_sansarret = analyse_bloc();
    }

    if (apparie(GenreLexeme::SINON)) {
        consomme();
        noeud->bloc_sinon = analyse_bloc();
    }

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_pousse_contexte()
{
    auto noeud = m_tacheronne.assembleuse->crée_pousse_contexte(lexeme_courant());
    consomme();

    noeud->expression = analyse_expression({}, GenreLexeme::POUSSE_CONTEXTE, GenreLexeme::INCONNU);
    noeud->bloc = analyse_bloc(true);

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_repete()
{
    auto noeud = m_tacheronne.assembleuse->crée_repete(lexeme_courant());
    consomme();

    noeud->bloc = analyse_bloc();
    noeud->bloc->appartiens_a_boucle = noeud;

    consomme(GenreLexeme::TANTQUE, "Attendu une 'tantque' après le bloc de 'répète'");

    noeud->condition = analyse_expression({}, GenreLexeme::REPETE, GenreLexeme::INCONNU);

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_si(GenreNoeud genre_noeud)
{
    empile_etat("dans l'analyse de l'instruction si", lexeme_courant());

    auto noeud = m_tacheronne.assembleuse->crée_si(lexeme_courant(), genre_noeud);
    consomme();

    noeud->condition = analyse_expression({}, GenreLexeme::SI, GenreLexeme::INCONNU);

    noeud->bloc_si_vrai = analyse_bloc();

    if (apparie(GenreLexeme::SINON)) {
        consomme();

        if (apparie(GenreLexeme::SI)) {
            noeud->bloc_si_faux = analyse_instruction_si(GenreNoeud::INSTRUCTION_SI);
        }
        else if (apparie(GenreLexeme::SAUFSI)) {
            noeud->bloc_si_faux = analyse_instruction_si(GenreNoeud::INSTRUCTION_SAUFSI);
        }
        else if (apparie(GenreLexeme::TANTQUE)) {
            noeud->bloc_si_faux = analyse_instruction_tantque();
        }
        else if (apparie(GenreLexeme::POUR)) {
            noeud->bloc_si_faux = analyse_instruction_pour();
        }
        else if (apparie(GenreLexeme::BOUCLE)) {
            noeud->bloc_si_faux = analyse_instruction_boucle();
        }
        else if (apparie(GenreLexeme::REPETE)) {
            noeud->bloc_si_faux = analyse_instruction_repete();
        }
        else if (apparie(GenreLexeme::DISCR)) {
            noeud->bloc_si_faux = analyse_instruction_discr();
        }
        else {
            noeud->bloc_si_faux = analyse_bloc();
        }
    }

    depile_etat();

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_si_statique(Lexeme *lexeme)
{
    empile_etat("dans l'analyse de l'instruction #si", lexeme);

    auto noeud = (lexeme->genre == GenreLexeme::SI) ?
                     m_tacheronne.assembleuse->crée_si_statique(lexeme) :
                     m_tacheronne.assembleuse->crée_saufsi_statique(lexeme);

    noeud->condition = analyse_expression({}, GenreLexeme::SI, GenreLexeme::INCONNU);

    noeud->bloc_si_vrai = analyse_bloc();

    if (apparie(GenreLexeme::SINON)) {
        consomme();

        if (apparie(GenreLexeme::SI)) {
            /* analyse_instruction_si_statique doit commencer par le lexème suivant vu la manière
             * dont les directives sont parsées. */
            lexeme = lexeme_courant();
            consomme();
            noeud->bloc_si_faux = analyse_instruction_si_statique(lexeme);
        }
        else if (apparie(GenreLexeme::SAUFSI)) {
            /* analyse_instruction_si_statique doit commencer par le lexème suivant vu la manière
             * dont les directives sont parsées. */
            lexeme = lexeme_courant();
            consomme();
            noeud->bloc_si_faux = analyse_instruction_si_statique(lexeme);
        }
        else if (apparie(GenreLexeme::ACCOLADE_OUVRANTE)) {
            noeud->bloc_si_faux = analyse_bloc();
        }
        else {
            rapporte_erreur("l'instruction « sinon » des #si statiques doit être suivie par soit "
                            "un « si », soit un « saufsi », ou alors un bloc");
        }
    }

    depile_etat();

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_tantque()
{
    auto noeud = m_tacheronne.assembleuse->crée_tantque(lexeme_courant());
    consomme();

    noeud->condition = analyse_expression({}, GenreLexeme::TANTQUE, GenreLexeme::INCONNU);
    noeud->bloc = analyse_bloc();
    noeud->bloc->appartiens_a_boucle = noeud;

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_expression_avec_virgule(GenreLexeme lexème_racine)
{
    kuri::tablet<NoeudExpression *, 6> expressions;
    Lexeme *lexeme_virgule = nullptr;

    while (!fini()) {
        auto expr = analyse_expression({}, lexème_racine, GenreLexeme::VIRGULE);
        expressions.ajoute(expr);

        if (!apparie(GenreLexeme::VIRGULE)) {
            break;
        }

        if (lexeme_virgule == nullptr) {
            lexeme_virgule = lexeme_courant();
        }

        consomme();
    }

    if (expressions.taille() == 1) {
        return expressions[0];
    }

    auto virgule = m_tacheronne.assembleuse->crée_virgule(lexeme_virgule);
    copie_tablet_tableau(expressions, virgule->expressions);
    m_noeud_expression_virgule = nullptr;
    return virgule;
}

void Syntaxeuse::analyse_annotations(kuri::tableau<Annotation, int> &annotations)
{
    while (!fini() && apparie(GenreLexeme::AROBASE)) {
        consomme();

        if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
            rapporte_erreur("Attendu une chaine de caractère après '@'");
        }

        auto lexeme_annotation = lexeme_courant();
        auto annotation = Annotation{};
        annotation.nom = lexeme_annotation->chaine;

        consomme();

        if (apparie(GenreLexeme::CHAINE_LITTERALE)) {
            auto chaine = lexeme_courant()->chaine;
            annotation.valeur = chaine;
            consomme();
        }

        annotations.ajoute(annotation);
    }
}

NoeudExpression *Syntaxeuse::analyse_declaration_enum(NoeudExpression *gauche)
{
    auto lexeme = lexeme_courant();
    empile_etat("dans le syntaxage de l'énumération", lexeme);
    consomme();

    auto noeud_decl = m_tacheronne.assembleuse->crée_type_enum(gauche->lexeme);

    if (lexeme->genre != GenreLexeme::ERREUR) {
        if (!apparie(GenreLexeme::ACCOLADE_OUVRANTE)) {
            noeud_decl->expression_type = analyse_expression_primaire(GenreLexeme::ENUM,
                                                                      GenreLexeme::INCONNU);
        }

        auto type = m_compilatrice.typeuse.reserve_type_enum(noeud_decl);
        type->est_drapeau = lexeme->genre == GenreLexeme::ENUM_DRAPEAU;
        noeud_decl->est_énum_drapeau = type->est_drapeau;
        noeud_decl->type = type;
    }
    else {
        auto type = m_compilatrice.typeuse.reserve_type_erreur(noeud_decl);
        type->est_erreur = true;
        noeud_decl->est_erreur = true;
        noeud_decl->type = type;
    }

    auto lexeme_bloc = lexeme_courant();
    consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu '{' après 'énum'");

    auto bloc = m_tacheronne.assembleuse->empile_bloc(lexeme_bloc, nullptr);

    auto expressions = kuri::tablet<NoeudExpression *, 16>();

    while (!fini() && !apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
        if (ignore_point_virgule_implicite()) {
            continue;
        }

        if (!apparie_expression()) {
            rapporte_erreur("Attendu une expression dans le bloc de l'énumération");
            continue;
        }

        auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);

        if (noeud->est_reference_declaration()) {
            auto decl_variable = m_tacheronne.assembleuse->crée_declaration_variable(
                noeud->comme_reference_declaration());
            decl_variable->drapeaux |= DrapeauxNoeud::EST_CONSTANTE;
            expressions.ajoute(decl_variable);
        }
        else {
            expressions.ajoute(noeud);
        }
    }

    copie_tablet_tableau(expressions, *bloc->expressions.verrou_ecriture());

    m_tacheronne.assembleuse->depile_bloc();
    noeud_decl->bloc = bloc;

    consomme(GenreLexeme::ACCOLADE_FERMANTE, "Attendu '}' à la fin de la déclaration de l'énum");

    analyse_annotations(noeud_decl->annotations);

    depile_etat();

    /* Attend d'avoir toutes les informations avant d'ajouter aux membres. */
    noeud_decl->bloc_parent->ajoute_membre(noeud_decl);
    return noeud_decl;
}

// Le macro DIFFERE perturbe le GenreLexem::DIFFERE...
#include "biblinternes/outils/garde_portee.h"

NoeudDeclarationEnteteFonction *Syntaxeuse::analyse_declaration_fonction(Lexeme const *lexeme)
{
    auto lexeme_mot_cle = lexeme_courant();

    empile_etat("dans le syntaxage de la fonction", lexeme_mot_cle);
    consomme();

    auto noeud = m_tacheronne.assembleuse->crée_entete_fonction(lexeme);
    noeud->est_coroutine = lexeme_mot_cle->genre == GenreLexeme::COROUT;

    // @concurrence critique, si nous avons plusieurs définitions
    if (noeud->ident == ID::principale) {
        if (m_unite->espace->fonction_principale) {
            m_unite->espace
                ->rapporte_erreur(noeud, "Redéfinition de la fonction principale pour cet espace.")
                .ajoute_message("La fonction principale fut déjà définie ici :\n")
                .ajoute_site(m_unite->espace->fonction_principale);
        }

        m_unite->espace->fonction_principale = noeud;
        noeud->drapeaux_fonction |= DrapeauxNoeudFonction::EST_RACINE;
    }

    auto lexeme_bloc = lexeme_courant();
    consomme(GenreLexeme::PARENTHESE_OUVRANTE,
             "Attendu une parenthèse ouvrante après le nom de la fonction");

    noeud->bloc_constantes = m_tacheronne.assembleuse->empile_bloc(lexeme_bloc, noeud);
    noeud->bloc_parametres = m_tacheronne.assembleuse->empile_bloc(lexeme_bloc, noeud);

    bloc_constantes_polymorphiques.empile(noeud->bloc_constantes);
    DIFFERE {
        auto bloc_constantes = bloc_constantes_polymorphiques.depile();
        if (bloc_constantes->nombre_de_membres() != 0) {
            noeud->drapeaux_fonction |= DrapeauxNoeudFonction::EST_POLYMORPHIQUE;
        }

        /* Ajoute les constantes polymorphiques de ce type dans ceux du bloc de constantes
         * polymorphiques du bloc parent.
         * Ceci nous permet par exemple d'avoir des déclarations du style :
         * ma_fonction :: (rappel: fonc(z32)($T)) -> T
         * où ma_fonction doit être marquée comme polymorphique.
         */
        if (noeud->est_declaration_type && !bloc_constantes_polymorphiques.est_vide()) {
            auto bloc_constantes_pere = bloc_constantes_polymorphiques.haut();
            bloc_constantes_pere->fusionne_membres(noeud->bloc_constantes);
        }
    };

    /* analyse les paramètres de la fonction */
    auto params = kuri::tablet<NoeudExpression *, 16>();

    auto eu_declarations = false;

    while (!fini() && !apparie(GenreLexeme::PARENTHESE_FERMANTE)) {
        auto param = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::VIRGULE);

        if (param->est_declaration_variable()) {
            auto decl_var = static_cast<NoeudDeclarationVariable *>(param);
            decl_var->drapeaux |= DrapeauxNoeud::EST_PARAMETRE;
            params.ajoute(decl_var);

            eu_declarations = true;

            analyse_annotations(decl_var->annotations);
        }
        else if (param->est_empl()) {
            auto decl_var = param->comme_empl()->expression->comme_declaration_variable();
            analyse_annotations(decl_var->annotations);
            params.ajoute(param);
        }
        else {
            params.ajoute(param);
        }

        if (!apparie(GenreLexeme::VIRGULE)) {
            break;
        }

        consomme();
    }

    copie_tablet_tableau(params, noeud->params);

    consomme(GenreLexeme::PARENTHESE_FERMANTE,
             "Attendu ')' à la fin des paramètres de la fonction");

    /* analyse les types de retour de la fonction */
    if (apparie(GenreLexeme::PARENTHESE_OUVRANTE)) {
        // nous avons la déclaration d'un type
        noeud->est_declaration_type = true;
        consomme();

        if (eu_declarations) {
            POUR (noeud->params) {
                if (it->est_declaration_variable()) {
                    m_unite->espace->rapporte_erreur(it,
                                                     "Obtenu la déclaration d'une variable dans "
                                                     "la déclaration d'un type de fonction");
                }
            }
        }

        while (!fini()) {
            auto type_declare = analyse_expression({}, GenreLexeme::FONC, GenreLexeme::VIRGULE);
            noeud->params_sorties.ajoute(static_cast<NoeudDeclarationVariable *>(type_declare));

            if (!apparie(GenreLexeme::VIRGULE)) {
                break;
            }

            consomme();
        }

        consomme(GenreLexeme::PARENTHESE_FERMANTE, "attendu une parenthèse fermante");
    }
    else {
        // nous avons la déclaration d'une fonction
        if (apparie(GenreLexeme::RETOUR_TYPE)) {
            consomme();
            analyse_expression_retour_type(noeud, false);
        }
        else {
            Lexeme *lexeme_rien = m_tacheronne.lexemes_extra.ajoute_element();
            *lexeme_rien = *lexeme;
            lexeme_rien->genre = GenreLexeme::RIEN;
            lexeme_rien->chaine = "";

            auto decl = crée_retour_défaut_fonction(m_tacheronne.assembleuse, lexeme_rien);

            noeud->params_sorties.ajoute(decl);
            noeud->param_sortie = noeud->params_sorties[0]->comme_declaration_variable();
        }

        ignore_point_virgule_implicite();

        DrapeauxNoeudFonction drapeaux_fonction = DrapeauxNoeudFonction::AUCUN;
        while (!fini() && apparie(GenreLexeme::DIRECTIVE)) {
            consomme();

            auto ident_directive = lexeme_courant()->ident;

            if (ident_directive == ID::enligne) {
                drapeaux_fonction |= DrapeauxNoeudFonction::FORCE_ENLIGNE;
            }
            else if (ident_directive == ID::horsligne) {
                drapeaux_fonction |= DrapeauxNoeudFonction::FORCE_HORSLIGNE;
            }
            else if (ident_directive == ID::externe) {
                noeud->drapeaux |= DrapeauxNoeud::EST_EXTERNE;
                drapeaux_fonction |= DrapeauxNoeudFonction::EST_EXTERNE;

                if (lexeme_mot_cle->genre == GenreLexeme::COROUT) {
                    rapporte_erreur("Une coroutine ne peut pas être externe");
                }

                consomme();
                analyse_directive_symbole_externe(noeud);
                /* recule car nous consommons à la fin de la boucle */
                recule();
            }
            else if (ident_directive == ID::principale) {
                if (noeud->ident != ID::__principale) {
                    rapporte_erreur("#principale utilisée sur une fonction n'étant pas la "
                                    "fonction __principale");
                }

                noeud->drapeaux |= DrapeauxNoeud::EST_EXTERNE;
                drapeaux_fonction |= DrapeauxNoeudFonction::EST_EXTERNE;
            }
            else if (ident_directive == ID::sanstrace) {
                drapeaux_fonction |= DrapeauxNoeudFonction::FORCE_SANSTRACE;
            }
            else if (ident_directive == ID::interface) {
                m_compilatrice.interface_kuri->mute_membre(noeud);
            }
            else if (ident_directive == ID::creation_contexte) {
                drapeaux_fonction |= DrapeauxNoeudFonction::FORCE_SANSTRACE;
                drapeaux_fonction |= DrapeauxNoeudFonction::EST_RACINE;
                m_compilatrice.interface_kuri->decl_creation_contexte = noeud;
            }
            else if (ident_directive == ID::compilatrice) {
                drapeaux_fonction |= (DrapeauxNoeudFonction::FORCE_SANSTRACE |
                                      DrapeauxNoeudFonction::EST_IPA_COMPILATRICE |
                                      DrapeauxNoeudFonction::EST_EXTERNE);

                if (!est_fonction_compilatrice(noeud->ident)) {
                    rapporte_erreur("#compilatrice utilisé sur une fonction ne faisant pas partie "
                                    "de l'IPA de la Compilatrice");
                }
            }
            else if (ident_directive == ID::sansbroyage) {
                drapeaux_fonction |= (DrapeauxNoeudFonction::FORCE_SANSBROYAGE);
            }
            else if (ident_directive == ID::racine) {
                drapeaux_fonction |= (DrapeauxNoeudFonction::EST_RACINE);
            }
            else if (ident_directive == ID::corps_texte) {
                noeud->corps->est_corps_texte = true;
            }
            else if (ident_directive == ID::cliche) {
                consomme();
                if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
                    rapporte_erreur("Attendu un identifiant après #cliche");
                }

                while (apparie(GenreLexeme::CHAINE_CARACTERE)) {
                    auto ident_cliché = lexeme_courant()->ident;

                    if (ident_cliché == ID::asa) {
                        drapeaux_fonction |= DrapeauxNoeudFonction::CLICHÉ_ASA_FUT_REQUIS;
                    }
                    else if (ident_cliché == ID::asa_canon) {
                        drapeaux_fonction |=
                            DrapeauxNoeudFonction::CLICHÉ_ASA_CANONIQUE_FUT_REQUIS;
                    }
                    else if (ident_cliché == ID::ri) {
                        drapeaux_fonction |= DrapeauxNoeudFonction::CLICHÉ_RI_FUT_REQUIS;
                    }
                    else if (ident_cliché == ID::ri_finale) {
                        drapeaux_fonction |= DrapeauxNoeudFonction::CLICHÉ_RI_FINALE_FUT_REQUIS;
                    }
                    else if (ident_cliché == ID::inst_mv) {
                        drapeaux_fonction |= DrapeauxNoeudFonction::CLICHÉ_CODE_BINAIRE_FUT_REQUIS;
                    }
                    else {
                        rapporte_erreur("Identifiant inconnu après #cliche");
                    }
                    consomme();

                    if (!apparie(GenreLexeme::VIRGULE)) {
                        break;
                    }
                    consomme();
                }

                /* Pour le consomme plus bas qui est sensé consommer l'identifiant de la directive.
                 */
                recule();
            }
            else if (ident_directive == ID::intrinsèque) {
                drapeaux_fonction |= DrapeauxNoeudFonction::FORCE_SANSTRACE;
                drapeaux_fonction |= DrapeauxNoeudFonction::EST_INTRINSÈQUE;
                drapeaux_fonction |= DrapeauxNoeudFonction::EST_EXTERNE;

                consomme();

                if (!apparie(GenreLexeme::CHAINE_LITTERALE)) {
                    rapporte_erreur("Attendu le symbole de l'intrinsèque");
                }

                noeud->données_externes =
                    m_tacheronne.allocatrice_noeud.crée_données_symbole_externe();
                noeud->données_externes->nom_symbole = lexeme_courant()->chaine;
            }
            else if (ident_directive == ID::interne) {
                noeud->visibilité_symbole = VisibilitéSymbole::INTERNE;
            }
            else if (ident_directive == ID::exporte) {
                noeud->visibilité_symbole = VisibilitéSymbole::EXPORTÉ;
            }
            else {
                rapporte_erreur("Directive de fonction inconnue.");
            }

            consomme();
        }

        noeud->drapeaux_fonction |= drapeaux_fonction;

        if (noeud->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE)) {
            if (noeud->params_sorties.taille() > 1) {
                rapporte_erreur(
                    "Ne peut avoir plusieurs valeur de retour pour une fonction externe");
            }

            /* ajoute un bloc même pour les fonctions externes, afin de stocker les paramètres */
            noeud->corps->bloc = m_tacheronne.assembleuse->empile_bloc(lexeme_courant(), noeud);
            m_tacheronne.assembleuse->depile_bloc();

            /* Si la déclaration est à la fin du fichier, il peut ne pas y avoir de point-virgule,
             * donc ne générons pas d'erreur s'il n'y en a pas. */
            ignore_point_virgule_implicite();
        }
        else {
            ignore_point_virgule_implicite();

            auto noeud_corps = noeud->corps;
            fonctions_courantes.empile(noeud);

            if (apparie(GenreLexeme::POUSSE_CONTEXTE)) {
                empile_etat("dans l'analyse du bloc", lexeme_courant());
                noeud_corps->bloc = m_tacheronne.assembleuse->empile_bloc(lexeme_courant(), noeud);
                auto pousse_contexte = analyse_instruction_pousse_contexte();
                noeud_corps->bloc->ajoute_expression(pousse_contexte);
                m_tacheronne.assembleuse->depile_bloc();
                depile_etat();
            }
            else {
                noeud_corps->bloc = analyse_bloc();
                noeud_corps->bloc->ident = noeud->ident;
            }

            analyse_annotations(noeud->annotations);
            fonctions_courantes.depile();
        }
    }

    /* dépile le bloc des paramètres */
    m_tacheronne.assembleuse->depile_bloc();

    /* dépile le bloc des constantes */
    m_tacheronne.assembleuse->depile_bloc();

    depile_etat();

    /* Faisons ceci à la fin afin que le corps soit disponible lors de la création de la copie pour
     * les différents espaces de travail, évitant une potentielle concurrence critique. */
    if (noeud->ident == ID::__point_d_entree_systeme) {
        m_compilatrice.fonction_point_d_entree = noeud;
    }
    else if (noeud->ident == ID::__point_d_entree_dynamique) {
        m_compilatrice.fonction_point_d_entree_dynamique = noeud;
    }
    else if (noeud->ident == ID::__point_de_sortie_dynamique) {
        m_compilatrice.fonction_point_de_sortie_dynamique = noeud;
    }

    /* Attend d'avoir toutes les informations avant d'ajouter aux membres. */
    if (!noeud->est_declaration_type) {
        noeud->bloc_parent->ajoute_membre(noeud);
    }

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_declaration_operateur()
{
    empile_etat("dans le syntaxage de l'opérateur", lexeme_courant());
    consomme();

    auto lexeme = lexeme_courant();
    auto genre_operateur = lexeme->genre;

    if (!est_operateur_surchargeable(genre_operateur)) {
        rapporte_erreur("L'opérateur n'est pas surchargeable");
        return nullptr;
    }

    consomme();

    if (genre_operateur == GenreLexeme::CROCHET_OUVRANT) {
        consomme(GenreLexeme::CROCHET_FERMANT,
                 "Attendu ']' après '[' pour la déclaration de l'opérateur");
    }

    // :: fonc
    consomme(GenreLexeme::DECLARATION_CONSTANTE, "Attendu :: après la déclaration de l'opérateur");
    consomme(GenreLexeme::FONC, "Attendu fonc après ::");

    auto noeud = (genre_operateur == GenreLexeme::POUR) ?
                     m_tacheronne.assembleuse->crée_operateur_pour(lexeme) :
                     m_tacheronne.assembleuse->crée_entete_fonction(lexeme);
    noeud->est_operateur = true;

    auto lexeme_bloc = lexeme_courant();
    consomme(GenreLexeme::PARENTHESE_OUVRANTE,
             "Attendu une parenthèse ouvrante après le nom de la fonction");

    noeud->bloc_constantes = m_tacheronne.assembleuse->empile_bloc(lexeme_bloc, noeud);
    noeud->bloc_parametres = m_tacheronne.assembleuse->empile_bloc(lexeme_bloc, noeud);

    /* analyse les paramètres de la fonction */
    auto params = kuri::tablet<NoeudExpression *, 16>();

    while (!fini() && !apparie(GenreLexeme::PARENTHESE_FERMANTE)) {
        auto param = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::VIRGULE);

        if (!param->est_declaration_variable()) {
            m_unite->espace->rapporte_erreur(
                param, "Expression inattendue dans la déclaration des paramètres de l'opérateur");
        }

        auto decl_var = param->comme_declaration_variable();
        decl_var->drapeaux |= DrapeauxNoeud::EST_PARAMETRE;
        params.ajoute(decl_var);

        if (!apparie(GenreLexeme::VIRGULE)) {
            break;
        }

        consomme();
    }

    copie_tablet_tableau(params, noeud->params);

    if (noeud->params.taille() > 2) {
        m_unite->espace->rapporte_erreur(
            noeud, "La surcharge d'opérateur ne peut prendre au plus 2 paramètres");
    }
    else if (noeud->params.taille() == 1) {
        if (genre_operateur == GenreLexeme::PLUS) {
            lexeme->genre = GenreLexeme::PLUS_UNAIRE;
        }
        else if (genre_operateur == GenreLexeme::MOINS) {
            lexeme->genre = GenreLexeme::MOINS_UNAIRE;
        }
        else if (!dls::outils::est_element(genre_operateur,
                                           GenreLexeme::TILDE,
                                           GenreLexeme::EXCLAMATION,
                                           GenreLexeme::PLUS_UNAIRE,
                                           GenreLexeme::MOINS_UNAIRE,
                                           GenreLexeme::POUR)) {
            rapporte_erreur("La surcharge d'opérateur unaire n'est possible que "
                            "pour '+', '-', '~', '!', ou 'pour'");
            return nullptr;
        }
    }

    consomme(GenreLexeme::PARENTHESE_FERMANTE,
             "Attendu ')' à la fin des paramètres de la fonction");

    /* analyse les types de retour de la fonction */
    consomme(GenreLexeme::RETOUR_TYPE, "Attendu un retour de type");

    analyse_expression_retour_type(noeud, true);

    while (!fini() && apparie(GenreLexeme::DIRECTIVE)) {
        consomme();

        auto directive = lexeme_courant()->ident;

        if (directive == ID::enligne) {
            noeud->drapeaux_fonction |= DrapeauxNoeudFonction::FORCE_ENLIGNE;
        }
        else if (directive == ID::horsligne) {
            noeud->drapeaux_fonction |= DrapeauxNoeudFonction::FORCE_HORSLIGNE;
        }
        else {
            rapporte_erreur("Directive d'opérateur inconnue.");
        }

        consomme();
    }

    if (!noeud->possède_drapeau(DrapeauxNoeudFonction::FORCE_HORSLIGNE)) {
        noeud->drapeaux_fonction |= DrapeauxNoeudFonction::FORCE_ENLIGNE;
    }

    ignore_point_virgule_implicite();

    fonctions_courantes.empile(noeud);
    auto noeud_corps = noeud->corps;
    noeud_corps->bloc = analyse_bloc();

    analyse_annotations(noeud->annotations);
    fonctions_courantes.depile();

    /* dépile le bloc des paramètres */
    m_tacheronne.assembleuse->depile_bloc();

    /* dépile le bloc des constantes */
    m_tacheronne.assembleuse->depile_bloc();

    depile_etat();

    return noeud;
}

void Syntaxeuse::analyse_expression_retour_type(NoeudDeclarationEnteteFonction *noeud,
                                                bool pour_operateur)
{
    auto eu_parenthese = false;
    if (apparie(GenreLexeme::PARENTHESE_OUVRANTE)) {
        consomme();
        eu_parenthese = true;
    }

    while (!fini()) {
        auto decl_sortie = analyse_expression({}, GenreLexeme::FONC, GenreLexeme::VIRGULE);

        if (!decl_sortie) {
            /* Nous avons une erreur, nous pouvons retourner. */
            return;
        }

        if (!decl_sortie->est_declaration_variable()) {
            auto ident = m_compilatrice.donne_nom_défaut_valeur_retour(
                noeud->params_sorties.taille());

            auto ref = m_tacheronne.assembleuse->crée_reference_declaration(decl_sortie->lexeme);
            ref->ident = ident;

            auto decl = m_tacheronne.assembleuse->crée_declaration_variable(ref);
            decl->expression_type = decl_sortie;
            decl->bloc_parent = decl_sortie->bloc_parent;

            decl_sortie = decl;
        }

        decl_sortie->drapeaux |= DrapeauxNoeud::EST_PARAMETRE;

        noeud->params_sorties.ajoute(decl_sortie->comme_declaration_variable());

        if (!apparie(GenreLexeme::VIRGULE)) {
            break;
        }

        consomme();
    }

    auto const nombre_de_valeurs_retournees = noeud->params_sorties.taille();
    if (nombre_de_valeurs_retournees == 0) {
        rapporte_erreur("Attendu au moins une déclaration de valeur retournée");
        return;
    }

    if (nombre_de_valeurs_retournees > 1) {
        if (pour_operateur) {
            rapporte_erreur("Il est impossible d'avoir plusieurs de sortie pour un opérateur");
            return;
        }

        auto ref = m_tacheronne.assembleuse->crée_reference_declaration(
            noeud->params_sorties[0]->lexeme);
        /* il nous faut un identifiant valide */
        ref->ident = m_compilatrice.table_identifiants->identifiant_pour_nouvelle_chaine(
            "valeur_de_retour");
        noeud->param_sortie = m_tacheronne.assembleuse->crée_declaration_variable(ref);
    }
    else {
        noeud->param_sortie = noeud->params_sorties[0]->comme_declaration_variable();
    }

    noeud->param_sortie->drapeaux |= DrapeauxNoeud::EST_PARAMETRE;

    if (eu_parenthese) {
        consomme(GenreLexeme::PARENTHESE_FERMANTE,
                 "attendu une parenthèse fermante après la liste des retours de la fonction");
    }
}

/* ------------------------------------------------------------------------- */
/** \name Structures et unions.
 * \{ */

NoeudExpression *Syntaxeuse::analyse_declaration_structure(NoeudExpression *gauche)
{
    auto lexeme_mot_cle = lexeme_courant();
    empile_etat("dans le syntaxage de la structure", lexeme_mot_cle);
    consomme();

    if (!gauche->est_reference_declaration()) {
        this->rapporte_erreur("Expression inattendue pour nommer la structure");
    }

    auto noeud_decl = m_tacheronne.assembleuse->crée_type_structure(gauche->lexeme);
    noeud_decl->est_union = false;

    if (gauche->ident == ID::InfoType) {
        noeud_decl->type = m_compilatrice.typeuse.type_info_type_;
        auto type_info_type = m_compilatrice.typeuse.type_info_type_->comme_type_structure();
        type_info_type->decl = noeud_decl;
        type_info_type->nom = noeud_decl->ident;
    }
    else if (gauche->ident == ID::ContexteProgramme) {
        auto type_contexte = m_compilatrice.typeuse.type_contexte->comme_type_structure();
        noeud_decl->type = type_contexte;
        type_contexte->decl = noeud_decl;
        type_contexte->nom = noeud_decl->ident;
    }
    else {
        noeud_decl->type = m_compilatrice.typeuse.reserve_type_structure(noeud_decl);
    }

    analyse_paramètres_polymorphiques_structure_ou_union(noeud_decl);
    analyse_directives_structure_ou_union(noeud_decl);
    analyse_membres_structure_ou_union(noeud_decl);
    analyse_annotations(noeud_decl->annotations);

    if (noeud_decl->bloc_constantes) {
        m_tacheronne.assembleuse->depile_bloc();
    }

    depile_etat();

    /* N'ajoute la structure au bloc parent que lorsque nous avons son bloc, ou la validation
     * sémantique pourrait accéder un bloc nul. */
    noeud_decl->bloc_parent->ajoute_membre(noeud_decl);

    return noeud_decl;
}

NoeudExpression *Syntaxeuse::analyse_declaration_union(NoeudExpression *gauche)
{
    auto lexeme_mot_cle = lexeme_courant();
    empile_etat("dans le syntaxage de l'union", lexeme_mot_cle);
    consomme();

    if (!gauche->est_reference_declaration()) {
        this->rapporte_erreur("Expression inattendue pour nommer l'union");
    }

    auto noeud_decl = m_tacheronne.assembleuse->crée_type_structure(gauche->lexeme);
    noeud_decl->est_union = true;
    noeud_decl->type = m_compilatrice.typeuse.reserve_type_union(noeud_decl);

    if (apparie(GenreLexeme::NONSUR)) {
        noeud_decl->est_nonsure = true;
        consomme();
    }

    analyse_paramètres_polymorphiques_structure_ou_union(noeud_decl);
    analyse_directives_structure_ou_union(noeud_decl);
    analyse_membres_structure_ou_union(noeud_decl);
    analyse_annotations(noeud_decl->annotations);

    if (noeud_decl->bloc_constantes) {
        m_tacheronne.assembleuse->depile_bloc();
    }

    depile_etat();

    /* N'ajoute la structure au bloc parent que lorsque nous avons son bloc, ou la validation
     * sémantique pourrait accéder un bloc nul. */
    noeud_decl->bloc_parent->ajoute_membre(noeud_decl);

    return noeud_decl;
}

void Syntaxeuse::analyse_directives_structure_ou_union(NoeudStruct *noeud)
{
    while (!fini() && apparie(GenreLexeme::DIRECTIVE)) {
        consomme();

        auto ident_directive = lexeme_courant()->ident;

        if (ident_directive == ID::interface) {
            renseigne_type_interface(m_compilatrice.typeuse, noeud->ident, noeud->type);
        }
        else if (ident_directive == ID::externe) {
            noeud->est_externe = true;
            // #externe implique nonsûr
            noeud->est_nonsure = noeud->est_union;
        }
        else if (ident_directive == ID::corps_texte) {
            noeud->est_corps_texte = true;
        }
        else if (ident_directive == ID::compacte) {
            if (noeud->est_union) {
                rapporte_erreur("Directive « compacte » impossible pour une union\n");
            }

            noeud->est_compacte = true;
        }
        else if (ident_directive == ID::aligne) {
            consomme();

            if (!apparie(GenreLexeme::NOMBRE_ENTIER)) {
                rapporte_erreur("Un nombre entier est requis après « aligne »");
            }

            noeud->alignement_desire = static_cast<uint32_t>(lexeme_courant()->valeur_entiere);

            if (!est_puissance_de_2(noeud->alignement_desire)) {
                rapporte_erreur("Un alignement doit être une puissance de 2");
            }
        }
        else {
            rapporte_erreur("Directive de structure inconnue.");
        }

        consomme();
    }
}

void Syntaxeuse::analyse_paramètres_polymorphiques_structure_ou_union(NoeudStruct *noeud)
{
    if (!apparie(GenreLexeme::PARENTHESE_OUVRANTE)) {
        return;
    }

    noeud->bloc_constantes = m_tacheronne.assembleuse->empile_bloc(lexeme_courant(), nullptr);

    bloc_constantes_polymorphiques.empile(noeud->bloc_constantes);
    DIFFERE {
        auto bloc_constantes = bloc_constantes_polymorphiques.depile();
        if (bloc_constantes->nombre_de_membres() != 0) {
            noeud->est_polymorphe = true;
        }
    };

    consomme();

    while (!fini() && !apparie(GenreLexeme::PARENTHESE_FERMANTE)) {
        auto expression = analyse_expression(
            {}, GenreLexeme::PARENTHESE_OUVRANTE, GenreLexeme::VIRGULE);

        if (!expression->est_declaration_variable()) {
            m_unite->espace->rapporte_erreur(expression,
                                             "Attendu une déclaration de variable dans les "
                                             "paramètres polymorphiques de la structure");
        }

        if (!apparie(GenreLexeme::VIRGULE)) {
            break;
        }

        consomme();
    }

    consomme();
}

void Syntaxeuse::analyse_membres_structure_ou_union(NoeudStruct *decl_struct)
{
    if (decl_struct->est_externe && ignore_point_virgule_implicite()) {
        decl_struct->type->drapeaux |= TYPE_NE_REQUIERS_PAS_D_INITIALISATION;
        return;
    }

    NoeudBloc *bloc;
    if (decl_struct->est_corps_texte) {
        bloc = analyse_bloc();
    }
    else {
        bloc = analyse_bloc_membres_structure_ou_union(decl_struct);
    }

    decl_struct->bloc = bloc;
    bloc->ident = decl_struct->ident;
}

static bool expression_est_valide_pour_bloc_structure(NoeudExpression *noeud)
{
    if (noeud->est_execute()) {
        return noeud->ident == ID::assert_;
    }

    return noeud->est_declaration() || noeud->est_assignation_variable() || noeud->est_empl() ||
           noeud->est_reference_declaration();
}

NoeudBloc *Syntaxeuse::analyse_bloc_membres_structure_ou_union(NoeudStruct *decl_struct)
{
    auto bloc = m_tacheronne.assembleuse->empile_bloc(lexeme_courant(), nullptr);
    consomme(GenreLexeme::ACCOLADE_OUVRANTE, "Attendu '{' après le nom de la structure");

    auto expressions = kuri::tablet<NoeudExpression *, 16>();

    while (!fini() && !apparie(GenreLexeme::ACCOLADE_FERMANTE)) {
        if (ignore_point_virgule_implicite()) {
            continue;
        }

        if (!apparie_expression()) {
            rapporte_erreur("attendu une expression");
            continue;
        }

        auto noeud = analyse_expression({}, GenreLexeme::INCONNU, GenreLexeme::INCONNU);

        if (!expression_est_valide_pour_bloc_structure(noeud)) {
            m_unite->espace->rapporte_erreur(noeud,
                                             "Expression invalide pour le bloc de la structure");
        }

        if (noeud->est_reference_declaration()) {
            if (!decl_struct->est_union || decl_struct->est_nonsure) {
                m_unite->espace->rapporte_erreur(
                    noeud, "Seules les unions sûres peuvent avoir des déclarations sans type");
            }

            auto decl_membre = m_tacheronne.assembleuse->crée_declaration_variable(
                noeud->comme_reference_declaration());
            noeud = decl_membre;

            static const Lexeme lexeme_rien = {"rien", {}, GenreLexeme::RIEN, 0, 0, 0};
            auto type_declare = m_tacheronne.assembleuse->crée_reference_type(&lexeme_rien);
            decl_membre->expression_type = type_declare;
        }

        if (noeud->est_declaration_variable()) {
            auto decl_membre = noeud->comme_declaration_variable();
            analyse_annotations(decl_membre->annotations);
            noeud->drapeaux |= DrapeauxNoeud::EST_MEMBRE_STRUCTURE;
        }

        expressions.ajoute(noeud);
    }

    copie_tablet_tableau(expressions, *bloc->expressions.verrou_ecriture());

    consomme(GenreLexeme::ACCOLADE_FERMANTE,
             "Attendu '}' à la fin de la déclaration de la structure");

    m_tacheronne.assembleuse->depile_bloc();

    return bloc;
}

/** \} */

void Syntaxeuse::gere_erreur_rapportee(const kuri::chaine &message_erreur)
{
    m_unite->espace->rapporte_erreur(
        SiteSource::cree(m_fichier, lexeme_courant()), message_erreur, erreur::Genre::SYNTAXAGE);
    /* Avance le curseur pour ne pas être bloqué. */
    consomme();
}

void Syntaxeuse::requiers_typage(NoeudExpression *noeud)
{
    /* N'envoie plus rien vers le typage si nous avons une erreur. */
    if (m_possède_erreur) {
        return;
    }

    m_fichier->noeuds_à_valider.ajoute(noeud);
}

bool Syntaxeuse::ignore_point_virgule_implicite()
{
    if (apparie(GenreLexeme::POINT_VIRGULE)) {
        consomme();
        return true;
    }

    return false;
}

void Syntaxeuse::analyse_directive_déclaration_variable(NoeudDeclarationVariable *déclaration)
{
    if (!apparie(GenreLexeme::DIRECTIVE)) {
        return;
    }

    consomme();

    if (!fonctions_courantes.est_vide()) {
        rapporte_erreur("Utilisation d'une directive sur une variable non-globale.");
        return;
    }

    auto lexème_directive = lexeme_courant();
    if (lexème_directive->ident == ID::externe) {
        if (déclaration->expression) {
            rapporte_erreur("Utilisation de #externe sur une déclaration initialisée. Les "
                            "variables externes ne peuvent pas être initialisées.");
            return;
        }

        consomme();
        analyse_directive_symbole_externe(déclaration);
        déclaration->drapeaux |= DrapeauxNoeud::EST_EXTERNE;
        return;
    }

    if (lexème_directive->ident == ID::interne) {
        consomme();
        déclaration->visibilité_symbole = VisibilitéSymbole::INTERNE;
        return;
    }

    if (lexème_directive->ident == ID::exporte) {
        consomme();
        déclaration->visibilité_symbole = VisibilitéSymbole::EXPORTÉ;
        return;
    }

    rapporte_erreur("Directive de déclaration de variable inconnue.");
}

void Syntaxeuse::analyse_directive_symbole_externe(NoeudDeclarationSymbole *déclaration_symbole)
{
    if (!apparie(GenreLexeme::CHAINE_CARACTERE)) {
        rapporte_erreur("attendu une chaine de caractère après #externe");
    }

    déclaration_symbole->données_externes =
        m_tacheronne.allocatrice_noeud.crée_données_symbole_externe();
    auto données_externes = déclaration_symbole->données_externes;
    données_externes->ident_bibliotheque = lexeme_courant()->ident;
    consomme();

    if (apparie(GenreLexeme::CHAINE_LITTERALE)) {
        données_externes->nom_symbole = lexeme_courant()->chaine;
        consomme();
    }
    else {
        données_externes->nom_symbole = déclaration_symbole->ident->nom;
    }
}
