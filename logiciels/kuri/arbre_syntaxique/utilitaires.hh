/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include <iosfwd>

#include "biblinternes/outils/definitions.h"

#include "compilation/transformation_type.hh"

#include "structures/tableau_compresse.hh"

struct AssembleuseArbre;
struct Compilatrice;
struct EspaceDeTravail;
struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationVariable;
struct NoeudExpression;
struct NoeudExpressionReference;
struct Type;
struct Typeuse;

namespace kuri {
struct chaine;
struct chaine_statique;
}  // namespace kuri

/* ------------------------------------------------------------------------- */
/** \name Drapeaux génériques pour les noeuds.
 * \{ */

enum class DrapeauxNoeud : uint32_t {
    AUCUN = 0,
    EMPLOYE = (1 << 0),                         // decl var
    EST_EXTERNE = (1 << 1),                     // decl var
    EST_MEMBRE_STRUCTURE = (1 << 2),            // decl structure, decl union
    EST_ASSIGNATION_COMPOSEE = (1 << 3),        // operateur binaire
    EST_VARIADIQUE = (1 << 4),                  // decl var
    EST_IMPLICITE = (1 << 5),                   // controle boucle
    EST_GLOBALE = (1 << 6),                     // decl var
    EST_CONSTANTE = (1 << 7),                   // decl var
    DECLARATION_TYPE_POLYMORPHIQUE = (1 << 8),  // decl var
    DECLARATION_FUT_VALIDEE = (1 << 9),         // déclaration
    RI_FUT_GENEREE = (1 << 10),                 // déclaration
    CODE_BINAIRE_FUT_GENERE = (1 << 11),        // déclaration
    TRANSTYPAGE_IMPLICITE = (1 << 12),          // expr comme
    EST_PARAMETRE = (1 << 13),                  // decl var
    EST_VALEUR_POLYMORPHIQUE = (1 << 14),       // decl var
    POUR_CUISSON = (1 << 15),                   // appel
    ACCES_EST_ENUM_DRAPEAU = (1 << 16),         // accès membre
    EST_UTILISEE = (1 << 17),                   // decl var
    METAPROGRAMME_CORPS_TEXTE_FUT_CREE = (1 << 18),
    NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE = (1 << 19),
    DÉPENDANCES_FURENT_RÉSOLUES = (1 << 20),
    IDENTIFIANT_EST_ACCENTUÉ_GRAVE = (1u << 21),
    /* Certaines assertions dans le code se base sur les lexèmes des littérales, mais la
     * canonicalisation peut réutiliser les lexèmes des sites sources faisant échouer les précitées
     * assertions. */
    LEXÈME_EST_RÉUTILISÉ_POUR_SUBSTITUTION = (1u << 22),

    /* Drapeaux pour définir où se trouve le noeud dans l'arbre syntaxique. */

    /* Le noeud est à droite de '=' ou ':='. */
    DROITE_ASSIGNATION = (1u << 23),
    /* Le noeud est utilisé comme condition pour une boucle ou si/saufsi. */
    DROITE_CONDITION = (1u << 24),
    /* Le noeud est utilisé comme expression d'appel (p.e. noeud(...)). */
    GAUCHE_EXPRESSION_APPEL = (1 << 25),
    /* Le noeud est une expression de bloc d'une instruction si. */
    EXPRESSION_BLOC_SI = (1u << 26),
    /* Le noeud est une expression de test d'une discrimination (NoeudPaireDiscr.expression). */
    EXPRESSION_TEST_DISCRIMINATION = (1u << 27),

    EST_LOCALE = (1u << 28),  // decl var
    /* La déclaration est celle d'une variable déclarée dans une expression virgule
     * (p.e. a, b := ...). */
    EST_DÉCLARATION_EXPRESSION_VIRGULE = (1u << 29),  // decl var
};

DEFINIS_OPERATEURS_DRAPEAU(DrapeauxNoeud)

std::ostream &operator<<(std::ostream &os, DrapeauxNoeud const drapeaux);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Drapeaux pour les fonctions.
 * \{ */

enum class DrapeauxNoeudFonction : uint32_t {
    AUCUN = 0,
    /* DISPONIBLE = (1 << 0), */

    FORCE_ENLIGNE = (1 << 1),
    FORCE_HORSLIGNE = (1 << 2),
    FORCE_SANSTRACE = (1 << 3),
    FORCE_SANSBROYAGE = (1 << 4),

    EST_EXTERNE = (1 << 5),
    EST_IPA_COMPILATRICE = (1 << 6),
    EST_RACINE = (1 << 7),
    EST_INTRINSÈQUE = (1 << 8),
    EST_INITIALISATION_TYPE = (1 << 9),
    EST_MÉTAPROGRAMME = (1 << 10),
    EST_VARIADIQUE = (1 << 11),
    EST_POLYMORPHIQUE = (1 << 12),
    EST_MONOMORPHISATION = (1 << 13),

    FUT_GÉNÉRÉE_PAR_LA_COMPILATRICE = (1 << 14),

    /* La fontion fut marquée avec #cliche asa */
    CLICHÉ_ASA_FUT_REQUIS = (1 << 15),
    /* La fontion fut marquée avec #cliche asa_canon */
    CLICHÉ_ASA_CANONIQUE_FUT_REQUIS = (1 << 16),
    /* La fontion fut marquée avec #cliche ri */
    CLICHÉ_RI_FUT_REQUIS = (1 << 17),
    /* La fontion fut marquée avec #cliche inst_mv */
    CLICHÉ_CODE_BINAIRE_FUT_REQUIS = (1 << 18),

    /* Ne copions pas certains bits. */
    BITS_COPIABLES = ~(EST_POLYMORPHIQUE | EST_VARIADIQUE | EST_MONOMORPHISATION |
                       EST_INITIALISATION_TYPE | EST_INTRINSÈQUE | EST_MÉTAPROGRAMME),
};
DEFINIS_OPERATEURS_DRAPEAU(DrapeauxNoeudFonction)

std::ostream &operator<<(std::ostream &os, DrapeauxNoeudFonction const drapeaux);

/** \} */

enum {
    /* instruction 'pour' */
    GENERE_BOUCLE_PLAGE,
    GENERE_BOUCLE_PLAGE_IMPLICITE,
    GENERE_BOUCLE_TABLEAU,
    GENERE_BOUCLE_COROUTINE,
    BOUCLE_POUR_OPÉRATEUR,

    CONSTRUIT_OPAQUE,
    MONOMORPHE_TYPE_OPAQUE,
    CONSTRUIT_OPAQUE_DEPUIS_STRUCTURE,

    /* pour ne pas avoir à générer des conditions de vérification pour par
     * exemple les accès à des membres d'unions */
    IGNORE_VERIFICATION,

    /* instruction 'retourne' */
    REQUIERS_CODE_EXTRA_RETOUR,
    RETOURNE_UNE_UNION_VIA_RIEN,
    REQUIERS_RETOUR_UNION_VIA_RIEN,
};

/* Le genre d'une valeur, gauche, droite, ou transcendantale.
 *
 * Une valeur gauche est une valeur qui peut être assignée, donc à
 * gauche de '=', et comprend :
 * - les variables et accès de membres de structures
 * - les déréférencements (via mémoire(...))
 * - les opérateurs []
 *
 * Une valeur droite est une valeur qui peut être utilisée dans une
 * assignation, donc à droite de '=', et comprend :
 * - les valeurs littéralles (0, 1.5, "chaine", 'a', vrai)
 * - les énumérations
 * - les variables et accès de membres de structures
 * - les pointeurs de fonctions
 * - les déréférencements (via mémoire(...))
 * - les opérateurs []
 * - les transtypages
 * - les prises d'addresses (via *...)
 *
 * Une valeur transcendantale est une valeur droite qui peut aussi être
 * une valeur gauche (l'intersection des deux ensembles).
 */
enum GenreValeur : char {
    INVALIDE = 0,
    GAUCHE = (1 << 1),
    DROITE = (1 << 2),
    TRANSCENDANTALE = GAUCHE | DROITE,
};

DEFINIS_OPERATEURS_DRAPEAU(GenreValeur)

inline bool est_valeur_gauche(GenreValeur type_valeur)
{
    return (type_valeur & GenreValeur::GAUCHE) != GenreValeur::INVALIDE;
}

inline bool est_valeur_droite(GenreValeur type_valeur)
{
    return (type_valeur & GenreValeur::DROITE) != GenreValeur::INVALIDE;
}

struct DonneesAssignations {
    NoeudExpression *expression = nullptr;
    bool multiple_retour = false;
    kuri::tableau_compresse<NoeudExpression *, int> variables{};
    kuri::tableau_compresse<TransformationType, int> transformations{};

    void efface()
    {
        expression = nullptr;
        multiple_retour = false;
        variables.efface();
        transformations.efface();
    }

    bool operator==(DonneesAssignations const &autre) const
    {
        if (this == &autre) {
            return true;
        }

        return false;
    }
};

void aplatis_arbre(NoeudExpression *declaration);

void imprime_details_fonction(EspaceDeTravail *espace,
                              NoeudDeclarationEnteteFonction const *entete,
                              std::ostream &os);

/* Retourne un texte lisible pour le nom du noeud. Par exemple, si le noeud est la fonction
 * d'initialisation du type z32, retourne "init_de(z32)". */
kuri::chaine nom_humainement_lisible(NoeudExpression const *noeud);

NoeudDeclarationEnteteFonction *cree_entete_pour_initialisation_type(Type *type,
                                                                     Compilatrice &compilatrice,
                                                                     AssembleuseArbre *assembleuse,
                                                                     Typeuse &typeuse);

void cree_noeud_initialisation_type(EspaceDeTravail *espace,
                                    Type *type,
                                    AssembleuseArbre *assembleuse);

bool possede_annotation(NoeudDeclarationVariable const *decl, kuri::chaine_statique annotation);

bool est_déclaration_polymorphique(NoeudDeclaration const *decl);
