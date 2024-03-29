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

#include "analyseuse_grammaire.h"

#undef DEBOGUE_EXPRESSION

#include "biblinternes/langage/debogage.hh"
#include "biblinternes/structures/dico_fixe.hh"

#include "assembleuse_arbre.h"
#include "contexte_generation_code.h"
#include "expression.h"
#include "modules.hh"

/* ************************************************************************** */

static auto dico_params = dls::cree_dico(
    dls::paire(dls::vue_chaine("décimal"), lcc::type_var::DEC),
    dls::paire(dls::vue_chaine("entier"), lcc::type_var::ENT32),
    dls::paire(dls::vue_chaine("vecteur"), lcc::type_var::VEC3));

enum {
    PROP_PARAM_MAX,
    PROP_PARAM_MIN,
    PROP_PARAM_NOM,
    PROP_PARAM_VALEUR,
};

static auto dico_props_params = dls::cree_dico(
    dls::paire(dls::vue_chaine("max"), PROP_PARAM_MAX),
    dls::paire(dls::vue_chaine("min"), PROP_PARAM_MIN),
    dls::paire(dls::vue_chaine("nom"), PROP_PARAM_NOM),
    dls::paire(dls::vue_chaine("valeur"), PROP_PARAM_VALEUR));

/* ************************************************************************** */

/**
 * Pointeur spécial utilisé pour représenter un noeud de type paranthèse
 * ouvrante dans l'arbre syntactic. Ce noeud n'est pas inséré dans l'arbre,
 * mais simplement utilisé pour compiler les arbres syntactics des expressions.
 */
static auto NOEUD_PARENTHESE = reinterpret_cast<lcc::noeud::base *>(
    id_morceau::PARENTHESE_OUVRANTE);

#if 0
static bool est_specifiant_type(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::FOIS:
		case id_morceau::ESPERLUETTE:
		case id_morceau::CROCHET_OUVRANT:
			return true;
		default:
			return false;
	}
}

static bool est_identifiant_type(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::DEC:
		case id_morceau::BOOL:
		case id_morceau::ENT:
		case id_morceau::CHAINE_CARACTERE:
			return true;
		default:
			return false;
	}
}
#endif

static bool est_nombre_entier(id_morceau identifiant)
{
    switch (identifiant) {
        case id_morceau::NOMBRE_BINAIRE:
        case id_morceau::NOMBRE_ENTIER:
        case id_morceau::NOMBRE_HEXADECIMAL:
        case id_morceau::NOMBRE_OCTAL:
            return true;
        default:
            return false;
    }
}

static bool est_nombre(id_morceau identifiant)
{
    return est_nombre_entier(identifiant) || (identifiant == id_morceau::NOMBRE_REEL);
}

static bool est_operateur_unaire(id_morceau identifiant)
{
    switch (identifiant) {
        case id_morceau::AROBASE:
        case id_morceau::DOLLAR:
        case id_morceau::EXCLAMATION:
        case id_morceau::TILDE:
        case id_morceau::CROCHET_OUVRANT:
        case id_morceau::PLUS_UNAIRE:
        case id_morceau::MOINS_UNAIRE:
            return true;
        default:
            return false;
    }
}

static bool est_operateur_binaire(id_morceau identifiant)
{
    switch (identifiant) {
        case id_morceau::PLUS:
        case id_morceau::MOINS:
        case id_morceau::FOIS:
        case id_morceau::DIVISE:
        case id_morceau::ESPERLUETTE:
        case id_morceau::POURCENT:
        case id_morceau::INFERIEUR:
        case id_morceau::INFERIEUR_EGAL:
        case id_morceau::SUPERIEUR:
        case id_morceau::SUPERIEUR_EGAL:
        case id_morceau::DECALAGE_DROITE:
        case id_morceau::DECALAGE_GAUCHE:
        case id_morceau::DIFFERENCE:
        case id_morceau::ESP_ESP:
        case id_morceau::EGALITE:
        case id_morceau::BARRE_BARRE:
        case id_morceau::BARRE:
        case id_morceau::CHAPEAU:
        case id_morceau::POINT:
        case id_morceau::EGAL:
        case id_morceau::TROIS_POINTS:
        case id_morceau::VIRGULE:
        case id_morceau::PLUS_EGAL:
        case id_morceau::MOINS_EGAL:
        case id_morceau::DIVISE_EGAL:
        case id_morceau::FOIS_EGAL:
            return true;
        default:
            return false;
    }
}

#if 0
static bool est_operateur_constant(id_morceau identifiant)
{
	switch (identifiant) {
		case id_morceau::PLUS:
		case id_morceau::MOINS:
		case id_morceau::FOIS:
		case id_morceau::DIVISE:
		case id_morceau::ESPERLUETTE:
		case id_morceau::POURCENT:
		case id_morceau::INFERIEUR:
		case id_morceau::INFERIEUR_EGAL:
		case id_morceau::SUPERIEUR:
		case id_morceau::SUPERIEUR_EGAL:
		case id_morceau::DECALAGE_DROITE:
		case id_morceau::DECALAGE_GAUCHE:
		case id_morceau::DIFFERENCE:
		case id_morceau::ESP_ESP:
		case id_morceau::EGALITE:
		case id_morceau::BARRE_BARRE:
		case id_morceau::BARRE:
		case id_morceau::CHAPEAU:
		case id_morceau::EXCLAMATION:
		case id_morceau::TILDE:
		case id_morceau::PLUS_UNAIRE:
		case id_morceau::MOINS_UNAIRE:
			return true;
		default:
			return false;
	}
}
#endif

/**
 * Retourne vrai si l'identifiant passé en paramètre peut-être un identifiant
 * valide pour précèder un opérateur unaire '+' ou '-'.
 */
static bool precede_unaire_valide(id_morceau dernier_identifiant)
{
    if (dernier_identifiant == id_morceau::PARENTHESE_FERMANTE) {
        return false;
    }

    if (dernier_identifiant == id_morceau::CROCHET_FERMANT) {
        return false;
    }

    if (dernier_identifiant == id_morceau::CHAINE_CARACTERE) {
        return false;
    }

    if (dernier_identifiant == id_morceau::CARACTERE) {
        return false;
    }

    if (dernier_identifiant == id_morceau::DOLLAR) {
        return false;
    }

    if (dernier_identifiant == id_morceau::AROBASE) {
        return false;
    }

    if (est_nombre(dernier_identifiant)) {
        return false;
    }

    return true;
}

/* ************************************************************************** */

analyseuse_grammaire::analyseuse_grammaire(ContexteGenerationCode &contexte, DonneesModule *module)
    : lng::analyseuse<DonneesMorceaux>(module->morceaux), m_contexte(contexte),
      m_assembleuse(contexte.assembleuse), m_paires_vecteurs(PROFONDEUR_EXPRESSION_MAX),
      m_module(module)
{
}

void analyseuse_grammaire::lance_analyse(std::ostream &os)
{
    INUTILISE(os);

    m_position = 0;

    if (m_identifiants.taille() == 0) {
        return;
    }

    while (m_position != m_identifiants.taille()) {
        analyse_bloc();
    }
}

void analyseuse_grammaire::analyse_bloc()
{
    while (m_position != m_identifiants.taille()) {
        if (est_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
            break;
        }

        if (est_identifiant(id_morceau::SI)) {
            analyse_controle_si();
        }
        else if (est_identifiant(id_morceau::POUR)) {
            analyse_controle_pour();
        }
        else if (est_identifiant(id_morceau::ARRETE)) {
            avance();
            m_assembleuse->empile_noeud(
                lcc::noeud::type_noeud::ARRETE, m_contexte, m_identifiants[position()]);

            if (!requiers_identifiant(id_morceau::POINT_VIRGULE)) {
                lance_erreur("Attendu un point-virgule ';' après « arrête »");
            }

            m_assembleuse->depile_noeud(lcc::noeud::type_noeud::ARRETE);
        }
        else if (est_identifiant(id_morceau::CONTINUE)) {
            avance();
            m_assembleuse->empile_noeud(
                lcc::noeud::type_noeud::CONTINUE, m_contexte, m_identifiants[position()]);

            if (!requiers_identifiant(id_morceau::POINT_VIRGULE)) {
                lance_erreur("Attendu un point-virgule ';' après « continue »");
            }

            m_assembleuse->depile_noeud(lcc::noeud::type_noeud::CONTINUE);
        }
        else if (est_identifiant(id_morceau::RETOURNE)) {
            avance();
            m_assembleuse->empile_noeud(
                lcc::noeud::type_noeud::RETOURNE, m_contexte, m_identifiants[position()]);

            if (!requiers_identifiant(id_morceau::POINT_VIRGULE)) {
                lance_erreur("Attendu un point-virgule ';' après « retourne »");
            }

            m_assembleuse->depile_noeud(lcc::noeud::type_noeud::RETOURNE);
        }
        else if (est_identifiant(id_morceau::DIRECTIVE)) {
            avance();
            analyse_directive();
        }
        else {
            analyse_expression(id_morceau::POINT_VIRGULE, false);
        }
    }
}

void analyseuse_grammaire::analyse_controle_si()
{
    if (!requiers_identifiant(id_morceau::SI)) {
        lance_erreur("Attendu la déclaration 'si'");
    }

    m_assembleuse->empile_noeud(
        lcc::noeud::type_noeud::SI, m_contexte, m_identifiants[position()]);

    analyse_expression(id_morceau::ACCOLADE_OUVRANTE, false);

    m_assembleuse->empile_noeud(
        lcc::noeud::type_noeud::BLOC, m_contexte, m_identifiants[position()]);

    analyse_bloc();

    m_assembleuse->depile_noeud(lcc::noeud::type_noeud::BLOC);

    if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
        lance_erreur("Attendu une accolade fermante à la fin du contrôle 'si'");
    }

    if (est_identifiant(id_morceau::SINON)) {
        avance();

        /* Peu importe que le 'sinon' contient un 'si' ou non, nous ajoutons un
         * bloc pour créer un niveau d'indirection, afin d'éviter les problèmes
         * de branchages.
         * Originellement, cela vient du code de 'kuri'. */
        m_assembleuse->empile_noeud(
            lcc::noeud::type_noeud::BLOC, m_contexte, m_identifiants[position()]);

        if (est_identifiant(id_morceau::SI)) {
            analyse_controle_si();
        }
        else {
            if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
                lance_erreur("Attendu une accolade ouvrante après 'sinon'");
            }

            analyse_bloc();

            if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
                lance_erreur("Attendu une accolade fermante à la fin du contrôle 'sinon'");
            }
        }

        m_assembleuse->depile_noeud(lcc::noeud::type_noeud::BLOC);
    }

    m_assembleuse->depile_noeud(lcc::noeud::type_noeud::SI);
}

void analyseuse_grammaire::analyse_controle_pour()
{
    if (!requiers_identifiant(id_morceau::POUR)) {
        lance_erreur("Attendu la déclaration 'pour'");
    }

    m_assembleuse->empile_noeud(
        lcc::noeud::type_noeud::POUR, m_contexte, m_identifiants[position()]);

    if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
        lance_erreur("Attendu une chaîne de caractère après 'pour'");
    }

    /* enfant 1 : déclaration variable */

    auto noeud = m_assembleuse->cree_noeud(
        lcc::noeud::type_noeud::VARIABLE, m_contexte, m_identifiants[position()]);
    m_assembleuse->ajoute_noeud(noeud);

    if (!requiers_identifiant(id_morceau::DANS)) {
        lance_erreur("Attendu le mot 'dans' après la chaîne de caractère");
    }

    /* enfant 2 : expr */

    analyse_expression(id_morceau::ACCOLADE_OUVRANTE);

    /* enfant 3 : bloc */

    m_assembleuse->empile_noeud(
        lcc::noeud::type_noeud::BLOC, m_contexte, m_identifiants[position()]);

    analyse_bloc();

    m_assembleuse->depile_noeud(lcc::noeud::type_noeud::BLOC);

    if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
        lance_erreur("Attendu une accolade fermante '}' à la fin du contrôle 'pour'");
    }

    m_assembleuse->depile_noeud(lcc::noeud::type_noeud::POUR);
}

void analyseuse_grammaire::analyse_expression(id_morceau identifiant_final,
                                              bool const /*calcul_expression*/)
{
    /* Algorithme de Dijkstra pour générer une notation polonaise inversée. */
    auto profondeur = m_profondeur++;

    if (profondeur >= m_paires_vecteurs.taille()) {
        lance_erreur("Excès de la pile d'expression autorisée");
    }

    auto &expression = m_paires_vecteurs[profondeur].first;
    expression.efface();

    auto &pile = m_paires_vecteurs[profondeur].second;
    pile.efface();

    auto vide_pile_operateur = [&](id_morceau id_operateur) {
        while (!pile.est_vide() && pile.back() != NOEUD_PARENTHESE &&
               (precedence_faible(id_operateur, pile.back()->identifiant()))) {
            expression.ajoute(pile.back());
            pile.pop_back();
        }
    };

    /* Nous tenons compte du nombre de paranthèse pour pouvoir nous arrêter en
     * cas d'analyse d'une expression en dernier paramètre d'un appel de
     * fontion. */
    auto paren = 0;
    auto dernier_identifiant =
        m_identifiants[m_position == 0 ? m_position : position()].genre;  // XXX - À FAIRE

    /* utilisé pour terminer la boucle quand elle nous atteignons une parenthèse
     * fermante */
    auto termine_boucle = false;

    DEB_LOG_EXPRESSION << tabulations[profondeur] << "Vecteur :" << FIN_LOG_EXPRESSION;

    while (!requiers_identifiant(identifiant_final)) {
        auto &morceau = m_identifiants[position()];

        DEB_LOG_EXPRESSION << tabulations[profondeur] << '\t' << chaine_identifiant(morceau.genre)
                           << FIN_LOG_EXPRESSION;

        switch (morceau.genre) {
            case id_morceau::CHAINE_CARACTERE:
            {
                /* appel fonction : chaine + ( */
                if (est_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
                    avance();

                    auto noeud = m_assembleuse->empile_noeud(
                        lcc::noeud::type_noeud::FONCTION, m_contexte, morceau, false);

                    analyse_appel_fonction();

                    m_assembleuse->depile_noeud(lcc::noeud::type_noeud::FONCTION);

                    expression.ajoute(noeud);
                }
                /* variable : chaine */
                else {
                    auto noeud = m_assembleuse->cree_noeud(
                        lcc::noeud::type_noeud::VARIABLE, m_contexte, morceau);
                    expression.ajoute(noeud);
                }

                break;
            }
            case id_morceau::NOMBRE_REEL:
            {
                auto noeud = m_assembleuse->cree_noeud(
                    lcc::noeud::type_noeud::VALEUR, m_contexte, morceau);
                expression.ajoute(noeud);
                break;
            }
            case id_morceau::NOMBRE_BINAIRE:
            case id_morceau::NOMBRE_ENTIER:
            case id_morceau::NOMBRE_HEXADECIMAL:
            case id_morceau::NOMBRE_OCTAL:
            {
                auto noeud = m_assembleuse->cree_noeud(
                    lcc::noeud::type_noeud::VALEUR, m_contexte, morceau);
                expression.ajoute(noeud);
                break;
            }
            case id_morceau::VEC2:
            case id_morceau::VEC3:
            case id_morceau::VEC4:
            case id_morceau::MAT3:
            case id_morceau::MAT4:
            {
                auto noeud = m_assembleuse->empile_noeud(
                    lcc::noeud::type_noeud::VALEUR, m_contexte, morceau, false);

                if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
                    lance_erreur(
                        "Attendu une paranthèse ouvrante après la déclaration du constructeur");
                }

                analyse_expression(id_morceau::POINT_VIRGULE);

                if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
                    lance_erreur(
                        "Attendu une paranthèse fermante après la déclaration du constructeur");
                }

                m_assembleuse->depile_noeud(lcc::noeud::type_noeud::VALEUR);

                expression.ajoute(noeud);
                break;
            }
            case id_morceau::CHAINE_LITTERALE:
            {
                auto noeud = m_assembleuse->cree_noeud(
                    lcc::noeud::type_noeud::VALEUR, m_contexte, morceau);
                expression.ajoute(noeud);
                break;
            }
            case id_morceau::CARACTERE:
            {
                auto noeud = m_assembleuse->cree_noeud(
                    lcc::noeud::type_noeud::VALEUR, m_contexte, morceau);
                expression.ajoute(noeud);
                break;
            }
            case id_morceau::NUL:
            {
                auto noeud = m_assembleuse->cree_noeud(
                    lcc::noeud::type_noeud::VALEUR, m_contexte, morceau);
                expression.ajoute(noeud);
                break;
            }
            case id_morceau::VRAI:
            case id_morceau::FAUX:
            {
                /* remplace l'identifiant par id_morceau::BOOL */
                morceau.genre = id_morceau::BOOL;
                auto noeud = m_assembleuse->cree_noeud(
                    lcc::noeud::type_noeud::VALEUR, m_contexte, morceau);
                expression.ajoute(noeud);
                break;
            }
            case id_morceau::PARENTHESE_OUVRANTE:
            {
                ++paren;
                pile.ajoute(NOEUD_PARENTHESE);
                break;
            }
            case id_morceau::PARENTHESE_FERMANTE:
            {
                /* S'il n'y a pas de parenthèse ouvrante, c'est que nous avons
                 * atteint la fin d'une déclaration d'appel de fonction. */
                if (paren == 0) {
                    /* recule pour être synchroniser avec la sortie dans
                     * analyse_appel_fonction() */
                    recule();

                    termine_boucle = true;
                    break;
                }

                if (pile.est_vide()) {
                    lance_erreur("Il manque une paranthèse dans l'expression !");
                }

                while (pile.back() != NOEUD_PARENTHESE) {
                    expression.ajoute(pile.back());
                    pile.pop_back();
                }

                /* Enlève la parenthèse restante de la pile. */
                pile.pop_back();

                --paren;
                break;
            }
            /* opérations binaire */
            case id_morceau::PLUS:
            case id_morceau::MOINS:
            {
                auto id_operateur = morceau.genre;
                auto noeud = static_cast<lcc::noeud::base *>(nullptr);

                if (precede_unaire_valide(dernier_identifiant)) {
                    if (id_operateur == id_morceau::PLUS) {
                        id_operateur = id_morceau::PLUS_UNAIRE;
                    }
                    else if (id_operateur == id_morceau::MOINS) {
                        id_operateur = id_morceau::MOINS_UNAIRE;
                    }

                    morceau.genre = id_operateur;
                    noeud = m_assembleuse->cree_noeud(
                        lcc::noeud::type_noeud::OPERATION_UNAIRE, m_contexte, morceau);
                }
                else {
                    noeud = m_assembleuse->cree_noeud(
                        lcc::noeud::type_noeud::OPERATION_BINAIRE, m_contexte, morceau);
                }

                vide_pile_operateur(id_operateur);

                pile.ajoute(noeud);

                break;
            }
            case id_morceau::FOIS:
            case id_morceau::DIVISE:
            case id_morceau::ESPERLUETTE:
            case id_morceau::POURCENT:
            case id_morceau::INFERIEUR:
            case id_morceau::INFERIEUR_EGAL:
            case id_morceau::SUPERIEUR:
            case id_morceau::SUPERIEUR_EGAL:
            case id_morceau::DECALAGE_DROITE:
            case id_morceau::DECALAGE_GAUCHE:
            case id_morceau::DIFFERENCE:
            case id_morceau::ESP_ESP:
            case id_morceau::EGALITE:
            case id_morceau::BARRE_BARRE:
            case id_morceau::BARRE:
            case id_morceau::CHAPEAU:
            case id_morceau::VIRGULE:
            case id_morceau::PLUS_EGAL:
            case id_morceau::MOINS_EGAL:
            case id_morceau::DIVISE_EGAL:
            case id_morceau::FOIS_EGAL:
            {
                /* Correction de crash d'aléatest, improbable dans la vrai vie. */
                if (expression.est_vide() && est_operateur_binaire(morceau.genre)) {
                    lance_erreur("Opérateur binaire utilisé en début d'expression");
                }

                vide_pile_operateur(morceau.genre);
                auto noeud = m_assembleuse->cree_noeud(
                    lcc::noeud::type_noeud::OPERATION_BINAIRE, m_contexte, morceau);
                pile.ajoute(noeud);

                break;
            }
            case id_morceau::EGAL:
            {
                vide_pile_operateur(morceau.genre);

                auto noeud = m_assembleuse->cree_noeud(
                    lcc::noeud::type_noeud::ASSIGNATION, m_contexte, morceau);
                pile.ajoute(noeud);
                break;
            }
            case id_morceau::CROCHET_OUVRANT:
            {
                /* l'accès à un élément d'un tableau est chaine[index] */
                if (dernier_identifiant == id_morceau::CHAINE_CARACTERE) {
                    vide_pile_operateur(morceau.genre);

                    auto noeud = m_assembleuse->empile_noeud(
                        lcc::noeud::type_noeud::OPERATION_BINAIRE, m_contexte, morceau, false);
                    pile.ajoute(noeud);

                    analyse_expression(id_morceau::CROCHET_FERMANT);

                    m_assembleuse->depile_noeud(lcc::noeud::type_noeud::OPERATION_BINAIRE);
                }
                else {
                    morceau.genre = id_morceau::TABLEAU;
                    auto noeud = m_assembleuse->empile_noeud(
                        lcc::noeud::type_noeud::VALEUR, m_contexte, morceau, false);
                    pile.ajoute(noeud);

                    analyse_expression(id_morceau::CROCHET_FERMANT);

                    m_assembleuse->depile_noeud(lcc::noeud::type_noeud::VALEUR);
                }

                break;
            }
            /* opérations unaire */
            case id_morceau::EXCLAMATION:
            case id_morceau::TILDE:
            case id_morceau::PLUS_UNAIRE:
            case id_morceau::MOINS_UNAIRE:
            {
                vide_pile_operateur(morceau.genre);
                auto noeud = m_assembleuse->cree_noeud(
                    lcc::noeud::type_noeud::OPERATION_UNAIRE, m_contexte, morceau);
                pile.ajoute(noeud);
                break;
            }
            case id_morceau::DOLLAR:
            {
                vide_pile_operateur(morceau.genre);

                if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
                    lance_erreur("Attendu un nom après '$'");
                }

                auto noeud = m_assembleuse->cree_noeud(
                    lcc::noeud::type_noeud::PROPRIETE, m_contexte, m_identifiants[position()]);

                expression.ajoute(noeud);

                break;
            }
            case id_morceau::AROBASE:
            {
                vide_pile_operateur(morceau.genre);

                if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
                    lance_erreur("Attendu un nom après '@'");
                }

                auto noeud = m_assembleuse->cree_noeud(
                    lcc::noeud::type_noeud::ATTRIBUT, m_contexte, m_identifiants[position()]);

                expression.ajoute(noeud);

                break;
            }
            case id_morceau::POINT:
            {
                vide_pile_operateur(morceau.genre);
                auto noeud = m_assembleuse->cree_noeud(
                    lcc::noeud::type_noeud::ACCES_MEMBRE_POINT, m_contexte, morceau);
                pile.ajoute(noeud);
                break;
            }
            case id_morceau::TROIS_POINTS:
            {
                vide_pile_operateur(morceau.genre);
                auto noeud = m_assembleuse->cree_noeud(
                    lcc::noeud::type_noeud::PLAGE, m_contexte, morceau);
                pile.ajoute(noeud);
                break;
            }
            default:
            {
                lance_erreur("Identifiant inattendu dans l'expression");
            }
        }

        if (termine_boucle) {
            break;
        }

        dernier_identifiant = morceau.genre;
    }

    /* Retourne s'il n'y a rien dans l'expression, ceci est principalement pour
     * éviter de crasher lors des fuzz-tests. */
    if (expression.est_vide()) {
        --m_profondeur;
        return;
    }

    while (!pile.est_vide()) {
        if (pile.back() == NOEUD_PARENTHESE) {
            lance_erreur("Il manque une paranthèse dans l'expression !");
        }

        expression.ajoute(pile.back());
        pile.pop_back();
    }

    pile.reserve(expression.taille());

    DEB_LOG_EXPRESSION << tabulations[profondeur] << "Expression :" << FIN_LOG_EXPRESSION;

    for (lcc::noeud::base *noeud : expression) {
        DEB_LOG_EXPRESSION << tabulations[profondeur] << '\t'
                           << chaine_identifiant(noeud->identifiant()) << FIN_LOG_EXPRESSION;

        if (est_operateur_binaire(noeud->identifiant())) {
            if (pile.taille() < 2) {
                erreur::lance_erreur("Expression malformée",
                                     m_contexte,
                                     noeud->donnees_morceau(),
                                     erreur::type_erreur::NORMAL);
            }

            auto n2 = pile.back();
            pile.pop_back();

            auto n1 = pile.back();
            pile.pop_back();

            noeud->ajoute_noeud(n1);
            noeud->ajoute_noeud(n2);

            pile.ajoute(noeud);
        }
        else if (est_operateur_unaire(noeud->identifiant())) {
            auto n1 = pile.back();
            pile.pop_back();

            noeud->ajoute_noeud(n1);

            pile.ajoute(noeud);
        }
        else {
            pile.ajoute(noeud);
        }
    }

    m_assembleuse->ajoute_noeud(pile.back());
    pile.pop_back();

    if (pile.taille() != 0) {
        auto premier_noeud = pile.back();
        auto dernier_noeud = premier_noeud;
        pile.pop_back();

        auto pos_premier = premier_noeud->donnees_morceau().ligne_pos & 0xffffffff;
        auto pos_dernier = pos_premier;

        while (!pile.est_vide()) {
            auto n = pile.back();
            pile.pop_back();

            auto pos_n = n->donnees_morceau().ligne_pos & 0xffffffff;

            if (pos_n < pos_premier) {
                premier_noeud = n;
            }
            if (pos_n > pos_dernier) {
                dernier_noeud = n;
            }
        }

        erreur::lance_erreur_plage(
            "Expression malformée, il est possible qu'il manque un opérateur",
            m_contexte,
            premier_noeud->donnees_morceau(),
            dernier_noeud->donnees_morceau());
    }

    --m_profondeur;
}

void analyseuse_grammaire::analyse_appel_fonction()
{
    /* ici nous devons être au niveau du premier paramètre */

    while (true) {
        /* aucun paramètre, ou la liste de paramètre est vide */
        if (est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
            avance();
            return;
        }

        /* À FAIRE : le dernier paramètre s'arrête à une parenthèse fermante.
         * si identifiant final == ')', alors l'algorithme s'arrête quand une
         * paranthèse fermante est trouvé et que la pile est vide */
        analyse_expression(id_morceau::VIRGULE);
    }
}

void analyseuse_grammaire::analyse_directive()
{
    avance();

    if (donnees().chaine == "param") {
        avance();

        auto plg_dp = dico_params.trouve(donnees().chaine);

        if (plg_dp.est_finie()) {
            lance_erreur("Type paramètre inconnu");
        }

        auto type_param = plg_dp.front().second;

        analyse_parametre(type_param);
    }
    else {
        lance_erreur("Directive inconnue");
    }
}

void analyseuse_grammaire::analyse_parametre(lcc::type_var type_param)
{
    if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
        lance_erreur("Attendu une parenthèse ouvrante '(' après la déclaration du paramètre");
    }

    auto param = DonneesDeclarationParam();
    param.type = type_param;

    while (true) {
        /* aucun paramètre, ou la liste de paramètre est vide */
        if (est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
            avance();

            if (param.nom == "") {
                lance_erreur("Le paramètre n'a pas de nom !");
            }

            m_contexte.params_declare.ajoute(param);

            return;
        }

        avance();

        auto plg_prop = dico_props_params.trouve(donnees().chaine);

        if (plg_prop.est_finie()) {
            lance_erreur("Propriété paramètre inconnue");
        }

        if (!requiers_identifiant(id_morceau::EGAL)) {
            lance_erreur("Attendu '=' après le nom de la propriété du paramètre");
        }

        auto type_prop = plg_prop.front().second;

        switch (type_prop) {
            case PROP_PARAM_MAX:
            {
                avance();
                if (!est_nombre(donnees().genre)) {
                    lance_erreur("Attendu un nombre pour « max »");
                }

                break;
            }
            case PROP_PARAM_MIN:
            {
                avance();
                if (!est_nombre(donnees().genre)) {
                    lance_erreur("Attendu un nombre pour « min »");
                }

                break;
            }
            case PROP_PARAM_NOM:
            {
                if (!requiers_identifiant(id_morceau::CHAINE_LITTERALE)) {
                    lance_erreur("Attendu une chaine littérale pour « nom »");
                }

                param.nom = donnees().chaine;

                break;
            }
            case PROP_PARAM_VALEUR:
            {
                avance();

                if (type_param == lcc::type_var::VEC3) {
                    if (!est_identifiant(id_morceau::CHAINE_LITTERALE)) {
                        lance_erreur("Attendu une chaine littérale pour « valeur »");
                    }
                }
                else {
                    if (!est_nombre(donnees().genre)) {
                        lance_erreur("Attendu un nombre pour « valeur »");
                    }
                }

                break;
            }
        }
    }
}

void analyseuse_grammaire::lance_erreur(const dls::chaine &quoi, erreur::type_erreur type)
{
    erreur::lance_erreur(quoi, m_contexte, m_identifiants[position()], type);
}
