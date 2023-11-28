/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include <iosfwd>

#include "compilation/transformation_type.hh"

#include "structures/chaine_statique.hh"
#include "structures/tableau_compresse.hh"

#include "utilitaires/macros.hh"

struct AssembleuseArbre;
struct Compilatrice;
struct EspaceDeTravail;
struct IdentifiantCode;
struct Lexeme;
struct NoeudBloc;
struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationVariable;
struct NoeudExpression;
struct NoeudExpressionReference;
struct NoeudExpressionUnaire;
struct Symbole;
struct Type;
struct TypePointeur;
struct Typeuse;

namespace kuri {
struct chaine;
template <typename T, uint64_t>
struct tablet;
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
    EST_MARQUÉE_INUTILISÉE = (1 << 18),         // decl var
    METAPROGRAMME_CORPS_TEXTE_FUT_CREE = (1 << 19),
    NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE = (1 << 20),
    DÉPENDANCES_FURENT_RÉSOLUES = (1 << 21),
    IDENTIFIANT_EST_ACCENTUÉ_GRAVE = (1u << 22),
    /* Certaines assertions dans le code se base sur les lexèmes des littérales, mais la
     * canonicalisation peut réutiliser les lexèmes des sites sources faisant échouer les précitées
     * assertions. */
    LEXÈME_EST_RÉUTILISÉ_POUR_SUBSTITUTION = (1u << 23),

    /* Drapeaux pour définir où se trouve le noeud dans l'arbre syntaxique. */

    /* Le noeud est à droite de '=' ou ':='. */
    DROITE_ASSIGNATION = (1u << 24),
    /* Le noeud est utilisé comme condition pour une boucle ou si/saufsi. */
    DROITE_CONDITION = (1u << 25),
    /* Le noeud est utilisé comme expression d'appel (p.e. noeud(...)). */
    GAUCHE_EXPRESSION_APPEL = (1 << 26),
    /* Le noeud est une expression de bloc d'une instruction si. */
    EXPRESSION_BLOC_SI = (1u << 27),
    /* Le noeud est une expression de test d'une discrimination (NoeudPaireDiscr.expression). */
    EXPRESSION_TEST_DISCRIMINATION = (1u << 28),

    EST_LOCALE = (1u << 29),  // decl var
    /* La déclaration est celle d'une variable déclarée dans une expression virgule
     * (p.e. a, b := ...). */
    EST_DÉCLARATION_EXPRESSION_VIRGULE = (1u << 30),  // decl var
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
    /* La fontion fut marquée avec #cliche ri_finale */
    CLICHÉ_RI_FINALE_FUT_REQUIS = (1 << 18),
    /* La fontion fut marquée avec #cliche inst_mv */
    CLICHÉ_CODE_BINAIRE_FUT_REQUIS = (1 << 19),

    /* Ne copions pas certains bits. */
    BITS_COPIABLES = ~(EST_POLYMORPHIQUE | EST_VARIADIQUE | EST_MONOMORPHISATION |
                       EST_INITIALISATION_TYPE | EST_INTRINSÈQUE | EST_MÉTAPROGRAMME),
};
DEFINIS_OPERATEURS_DRAPEAU(DrapeauxNoeudFonction)

std::ostream &operator<<(std::ostream &os, DrapeauxNoeudFonction const drapeaux);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Visibilité symbole.
 * Définis la visibilité des symboles dans les bibliothèques partagées.
 * \{ */

enum class VisibilitéSymbole : uint8_t {
    /* Le symbole ne sera pas visible. */
    INTERNE,
    /* Le symbole sera visible. */
    EXPORTÉ,
};

std::ostream &operator<<(std::ostream &os, VisibilitéSymbole visibilité);

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

    /* Référence membre. */
    PEUT_ÊTRE_APPEL_UNIFORME,
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

/* ------------------------------------------------------------------------- */
/** \name Données symboles externes.
 * \{ */

/* Pour les fonctions et globales externes :
 * - nom du symbole dans la bibliothèque
 * - la bibliothèque où se trouve le Symbole
 * - le symbole lui-même */
struct DonnéesSymboleExterne {
    kuri::chaine_statique nom_symbole = "";
    IdentifiantCode *ident_bibliotheque = nullptr;
    Symbole *symbole = nullptr;
};

/** \} */

void aplatis_arbre(NoeudExpression *declaration);

NoeudExpressionUnaire *crée_prise_adresse(AssembleuseArbre *assem,
                                          Lexeme const *lexème,
                                          NoeudExpression *expression,
                                          TypePointeur *type_résultat);

NoeudDeclarationVariable *crée_retour_défaut_fonction(AssembleuseArbre *assembleuse,
                                                      Lexeme const *lexème);

void imprime_détails_fonction(EspaceDeTravail *espace,
                              NoeudDeclarationEnteteFonction const *entête,
                              std::ostream &os);

/**
 * Traverse l'expression donnée et retourne la première sous-expression non-constante de celle-ci.
 * Retourne nul si toutes les sous-expressions sont constantes.
 */
NoeudExpression const *trouve_expression_non_constante(NoeudExpression const *expression);

/**
 * Traverse l'expression donnée et retourne vrai si l'expression et ses sous-expression sont
 * toutes des expressions constantes utilisables pour générer une initialisation constante
 * dans la RI.
 */
bool peut_être_utilisée_pour_initialisation_constante_globale(NoeudExpression const *expression);

/* Retourne un texte lisible pour le nom du noeud. Par exemple, si le noeud est la fonction
 * d'initialisation du type z32, retourne "init_de(z32)". */
kuri::chaine nom_humainement_lisible(NoeudExpression const *noeud);

/**
 * Utilisé pour déterminer le type effectivement accédé dans une expression de référence de membre.
 * Ceci supprime les pointeurs et références, ainsi que les opacifications.
 */
Type *donne_type_accédé_effectif(Type *type_accédé);

/**
 * Retourne les noms des blocs constituant la hiérarchie de blocs de ce bloc. Les noms incluent
 * celui du bloc passé en paramètre, et sont ordonnés du plus « jeune » ou plus « vieux » (c-à-d,
 * le nom du module, le bloc le plus vieux, sera le dernier).
 */
kuri::tablet<IdentifiantCode *, 6> donne_les_noms_de_la_hiérarchie(NoeudBloc *bloc);

NoeudDeclarationEnteteFonction *crée_entête_pour_initialisation_type(Type *type,
                                                                     Compilatrice &compilatrice,
                                                                     AssembleuseArbre *assembleuse,
                                                                     Typeuse &typeuse);

void crée_noeud_initialisation_type(EspaceDeTravail *espace,
                                    Type *type,
                                    AssembleuseArbre *assembleuse);

bool possède_annotation(NoeudDeclarationVariable const *decl, kuri::chaine_statique annotation);

bool est_déclaration_polymorphique(NoeudDeclaration const *decl);

void imprime_membres_blocs_récursifs(NoeudBloc const *bloc);

UniteCompilation **donne_adresse_unité(NoeudExpression *noeud);
