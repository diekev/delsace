/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include "monomorphisations.hh"

#include <optional>

#include "parsage/lexemes.hh"

#include "structures/chaine.hh"
#include "structures/chaine_statique.hh"
#include "structures/tablet.hh"

struct Attente;
struct EspaceDeTravail;
struct NoeudExpressionBinaire;
struct NoeudExpressionConstructionStructure;
struct NoeudExpressionExpansionVariadique;
struct NoeudExpressionReference;
struct NoeudExpressionTypeFonction;
struct NoeudExpressionTypeTableauDynamique;
struct NoeudExpressionTypeTableauFixe;
struct NoeudExpressionTypeTranche;
struct NoeudStruct;
struct Typeuse;

/* État de la résolution d'une contrainte. Une contrainte est le type ou la valeur d'une valeur
 * polymorphique.
 *
 * Pour une déclaration « $T: type_de_données », la contrainte s'appliquant est que la valeur
 * utilisée pour satisfaire T doit être un type.
 *
 * Pour une déclaration « $N: z32 », la contrainte s'appliquant est que la valeur utilisée pour
 * satisfaire N doit être un nombre relatif.
 */
enum class ÉtatRésolutionContrainte {
    /* La contrainte s'applique correctement. */
    Ok,
    /* Nous avons reçu un type pour une valeur, ou une valeur pour un type. */
    PasLaMêmeChose,
    /* Le type reçu n'est pas le bon. Par exemple « $N: z32 » recevant un r32. */
    PasLeMêmeType,
};

/* État de la résolution d'une valeur ou d'un type polymorphique, pour les expressions « a: $T ».
 */
enum class ÉtatRésolutionCompatibilité {
    /* Les types ou valeurs sont compatibles. */
    Ok,
    /* Les types ne sont pas compatibles. */
    PasLeMêmeType,
    /* Nous avons reçus des valeurs différentes. */
    PasLaMêmeValeur,
};

/** ---------------------------------------------------------------------------
 *  \name Données pour les erreurs.
 *
 *  Les différentes données pour pouvoir imprimer des messages d'erreur précis.
 *  \{ */

struct DonnéesErreurCompatibilité {
    ÉtatRésolutionCompatibilité résultat{};
    ItemMonomorphisation item_contrainte{};
    ItemMonomorphisation item_reçu{};
};

struct DonnéesErreurContrainte {
    ÉtatRésolutionContrainte résultat{};
    ItemMonomorphisation item_contrainte{};
    ItemMonomorphisation item_reçu{};
};

struct DonnéesErreurItemManquante {
    ItemMonomorphisation item_reçu{};
};

struct DonnéesErreurInterne {
    kuri::chaine message{};
};

struct DonnéesErreurGenreType {
    const Type *type_reçu{};
    kuri::chaine_statique message{};
};

/* Erreur interne : nous ne gérons pas un opérateur qui défini un type. */
struct DonnéesErreurOpérateurNonGéré {
    GenreLexeme lexeme{};
};

/* Erreur interne : nous n'avons pas découvert une référence à une déclaration polymorphique. */
struct DonnéesErreurRéférenceInconnue {
    const IdentifiantCode *ident{};
};

struct DonnéesErreurMonomorphisationManquante {
    kuri::tablet<ItemMonomorphisation, 6> items{};
    const Monomorphisations *monomorphisations{};
};

struct DonnéesErreurSémantique {
    kuri::chaine_statique message{};
};

using DonnéesErreur = std::variant<DonnéesErreurCompatibilité,
                                   DonnéesErreurContrainte,
                                   DonnéesErreurInterne,
                                   DonnéesErreurItemManquante,
                                   DonnéesErreurGenreType,
                                   DonnéesErreurOpérateurNonGéré,
                                   DonnéesErreurRéférenceInconnue,
                                   DonnéesErreurMonomorphisationManquante,
                                   DonnéesErreurSémantique>;

/** \} */

/** ---------------------------------------------------------------------------
 *  \name Erreur de monomorphisation.
 *  \{ */

struct ErreurMonomorphisation {
    /* Le polymorphe ayant été à l'origine de la monomorphisation. */
    const NoeudExpression *polymorphe = nullptr;
    /* Le site où l'erreur s'est produite. */
    const NoeudExpression *site = nullptr;
    DonnéesErreur donnees = {};

    kuri::chaine message() const;
};

/** \} */

/** ---------------------------------------------------------------------------
 *  \name Types de résultat.
 *  \{ */

/*  */
using RésultatUnification = std::variant<ErreurMonomorphisation, Attente, bool>;

using RésultatCompatibilité = std::variant<ÉtatRésolutionCompatibilité, Attente>;

using RésultatContrainte = std::variant<ÉtatRésolutionContrainte, Attente>;

using RésultatMonomorphisation = std::variant<ErreurMonomorphisation, Attente, bool>;

/* Représente un type et son poids d'appariement dérivant de la précision d'appariement du type
 * selon l'expression polymorphique dont le type est une monomorphisation. */
struct TypeAppariéPesé {
    Type *type = nullptr;
    double poids_appariement = 0.0;
};

using RésultatRésolutionType = std::variant<ErreurMonomorphisation, TypeAppariéPesé>;

/** \} */

/** ---------------------------------------------------------------------------
 *  \name Monomorpheuse.
 *
 * La monomorpheuse s'occupe de déterminer si la monomorphisation d'une déclaration polymorphique
 * est possible étant donné les arguments non polymorphiques passés dans une expression d'appel.
 *  \{ */

class Monomorpheuse {
  private:
    EspaceDeTravail &espace;

    using TypeTableauItem = kuri::tablet<ItemMonomorphisation, 6>;

    TypeTableauItem items{};
    TypeTableauItem candidats{};
    TypeTableauItem items_résultat{};

    const NoeudExpression *polymorphe = nullptr;

    std::optional<ErreurMonomorphisation> erreur_courante{};

    /* Profondeur, dans l'expression polymorphique, à laquelle le type final fut résolu. Voir
     * #résoud_type_final. Utilisé pour détermine le poids d'appariement du type. */
    int profondeur_appariement_type = 0;

  public:
    Monomorpheuse(EspaceDeTravail &ref_espace, const NoeudDeclarationEnteteFonction *entete);

    EMPECHE_COPIE(Monomorpheuse);

    /**
     * Traverse l'expression polymorphique et ajoute chaque identifiant préfixé de '$' à la liste
     * des items à résoudre.
     */
    void parse_candidats(const NoeudExpression *expression_polymorphique,
                         const NoeudExpression *site,
                         const Type *type_reçu);

    /**
     * Ajoute une item à résoudre pour la déclaration d'un type polymorphique. Par exemple : a: $T.
     */
    void ajoute_candidat(const IdentifiantCode *ident, const Type *type_reçu);

    /**
     * Ajoute une item à résoudre pour la déclaration d'une valeur polymorphique.
     * Par exemple : $T: type_de_données.
     */
    void ajoute_candidat_valeur(const IdentifiantCode *ident,
                                const Type *type,
                                const ValeurExpression valeur);

    /**
     * Évalue la valeur d'une expression constante.
     */
    ValeurExpression evalue_valeur(const NoeudExpression *expr);

    /**
     * Trouve pour tous les items à résoudre quel est le type commun ou la valeur commune pour le
     * type ou la valeur polymorphique qu'elles références.
     *
     * Si aucun type ou aucune valeur commune n'existe, une erreur est retournée.
     */
    RésultatUnification unifie();

    /**
     * Retourne les #ItemMonomorphisation pour pouvoir monomorpher la déclaration, si l'unification
     * a réussi.
     */
    const TypeTableauItem &résultat_pour_monomorphisation() const
    {
        return items_résultat;
    }

    /**
     * Retourne si une erreur de monomorphisation existe.
     */
    bool a_une_erreur() const
    {
        return erreur_courante.has_value();
    }

    /**
     * Retourne l'erreur de monomorphisation existante.
     */
    std::optional<ErreurMonomorphisation> erreur() const
    {
        return erreur_courante;
    }

    /**
     * Retourne parmis les candidats un item du nom de l'identifiant passé, ou nul si aucun
     * n'existe.
     */
    ItemMonomorphisation *item_pour_ident(IdentifiantCode const *ident);

    /**
     * Applique le résultat de la monomorphisation à l'expression polymorphique passée pour
     * produire un type non-polymorphique.
     *
     * Pour les déclaration de structures polymorphiques, un type est retourné uniquement si
     * une monomorphisation de la structure existe et étant satisfaite par les items résultat.
     *
     * Par exemple pour « a: MaStructure($T, $V) » si le résultat indique que T est z32, et V est
     * 5, retourne le type MaStructure(z32, 5) si nous avions déjà monomorphé MaStructure avec de
     * telles valeurs.
     *
     * Retourne soit une erreur si un type ne peut être résolu pour l'expression polymorphique
     * selon, ou un TypeAppariéPesé contenant le type et son poids de monomorphisation. Le poids
     * dépends de la précision de l'appariement. Par exemple, l'expression $T aura un poids
     * inférieur à [..]$T pour l'appariement d'un tableau.
     */
    RésultatRésolutionType résoud_type_final(const NoeudExpression *expression_polymorphique);

    /**
     * Imprime des informations l'état de la monomorpheuse.
     */
    void logue() const;

  private:
    /* Ajout de candidats. */

    void ajoute_candidat_depuis_reference_declaration(const NoeudExpressionReference *reference,
                                                      const Type *type_reçu);
    void ajoute_candidats_depuis_type_fonction(
        const NoeudExpressionTypeFonction *decl_type_fonction,
        const NoeudExpression *site,
        const Type *type_reçu);
    void ajoute_candidats_depuis_declaration_structure(const NoeudStruct *structure,
                                                       const Type *type_reçu);
    void ajoute_candidats_depuis_construction_structure(
        const NoeudExpressionConstructionStructure *construction,
        const NoeudExpression *site,
        const Type *type_reçu);
    void ajoute_candidats_depuis_construction_opaque(
        const NoeudExpressionConstructionStructure *construction,
        const NoeudExpression *site,
        const Type *type_reçu);
    void ajoute_candidats_depuis_declaration_tranche(
        const NoeudExpressionTypeTranche *expr_type_tranche,
        const NoeudExpression *site,
        const Type *type_reçu);
    void ajoute_candidats_depuis_declaration_tableau(
        const NoeudExpressionTypeTableauDynamique *expr_type_tableau,
        const NoeudExpression *site,
        const Type *type_reçu);
    void ajoute_candidats_depuis_declaration_tableau(
        const NoeudExpressionTypeTableauFixe *expr_type_tableau,
        const NoeudExpression *site,
        const Type *type_reçu);
    void ajoute_candidats_depuis_expansion_variadique(
        const NoeudExpressionExpansionVariadique *expansion,
        const NoeudExpression *site,
        const Type *type_reçu);

    /* Resolution de type final. */

    Type *résoud_type_final_impl(const NoeudExpression *expression_polymorphique);

    Type *résoud_type_final_pour_référence_déclaration(const NoeudExpressionReference *reference);
    Type *résoud_type_final_pour_type_fonction(
        const NoeudExpressionTypeFonction *decl_type_fonction);
    Type *résoud_type_final_pour_construction_structure(
        const NoeudExpressionConstructionStructure *construction);
    Type *résoud_type_final_pour_construction_opaque(
        const NoeudExpressionConstructionStructure *construction);
    Type *résoud_type_final_pour_déclaration_tranche(
        const NoeudExpressionTypeTranche *expr_tranche);
    Type *résoud_type_final_pour_déclaration_tableau_dynamique(
        const NoeudExpressionTypeTableauDynamique *expr_tableau_dynamique);
    Type *résoud_type_final_pour_déclaration_tableau_fixe(
        const NoeudExpressionTypeTableauFixe *expr_tableau_fixe);
    Type *résoud_type_final_pour_expansion_variadique(
        const NoeudExpressionExpansionVariadique *expansion);

    /* Erreurs. */

    void ajoute_erreur(const NoeudExpression *site, DonnéesErreur donnees);
    void erreur_interne(const NoeudExpression *site, kuri::chaine message);
    void erreur_contrainte(const NoeudExpression *site,
                           ÉtatRésolutionContrainte résultat,
                           ItemMonomorphisation item_contrainte,
                           ItemMonomorphisation item_reçu);
    void erreur_compatibilité(const NoeudExpression *site,
                              ÉtatRésolutionCompatibilité résultat,
                              ItemMonomorphisation item_contrainte,
                              ItemMonomorphisation item_reçu);
    void erreur_item_manquant(const NoeudExpression *site, ItemMonomorphisation item_reçu);
    void erreur_genre_type(const NoeudExpression *site,
                           const Type *type_reçu,
                           kuri::chaine_statique message);
    void erreur_opérateur_non_géré(const NoeudExpression *site, GenreLexeme lexeme);
    void erreur_référence_inconnue(const NoeudExpression *site);
    void erreur_monomorphisation_inconnue(const NoeudExpression *site,
                                          const kuri::tablet<ItemMonomorphisation, 6> &items,
                                          const Monomorphisations *monomorphisations);
    void erreur_sémantique(const NoeudExpression *site, kuri::chaine_statique message);

    /* Unification. */

    RésultatContrainte applique_contrainte(ItemMonomorphisation const &item,
                                           ItemMonomorphisation const &candidat);

    RésultatCompatibilité sont_compatibles(ItemMonomorphisation const &item,
                                           ItemMonomorphisation const &candidat);

    /* Autres. */

    Typeuse &typeuse();

    ItemMonomorphisation *item_résultat_pour_ident(IdentifiantCode const *ident);
};

/** \} */

/**
 * Détermine la monomorphisation possible de la fonction selon les arguments reçus lors de son
 * appel.
 *
 * Les arguments reçus doivent être ordonnés : les appels utilisant le nommage d'arguments pour
 * passer les arguments dans le désordre doivent avoir été trié avant la détermination. Sinon,
 * une erreur de typage sera émise.
 */
RésultatMonomorphisation détermine_monomorphisation(
    Monomorpheuse &monomorpheuse,
    const NoeudDeclarationEnteteFonction *entête,
    const kuri::tableau_statique<NoeudExpression *> &arguments_reçus);
