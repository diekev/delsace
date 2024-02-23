/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "syntaxeuse.hh"

#include <array>
#include <iostream>

#include "biblinternes/outils/assert.hh"
#include "biblinternes/outils/conditions.h"

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
#include "validation_semantique.hh"

#include "utilitaires/garde_portee.hh"
#include "utilitaires/log.hh"

enum {
    OPÉRATEUR_EST_SURCHARGEABLE = (1 << 0),
    EST_EXPRESSION = (1 << 1),
    EST_EXPRESSION_UNAIRE = (1 << 2),
    EST_EXPRESSION_SECONDAIRE = (1 << 3),
    EST_LEXÈME_TYPE = (1 << 4),
    EST_INSTRUCTION = (1 << 5),
};

static constexpr auto table_drapeaux_lexèmes = [] {
    std::array<u_char, 256> t{};

    for (auto i = 0u; i < 256; ++i) {
        t[i] = 0;

        switch (static_cast<GenreLexème>(i)) {
            default:
            {
                break;
            }
            case GenreLexème::INFERIEUR:
            case GenreLexème::INFERIEUR_EGAL:
            case GenreLexème::SUPERIEUR:
            case GenreLexème::SUPERIEUR_EGAL:
            case GenreLexème::DIFFERENCE:
            case GenreLexème::EGALITE:
            case GenreLexème::PLUS:
            case GenreLexème::PLUS_UNAIRE:
            case GenreLexème::MOINS:
            case GenreLexème::MOINS_UNAIRE:
            case GenreLexème::FOIS:
            case GenreLexème::FOIS_UNAIRE:
            case GenreLexème::DIVISE:
            case GenreLexème::DECALAGE_DROITE:
            case GenreLexème::DECALAGE_GAUCHE:
            case GenreLexème::POURCENT:
            case GenreLexème::ESPERLUETTE:
            case GenreLexème::BARRE:
            case GenreLexème::TILDE:
            case GenreLexème::EXCLAMATION:
            case GenreLexème::CHAPEAU:
            case GenreLexème::CROCHET_OUVRANT:
            case GenreLexème::POUR:
            {
                t[i] |= OPÉRATEUR_EST_SURCHARGEABLE;
                break;
            }
        }

        // expression unaire
        switch (static_cast<GenreLexème>(i)) {
            case GenreLexème::EXCLAMATION:
            case GenreLexème::MOINS:
            case GenreLexème::MOINS_UNAIRE:
            case GenreLexème::PLUS:
            case GenreLexème::PLUS_UNAIRE:
            case GenreLexème::TILDE:
            case GenreLexème::FOIS:
            case GenreLexème::FOIS_UNAIRE:
            case GenreLexème::ESP_UNAIRE:
            case GenreLexème::ESPERLUETTE:
            case GenreLexème::TROIS_POINTS:
            case GenreLexème::EXPANSION_VARIADIQUE:
            case GenreLexème::ACCENT_GRAVE:
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
        switch (static_cast<GenreLexème>(i)) {
            case GenreLexème::CARACTERE:
            case GenreLexème::CHAINE_CARACTERE:
            case GenreLexème::CHAINE_LITTERALE:
            case GenreLexème::COROUT:
            case GenreLexème::CROCHET_OUVRANT:  // construit tableau
            case GenreLexème::DIRECTIVE:
            case GenreLexème::DOLLAR:
            case GenreLexème::EMPL:
            case GenreLexème::ENUM:
            case GenreLexème::ENUM_DRAPEAU:
            case GenreLexème::ERREUR:
            case GenreLexème::FAUX:
            case GenreLexème::FONC:
            case GenreLexème::INFO_DE:
            case GenreLexème::INIT_DE:
            case GenreLexème::MEMOIRE:
            case GenreLexème::NOMBRE_ENTIER:
            case GenreLexème::NOMBRE_REEL:
            case GenreLexème::NON_INITIALISATION:
            case GenreLexème::NUL:
            case GenreLexème::OPERATEUR:
            case GenreLexème::PARENTHESE_OUVRANTE:  // expression entre parenthèse
            case GenreLexème::STRUCT:
            case GenreLexème::TABLEAU:
            case GenreLexème::TAILLE_DE:
            case GenreLexème::TYPE_DE:
            case GenreLexème::TENTE:
            case GenreLexème::UNION:
            case GenreLexème::VRAI:
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
        switch (static_cast<GenreLexème>(i)) {
            case GenreLexème::N8:
            case GenreLexème::N16:
            case GenreLexème::N32:
            case GenreLexème::N64:
            case GenreLexème::R16:
            case GenreLexème::R32:
            case GenreLexème::R64:
            case GenreLexème::Z8:
            case GenreLexème::Z16:
            case GenreLexème::Z32:
            case GenreLexème::Z64:
            case GenreLexème::BOOL:
            case GenreLexème::RIEN:
            case GenreLexème::EINI:
            case GenreLexème::CHAINE:
            case GenreLexème::CHAINE_CARACTERE:
            case GenreLexème::OCTET:
            case GenreLexème::TYPE_DE_DONNEES:
            case GenreLexème::ADRESSE_FONCTION:
            {
                t[i] |= EST_LEXÈME_TYPE;
                t[i] |= EST_EXPRESSION;
                break;
            }
            default:
                break;
        }

        // expresssion secondaire
        switch (static_cast<GenreLexème>(i)) {
            case GenreLexème::BARRE:
            case GenreLexème::BARRE_BARRE:
            case GenreLexème::CHAPEAU:
            case GenreLexème::CROCHET_OUVRANT:
            case GenreLexème::DECALAGE_DROITE:
            case GenreLexème::DECALAGE_GAUCHE:
            case GenreLexème::DECLARATION_CONSTANTE:
            case GenreLexème::DECLARATION_VARIABLE:
            case GenreLexème::DEC_DROITE_EGAL:
            case GenreLexème::DEC_GAUCHE_EGAL:
            case GenreLexème::DIFFERENCE:
            case GenreLexème::DIVISE:
            case GenreLexème::DIVISE_EGAL:
            case GenreLexème::EGAL:
            case GenreLexème::EGALITE:
            case GenreLexème::ESPERLUETTE:
            case GenreLexème::ESP_ESP:
            case GenreLexème::ET_EGAL:
            case GenreLexème::FOIS:
            case GenreLexème::INFERIEUR:
            case GenreLexème::INFERIEUR_EGAL:
            case GenreLexème::MODULO_EGAL:
            case GenreLexème::MOINS:
            case GenreLexème::MOINS_EGAL:
            case GenreLexème::MULTIPLIE_EGAL:
            case GenreLexème::OUX_EGAL:
            case GenreLexème::OU_EGAL:
            case GenreLexème::ESP_ESP_EGAL:
            case GenreLexème::BARRE_BARRE_EGAL:
            case GenreLexème::PARENTHESE_OUVRANTE:
            case GenreLexème::PLUS:
            case GenreLexème::PLUS_EGAL:
            case GenreLexème::POINT:
            case GenreLexème::POURCENT:
            case GenreLexème::SUPERIEUR:
            case GenreLexème::SUPERIEUR_EGAL:
            case GenreLexème::TROIS_POINTS:
            case GenreLexème::VIRGULE:
            case GenreLexème::COMME:
            case GenreLexème::DOUBLE_POINTS:
            {
                t[i] |= EST_EXPRESSION_SECONDAIRE;
                break;
            }
            default:
            {
                break;
            }
        }

        switch (static_cast<GenreLexème>(i)) {
            case GenreLexème::ACCOLADE_OUVRANTE:
            case GenreLexème::ARRETE:
            case GenreLexème::BOUCLE:
            case GenreLexème::CONTINUE:
            case GenreLexème::DIFFERE:
            case GenreLexème::DISCR:
            case GenreLexème::NONSUR:
            case GenreLexème::POUR:
            case GenreLexème::POUSSE_CONTEXTE:
            case GenreLexème::REPETE:
            case GenreLexème::REPRENDS:
            case GenreLexème::RETIENS:
            case GenreLexème::RETOURNE:
            case GenreLexème::SAUFSI:
            case GenreLexème::SI:
            case GenreLexème::TANTQUE:
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

static constexpr auto table_associativité_lexèmes = [] {
    std::array<Associativité, 256> t{};

    for (auto i = 0u; i < 256; ++i) {
        t[i] = static_cast<Associativité>(-1);

        switch (static_cast<GenreLexème>(i)) {
            case GenreLexème::VIRGULE:
            case GenreLexème::TROIS_POINTS:
            case GenreLexème::EGAL:
            case GenreLexème::DECLARATION_VARIABLE:
            case GenreLexème::DECLARATION_CONSTANTE:
            case GenreLexème::PLUS_EGAL:
            case GenreLexème::MOINS_EGAL:
            case GenreLexème::DIVISE_EGAL:
            case GenreLexème::MULTIPLIE_EGAL:
            case GenreLexème::MODULO_EGAL:
            case GenreLexème::ET_EGAL:
            case GenreLexème::OU_EGAL:
            case GenreLexème::OUX_EGAL:
            case GenreLexème::DEC_DROITE_EGAL:
            case GenreLexème::DEC_GAUCHE_EGAL:
            case GenreLexème::BARRE_BARRE:
            case GenreLexème::ESP_ESP:
            case GenreLexème::ESP_ESP_EGAL:
            case GenreLexème::BARRE_BARRE_EGAL:
            case GenreLexème::BARRE:
            case GenreLexème::CHAPEAU:
            case GenreLexème::ESPERLUETTE:
            case GenreLexème::DIFFERENCE:
            case GenreLexème::EGALITE:
            case GenreLexème::INFERIEUR:
            case GenreLexème::INFERIEUR_EGAL:
            case GenreLexème::SUPERIEUR:
            case GenreLexème::SUPERIEUR_EGAL:
            case GenreLexème::DECALAGE_GAUCHE:
            case GenreLexème::DECALAGE_DROITE:
            case GenreLexème::PLUS:
            case GenreLexème::MOINS:
            case GenreLexème::FOIS:
            case GenreLexème::DIVISE:
            case GenreLexème::POURCENT:
            case GenreLexème::POINT:
            case GenreLexème::CROCHET_OUVRANT:
            case GenreLexème::PARENTHESE_OUVRANTE:
            case GenreLexème::COMME:
            case GenreLexème::DOUBLE_POINTS:
            {
                t[i] = Associativité::GAUCHE;
                break;
            }
            case GenreLexème::EXCLAMATION:
            case GenreLexème::TILDE:
            case GenreLexème::PLUS_UNAIRE:
            case GenreLexème::FOIS_UNAIRE:
            case GenreLexème::ESP_UNAIRE:
            case GenreLexème::MOINS_UNAIRE:
            case GenreLexème::EXPANSION_VARIADIQUE:
            case GenreLexème::ACCENT_GRAVE:
            {
                t[i] = Associativité::DROITE;
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

static constexpr int PRÉCÉDENCE_VIRGULE = 3;
static constexpr int PRÉCÉDENCE_TYPE = 4;

static constexpr auto table_précédence_lexèmes = [] {
    std::array<char, 256> t{};

    for (auto i = 0u; i < 256; ++i) {
        t[i] = -1;

        switch (static_cast<GenreLexème>(i)) {
            case GenreLexème::TROIS_POINTS:
            {
                t[i] = 1;
                break;
            }
            case GenreLexème::EGAL:
            case GenreLexème::DECLARATION_VARIABLE:
            case GenreLexème::DECLARATION_CONSTANTE:
            case GenreLexème::PLUS_EGAL:
            case GenreLexème::MOINS_EGAL:
            case GenreLexème::DIVISE_EGAL:
            case GenreLexème::MULTIPLIE_EGAL:
            case GenreLexème::MODULO_EGAL:
            case GenreLexème::ET_EGAL:
            case GenreLexème::OU_EGAL:
            case GenreLexème::OUX_EGAL:
            case GenreLexème::DEC_DROITE_EGAL:
            case GenreLexème::DEC_GAUCHE_EGAL:
            case GenreLexème::ESP_ESP_EGAL:
            case GenreLexème::BARRE_BARRE_EGAL:
            {
                t[i] = 2;
                break;
            }
            case GenreLexème::VIRGULE:
            {
                t[i] = PRÉCÉDENCE_VIRGULE;
                break;
            }
            case GenreLexème::DOUBLE_POINTS:
            {
                t[i] = PRÉCÉDENCE_TYPE;
                break;
            }
            case GenreLexème::BARRE_BARRE:
            {
                t[i] = 5;
                break;
            }
            case GenreLexème::ESP_ESP:
            {
                t[i] = 6;
                break;
            }
            case GenreLexème::BARRE:
            {
                t[i] = 7;
                break;
            }
            case GenreLexème::CHAPEAU:
            {
                t[i] = 8;
                break;
            }
            case GenreLexème::ESPERLUETTE:
            {
                t[i] = 9;
                break;
            }
            case GenreLexème::DIFFERENCE:
            case GenreLexème::EGALITE:
            {
                t[i] = 10;
                break;
            }
            case GenreLexème::INFERIEUR:
            case GenreLexème::INFERIEUR_EGAL:
            case GenreLexème::SUPERIEUR:
            case GenreLexème::SUPERIEUR_EGAL:
            {
                t[i] = 11;
                break;
            }
            case GenreLexème::DECALAGE_GAUCHE:
            case GenreLexème::DECALAGE_DROITE:
            {
                t[i] = 12;
                break;
            }
            case GenreLexème::PLUS:
            case GenreLexème::MOINS:
            {
                t[i] = 13;
                break;
            }
            case GenreLexème::FOIS:
            case GenreLexème::DIVISE:
            case GenreLexème::POURCENT:
            {
                t[i] = 14;
                break;
            }
            case GenreLexème::COMME:
            {
                t[i] = 15;
                break;
            }
            case GenreLexème::EXCLAMATION:
            case GenreLexème::TILDE:
            case GenreLexème::PLUS_UNAIRE:
            case GenreLexème::MOINS_UNAIRE:
            case GenreLexème::FOIS_UNAIRE:
            case GenreLexème::ESP_UNAIRE:
            case GenreLexème::EXPANSION_VARIADIQUE:
            case GenreLexème::ACCENT_GRAVE:
            {
                t[i] = 16;
                break;
            }
            case GenreLexème::PARENTHESE_OUVRANTE:
            case GenreLexème::POINT:
            case GenreLexème::CROCHET_OUVRANT:
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

static inline bool est_opérateur_surchargeable(GenreLexème genre)
{
    return (table_drapeaux_lexèmes[static_cast<size_t>(genre)] & OPÉRATEUR_EST_SURCHARGEABLE) != 0;
}

static inline int précédence_pour_opérateur(GenreLexème genre_opérateur)
{
    int précédence = table_précédence_lexèmes[static_cast<size_t>(genre_opérateur)];
    assert_rappel(précédence != -1, [&]() {
        dbg() << "Aucune précédence pour l'opérateur : " << chaine_du_lexème(genre_opérateur);
    });
    return précédence;
}

static inline Associativité associativité_pour_opérateur(GenreLexème genre_opérateur)
{
    auto associativité = table_associativité_lexèmes[static_cast<size_t>(genre_opérateur)];
    assert_rappel(associativité != static_cast<Associativité>(-1), [&]() {
        dbg() << "Aucune précédence pour l'opérateur : " << chaine_du_lexème(genre_opérateur);
    });
    return associativité;
}

Syntaxeuse::Syntaxeuse(Tacheronne &tacheronne, UniteCompilation const *unite)
    : BaseSyntaxeuse(unite->fichier), m_compilatrice(tacheronne.compilatrice),
      m_tacheronne(tacheronne), m_unité(unite)
{
    auto module = m_fichier->module;

    m_tacheronne.assembleuse->dépile_tout();

    module->mutex.lock();
    {
        if (module->bloc == nullptr) {
            module->bloc = m_tacheronne.assembleuse->empile_bloc(lexème_courant(), nullptr);

            if (module->nom() != ID::Kuri) {
                /* Crée un membre pour l'import implicite du module Kuri afin de pouvoir accéder
                 * aux fonctions de ce module via une expression de référence de membre :
                 * « Kuri.fonction(...) ». */
                static Lexème lexème_ident_kuri = {};
                lexème_ident_kuri.genre = GenreLexème::CHAINE_CARACTERE;
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

    auto métaprogramme = m_fichier->métaprogramme_corps_texte;

    if (métaprogramme->corps_texte_pour_fonction) {
        auto récipiente = métaprogramme->corps_texte_pour_fonction;
        m_tacheronne.assembleuse->bloc_courant(récipiente->bloc_parametres);

        fonctions_courantes.empile(récipiente);
        récipiente->corps->bloc = analyse_bloc(false);
        récipiente->corps->est_corps_texte = false;
        récipiente->drapeaux_fonction &= ~DrapeauxNoeudFonction::EST_MÉTAPROGRAMME;
        fonctions_courantes.depile();
    }
    else if (métaprogramme->corps_texte_pour_structure) {
        auto récipiente = métaprogramme->corps_texte_pour_structure;
        if (récipiente->unité && récipiente->unité->arbre_aplatis) {
            récipiente->unité->arbre_aplatis->réinitialise();
        }

        auto type_structure = récipiente->comme_declaration_classe();
        auto bloc_parent = (type_structure->bloc_constantes) ? type_structure->bloc_constantes :
                                                               type_structure->bloc_parent;
        m_tacheronne.assembleuse->bloc_courant(bloc_parent);

        type_structure->bloc = analyse_bloc(false);
        type_structure->bloc->fusionne_membres(type_structure->bloc_constantes);
        type_structure->est_corps_texte = false;
    }

    métaprogramme->fut_execute = true;
}

void Syntaxeuse::quand_termine()
{
    m_tacheronne.assembleuse->dépile_tout();
}

void Syntaxeuse::analyse_une_chose()
{
    if (ignore_point_virgule_implicite()) {
        return;
    }

    auto lexème = lexème_courant();
    auto genre_lexème = lexème->genre;

    if (genre_lexème == GenreLexème::IMPORTE) {
        consomme();

        auto noeud = m_tacheronne.assembleuse->crée_importe(lexème);
        noeud->bloc_parent->ajoute_expression(noeud);

        if (!apparie(GenreLexème::CHAINE_LITTERALE) && !apparie(GenreLexème::CHAINE_CARACTERE)) {
            rapporte_erreur("Attendu une chaine littérale après 'importe'");
        }

        noeud->expression = m_tacheronne.assembleuse->crée_reference_declaration(lexème_courant());

        requiers_typage(noeud);
        m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::IMPORTE;

        consomme();
    }
    else if (genre_lexème == GenreLexème::CHARGE) {
        consomme();

        auto noeud = m_tacheronne.assembleuse->crée_charge(lexème);
        noeud->bloc_parent->ajoute_expression(noeud);

        if (!apparie(GenreLexème::CHAINE_LITTERALE) && !apparie(GenreLexème::CHAINE_CARACTERE)) {
            rapporte_erreur("Attendu une chaine littérale après 'charge'");
        }

        noeud->expression = m_tacheronne.assembleuse->crée_reference_declaration(lexème_courant());
        noeud->expression->ident = nullptr;

        requiers_typage(noeud);
        m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::CHARGE;

        consomme();
    }
    else if (apparie_expression()) {
        auto noeud = analyse_expression({}, GenreLexème::INCONNU, GenreLexème::INCONNU);

        if (!noeud) {
            /* Ceci peut arriver si nous avons une erreur. */
            return;
        }

        if (noeud->est_declaration()) {
            noeud->drapeaux |= DrapeauxNoeud::EST_GLOBALE;

            if (noeud->est_base_declaration_variable()) {
                assert_rappel(noeud->bloc_parent, [&]() {
                    dbg() << erreur::imprime_site(*m_unité->espace, noeud) << "\n" << noeud->genre;
                });
                noeud->bloc_parent->ajoute_membre(noeud->comme_base_declaration_variable());
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
            rapporte_erreur_avec_site(
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
    auto genre = lexème_courant()->genre;
    return (table_drapeaux_lexèmes[static_cast<size_t>(genre)] & EST_EXPRESSION) != 0;
}

bool Syntaxeuse::apparie_expression_unaire() const
{
    auto genre = lexème_courant()->genre;
    return (table_drapeaux_lexèmes[static_cast<size_t>(genre)] & EST_EXPRESSION_UNAIRE) != 0;
}

bool Syntaxeuse::apparie_expression_secondaire() const
{
    auto genre = lexème_courant()->genre;
    return (table_drapeaux_lexèmes[static_cast<size_t>(genre)] & EST_EXPRESSION_SECONDAIRE) != 0;
}

bool Syntaxeuse::apparie_instruction() const
{
    auto genre = lexème_courant()->genre;
    return (table_drapeaux_lexèmes[static_cast<size_t>(genre)] & EST_INSTRUCTION) != 0;
}

NoeudExpression *Syntaxeuse::analyse_expression(DonnéesPrécédence const &données_précédence,
                                                GenreLexème racine_expression,
                                                GenreLexème lexème_final)
{
    auto expression = analyse_expression_primaire(racine_expression, lexème_final);

    while (!fini() && apparie_expression_secondaire() && lexème_courant()->genre != lexème_final) {
        auto nouvelle_précédence = précédence_pour_opérateur(lexème_courant()->genre);

        if (nouvelle_précédence < données_précédence.précédence) {
            break;
        }

        if (nouvelle_précédence == données_précédence.précédence &&
            données_précédence.associativité == Associativité::GAUCHE) {
            break;
        }

        auto nouvelle_associativité = associativité_pour_opérateur(lexème_courant()->genre);
        expression = analyse_expression_secondaire(expression,
                                                   {nouvelle_précédence, nouvelle_associativité},
                                                   racine_expression,
                                                   lexème_final);
    }

    if (!expression) {
        rapporte_erreur("Attendu une expression primaire");
    }

    return expression;
}

NoeudExpression *Syntaxeuse::analyse_expression_unaire(GenreLexème lexème_final)
{
    auto lexème = lexème_courant();
    auto genre_noeud = GenreNoeud::OPERATEUR_UNAIRE;

    switch (lexème->genre) {
        case GenreLexème::MOINS:
        {
            lexème->genre = GenreLexème::MOINS_UNAIRE;
            break;
        }
        case GenreLexème::PLUS:
        {
            lexème->genre = GenreLexème::PLUS_UNAIRE;
            break;
        }
        case GenreLexème::EXPANSION_VARIADIQUE:
        case GenreLexème::TROIS_POINTS:
        {
            lexème->genre = GenreLexème::EXPANSION_VARIADIQUE;
            genre_noeud = GenreNoeud::EXPANSION_VARIADIQUE;
            break;
        }
        case GenreLexème::FOIS:
        {
            lexème->genre = GenreLexème::FOIS_UNAIRE;
            break;
        }
        case GenreLexème::ESPERLUETTE:
        {
            lexème->genre = GenreLexème::ESP_UNAIRE;
            break;
        }
        case GenreLexème::ESP_UNAIRE:
        case GenreLexème::EXCLAMATION:
        case GenreLexème::TILDE:
        case GenreLexème::PLUS_UNAIRE:
        case GenreLexème::MOINS_UNAIRE:
        case GenreLexème::FOIS_UNAIRE:
        {
            break;
        }
        case GenreLexème::ACCENT_GRAVE:
        {
            consomme();
            if (fini()) {
                return nullptr;
            }

            lexème = lexème_courant();
            consomme();
            auto noeud = m_tacheronne.assembleuse->crée_reference_declaration(lexème);
            noeud->drapeaux |= DrapeauxNoeud::IDENTIFIANT_EST_ACCENTUÉ_GRAVE;
            return noeud;
        }
        default:
        {
            assert_rappel(false, [&]() {
                dbg() << "Lexème inattendu comme opérateur unaire : "
                      << chaine_du_lexème(lexème->genre) << '\n'
                      << crée_message_erreur("");
            });
            return nullptr;
        }
    }

    consomme();

    auto précédence = précédence_pour_opérateur(lexème->genre);
    auto associativité = associativité_pour_opérateur(lexème->genre);

    if (lexème->genre == GenreLexème::FOIS_UNAIRE) {
        auto opérande = analyse_expression(
            {précédence, associativité}, GenreLexème::INCONNU, lexème_final);

        auto noeud = m_tacheronne.assembleuse->crée_prise_adresse(lexème);
        noeud->opérande = opérande;
        return noeud;
    }

    if (lexème->genre == GenreLexème::ESP_UNAIRE) {
        auto opérande = analyse_expression(
            {précédence, associativité}, GenreLexème::INCONNU, lexème_final);

        auto noeud = m_tacheronne.assembleuse->crée_prise_reference(lexème);
        noeud->opérande = opérande;
        return noeud;
    }

    if (lexème->genre == GenreLexème::EXCLAMATION) {
        auto opérande = analyse_expression(
            {précédence, associativité}, GenreLexème::INCONNU, lexème_final);

        auto noeud = m_tacheronne.assembleuse->crée_negation_logique(lexème);
        noeud->opérande = opérande;
        return noeud;
    }

    auto noeud = m_tacheronne.assembleuse->crée_noeud<GenreNoeud::OPERATEUR_UNAIRE>(lexème)
                     ->comme_expression_unaire();
    noeud->genre = genre_noeud;

    // cette vérification n'est utile que pour les arguments variadiques sans type
    if (apparie_expression()) {
        noeud->operande = analyse_expression(
            {précédence, associativité}, GenreLexème::INCONNU, lexème_final);
    }

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_expression_primaire(GenreLexème racine_expression,
                                                         GenreLexème lexème_final)
{
    if (apparie_expression_unaire()) {
        return analyse_expression_unaire(lexème_final);
    }

    auto lexème = lexème_courant();

    switch (lexème->genre) {
        case GenreLexème::CARACTERE:
        {
            consomme();
            return m_tacheronne.assembleuse->crée_litterale_caractere(lexème);
        }
        case GenreLexème::CHAINE_CARACTERE:
        {
            consomme();
            return analyse_référence_déclaration(lexème);
        }
        case GenreLexème::CHAINE_LITTERALE:
        {
            consomme();
            return m_tacheronne.assembleuse->crée_litterale_chaine(lexème);
        }
        case GenreLexème::TABLEAU:
        case GenreLexème::CROCHET_OUVRANT:
        {
            consomme();
            lexème->genre = GenreLexème::TABLEAU;

            auto expression_entre_crochets = NoeudExpression::nul();
            if (apparie(GenreLexème::DEUX_POINTS)) {
                consomme();
                consomme(GenreLexème::CROCHET_FERMANT, "Attendu un crochet fermant");

                auto expression_type = analyse_expression(
                    {PRÉCÉDENCE_TYPE, Associativité::GAUCHE}, racine_expression, lexème_final);
                auto noeud = m_tacheronne.assembleuse->crée_expression_type_tableau_dynamique(
                    lexème);
                noeud->expression_type = expression_type;
                return noeud;
            }

            if (apparie(GenreLexème::CROCHET_FERMANT)) {
                consomme();
                auto expression_type = analyse_expression(
                    {PRÉCÉDENCE_TYPE, Associativité::GAUCHE}, racine_expression, lexème_final);
                auto noeud = m_tacheronne.assembleuse->crée_expression_type_tranche(lexème);
                noeud->expression_type = expression_type;
                return noeud;
            }

            if (apparie_expression()) {
                auto ancien_noeud_virgule = m_noeud_expression_virgule;
                m_noeud_expression_virgule = nullptr;
                expression_entre_crochets = analyse_expression(
                    {}, GenreLexème::CROCHET_OUVRANT, GenreLexème::INCONNU);
                m_noeud_expression_virgule = ancien_noeud_virgule;
            }

            ignore_point_virgule_implicite();

            consomme(GenreLexème::CROCHET_FERMANT, "Attendu un crochet fermant");

            if (apparie_expression()) {
                /* Nous avons l'expression d'un type tableau fixe. */
                auto expression_type = analyse_expression(
                    {PRÉCÉDENCE_TYPE, Associativité::GAUCHE}, racine_expression, lexème_final);

                if (expression_entre_crochets->possède_drapeau(
                        DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE)) {
                    expression_entre_crochets->drapeaux &=
                        ~DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE;
                    expression_entre_crochets->drapeaux |= DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE;

                    expression_entre_crochets->comme_reference_declaration()
                        ->declaration_referee->drapeaux &=
                        ~DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE;
                    expression_entre_crochets->comme_reference_declaration()
                        ->declaration_referee->drapeaux |= DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE;
                }

                auto noeud = m_tacheronne.assembleuse->crée_expression_type_tableau_fixe(lexème);
                noeud->expression_taille = expression_entre_crochets;
                noeud->expression_type = expression_type;
                return noeud;
            }

            auto noeud = m_tacheronne.assembleuse->crée_construction_tableau(lexème);

            /* Le reste de la pipeline suppose que l'expression est une virgule,
             * donc créons une telle expression au cas où nous n'avons qu'un seul
             * élément dans le tableau. */
            if (!expression_entre_crochets->est_virgule()) {
                auto virgule = m_tacheronne.assembleuse->crée_virgule(lexème);
                virgule->expressions.ajoute(expression_entre_crochets);
                expression_entre_crochets = virgule;
            }

            noeud->expression = expression_entre_crochets;

            return noeud;
        }
        case GenreLexème::EMPL:
        {
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_empl(lexème);
            noeud->expression = analyse_expression({}, GenreLexème::EMPL, lexème_final);
            return noeud;
        }
        case GenreLexème::FAUX:
        case GenreLexème::VRAI:
        {
            consomme();
            return m_tacheronne.assembleuse->crée_litterale_bool(lexème);
        }
        case GenreLexème::INFO_DE:
        {
            consomme();
            consomme(GenreLexème::PARENTHESE_OUVRANTE, "Attendu '(' après 'info_de'");

            auto noeud = m_tacheronne.assembleuse->crée_info_de(lexème);
            noeud->expression = analyse_expression({}, GenreLexème::INFO_DE, GenreLexème::INCONNU);

            consomme(GenreLexème::PARENTHESE_FERMANTE,
                     "Attendu ')' après l'expression de 'info_de'");

            return noeud;
        }
        case GenreLexème::INIT_DE:
        {
            consomme();
            consomme(GenreLexème::PARENTHESE_OUVRANTE, "Attendu '(' après 'init_de'");

            auto noeud = m_tacheronne.assembleuse->crée_init_de(lexème);
            noeud->expression = analyse_expression({}, GenreLexème::INIT_DE, GenreLexème::INCONNU);

            consomme(GenreLexème::PARENTHESE_FERMANTE,
                     "Attendu ')' après l'expression de 'init_de'");

            return noeud;
        }
        case GenreLexème::MEMOIRE:
        {
            consomme();
            consomme(GenreLexème::PARENTHESE_OUVRANTE, "Attendu '(' après 'mémoire'");

            auto noeud = m_tacheronne.assembleuse->crée_memoire(lexème);
            noeud->expression = analyse_expression({}, GenreLexème::MEMOIRE, GenreLexème::INCONNU);

            consomme(GenreLexème::PARENTHESE_FERMANTE, "Attendu ')' après l'expression");

            return noeud;
        }
        case GenreLexème::NOMBRE_ENTIER:
        {
            consomme();
            return m_tacheronne.assembleuse->crée_litterale_entier(lexème);
        }
        case GenreLexème::NOMBRE_REEL:
        {
            consomme();
            return m_tacheronne.assembleuse->crée_litterale_reel(lexème);
        }
        case GenreLexème::NON_INITIALISATION:
        {
            consomme();
            return m_tacheronne.assembleuse->crée_non_initialisation(lexème);
        }
        case GenreLexème::NUL:
        {
            consomme();
            return m_tacheronne.assembleuse->crée_litterale_nul(lexème);
        }
        case GenreLexème::OPERATEUR:
        {
            return analyse_déclaration_opérateur();
        }
        case GenreLexème::PARENTHESE_OUVRANTE:
        {
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_parenthese(lexème);
            noeud->expression = analyse_expression(
                {}, GenreLexème::PARENTHESE_OUVRANTE, GenreLexème::INCONNU);

            consomme(GenreLexème::PARENTHESE_FERMANTE, "attendu une parenthèse fermante");

            return noeud;
        }
        case GenreLexème::TAILLE_DE:
        {
            consomme();
            consomme(GenreLexème::PARENTHESE_OUVRANTE, "Attendu '(' après 'taille_de'");

            auto noeud = m_tacheronne.assembleuse->crée_taille_de(lexème);
            noeud->expression = analyse_expression(
                {}, GenreLexème::TAILLE_DE, GenreLexème::INCONNU);

            consomme(GenreLexème::PARENTHESE_FERMANTE, "Attendu ')' après le type de 'taille_de'");

            return noeud;
        }
        case GenreLexème::TYPE_DE:
        {
            consomme();
            consomme(GenreLexème::PARENTHESE_OUVRANTE, "Attendu '(' après 'type_de'");

            auto noeud = m_tacheronne.assembleuse->crée_type_de(lexème);
            noeud->expression = analyse_expression({}, GenreLexème::TYPE_DE, GenreLexème::INCONNU);

            consomme(GenreLexème::PARENTHESE_FERMANTE, "Attendu ')' après le type de 'type_de'");

            return noeud;
        }
        case GenreLexème::TENTE:
        {
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_tente(lexème);
            noeud->expression_appelee = analyse_expression(
                {}, GenreLexème::TENTE, GenreLexème::INCONNU);

            if (apparie(GenreLexème::PIEGE)) {
                consomme();

                if (apparie(GenreLexème::NONATTEIGNABLE)) {
                    consomme();
                }
                else {
                    noeud->expression_piegee = analyse_expression(
                        {}, GenreLexème::PIEGE, GenreLexème::INCONNU);
                    noeud->bloc = analyse_bloc();
                }
            }

            return noeud;
        }
        case GenreLexème::DIRECTIVE:
        {
            consomme();

            lexème = lexème_courant();
            auto directive = lexème->ident;

            consomme();

            if (directive == ID::execute || directive == ID::assert_ || directive == ID::test) {
                auto noeud = m_tacheronne.assembleuse->crée_execute(lexème);
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
                        {}, GenreLexème::DIRECTIVE, GenreLexème::INCONNU);
                }

                return noeud;
            }
            else if (directive == ID::corps_boucle) {
                return m_tacheronne.assembleuse->crée_directive_corps_boucle(lexème);
            }
            else if (directive == ID::si) {
                m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::SI_STATIQUE;
                return analyse_instruction_si_statique(lexème);
            }
            else if (directive == ID::saufsi) {
                m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::SI_STATIQUE;
                return analyse_instruction_si_statique(lexème);
            }
            else if (directive == ID::cuisine) {
                auto noeud = m_tacheronne.assembleuse->crée_cuisine(lexème);
                noeud->ident = directive;
                noeud->expression = analyse_expression(
                    {}, GenreLexème::DIRECTIVE, GenreLexème::INCONNU);
                m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::CUISINE;
                return noeud;
            }
            else if (directive == ID::dependance_bibliotheque) {
                auto noeud = m_tacheronne.assembleuse->crée_dependance_bibliotheque(lexème);

                auto lexème_bibliothèque_dépendante = lexème_courant();
                consomme(
                    GenreLexème::CHAINE_CARACTERE,
                    "Attendue une chaine de caractère pour définir la bibliothèque dépendante");
                noeud->bibliothèque_dépendante =
                    m_tacheronne.assembleuse->crée_reference_declaration(
                        lexème_bibliothèque_dépendante);

                auto lexème_bibliothèque_dépendue = lexème_courant();
                consomme(GenreLexème::CHAINE_CARACTERE,
                         "Attendue une chaine de caractère pour définir la bibliothèque dépendue");
                noeud->bibliothèque_dépendue =
                    m_tacheronne.assembleuse->crée_reference_declaration(
                        lexème_bibliothèque_dépendue);
                return noeud;
            }
            else if (directive == ID::ajoute_init) {
                auto noeud = m_tacheronne.assembleuse->crée_ajoute_init(lexème);
                noeud->ident = directive;
                noeud->expression = analyse_expression(
                    {}, GenreLexème::DIRECTIVE, GenreLexème::INCONNU);
                m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::AJOUTE_INIT;
                return noeud;
            }
            else if (directive == ID::ajoute_fini) {
                auto noeud = m_tacheronne.assembleuse->crée_ajoute_fini(lexème);
                noeud->ident = directive;
                noeud->expression = analyse_expression(
                    {}, GenreLexème::DIRECTIVE, GenreLexème::INCONNU);
                m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::AJOUTE_FINI;
                return noeud;
            }
            else if (directive == ID::pre_executable) {
                auto noeud = m_tacheronne.assembleuse->crée_pre_executable(lexème);
                noeud->ident = directive;
                noeud->expression = analyse_expression(
                    {}, GenreLexème::DIRECTIVE, GenreLexème::INCONNU);
                m_fichier->fonctionnalités_utilisées |= FonctionnalitéLangage::PRÉ_EXÉCUTABLE;
                return noeud;
            }
            else if (directive == ID::nom_de_cette_fonction ||
                     directive == ID::chemin_de_ce_fichier ||
                     directive == ID::chemin_de_ce_module ||
                     directive == ID::type_de_cette_fonction ||
                     directive == ID::type_de_cette_structure) {
                auto noeud = m_tacheronne.assembleuse->crée_directive_instrospection(lexème);
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
        case GenreLexème::DOLLAR:
        {
            consomme();
            lexème = lexème_courant();

            consomme(GenreLexème::CHAINE_CARACTERE, "attendu une chaine de caractère après '$'");

            auto noeud = m_tacheronne.assembleuse->crée_reference_declaration(lexème);

            auto noeud_decl_param = m_tacheronne.assembleuse->crée_declaration_constante(lexème);
            noeud->declaration_referee = noeud_decl_param;

            if (!bloc_constantes_polymorphiques.est_vide()) {
                auto bloc_constantes = bloc_constantes_polymorphiques.haut();

                if (bloc_constantes->declaration_pour_ident(noeud->ident) != nullptr) {
                    recule();
                    rapporte_erreur("redéfinition du type polymorphique");
                }

                bloc_constantes->ajoute_membre(noeud_decl_param);
            }
            else if (!m_est_déclaration_type_opaque) {
                rapporte_erreur("déclaration d'un type polymorphique hors d'une fonction, d'une "
                                "structure, ou de la déclaration d'un type opaque");
            }

            if (apparie(GenreLexème::DOUBLE_POINTS)) {
                consomme();
                noeud_decl_param->expression_type = analyse_expression(
                    {}, racine_expression, lexème_final);
                /* Nous avons une déclaration de valeur polymorphique, retournons-la. */
                noeud_decl_param->drapeaux |= DrapeauxNoeud::EST_VALEUR_POLYMORPHIQUE;
                return noeud_decl_param;
            }

            noeud->drapeaux |= DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE;
            noeud_decl_param->drapeaux |= DrapeauxNoeud::DECLARATION_TYPE_POLYMORPHIQUE;
            return noeud;
        }
        case GenreLexème::FONC:
        {
            return analyse_déclaration_fonction(lexème);
        }
        /* Ceux-ci doivent déjà avoir été gérés. */
        case GenreLexème::COROUT:
        case GenreLexème::ENUM:
        case GenreLexème::ENUM_DRAPEAU:
        case GenreLexème::ERREUR:
        case GenreLexème::STRUCT:
        case GenreLexème::UNION:
        {
            auto message_erreur = enchaine(
                "« ",
                chaine_du_lexème(lexème->genre),
                " » ne peut pas être utilisé comme expression primaire.\n");
            rapporte_erreur(message_erreur);
            return nullptr;
        }
        case GenreLexème::SI:
        {
            return analyse_instruction_si(GenreNoeud::INSTRUCTION_SI);
        }
        case GenreLexème::SAUFSI:
        {
            return analyse_instruction_si(GenreNoeud::INSTRUCTION_SAUFSI);
        }
        default:
        {
            if (est_identifiant_type(lexème->genre)) {
                consomme();
                return m_tacheronne.assembleuse->crée_reference_type(lexème);
            }

            rapporte_erreur("attendu une expression primaire");
            return nullptr;
        }
    }
}

NoeudExpression *Syntaxeuse::analyse_expression_secondaire(
    NoeudExpression *gauche,
    const DonnéesPrécédence &données_précédence,
    GenreLexème racine_expression,
    GenreLexème lexème_final)
{
    auto lexème = lexème_courant();

    switch (lexème->genre) {
        case GenreLexème::BARRE:
        case GenreLexème::CHAPEAU:
        case GenreLexème::DECALAGE_DROITE:
        case GenreLexème::DECALAGE_GAUCHE:
        case GenreLexème::DEC_DROITE_EGAL:
        case GenreLexème::DEC_GAUCHE_EGAL:
        case GenreLexème::DIFFERENCE:
        case GenreLexème::DIVISE:
        case GenreLexème::DIVISE_EGAL:
        case GenreLexème::EGALITE:
        case GenreLexème::ESPERLUETTE:
        case GenreLexème::ET_EGAL:
        case GenreLexème::FOIS:
        case GenreLexème::INFERIEUR:
        case GenreLexème::INFERIEUR_EGAL:
        case GenreLexème::MODULO_EGAL:
        case GenreLexème::MOINS:
        case GenreLexème::MOINS_EGAL:
        case GenreLexème::MULTIPLIE_EGAL:
        case GenreLexème::OUX_EGAL:
        case GenreLexème::OU_EGAL:
        case GenreLexème::PLUS:
        case GenreLexème::PLUS_EGAL:
        case GenreLexème::POURCENT:
        case GenreLexème::SUPERIEUR:
        case GenreLexème::SUPERIEUR_EGAL:
        {
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_expression_binaire(lexème);
            noeud->operande_gauche = gauche;
            noeud->operande_droite = analyse_expression(
                données_précédence, racine_expression, lexème_final);
            return noeud;
        }
        case GenreLexème::BARRE_BARRE:
        case GenreLexème::ESP_ESP:
        {
            consomme();

            auto opérande_droite = analyse_expression(
                données_précédence, racine_expression, lexème_final);

            auto noeud = m_tacheronne.assembleuse->crée_expression_logique(lexème);
            noeud->opérande_gauche = gauche;
            noeud->opérande_droite = opérande_droite;
            return noeud;
        }
        case GenreLexème::BARRE_BARRE_EGAL:
        case GenreLexème::ESP_ESP_EGAL:
        {
            consomme();

            auto opérande_droite = analyse_expression(
                données_précédence, racine_expression, lexème_final);

            auto noeud = m_tacheronne.assembleuse->crée_assignation_logique(lexème);
            noeud->opérande_gauche = gauche;
            noeud->opérande_droite = opérande_droite;
            return noeud;
        }
        case GenreLexème::VIRGULE:
        {
            consomme();

            auto noeud_expression_virgule = m_noeud_expression_virgule;

            if (!m_noeud_expression_virgule) {
                m_noeud_expression_virgule = m_tacheronne.assembleuse->crée_virgule(lexème);
                m_noeud_expression_virgule->expressions.ajoute(gauche);
                noeud_expression_virgule = m_noeud_expression_virgule;
            }

            auto droite = analyse_expression(données_précédence, racine_expression, lexème_final);

            m_noeud_expression_virgule = noeud_expression_virgule;
            m_noeud_expression_virgule->expressions.ajoute(droite);

            return m_noeud_expression_virgule;
        }
        case GenreLexème::CROCHET_OUVRANT:
        {
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_indexage(lexème);
            noeud->operande_gauche = gauche;
            noeud->operande_droite = analyse_expression(
                {}, GenreLexème::CROCHET_OUVRANT, GenreLexème::INCONNU);

            consomme(GenreLexème::CROCHET_FERMANT, "attendu un crochet fermant");

            return noeud;
        }
        case GenreLexème::DECLARATION_CONSTANTE:
        {
            if (!gauche->est_reference_declaration()) {
                rapporte_erreur("Attendu une référence à une déclaration à gauche de « :: »");
                return nullptr;
            }

            consomme();

            m_est_déclaration_type_opaque = false;

            switch (lexème_courant()->genre) {
                default:
                {
                    break;
                }
                case GenreLexème::DIRECTIVE:
                {
                    auto position = m_position;

                    consomme();

                    auto directive = lexème_courant()->ident;

                    if (directive == ID::bibliotheque) {
                        consomme();
                        auto chaine_bib = lexème_courant()->chaine;
                        consomme(GenreLexème::CHAINE_LITTERALE,
                                 "Attendu une chaine littérale après la directive");
                        auto noeud = m_tacheronne.assembleuse->crée_declaration_bibliotheque(
                            lexème);
                        noeud->ident = gauche->ident;
                        noeud->bibliotheque =
                            m_compilatrice.gestionnaire_bibliothèques->crée_bibliotheque(
                                *m_unité->espace, noeud, gauche->ident, chaine_bib);
                        return noeud;
                    }

                    if (directive == ID::opaque) {
                        consomme();
                        auto noeud = m_tacheronne.assembleuse->crée_type_opaque(gauche->lexeme);
                        m_est_déclaration_type_opaque = true;
                        noeud->expression_type = analyse_expression(
                            données_précédence, racine_expression, lexème_final);
                        m_est_déclaration_type_opaque = false;
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

            auto noeud = m_tacheronne.assembleuse->crée_declaration_constante(lexème);
            noeud->ident = gauche->ident;
            noeud->expression = analyse_expression(
                données_précédence, racine_expression, lexème_final);

            if (gauche->est_reference_declaration()) {
                gauche->comme_reference_declaration()->declaration_referee = noeud;
            }

            return noeud;
        }
        case GenreLexème::DOUBLE_POINTS:
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
                        rapporte_erreur_avec_site(it,
                                                  "Obtenu une déclaration de variable au "
                                                  "sein d'une expression-virgule.");
                    }

                    if (!it->est_reference_declaration()) {
                        rapporte_erreur_avec_site(
                            it, "Expression inattendue dans l'expression virgule.");
                    }
                }

                auto decl = m_tacheronne.assembleuse->crée_declaration_variable_multiple(lexème);
                analyse_annotations(decl->annotations);
                decl->valeur = m_noeud_expression_virgule;
                decl->expression_type = analyse_expression(
                    données_précédence, racine_expression, lexème_final);
                analyse_annotations(decl->annotations);

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
                m_tacheronne.assembleuse->recycle_référence(gauche->comme_reference_declaration());
                decl->expression_type = analyse_expression(
                    données_précédence, racine_expression, lexème_final);
                if (!bloc_constantes_polymorphiques.est_vide()) {
                    decl->drapeaux |= DrapeauxNoeud::EST_LOCALE;
                }
                analyse_directive_déclaration_variable(decl);
                analyse_annotations(decl->annotations);
                return decl;
            }

            if (gauche->est_declaration_variable()) {
                // nous avons la déclaration d'une constante (a: z32 : 12)
                // À FAIRE : réutilise la mémoire
                auto decl = gauche->comme_declaration_variable();
                auto constante = m_tacheronne.assembleuse->crée_declaration_constante(lexème);
                constante->ident = decl->ident;
                constante->expression_type = decl->expression_type;
                constante->expression = analyse_expression(
                    données_précédence, racine_expression, lexème_final);
                return constante;
            }

            rapporte_erreur_avec_site(gauche, "Expression inattendu à gauche du double-point");
            return nullptr;
        }
        case GenreLexème::DECLARATION_VARIABLE:
        {
            if (gauche->est_declaration_variable()) {
                rapporte_erreur_avec_site(
                    gauche, "Utilisation de « := » alors qu'un type fut déclaré avec « : »");
            }

            if (gauche->est_virgule()) {
                auto noeud_virgule = gauche->comme_virgule();

                // détecte les expressions du style : a : z32, b := ... , a[0] := ..., etc.
                POUR (noeud_virgule->expressions) {
                    if (it->est_declaration_variable()) {
                        rapporte_erreur_avec_site(
                            it, "Utilisation de « := » alors qu'un type fut déclaré avec « : ».");
                    }

                    if (!it->est_reference_declaration()) {
                        rapporte_erreur_avec_site(it, "Expression inattendue à gauche de « := »");
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
                rapporte_erreur_avec_site(gauche, "Expression inattendue à gauche de « := »");
            }

            consomme();

            m_noeud_expression_virgule = nullptr;

            auto expression = analyse_expression(
                données_précédence, racine_expression, lexème_final);

            if (gauche->est_virgule()) {
                auto noeud = m_tacheronne.assembleuse->crée_declaration_variable_multiple(lexème);
                noeud->ident = gauche->ident;
                noeud->valeur = gauche;
                noeud->expression = expression;
                analyse_annotations(noeud->annotations);
                m_noeud_expression_virgule = nullptr;
                return noeud;
            }

            auto noeud = m_tacheronne.assembleuse->crée_declaration_variable(gauche->lexeme);
            /* Vérifie que nous avons une référence car nous ne nous arrêtons pas en cas d'erreur
             * de syntaxe. */
            if (gauche->est_reference_declaration()) {
                m_tacheronne.assembleuse->recycle_référence(gauche->comme_reference_declaration());
            }
            noeud->expression = expression;
            analyse_annotations(noeud->annotations);

            m_noeud_expression_virgule = nullptr;

            return noeud;
        }
        case GenreLexème::EGAL:
        {
            consomme();

            m_noeud_expression_virgule = nullptr;

            if (gauche->est_declaration_variable()) {
                auto decl = gauche->comme_declaration_variable();
                if (decl->expression) {
                    /* repositionne le lexème courant afin que les messages d'erreurs pointent au
                     * bon endroit */
                    recule();
                    rapporte_erreur("utilisation de '=' alors que nous somme à droite de ':='");
                }

                decl->expression = analyse_expression(
                    données_précédence, racine_expression, lexème_final);

                m_noeud_expression_virgule = nullptr;

                analyse_directive_déclaration_variable(decl);
                analyse_annotations(decl->annotations);

                return decl;
            }

            if (gauche->est_virgule()) {
                auto noeud_virgule = gauche->comme_virgule();

                // détecte les expressions du style : a : z32, b = ...
                POUR (noeud_virgule->expressions) {
                    if (it->est_declaration_variable()) {
                        rapporte_erreur_avec_site(
                            it,
                            "Obtenu une déclaration de variable dans l'expression séparée par "
                            "virgule à gauche d'une assignation. Les variables doivent être "
                            "déclarées avant leurs assignations.");
                    }
                }

                auto noeud = m_tacheronne.assembleuse->crée_assignation_multiple(lexème);
                noeud->assignées = noeud_virgule;
                noeud->expression = analyse_expression(
                    données_précédence, racine_expression, lexème_final);

                m_noeud_expression_virgule = nullptr;
                return noeud;
            }

            auto noeud = m_tacheronne.assembleuse->crée_assignation_variable(lexème);
            noeud->assignée = gauche;
            noeud->expression = analyse_expression(
                données_précédence, racine_expression, lexème_final);

            m_noeud_expression_virgule = nullptr;

            return noeud;
        }
        case GenreLexème::POINT:
        {
            consomme();

            if (apparie(GenreLexème::CROCHET_OUVRANT)) {
                consomme();

                auto ancien_noeud_virgule = m_noeud_expression_virgule;
                m_noeud_expression_virgule = nullptr;
                auto expression = analyse_expression(
                    {}, GenreLexème::CROCHET_OUVRANT, GenreLexème::INCONNU);
                m_noeud_expression_virgule = ancien_noeud_virgule;

                if (!apparie(GenreLexème::CROCHET_FERMANT)) {
                    rapporte_erreur("Attendu un crochet fermant.");
                }
                consomme();

                auto noeud = m_tacheronne.assembleuse->crée_construction_tableau_type(lexème);
                noeud->expression_type = gauche;
                noeud->expression = expression;
                return noeud;
            }

            if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
                rapporte_erreur("Attendu un identifiant après '.'");
            }

            lexème = lexème_courant();
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_reference_membre(lexème);
            noeud->accedee = gauche;
            return noeud;
        }
        case GenreLexème::TROIS_POINTS:
        {
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_plage(lexème);
            noeud->debut = gauche;
            noeud->fin = analyse_expression(données_précédence, racine_expression, lexème_final);
            return noeud;
        }
        case GenreLexème::PARENTHESE_OUVRANTE:
        {
            return analyse_appel_fonction(gauche);
        }
        case GenreLexème::COMME:
        {
            consomme();

            auto noeud = m_tacheronne.assembleuse->crée_comme(lexème);
            noeud->expression = gauche;
            noeud->expression_type = analyse_expression_primaire(GenreLexème::COMME,
                                                                 GenreLexème::INCONNU);
            return noeud;
        }
        default:
        {
            assert_rappel(false, [&]() {
                dbg() << "Lexème inattendu comme expression secondaire : "
                      << chaine_du_lexème(lexème->genre) << '\n'
                      << crée_message_erreur("");
            });
            return nullptr;
        }
    }
}

NoeudExpression *Syntaxeuse::analyse_instruction()
{
    auto lexème = lexème_courant();

    switch (lexème->genre) {
        case GenreLexème::ACCOLADE_OUVRANTE:
        {
            return analyse_bloc();
        }
        case GenreLexème::DIFFERE:
        {
            consomme();
            auto inst = m_tacheronne.assembleuse->crée_differe(lexème);

            if (apparie(GenreLexème::ACCOLADE_OUVRANTE)) {
                auto bloc = analyse_bloc();
                bloc->appartiens_a_differe = inst;
                inst->expression = bloc;
            }
            else {
                inst->expression = analyse_expression(
                    {}, GenreLexème::DIFFERE, GenreLexème::INCONNU);
            }

            return inst;
        }
        case GenreLexème::NONSUR:
        {
            consomme();
            auto bloc = analyse_bloc();
            bloc->est_nonsur = true;
            return bloc;
        }
        case GenreLexème::ARRETE:
        {
            auto noeud = m_tacheronne.assembleuse->crée_arrete(lexème);
            consomme();

            if (apparie(GenreLexème::CHAINE_CARACTERE)) {
                noeud->expression = m_tacheronne.assembleuse->crée_reference_declaration(
                    lexème_courant());
                consomme();
            }

            return noeud;
        }
        case GenreLexème::CONTINUE:
        {
            auto noeud = m_tacheronne.assembleuse->crée_continue(lexème);
            consomme();

            if (apparie(GenreLexème::CHAINE_CARACTERE)) {
                noeud->expression = m_tacheronne.assembleuse->crée_reference_declaration(
                    lexème_courant());
                consomme();
            }

            return noeud;
        }
        case GenreLexème::REPRENDS:
        {
            auto noeud = m_tacheronne.assembleuse->crée_reprends(lexème);
            consomme();

            if (apparie(GenreLexème::CHAINE_CARACTERE)) {
                noeud->expression = m_tacheronne.assembleuse->crée_reference_declaration(
                    lexème_courant());
                consomme();
            }

            return noeud;
        }
        case GenreLexème::RETIENS:
        {
            auto noeud = m_tacheronne.assembleuse->crée_retiens(lexème);
            consomme();

            if (apparie_expression()) {
                noeud->expression = analyse_expression_avec_virgule(GenreLexème::RETIENS);
            }

            return noeud;
        }
        case GenreLexème::RETOURNE:
        {
            consomme();

            auto expression = NoeudExpression::nul();
            if (apparie_expression()) {
                expression = analyse_expression_avec_virgule(GenreLexème::RETOURNE);
            }

            if (m_fonction_courante_retourne_plusieurs_valeurs) {
                auto noeud = m_tacheronne.assembleuse->crée_retourne_multiple(lexème);
                noeud->expression = expression;
                return noeud;
            }

            auto noeud = m_tacheronne.assembleuse->crée_retourne(lexème);
            noeud->expression = expression;
            return noeud;
        }
        case GenreLexème::BOUCLE:
        {
            return analyse_instruction_boucle();
        }
        case GenreLexème::DISCR:
        {
            return analyse_instruction_discr();
        }
        case GenreLexème::POUR:
        {
            return analyse_instruction_pour();
        }
        case GenreLexème::POUSSE_CONTEXTE:
        {
            return analyse_instruction_pousse_contexte();
        }
        case GenreLexème::REPETE:
        {
            return analyse_instruction_répète();
        }
        case GenreLexème::SAUFSI:
        {
            return analyse_instruction_si(GenreNoeud::INSTRUCTION_SAUFSI);
        }
        case GenreLexème::SI:
        {
            return analyse_instruction_si(GenreNoeud::INSTRUCTION_SI);
        }
        case GenreLexème::TANTQUE:
        {
            return analyse_instruction_tantque();
        }
        default:
        {
            assert_rappel(false, [&]() {
                dbg() << "Lexème inattendu comme instruction : " << chaine_du_lexème(lexème->genre)
                      << '\n'
                      << crée_message_erreur("");
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

    auto lexème = lexème_courant();
    empile_état("dans l'analyse du bloc", lexème);

    if (accolade_requise) {
        consomme(GenreLexème::ACCOLADE_OUVRANTE, "Attendu une accolade ouvrante '{'");
    }

    NoeudDeclarationEnteteFonction *fonction_courante = fonctions_courantes.est_vide() ?
                                                            nullptr :
                                                            fonctions_courantes.haut();
    auto bloc = m_tacheronne.assembleuse->empile_bloc(lexème, fonction_courante);

    auto expressions = kuri::tablet<NoeudExpression *, 32>();

    while (!fini() && !apparie(GenreLexème::ACCOLADE_FERMANTE)) {
        if (ignore_point_virgule_implicite()) {
            continue;
        }

        if (apparie_instruction()) {
            auto noeud = analyse_instruction();
            expressions.ajoute(noeud);
        }
        else if (apparie_expression()) {
            auto noeud = analyse_expression({}, GenreLexème::INCONNU, GenreLexème::INCONNU);
            expressions.ajoute(noeud);
        }
        else {
            rapporte_erreur("attendu une expression ou une instruction");
        }
    }

    copie_tablet_tableau(expressions, *bloc->expressions.verrou_ecriture());
    m_tacheronne.assembleuse->dépile_bloc();

    if (accolade_requise) {
        consomme(GenreLexème::ACCOLADE_FERMANTE, "Attendu une accolade fermante '}'");
    }

    dépile_état();

    return bloc;
}

NoeudExpression *Syntaxeuse::analyse_appel_fonction(NoeudExpression *gauche)
{
    auto noeud = m_tacheronne.assembleuse->crée_appel(lexème_courant());
    noeud->expression = gauche;

    consomme(GenreLexème::PARENTHESE_OUVRANTE, "attendu une parenthèse ouvrante");

    auto params = kuri::tablet<NoeudExpression *, 16>();

    while (!fini() && apparie_expression()) {
        auto expr = analyse_expression({}, GenreLexème::FONC, GenreLexème::VIRGULE);
        params.ajoute(expr);

        if (expr->est_declaration_variable()) {
            rapporte_erreur("Obtenu une déclaration de variable dans l'expression d'appel");
        }

        if (ignore_point_virgule_implicite()) {
            if (!apparie(GenreLexème::PARENTHESE_FERMANTE) && !apparie(GenreLexème::VIRGULE)) {
                rapporte_erreur(
                    "Attendu une parenthèse fermante ou une virgule après la nouvelle ligne");
            }

            continue;
        }

        if (!apparie(GenreLexème::VIRGULE)) {
            break;
        }

        consomme();
    }

    copie_tablet_tableau(params, noeud->parametres);

    consomme(GenreLexème::PARENTHESE_FERMANTE,
             "attendu ')' à la fin des arguments de l'expression d'appel");

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_boucle()
{
    auto noeud = m_tacheronne.assembleuse->crée_boucle(lexème_courant());
    consomme();
    noeud->bloc = analyse_bloc();
    noeud->bloc->appartiens_a_boucle = noeud;
    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_discr()
{
    auto noeud_discr = m_tacheronne.assembleuse->crée_discr(lexème_courant());
    consomme();

    noeud_discr->expression_discriminee = analyse_expression(
        {}, GenreLexème::DISCR, GenreLexème::INCONNU);

    noeud_discr->bloc = m_tacheronne.assembleuse->empile_bloc(lexème_courant(),
                                                              fonctions_courantes.haut());
    noeud_discr->bloc->appartiens_à_discr = noeud_discr;

    consomme(GenreLexème::ACCOLADE_OUVRANTE,
             "Attendu une accolade ouvrante '{' après l'expression de « discr »");

    auto sinon_rencontre = false;

    auto paires_discr = kuri::tablet<NoeudPaireDiscr *, 32>();

    while (!fini() && !apparie(GenreLexème::ACCOLADE_FERMANTE)) {
        if (apparie(GenreLexème::SINON)) {
            consomme();

            if (sinon_rencontre) {
                rapporte_erreur("Redéfinition d'un bloc sinon");
            }

            sinon_rencontre = true;

            noeud_discr->bloc_sinon = analyse_bloc();
        }
        else {
            auto expr = analyse_expression({}, GenreLexème::INCONNU, GenreLexème::INCONNU);
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

    consomme(GenreLexème::ACCOLADE_FERMANTE,
             "Attendu une accolade fermante '}' à la fin du bloc de « discr »");

    m_tacheronne.assembleuse->dépile_bloc();
    return noeud_discr;
}

void Syntaxeuse::analyse_specifiants_instruction_pour(NoeudPour *noeud)
{
    bool eu_direction = false;

    while (!fini()) {
        switch (lexème_courant()->genre) {
            default:
            {
                return;
            }
            case GenreLexème::ESPERLUETTE:
            case GenreLexème::ESP_UNAIRE:
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
            case GenreLexème::FOIS:
            case GenreLexème::FOIS_UNAIRE:
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
            case GenreLexème::INFERIEUR:
            case GenreLexème::SUPERIEUR:
            {
                if (eu_direction) {
                    rapporte_erreur(
                        "redéfinition d'une direction alors qu'une autre a déjà été spécifiée");
                }

                noeud->lexeme_op = lexème_courant()->genre;
                eu_direction = true;
                consomme();
                break;
            }
        }
    }
}

NoeudExpression *Syntaxeuse::analyse_instruction_pour()
{
    auto noeud = m_tacheronne.assembleuse->crée_pour(lexème_courant());
    consomme();

    analyse_specifiants_instruction_pour(noeud);

    auto expression = analyse_expression({}, GenreLexème::POUR, GenreLexème::INCONNU);

    if (apparie(GenreLexème::DANS)) {
        consomme();

        if (!expression->est_virgule()) {
            static Lexème lexème_virgule = {",", {}, GenreLexème::VIRGULE, 0, 0, 0};
            auto noeud_virgule = m_tacheronne.assembleuse->crée_virgule(&lexème_virgule);
            noeud_virgule->expressions.ajoute(expression);

            expression = noeud_virgule;
        }
        else {
            m_noeud_expression_virgule = nullptr;
        }

        noeud->variable = expression;
        noeud->expression = analyse_expression({}, GenreLexème::DANS, GenreLexème::INCONNU);
    }
    else {
        auto noeud_it = m_tacheronne.assembleuse->crée_reference_declaration(noeud->lexeme);
        noeud_it->ident = ID::it;

        auto noeud_index = m_tacheronne.assembleuse->crée_reference_declaration(noeud->lexeme);
        noeud_index->ident = ID::index_it;

        static Lexème lexème_virgule = {",", {}, GenreLexème::VIRGULE, 0, 0, 0};
        auto noeud_virgule = m_tacheronne.assembleuse->crée_virgule(&lexème_virgule);
        noeud_virgule->expressions.ajoute(noeud_it);
        noeud_virgule->expressions.ajoute(noeud_index);

        noeud->variable = noeud_virgule;
        noeud->expression = expression;
    }

    noeud->bloc = analyse_bloc();
    noeud->bloc->appartiens_a_boucle = noeud;

    if (apparie(GenreLexème::SANSARRET)) {
        consomme();
        noeud->bloc_sansarret = analyse_bloc();
    }

    if (apparie(GenreLexème::SINON)) {
        consomme();
        noeud->bloc_sinon = analyse_bloc();
    }

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_pousse_contexte()
{
    auto noeud = m_tacheronne.assembleuse->crée_pousse_contexte(lexème_courant());
    consomme();

    noeud->expression = analyse_expression({}, GenreLexème::POUSSE_CONTEXTE, GenreLexème::INCONNU);
    noeud->bloc = analyse_bloc(true);

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_répète()
{
    auto noeud = m_tacheronne.assembleuse->crée_repete(lexème_courant());
    consomme();

    noeud->bloc = analyse_bloc();
    noeud->bloc->appartiens_a_boucle = noeud;

    consomme(GenreLexème::TANTQUE, "Attendu une 'tantque' après le bloc de 'répète'");

    noeud->condition = analyse_expression({}, GenreLexème::REPETE, GenreLexème::INCONNU);

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_si(GenreNoeud genre_noeud)
{
    empile_état("dans l'analyse de l'instruction si", lexème_courant());

    auto noeud = m_tacheronne.assembleuse->crée_si(lexème_courant(), genre_noeud);
    consomme();

    noeud->condition = analyse_expression({}, GenreLexème::SI, GenreLexème::INCONNU);

    noeud->bloc_si_vrai = analyse_bloc();

    if (apparie(GenreLexème::SINON)) {
        consomme();

        if (apparie(GenreLexème::SI)) {
            noeud->bloc_si_faux = analyse_instruction_si(GenreNoeud::INSTRUCTION_SI);
        }
        else if (apparie(GenreLexème::SAUFSI)) {
            noeud->bloc_si_faux = analyse_instruction_si(GenreNoeud::INSTRUCTION_SAUFSI);
        }
        else if (apparie(GenreLexème::TANTQUE)) {
            noeud->bloc_si_faux = analyse_instruction_tantque();
        }
        else if (apparie(GenreLexème::POUR)) {
            noeud->bloc_si_faux = analyse_instruction_pour();
        }
        else if (apparie(GenreLexème::BOUCLE)) {
            noeud->bloc_si_faux = analyse_instruction_boucle();
        }
        else if (apparie(GenreLexème::REPETE)) {
            noeud->bloc_si_faux = analyse_instruction_répète();
        }
        else if (apparie(GenreLexème::DISCR)) {
            noeud->bloc_si_faux = analyse_instruction_discr();
        }
        else {
            noeud->bloc_si_faux = analyse_bloc();
        }
    }

    dépile_état();

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_si_statique(Lexème *lexème)
{
    empile_état("dans l'analyse de l'instruction #si", lexème);

    auto noeud = (lexème->genre == GenreLexème::SI) ?
                     m_tacheronne.assembleuse->crée_si_statique(lexème) :
                     m_tacheronne.assembleuse->crée_saufsi_statique(lexème);

    noeud->condition = analyse_expression({}, GenreLexème::SI, GenreLexème::INCONNU);

    noeud->bloc_si_vrai = analyse_bloc();

    if (apparie(GenreLexème::SINON)) {
        consomme();

        if (apparie(GenreLexème::SI)) {
            /* analyse_instruction_si_statique doit commencer par le lexème suivant vu la manière
             * dont les directives sont parsées. */
            lexème = lexème_courant();
            consomme();
            noeud->bloc_si_faux = analyse_instruction_si_statique(lexème);
        }
        else if (apparie(GenreLexème::SAUFSI)) {
            /* analyse_instruction_si_statique doit commencer par le lexème suivant vu la manière
             * dont les directives sont parsées. */
            lexème = lexème_courant();
            consomme();
            noeud->bloc_si_faux = analyse_instruction_si_statique(lexème);
        }
        else if (apparie(GenreLexème::ACCOLADE_OUVRANTE)) {
            noeud->bloc_si_faux = analyse_bloc();
        }
        else {
            rapporte_erreur("l'instruction « sinon » des #si statiques doit être suivie par soit "
                            "un « si », soit un « saufsi », ou alors un bloc");
        }
    }

    dépile_état();

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_instruction_tantque()
{
    auto noeud = m_tacheronne.assembleuse->crée_tantque(lexème_courant());
    consomme();

    noeud->condition = analyse_expression({}, GenreLexème::TANTQUE, GenreLexème::INCONNU);
    noeud->bloc = analyse_bloc();
    noeud->bloc->appartiens_a_boucle = noeud;

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_expression_avec_virgule(GenreLexème lexème_racine)
{
    kuri::tablet<NoeudExpression *, 6> expressions;
    Lexème *lexème_virgule = nullptr;

    while (!fini()) {
        auto expr = analyse_expression({}, lexème_racine, GenreLexème::VIRGULE);
        expressions.ajoute(expr);

        if (!apparie(GenreLexème::VIRGULE)) {
            break;
        }

        if (lexème_virgule == nullptr) {
            lexème_virgule = lexème_courant();
        }

        consomme();
    }

    if (expressions.taille() == 1) {
        return expressions[0];
    }

    auto virgule = m_tacheronne.assembleuse->crée_virgule(lexème_virgule);
    copie_tablet_tableau(expressions, virgule->expressions);
    m_noeud_expression_virgule = nullptr;
    return virgule;
}

NoeudExpression *Syntaxeuse::analyse_référence_déclaration(Lexème const *lexème_référence)
{
    sauvegarde_position_lexème();

    if (!m_noeud_expression_virgule && apparie(GenreLexème::DECLARATION_CONSTANTE)) {
        consomme();

        auto lexème = lexème_courant();
        switch (lexème->genre) {
            case GenreLexème::COROUT:
            case GenreLexème::FONC:
            {
                annule_sauvegarde_position();

                auto noeud_fonction = analyse_déclaration_fonction(lexème_référence);

                if (noeud_fonction->est_expression_type_fonction()) {
                    auto noeud = m_tacheronne.assembleuse->crée_declaration_constante(lexème);
                    noeud->ident = lexème_référence->ident;
                    noeud->expression = noeud_fonction;
                    return noeud;
                }

                return noeud_fonction;
            }
            case GenreLexème::STRUCT:
            {
                annule_sauvegarde_position();
                return analyse_déclaration_structure(lexème_référence);
            }
            case GenreLexème::UNION:
            {
                annule_sauvegarde_position();
                return analyse_déclaration_union(lexème_référence);
            }
            case GenreLexème::ENUM:
            case GenreLexème::ENUM_DRAPEAU:
            case GenreLexème::ERREUR:
            {
                annule_sauvegarde_position();
                return analyse_déclaration_enum(lexème_référence);
            }
            default:
            {
                break;
            }
        }
    }

    restaure_position_lexème();
    return m_tacheronne.assembleuse->crée_reference_declaration(lexème_référence);
}

void Syntaxeuse::analyse_annotations(kuri::tableau<Annotation, int> &annotations)
{
    while (!fini() && apparie(GenreLexème::AROBASE)) {
        consomme();

        if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
            rapporte_erreur("Attendu une chaine de caractère après '@'");
        }

        auto lexème_annotation = lexème_courant();
        auto annotation = Annotation{};
        annotation.nom = lexème_annotation->chaine;

        consomme();

        if (apparie(GenreLexème::CHAINE_LITTERALE)) {
            auto chaine = lexème_courant()->chaine;
            annotation.valeur = chaine;
            consomme();
        }

        annotations.ajoute(annotation);
    }
}

NoeudExpression *Syntaxeuse::analyse_déclaration_enum(Lexème const *lexème_nom)
{
    auto lexème = lexème_courant();
    empile_état("dans le syntaxage de l'énumération", lexème);
    consomme();

    auto noeud_decl = NoeudEnum::nul();

    if (lexème->genre == GenreLexème::ENUM) {
        noeud_decl = m_tacheronne.assembleuse->crée_type_enum(lexème_nom);
    }
    else if (lexème->genre == GenreLexème::ENUM_DRAPEAU) {
        noeud_decl = m_tacheronne.assembleuse->crée_type_enum_drapeau(lexème_nom);
    }
    else {
        assert(lexème->genre == GenreLexème::ERREUR);
        noeud_decl = m_tacheronne.assembleuse->crée_type_erreur(lexème_nom);
    }

    if (lexème->genre != GenreLexème::ERREUR) {
        if (!apparie(GenreLexème::ACCOLADE_OUVRANTE)) {
            noeud_decl->expression_type = analyse_expression_primaire(GenreLexème::ENUM,
                                                                      GenreLexème::INCONNU);
        }
    }

    auto lexème_bloc = lexème_courant();
    consomme(GenreLexème::ACCOLADE_OUVRANTE, "Attendu '{' après 'énum'");

    auto bloc = m_tacheronne.assembleuse->empile_bloc(lexème_bloc, nullptr);

    bloc->appartiens_à_type = noeud_decl;

    auto expressions = kuri::tablet<NoeudExpression *, 16>();

    while (!fini() && !apparie(GenreLexème::ACCOLADE_FERMANTE)) {
        if (ignore_point_virgule_implicite()) {
            continue;
        }

        if (!apparie_expression()) {
            rapporte_erreur("Attendu une expression dans le bloc de l'énumération");
            continue;
        }

        auto noeud = analyse_expression({}, GenreLexème::INCONNU, GenreLexème::INCONNU);

        if (noeud->est_reference_declaration()) {
            auto decl_variable = m_tacheronne.assembleuse->crée_declaration_constante(
                noeud->lexeme);
            expressions.ajoute(decl_variable);
        }
        else if (noeud->est_declaration_constante()) {
            expressions.ajoute(noeud);
        }
        else {
            rapporte_erreur("Expression inattendu dans la déclaration des membres de l'énum");
        }
    }

    copie_tablet_tableau(expressions, *bloc->expressions.verrou_ecriture());

    m_tacheronne.assembleuse->dépile_bloc();
    noeud_decl->bloc = bloc;

    consomme(GenreLexème::ACCOLADE_FERMANTE, "Attendu '}' à la fin de la déclaration de l'énum");

    analyse_annotations(noeud_decl->annotations);

    dépile_état();

    /* Attend d'avoir toutes les informations avant d'ajouter aux membres. */
    noeud_decl->bloc_parent->ajoute_membre(noeud_decl);
    return noeud_decl;
}

bool Syntaxeuse::est_déclaration_type_fonction()
{
    consomme(GenreLexème::PARENTHESE_OUVRANTE,
             "Attendu une parenthèse ouvrante après le nom de la fonction");

    auto profondeur_parenthèse = 0;

    while (!fini()) {
        if (apparie(GenreLexème::PARENTHESE_OUVRANTE)) {
            profondeur_parenthèse += 1;
            consomme();
            continue;
        }

        if (apparie(GenreLexème::PARENTHESE_FERMANTE)) {
            if (profondeur_parenthèse == 0) {
                consomme();
                break;
            }

            profondeur_parenthèse -= 1;
            consomme();
            continue;
        }

        consomme();
    }

    return apparie(GenreLexème::PARENTHESE_OUVRANTE);
}

NoeudExpression *Syntaxeuse::analyse_déclaration_fonction(Lexème const *lexème)
{
    auto lexème_mot_clé = lexème_courant();
    consomme();

    sauvegarde_position_lexème();
    auto const est_déclaration_type = est_déclaration_type_fonction();
    restaure_position_lexème();

    if (est_déclaration_type) {
        return analyse_déclaration_type_fonction(lexème_mot_clé);
    }

    empile_état("dans le syntaxage de la fonction", lexème_mot_clé);

    auto noeud = m_tacheronne.assembleuse->crée_entete_fonction(lexème);
    noeud->est_coroutine = lexème_mot_clé->genre == GenreLexème::COROUT;

    // @concurrence critique, si nous avons plusieurs définitions
    if (noeud->ident == ID::principale) {
        if (m_unité->espace->fonction_principale) {
            m_unité->espace
                ->rapporte_erreur(noeud, "Redéfinition de la fonction principale pour cet espace.")
                .ajoute_message("La fonction principale fut déjà définie ici :\n")
                .ajoute_site(m_unité->espace->fonction_principale);
        }

        m_unité->espace->fonction_principale = noeud;
        noeud->drapeaux_fonction |= DrapeauxNoeudFonction::EST_RACINE;
    }

    auto lexème_bloc = lexème_courant();
    consomme(GenreLexème::PARENTHESE_OUVRANTE,
             "Attendu une parenthèse ouvrante après le nom de la fonction");

    noeud->bloc_constantes = m_tacheronne.assembleuse->empile_bloc(lexème_bloc, noeud);
    noeud->bloc_parametres = m_tacheronne.assembleuse->empile_bloc(lexème_bloc, noeud);

    bloc_constantes_polymorphiques.empile(noeud->bloc_constantes);

    /* analyse les paramètres de la fonction */
    auto params = kuri::tablet<NoeudExpression *, 16>();

    while (!fini() && !apparie(GenreLexème::PARENTHESE_FERMANTE)) {
        auto param = analyse_expression({}, GenreLexème::INCONNU, GenreLexème::VIRGULE);
        params.ajoute(param);

        if (param->est_declaration_variable()) {
            auto decl_var = param->comme_declaration_variable();
            decl_var->drapeaux |= DrapeauxNoeud::EST_PARAMETRE;
        }
        else if (param->est_empl()) {
            auto decl_var = param->comme_empl()->expression->comme_declaration_variable();
            decl_var->drapeaux |= DrapeauxNoeud::EST_PARAMETRE;
        }

        if (!apparie(GenreLexème::VIRGULE)) {
            break;
        }

        consomme();
    }

    copie_tablet_tableau(params, noeud->params);

    consomme(GenreLexème::PARENTHESE_FERMANTE,
             "Attendu ')' à la fin des paramètres de la fonction");

    // nous avons la déclaration d'une fonction
    if (apparie(GenreLexème::RETOUR_TYPE)) {
        consomme();
        analyse_expression_retour_type(noeud, false);
    }
    else {
        Lexème *lexème_rien = m_tacheronne.lexemes_extra.ajoute_element();
        *lexème_rien = *lexème;
        lexème_rien->genre = GenreLexème::RIEN;
        lexème_rien->chaine = "";

        auto decl = crée_retour_défaut_fonction(m_tacheronne.assembleuse, lexème_rien);

        noeud->params_sorties.ajoute(decl);
        noeud->param_sortie = noeud->params_sorties[0]->comme_declaration_variable();
    }

    auto bloc_constantes = bloc_constantes_polymorphiques.depile();
    if (bloc_constantes->nombre_de_membres() != 0) {
        noeud->drapeaux_fonction |= DrapeauxNoeudFonction::EST_POLYMORPHIQUE;
    }

    ignore_point_virgule_implicite();

    DrapeauxNoeudFonction drapeaux_fonction = DrapeauxNoeudFonction::AUCUN;
    while (!fini() && apparie(GenreLexème::DIRECTIVE)) {
        consomme();

        auto ident_directive = lexème_courant()->ident;

        if (ident_directive == ID::enligne) {
            drapeaux_fonction |= DrapeauxNoeudFonction::FORCE_ENLIGNE;
        }
        else if (ident_directive == ID::horsligne) {
            drapeaux_fonction |= DrapeauxNoeudFonction::FORCE_HORSLIGNE;
        }
        else if (ident_directive == ID::externe) {
            noeud->drapeaux |= DrapeauxNoeud::EST_EXTERNE;
            drapeaux_fonction |= DrapeauxNoeudFonction::EST_EXTERNE;

            if (lexème_mot_clé->genre == GenreLexème::COROUT) {
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
            if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
                rapporte_erreur("Attendu un identifiant après #cliche");
            }

            while (apparie(GenreLexème::CHAINE_CARACTERE)) {
                auto ident_cliché = lexème_courant()->ident;

                if (ident_cliché == ID::asa) {
                    drapeaux_fonction |= DrapeauxNoeudFonction::CLICHÉ_ASA_FUT_REQUIS;
                }
                else if (ident_cliché == ID::asa_canon) {
                    drapeaux_fonction |= DrapeauxNoeudFonction::CLICHÉ_ASA_CANONIQUE_FUT_REQUIS;
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

                if (!apparie(GenreLexème::VIRGULE)) {
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

            if (!apparie(GenreLexème::CHAINE_LITTERALE)) {
                rapporte_erreur("Attendu le symbole de l'intrinsèque");
            }

            noeud->données_externes =
                m_tacheronne.allocatrice_noeud.crée_données_symbole_externe();
            noeud->données_externes->nom_symbole = lexème_courant()->chaine;
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
            rapporte_erreur("Ne peut avoir plusieurs valeur de retour pour une fonction externe");
        }

        /* ajoute un bloc même pour les fonctions externes, afin de stocker les paramètres */
        noeud->corps->bloc = m_tacheronne.assembleuse->empile_bloc(lexème_courant(), noeud);
        m_tacheronne.assembleuse->dépile_bloc();

        /* Si la déclaration est à la fin du fichier, il peut ne pas y avoir de point-virgule,
         * donc ne générons pas d'erreur s'il n'y en a pas. */
        ignore_point_virgule_implicite();
    }
    else {
        ignore_point_virgule_implicite();

        auto ancien_état_retour = m_fonction_courante_retourne_plusieurs_valeurs;
        m_fonction_courante_retourne_plusieurs_valeurs = noeud->params_sorties.taille() > 1;

        auto noeud_corps = noeud->corps;
        fonctions_courantes.empile(noeud);

        if (apparie(GenreLexème::POUSSE_CONTEXTE)) {
            empile_état("dans l'analyse du bloc", lexème_courant());
            noeud_corps->bloc = m_tacheronne.assembleuse->empile_bloc(lexème_courant(), noeud);
            auto pousse_contexte = analyse_instruction_pousse_contexte();
            noeud_corps->bloc->ajoute_expression(pousse_contexte);
            m_tacheronne.assembleuse->dépile_bloc();
            dépile_état();
        }
        else {
            noeud_corps->bloc = analyse_bloc();
            noeud_corps->bloc->ident = noeud->ident;
        }

        analyse_annotations(noeud->annotations);
        fonctions_courantes.depile();
        m_fonction_courante_retourne_plusieurs_valeurs = ancien_état_retour;
    }

    /* dépile le bloc des paramètres */
    m_tacheronne.assembleuse->dépile_bloc();

    /* dépile le bloc des constantes */
    m_tacheronne.assembleuse->dépile_bloc();

    dépile_état();

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
    noeud->bloc_parent->ajoute_membre(noeud);

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_déclaration_type_fonction(Lexème const *lexème)
{
    auto noeud = m_tacheronne.assembleuse->crée_expression_type_fonction(lexème);

    consomme(GenreLexème::PARENTHESE_OUVRANTE,
             "Attendu une parenthèse ouvrante après le nom de la fonction");

    /* analyse les paramètres de la fonction */
    auto params = kuri::tablet<NoeudExpression *, 16>();

    while (!fini() && !apparie(GenreLexème::PARENTHESE_FERMANTE)) {
        auto param = analyse_expression({}, GenreLexème::INCONNU, GenreLexème::VIRGULE);
        params.ajoute(param);

        if (param->est_declaration_variable()) {
            rapporte_erreur_avec_site(param,
                                      "Obtenu la déclaration d'une variable dans "
                                      "la déclaration d'un type de fonction");
        }
        else if (param->est_empl()) {
            rapporte_erreur_avec_site(param,
                                      "Obtenu la déclaration d'une variable dans "
                                      "la déclaration d'un type de fonction");
        }

        if (!apparie(GenreLexème::VIRGULE)) {
            break;
        }

        consomme();
    }
    copie_tablet_tableau(params, noeud->types_entrée);

    consomme(GenreLexème::PARENTHESE_FERMANTE,
             "Attendu ')' à la fin des paramètres du type fonction");

    consomme(GenreLexème::PARENTHESE_OUVRANTE,
             "Attendu une parenthèse ouvrante pour le type de retour du type fonction");

    while (!fini()) {
        auto type_declare = analyse_expression({}, GenreLexème::FONC, GenreLexème::VIRGULE);
        noeud->types_sortie.ajoute(type_declare);

        if (!apparie(GenreLexème::VIRGULE)) {
            break;
        }

        consomme();
    }

    consomme(GenreLexème::PARENTHESE_FERMANTE,
             "attendu une parenthèse fermante après le type de retour du type fonction");

    return noeud;
}

NoeudExpression *Syntaxeuse::analyse_déclaration_opérateur()
{
    empile_état("dans le syntaxage de l'opérateur", lexème_courant());
    consomme();

    auto lexème = lexème_courant();
    auto genre_opérateur = lexème->genre;

    if (!est_opérateur_surchargeable(genre_opérateur)) {
        rapporte_erreur("L'opérateur n'est pas surchargeable");
        return nullptr;
    }

    consomme();

    if (genre_opérateur == GenreLexème::CROCHET_OUVRANT) {
        consomme(GenreLexème::CROCHET_FERMANT,
                 "Attendu ']' après '[' pour la déclaration de l'opérateur");
    }

    // :: fonc
    consomme(GenreLexème::DECLARATION_CONSTANTE, "Attendu :: après la déclaration de l'opérateur");
    consomme(GenreLexème::FONC, "Attendu fonc après ::");

    auto noeud = (genre_opérateur == GenreLexème::POUR) ?
                     m_tacheronne.assembleuse->crée_operateur_pour(lexème) :
                     m_tacheronne.assembleuse->crée_entete_fonction(lexème);
    noeud->est_operateur = true;

    auto lexème_bloc = lexème_courant();
    consomme(GenreLexème::PARENTHESE_OUVRANTE,
             "Attendu une parenthèse ouvrante après le nom de la fonction");

    noeud->bloc_constantes = m_tacheronne.assembleuse->empile_bloc(lexème_bloc, noeud);
    noeud->bloc_parametres = m_tacheronne.assembleuse->empile_bloc(lexème_bloc, noeud);

    /* analyse les paramètres de la fonction */
    auto params = kuri::tablet<NoeudExpression *, 16>();

    while (!fini() && !apparie(GenreLexème::PARENTHESE_FERMANTE)) {
        auto param = analyse_expression({}, GenreLexème::INCONNU, GenreLexème::VIRGULE);

        if (!param->est_declaration_variable()) {
            rapporte_erreur_avec_site(
                param, "Expression inattendue dans la déclaration des paramètres de l'opérateur");
        }

        auto decl_var = param->comme_declaration_variable();
        decl_var->drapeaux |= DrapeauxNoeud::EST_PARAMETRE;
        params.ajoute(decl_var);

        if (!apparie(GenreLexème::VIRGULE)) {
            break;
        }

        consomme();
    }

    copie_tablet_tableau(params, noeud->params);

    if (noeud->params.taille() > 2) {
        rapporte_erreur_avec_site(noeud,
                                  "La surcharge d'opérateur ne peut prendre au plus 2 paramètres");
    }
    else if (noeud->params.taille() == 1) {
        if (genre_opérateur == GenreLexème::PLUS) {
            lexème->genre = GenreLexème::PLUS_UNAIRE;
        }
        else if (genre_opérateur == GenreLexème::MOINS) {
            lexème->genre = GenreLexème::MOINS_UNAIRE;
        }
        else if (!dls::outils::est_element(genre_opérateur,
                                           GenreLexème::TILDE,
                                           GenreLexème::PLUS_UNAIRE,
                                           GenreLexème::MOINS_UNAIRE,
                                           GenreLexème::POUR)) {
            rapporte_erreur("La surcharge d'opérateur unaire n'est possible que "
                            "pour '+', '-', '~', ou 'pour'");
            return nullptr;
        }
    }

    consomme(GenreLexème::PARENTHESE_FERMANTE,
             "Attendu ')' à la fin des paramètres de la fonction");

    /* analyse les types de retour de la fonction */
    consomme(GenreLexème::RETOUR_TYPE, "Attendu un retour de type");

    analyse_expression_retour_type(noeud, true);

    while (!fini() && apparie(GenreLexème::DIRECTIVE)) {
        consomme();

        auto directive = lexème_courant()->ident;

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
    m_tacheronne.assembleuse->dépile_bloc();

    /* dépile le bloc des constantes */
    m_tacheronne.assembleuse->dépile_bloc();

    dépile_état();

    return noeud;
}

void Syntaxeuse::analyse_expression_retour_type(NoeudDeclarationEnteteFonction *noeud,
                                                bool pour_opérateur)
{
    auto eu_parenthèse = false;
    if (apparie(GenreLexème::PARENTHESE_OUVRANTE)) {
        consomme();
        eu_parenthèse = true;
    }

    while (!fini()) {
        auto decl_sortie = analyse_expression({}, GenreLexème::FONC, GenreLexème::VIRGULE);

        if (!decl_sortie) {
            /* Nous avons une erreur, nous pouvons retourner. */
            return;
        }

        if (!decl_sortie->est_declaration_variable()) {
            auto ident = m_compilatrice.donne_nom_défaut_valeur_retour(
                noeud->params_sorties.taille());

            auto decl = m_tacheronne.assembleuse->crée_declaration_variable(decl_sortie->lexeme);
            decl->ident = ident;
            decl->expression_type = decl_sortie;
            decl->bloc_parent = decl_sortie->bloc_parent;

            decl_sortie = decl;
        }

        decl_sortie->drapeaux |= DrapeauxNoeud::EST_PARAMETRE;

        noeud->params_sorties.ajoute(decl_sortie->comme_declaration_variable());

        if (!apparie(GenreLexème::VIRGULE)) {
            break;
        }

        consomme();
    }

    auto const nombre_de_valeurs_retournées = noeud->params_sorties.taille();
    if (nombre_de_valeurs_retournées == 0) {
        rapporte_erreur("Attendu au moins une déclaration de valeur retournée");
        return;
    }

    if (nombre_de_valeurs_retournées > 1) {
        if (pour_opérateur) {
            rapporte_erreur("Il est impossible d'avoir plusieurs de sortie pour un opérateur");
            return;
        }

        /* il nous faut un identifiant valide */
        noeud->param_sortie = m_tacheronne.assembleuse->crée_declaration_variable(
            noeud->params_sorties[0]->lexeme);
        noeud->param_sortie->ident = m_compilatrice.table_identifiants
                                         ->identifiant_pour_nouvelle_chaine("valeur_de_retour");
    }
    else {
        noeud->param_sortie = noeud->params_sorties[0]->comme_declaration_variable();
    }

    noeud->param_sortie->drapeaux |= DrapeauxNoeud::EST_PARAMETRE;

    if (eu_parenthèse) {
        consomme(GenreLexème::PARENTHESE_FERMANTE,
                 "attendu une parenthèse fermante après la liste des retours de la fonction");
    }
}

/* ------------------------------------------------------------------------- */
/** \name Structures et unions.
 * \{ */

NoeudExpression *Syntaxeuse::analyse_déclaration_structure(Lexème const *lexème_nom)
{
    auto lexème_mot_clé = lexème_courant();
    empile_état("dans le syntaxage de la structure", lexème_mot_clé);
    consomme();

    auto noeud_decl = m_tacheronne.assembleuse->crée_type_structure(lexème_nom);

    if (lexème_nom->ident == ID::InfoType) {
        m_compilatrice.typeuse.type_info_type_ = noeud_decl;
    }
    else if (lexème_nom->ident == ID::ContexteProgramme) {
        m_compilatrice.typeuse.type_contexte = noeud_decl;
    }

    analyse_paramètres_polymorphiques_structure_ou_union(noeud_decl);
    analyse_directives_structure(noeud_decl);
    analyse_membres_structure_ou_union(noeud_decl);
    analyse_annotations(noeud_decl->annotations);

    if (noeud_decl->bloc_constantes) {
        m_tacheronne.assembleuse->dépile_bloc();
    }

    dépile_état();

    /* N'ajoute la structure au bloc parent que lorsque nous avons son bloc, ou la validation
     * sémantique pourrait accéder un bloc nul. */
    noeud_decl->bloc_parent->ajoute_membre(noeud_decl);

    return noeud_decl;
}

NoeudExpression *Syntaxeuse::analyse_déclaration_union(Lexème const *lexème_nom)
{
    auto lexème_mot_clé = lexème_courant();
    empile_état("dans le syntaxage de l'union", lexème_mot_clé);
    consomme();

    auto noeud_decl = m_tacheronne.assembleuse->crée_type_union(lexème_nom);

    if (apparie(GenreLexème::NONSUR)) {
        noeud_decl->est_nonsure = true;
        consomme();
    }

    analyse_paramètres_polymorphiques_structure_ou_union(noeud_decl);
    analyse_directives_union(noeud_decl);
    analyse_membres_structure_ou_union(noeud_decl);
    analyse_annotations(noeud_decl->annotations);

    if (noeud_decl->bloc_constantes) {
        m_tacheronne.assembleuse->dépile_bloc();
    }

    dépile_état();

    /* N'ajoute la structure au bloc parent que lorsque nous avons son bloc, ou la validation
     * sémantique pourrait accéder un bloc nul. */
    noeud_decl->bloc_parent->ajoute_membre(noeud_decl);

    return noeud_decl;
}

void Syntaxeuse::analyse_directives_structure(NoeudStruct *noeud)
{
    while (!fini() && apparie(GenreLexème::DIRECTIVE)) {
        consomme();

        auto ident_directive = lexème_courant()->ident;

        if (ident_directive == ID::interface) {
            renseigne_type_interface(m_compilatrice.typeuse, noeud->ident, noeud);
            if (noeud->ident == ID::InfoType) {
                TypeBase::EINI->comme_type_compose()->membres[1].type =
                    m_compilatrice.typeuse.type_pointeur_pour(noeud);
            }
        }
        else if (ident_directive == ID::externe) {
            noeud->est_externe = true;
        }
        else if (ident_directive == ID::corps_texte) {
            noeud->est_corps_texte = true;
        }
        else if (ident_directive == ID::compacte) {
            noeud->est_compacte = true;
        }
        else if (ident_directive == ID::aligne) {
            consomme();

            if (!apparie(GenreLexème::NOMBRE_ENTIER)) {
                rapporte_erreur("Un nombre entier est requis après « aligne »");
            }

            noeud->alignement_desire = static_cast<uint32_t>(lexème_courant()->valeur_entiere);

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

void Syntaxeuse::analyse_directives_union(NoeudUnion *noeud)
{
    while (!fini() && apparie(GenreLexème::DIRECTIVE)) {
        consomme();

        auto ident_directive = lexème_courant()->ident;

        if (ident_directive == ID::externe) {
            noeud->est_externe = true;
            /* #externe implique nonsûr */
            noeud->est_nonsure = true;
        }
        else if (ident_directive == ID::corps_texte) {
            noeud->est_corps_texte = true;
        }
        else {
            rapporte_erreur("Directive impossible pour une union");
        }

        consomme();
    }
}

void Syntaxeuse::analyse_paramètres_polymorphiques_structure_ou_union(
    NoeudDeclarationClasse *noeud)
{
    if (!apparie(GenreLexème::PARENTHESE_OUVRANTE)) {
        return;
    }

    noeud->bloc_constantes = m_tacheronne.assembleuse->empile_bloc(lexème_courant(), nullptr);

    bloc_constantes_polymorphiques.empile(noeud->bloc_constantes);
    SUR_SORTIE_PORTEE {
        auto bloc_constantes = bloc_constantes_polymorphiques.depile();
        if (bloc_constantes->nombre_de_membres() != 0) {
            noeud->est_polymorphe = true;
        }
    };

    consomme();

    while (!fini() && !apparie(GenreLexème::PARENTHESE_FERMANTE)) {
        auto expression = analyse_expression(
            {}, GenreLexème::PARENTHESE_OUVRANTE, GenreLexème::VIRGULE);

        if (!expression->est_declaration_constante()) {
            rapporte_erreur_avec_site(expression,
                                      "Attendu une déclaration constante dans les "
                                      "paramètres polymorphiques de la structure");
        }

        if (!apparie(GenreLexème::VIRGULE)) {
            break;
        }

        consomme();
    }

    consomme();
}

void Syntaxeuse::analyse_membres_structure_ou_union(NoeudDeclarationClasse *decl_struct)
{
    if (decl_struct->est_externe && ignore_point_virgule_implicite()) {
        decl_struct->drapeaux_type |= DrapeauxTypes::TYPE_NE_REQUIERS_PAS_D_INITIALISATION;
        return;
    }

    auto ancien_état_retour = m_fonction_courante_retourne_plusieurs_valeurs;
    m_fonction_courante_retourne_plusieurs_valeurs = false;

    NoeudBloc *bloc;
    if (decl_struct->est_corps_texte) {
        bloc = analyse_bloc();
    }
    else {
        bloc = analyse_bloc_membres_structure_ou_union(decl_struct);
    }

    bloc->appartiens_à_type = decl_struct;

    m_fonction_courante_retourne_plusieurs_valeurs = ancien_état_retour;

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

NoeudBloc *Syntaxeuse::analyse_bloc_membres_structure_ou_union(NoeudDeclarationClasse *decl_struct)
{
    auto bloc = m_tacheronne.assembleuse->empile_bloc(lexème_courant(), nullptr);
    consomme(GenreLexème::ACCOLADE_OUVRANTE, "Attendu '{' après le nom de la structure");

    auto expressions = kuri::tablet<NoeudExpression *, 16>();

    while (!fini() && !apparie(GenreLexème::ACCOLADE_FERMANTE)) {
        if (ignore_point_virgule_implicite()) {
            continue;
        }

        if (!apparie_expression()) {
            rapporte_erreur("attendu une expression");
            continue;
        }

        auto noeud = analyse_expression({}, GenreLexème::INCONNU, GenreLexème::INCONNU);

        if (!expression_est_valide_pour_bloc_structure(noeud)) {
            rapporte_erreur_avec_site(noeud, "Expression invalide pour le bloc de la structure");
        }

        if (noeud->est_reference_declaration()) {
            if (decl_struct->est_type_union() && decl_struct->comme_type_union()->est_nonsure) {
                rapporte_erreur_avec_site(
                    noeud, "Seules les unions sûres peuvent avoir des déclarations sans type");
            }

            auto decl_membre = m_tacheronne.assembleuse->crée_declaration_variable(
                noeud->comme_reference_declaration());
            m_tacheronne.assembleuse->recycle_référence(noeud->comme_reference_declaration());
            noeud = decl_membre;

            static const Lexème lexème_rien = {"rien", {}, GenreLexème::RIEN, 0, 0, 0};
            auto type_declare = m_tacheronne.assembleuse->crée_reference_type(&lexème_rien);
            decl_membre->expression_type = type_declare;
        }

        if (noeud->est_declaration_variable()) {
            noeud->drapeaux |= DrapeauxNoeud::EST_MEMBRE_STRUCTURE;
        }

        expressions.ajoute(noeud);
    }

    copie_tablet_tableau(expressions, *bloc->expressions.verrou_ecriture());

    consomme(GenreLexème::ACCOLADE_FERMANTE,
             "Attendu '}' à la fin de la déclaration de la structure");

    m_tacheronne.assembleuse->dépile_bloc();

    return bloc;
}

/** \} */

void Syntaxeuse::gère_erreur_rapportée(const kuri::chaine &message_erreur)
{
    m_unité->espace->rapporte_erreur(
        SiteSource::cree(m_fichier, lexème_courant()), message_erreur, erreur::Genre::SYNTAXAGE);
    /* Avance le curseur pour ne pas être bloqué. */
    consomme();
}

void Syntaxeuse::rapporte_erreur_avec_site(const NoeudExpression *site,
                                           kuri::chaine_statique message)
{
    if (possède_erreur()) {
        return;
    }

    m_unité->espace->rapporte_erreur(site, message, erreur::Genre::SYNTAXAGE);
    m_possède_erreur = true;
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
    if (apparie(GenreLexème::POINT_VIRGULE)) {
        consomme();
        return true;
    }

    return false;
}

void Syntaxeuse::analyse_directive_déclaration_variable(NoeudDeclarationVariable *déclaration)
{
    if (!apparie(GenreLexème::DIRECTIVE)) {
        return;
    }

    consomme();

    if (!fonctions_courantes.est_vide()) {
        rapporte_erreur("Utilisation d'une directive sur une variable non-globale.");
        return;
    }

    auto lexème_directive = lexème_courant();
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
    if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
        rapporte_erreur("attendu une chaine de caractère après #externe");
    }

    déclaration_symbole->données_externes =
        m_tacheronne.allocatrice_noeud.crée_données_symbole_externe();
    auto données_externes = déclaration_symbole->données_externes;
    données_externes->ident_bibliotheque = lexème_courant()->ident;
    consomme();

    if (apparie(GenreLexème::CHAINE_LITTERALE)) {
        données_externes->nom_symbole = lexème_courant()->chaine;
        consomme();
    }
    else {
        données_externes->nom_symbole = déclaration_symbole->ident->nom;
    }
}
