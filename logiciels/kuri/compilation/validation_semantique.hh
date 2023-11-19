/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include <variant>

#include "arbre_syntaxique/utilitaires.hh"

#include "structures/ensemble.hh"
#include "structures/file_fixe.hh"
#include "structures/tablet.hh"

#include "attente.hh"

struct Compilatrice;
struct EspaceDeTravail;
struct Lexeme;
struct MetaProgramme;
struct NoeudAssignation;
struct NoeudBloc;
struct NoeudDeclarationCorpsFonction;
struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationTypeOpaque;
struct NoeudDeclarationVariable;
struct NoeudDirectiveCuisine;
struct NoeudDirectiveDependanceBibliotheque;
struct NoeudDirectiveExecute;
struct NoeudDeclarationOperateurPour;
struct NoeudDeclarationSymbole;
struct NoeudDiscr;
struct NoeudEnum;
struct NoeudExpressionBinaire;
struct NoeudExpressionLitteraleBool;
struct NoeudExpressionMembre;
struct NoeudInstructionImporte;
struct NoeudPour;
struct NoeudRetour;
struct NoeudSi;
struct NoeudStruct;
struct Tacheronne;
struct TransformationType;
struct TypeCompose;
struct TypeEnum;
struct TypeTableauFixe;
struct UniteCompilation;

namespace erreur {
enum class Genre : int;
}

enum class CodeRetourValidation : int {
    OK,
    Erreur,
};

using ResultatValidation = std::variant<CodeRetourValidation, Attente>;

inline bool est_attente(ResultatValidation const &resultat)
{
    return std::holds_alternative<Attente>(resultat);
}

inline bool est_erreur(ResultatValidation const &resultat)
{
    return std::holds_alternative<CodeRetourValidation>(resultat) &&
           std::get<CodeRetourValidation>(resultat) == CodeRetourValidation::Erreur;
}

inline bool est_ok(ResultatValidation const &resultat)
{
    return std::holds_alternative<CodeRetourValidation>(resultat) &&
           std::get<CodeRetourValidation>(resultat) == CodeRetourValidation::OK;
}

/* Structure utilisée pour récupérer la mémoire entre plusieurs validations de déclaration,
 * mais également éviter de construire les différentes structures de données y utilisées;
 * ces constructions se voyant dans les profils d'exécution, notamment pour les
 * DonneesAssignations. */
struct ContexteValidationDeclaration {
    struct DeclarationEtReference {
        NoeudExpression *ref_decl = nullptr;
        NoeudDeclarationVariable *decl = nullptr;
    };

    /* Les variables déclarées, entre les virgules, si quelqu'une. */
    kuri::tablet<NoeudExpression *, 6> feuilles_variables{};

    /* Les noeuds de déclarations des variables et les références pointant vers ceux-ci. */
    kuri::tablet<DeclarationEtReference, 6> decls_et_refs{};

    /* Les expressions pour les initialisations, entre les virgules, si quelqu'une. */
    kuri::tablet<NoeudExpression *, 6> feuilles_expressions{};

    /* Les variables à assigner, chaque expression le nombre de variables nécessaires pour recevoir
     * le résultat de son évaluation. */
    kuri::file_fixe<NoeudExpression *, 6> variables{};

    /* Les données finales pour les assignations, faisant correspondre les expressions aux
     * variables. */
    kuri::tablet<DonneesAssignations, 6> donnees_assignations{};

    /* Données temporaires pour la constructions des donnees_assignations. */
    DonneesAssignations donnees_temp{};
};

struct ContexteValidationCode {
    Compilatrice &m_compilatrice;
    Tacheronne &m_tacheronne;

    UniteCompilation *unite = nullptr;
    EspaceDeTravail *espace = nullptr;

    double temps_chargement = 0.0;

    ContexteValidationCode(Compilatrice &compilatrice,
                           Tacheronne &tacheronne,
                           UniteCompilation &unite);

    EMPECHE_COPIE(ContexteValidationCode);

    ResultatValidation valide();

    ResultatValidation valide_semantique_noeud(NoeudExpression *);
    ResultatValidation valide_acces_membre(NoeudExpressionMembre *expression_membre);

    ResultatValidation valide_entete_fonction(NoeudDeclarationEnteteFonction *);
    ResultatValidation valide_entete_operateur(NoeudDeclarationEnteteFonction *);
    ResultatValidation valide_entete_operateur_pour(NoeudDeclarationOperateurPour *);
    void valide_parametres_constants_fonction(NoeudDeclarationEnteteFonction *);
    ResultatValidation valide_parametres_fonction(NoeudDeclarationEnteteFonction *);
    ResultatValidation valide_types_parametres_fonction(NoeudDeclarationEnteteFonction *);
    ResultatValidation valide_definition_unique_fonction(NoeudDeclarationEnteteFonction *);
    ResultatValidation valide_definition_unique_operateur(NoeudDeclarationEnteteFonction *);
    ResultatValidation valide_symbole_externe(NoeudDeclarationSymbole *);
    ResultatValidation valide_fonction(NoeudDeclarationCorpsFonction *);
    ResultatValidation valide_operateur(NoeudDeclarationCorpsFonction *);

    template <int N>
    ResultatValidation valide_enum_impl(NoeudEnum *decl, TypeEnum *type_enum);
    ResultatValidation valide_enum(NoeudEnum *);

    ResultatValidation valide_structure(NoeudStruct *);
    ResultatValidation valide_union(NoeudStruct *);
    ResultatValidation valide_declaration_variable(NoeudDeclarationVariable *decl);
    ResultatValidation valide_assignation(NoeudAssignation *inst);
    ResultatValidation valide_arbre_aplatis(NoeudExpression *declaration,
                                            kuri::tableau<NoeudExpression *, int> &arbre_aplatis);
    ResultatValidation valide_expression_retour(NoeudRetour *inst_retour);
    ResultatValidation valide_cuisine(NoeudDirectiveCuisine *directive);
    ResultatValidation valide_référence_déclaration(NoeudExpressionReference *expr,
                                                    NoeudBloc *bloc_recherche);
    ResultatValidation valide_type_opaque(NoeudDeclarationTypeOpaque *decl);

    template <typename TypeControleBoucle>
    CodeRetourValidation valide_controle_boucle(TypeControleBoucle *inst);

    ResultatValidation valide_operateur_binaire(NoeudExpressionBinaire *expr);
    ResultatValidation valide_operateur_binaire_chaine(NoeudExpressionBinaire *expr);
    ResultatValidation valide_operateur_binaire_tableau(NoeudExpressionBinaire *expr);
    ResultatValidation valide_operateur_binaire_type(NoeudExpressionBinaire *expr);
    ResultatValidation valide_operateur_binaire_generique(NoeudExpressionBinaire *expr);
    ResultatValidation valide_comparaison_enum_drapeau_bool(
        NoeudExpressionBinaire *expr,
        NoeudExpressionMembre *expr_acces_enum,
        NoeudExpressionLitteraleBool *expr_bool);

    ResultatValidation valide_discrimination(NoeudDiscr *inst);
    ResultatValidation valide_discr_énum(NoeudDiscr *inst, Type *type);
    ResultatValidation valide_discr_union(NoeudDiscr *inst, Type *type);
    ResultatValidation valide_discr_union_anonyme(NoeudDiscr *inst, Type *type);
    ResultatValidation valide_discr_scalaire(NoeudDiscr *inst, Type *type);

    CodeRetourValidation resoud_type_final(NoeudExpression *expression_type, Type *&type_final);

    void rapporte_erreur(const char *message, const NoeudExpression *noeud);
    void rapporte_erreur(const char *message, const NoeudExpression *noeud, erreur::Genre genre);
    void rapporte_erreur_redefinition_symbole(NoeudExpression *decl, NoeudDeclaration *decl_prec);
    void rapporte_erreur_redefinition_fonction(NoeudDeclarationEnteteFonction *decl,
                                               NoeudDeclaration *decl_prec);
    void rapporte_erreur_type_arguments(NoeudExpression *type_arg, NoeudExpression *type_enf);
    void rapporte_erreur_assignation_type_differents(const Type *type_gauche,
                                                     const Type *type_droite,
                                                     NoeudExpression *noeud);
    void rapporte_erreur_type_operation(const Type *type_gauche,
                                        const Type *type_droite,
                                        NoeudExpression *noeud);
    void rapporte_erreur_acces_hors_limites(NoeudExpression *b,
                                            TypeTableauFixe *type_tableau,
                                            int64_t index_acces);
    void rapporte_erreur_membre_inconnu(NoeudExpression *acces,
                                        NoeudExpression *membre,
                                        TypeCompose *type);
    void rapporte_erreur_valeur_manquante_discr(
        NoeudExpression *expression,
        const kuri::ensemble<kuri::chaine_statique> &valeurs_manquantes);
    void rapporte_erreur_fonction_nulctx(NoeudExpression const *appl_fonc,
                                         NoeudExpression const *decl_fonc,
                                         NoeudExpression const *decl_appel);

    enum class RaisonTranstypageImplicite {
        /* Nous essayons de trouver un transtypage implicite pour une expression de test d'une
         * discrimination. */
        POUR_TEST_DISCRIMINATION,
        /* Nous essayons de trouver un transtypage implicite pour la valeur de l'index d'une
         * expression d'indexage. */
        POUR_EXPRESSION_INDEXAGE,
        /* Nous essayons de trouver un transtypage implicite pour une valeur de la construction
         * d'un tableau. */
        POUR_CONSTRUCTION_TABLEAU,
    };

    ResultatValidation crée_transtypage_implicite_si_possible(
        NoeudExpression *&expression, Type *type_cible, RaisonTranstypageImplicite const raison);
    void crée_transtypage_implicite_au_besoin(NoeudExpression *&expression,
                                              TransformationType const &transformation);

    NoeudExpression *racine_validation() const;

    NoeudDeclarationEnteteFonction *fonction_courante() const;

    Type *union_ou_structure_courante() const;

    MetaProgramme *crée_metaprogramme_corps_texte(NoeudBloc *bloc_corps_texte,
                                                  NoeudBloc *bloc_parent,
                                                  const Lexeme *lexeme);

    MetaProgramme *crée_metaprogramme_pour_directive(NoeudDirectiveExecute *directive);

    ResultatValidation valide_instruction_pour(NoeudPour *inst);

    ResultatValidation valide_instruction_si(NoeudSi *inst);

    ResultatValidation valide_dépendance_bibliothèque(NoeudDirectiveDependanceBibliotheque *noeud);

    ResultatValidation valide_instruction_importe(NoeudInstructionImporte *inst);
};
