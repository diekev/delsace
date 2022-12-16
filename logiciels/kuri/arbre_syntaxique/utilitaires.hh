/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "biblinternes/outils/definitions.h"

#include "compilation/transformation_type.hh"

#include "structures/chaine_statique.hh"
#include "structures/tableau_compresse.hh"

struct AssembleuseArbre;
struct EspaceDeTravail;
struct NoeudBloc;
struct NoeudDeclarationVariable;
struct NoeudExpression;
struct Typeuse;

namespace kuri {
struct chaine;
}

enum DrapeauxNoeud : unsigned int {
    AUCUN = 0,
    EMPLOYE = (1 << 0),                          // decl var
    EST_EXTERNE = (1 << 1),                      // decl var, decl fonction
    FORCE_ENLIGNE = (1 << 2),                    // decl fonction
    FORCE_HORSLIGNE = (1 << 3),                  // decl fonction
    EST_MEMBRE_STRUCTURE = (1 << 4),             // decl structure, decl union
    FORCE_SANSTRACE = (1 << 5),                  // decl fonction
    EST_ASSIGNATION_COMPOSEE = (1 << 6),         // operateur binaire
    EST_VARIADIQUE = (1 << 7),                   // decl var
    EST_IMPLICITE = (1 << 8),                    // controle boucle
    EST_GLOBALE = (1 << 9),                      // decl var
    EST_CONSTANTE = (1 << 10),                   // decl var
    DECLARATION_TYPE_POLYMORPHIQUE = (1 << 11),  // decl var
    DROITE_ASSIGNATION = (1 << 12),              // générique
    DECLARATION_FUT_VALIDEE = (1 << 13),         // déclaration
    RI_FUT_GENEREE = (1 << 14),                  // déclaration
    CODE_BINAIRE_FUT_GENERE = (1 << 15),         // déclaration
    COMPILATRICE = (1 << 16),                    // decl fonction
    FORCE_SANSBROYAGE = (1 << 17),               // decl fonction
    EST_RACINE = (1 << 18),                      // decl fonction
    TRANSTYPAGE_IMPLICITE = (1 << 19),           // expr comme
    EST_PARAMETRE = (1 << 20),                   // decl var
    EST_VALEUR_POLYMORPHIQUE = (1 << 21),        // decl var
    POUR_CUISSON = (1 << 22),                    // appel
    ACCES_EST_ENUM_DRAPEAU = (1 << 23),          // accès membre
    DROITE_CONDITION = (1 << 24),
    EST_UTILISEE = (1 << 25),  // decl var
    DEBOGUE = (1 << 26),
    METAPROGRAMME_CORPS_TEXTE_FUT_CREE = (1 << 27),
    GAUCHE_EXPRESSION_APPEL = (1 << 28),
    NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE = (1 << 29),
    DÉPENDANCES_FURENT_RÉSOLUES = (1 << 30),
};

DEFINIE_OPERATEURS_DRAPEAU(DrapeauxNoeud, unsigned int)

enum {
    /* instruction 'pour' */
    GENERE_BOUCLE_PLAGE,
    GENERE_BOUCLE_PLAGE_INDEX,
    GENERE_BOUCLE_PLAGE_IMPLICITE,
    GENERE_BOUCLE_PLAGE_IMPLICITE_INDEX,
    GENERE_BOUCLE_TABLEAU,
    GENERE_BOUCLE_TABLEAU_INDEX,
    GENERE_BOUCLE_COROUTINE,
    GENERE_BOUCLE_COROUTINE_INDEX,

    CONSTRUIT_OPAQUE,
    MONOMORPHE_TYPE_OPAQUE,

    /* pour ne pas avoir à générer des conditions de vérification pour par
     * exemple les accès à des membres d'unions */
    IGNORE_VERIFICATION,

    /* instruction 'retourne' */
    REQUIERS_CODE_EXTRA_RETOUR,
    RETOURNE_UNE_UNION_VIA_RIEN,
    REQUIERS_RETOUR_UNION_VIA_RIEN,

    /* expression construction structure */
    CONSTRUIT_UNION_DEPUIS_MEMBRE_TYPE_RIEN,
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

DEFINIE_OPERATEURS_DRAPEAU(GenreValeur, char)

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

void simplifie_arbre(EspaceDeTravail *espace,
                     AssembleuseArbre *assem,
                     Typeuse &typeuse,
                     NoeudExpression *arbre);

void aplatis_arbre(NoeudExpression *declaration);

void imprime_details_fonction(EspaceDeTravail *espace,
                              NoeudDeclarationEnteteFonction const *entete,
                              std::ostream &os);

/* Retourne un texte lisible pour le nom du noeud. Par exemple, si le noeud est la fonction
 * d'initialisation du type z32, retourne "init_de(z32)". */
kuri::chaine nom_humainement_lisible(NoeudExpression const *noeud);

void cree_noeud_initialisation_type(EspaceDeTravail *espace,
                                    Type *type,
                                    AssembleuseArbre *assembleuse);

NoeudExpressionReference *reference_declaration_acces_membre(NoeudExpression *expr);

bool possede_annotation(NoeudDeclarationVariable const *decl, kuri::chaine_statique annotation);
