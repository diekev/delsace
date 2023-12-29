/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include <variant>

#include "arbre_syntaxique/utilitaires.hh"

#include "statistiques/statistiques.hh"

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
struct NoeudDeclarationConstante;
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
struct NoeudComme;
struct NoeudExpressionLitteraleBool;
struct NoeudExpressionLogique;
struct NoeudExpressionMembre;
struct NoeudInstructionImporte;
struct NoeudExpressionTypeFonction;
struct NoeudExpressionTypeTableauDynamique;
struct NoeudExpressionTypeTableauFixe;
struct NoeudExpressionTypeTranche;
struct NoeudPour;
struct NoeudRetour;
struct NoeudSi;
struct NoeudStruct;
struct NoeudUnion;
struct Tacheronne;
struct TransformationType;
struct UniteCompilation;

struct NoeudEnum;
struct NoeudDeclarationTypeCompose;
struct NoeudDeclarationType;
struct NoeudDeclarationTypeTableauFixe;
using Type = NoeudDeclarationType;
using TypeCompose = NoeudDeclarationTypeCompose;
using TypeEnum = NoeudEnum;
using TypeTableauFixe = NoeudDeclarationTypeTableauFixe;

namespace erreur {
enum class Genre : int;
}

enum class TypeSymbole : uint8_t;

enum class CodeRetourValidation : int {
    OK,
    Erreur,
};

using RésultatValidation = std::variant<CodeRetourValidation, Attente>;

inline bool est_attente(RésultatValidation const &résultat)
{
    return std::holds_alternative<Attente>(résultat);
}

inline bool est_erreur(RésultatValidation const &résultat)
{
    return std::holds_alternative<CodeRetourValidation>(résultat) &&
           std::get<CodeRetourValidation>(résultat) == CodeRetourValidation::Erreur;
}

inline bool est_ok(RésultatValidation const &résultat)
{
    return std::holds_alternative<CodeRetourValidation>(résultat) &&
           std::get<CodeRetourValidation>(résultat) == CodeRetourValidation::OK;
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

/* ------------------------------------------------------------------------- */
/** \name ArbreAplatis.
 *
 * Pour la validation sémantique, les arbres syntaxiques sont aplatis afin de
 * pouvoir valider les noeuds séquentiellement. Ainsi, lorsque nous devons
 * mettre en pause la validation, pour la reprendre ultérieurement, nous
 * pouvons simplement retourner d'une fonction, au lieu de se soucier de
 * dépiler toute la pile d'exécution. De même, la reprise de la validation peut
 * se faire en itérant sur l'arbre aplatis à partir de l'index de la dernière
 * itération connue.
 * \{ */

struct ArbreAplatis {
    kuri::tableau<NoeudExpression *, int> noeuds{};
    /* Index courant lors de la validation sémantique. Utilisé pour pouvoir reprendre la validation
     * en cas d'attente. */
    int index_courant = 0;

    void réinitialise()
    {
        noeuds.efface();
        index_courant = 0;
    }
};

/** \} */

struct Sémanticienne {
  private:
    Compilatrice &m_compilatrice;
    Tacheronne *m_tacheronne = nullptr;

    UniteCompilation *m_unité = nullptr;
    EspaceDeTravail *m_espace = nullptr;

    double m_temps_chargement = 0.0;

    StatistiquesTypage m_stats_typage{};

    ContexteValidationDeclaration m_contexte_validation_declaration{};

    /* Les arbres aplatis créés. Nous en créons au besoin pour les unités de
     * compilation quand elles entrent en validation, et récupérons la mémoire
     * lorsque la validation est terminée. */
    kuri::tableau<ArbreAplatis *> m_arbres_aplatis{};

    /* L'arbre pour l'unité courante. */
    ArbreAplatis *m_arbre_courant = nullptr;

  public:
    Sémanticienne(Compilatrice &compilatrice);

    EMPECHE_COPIE(Sémanticienne);

    ~Sémanticienne();

    void réinitialise();
    void définis_tacheronne(Tacheronne &tacheronne);

    RésultatValidation valide(UniteCompilation *unité);

    NoeudDeclarationEnteteFonction *fonction_courante() const;

    Type *union_ou_structure_courante() const;

    AssembleuseArbre *donne_assembleuse();

    StatistiquesTypage &donne_stats_typage();

    ArbreAplatis *donne_arbre()
    {
        return m_arbre_courant;
    }

    double donne_temps_chargement() const
    {
        return m_temps_chargement;
    }

    void rapporte_erreur(const char *message, const NoeudExpression *noeud);

    void crée_transtypage_implicite_au_besoin(NoeudExpression *&expression,
                                              TransformationType const &transformation);

  private:
    RésultatValidation valide_semantique_noeud(NoeudExpression *);
    RésultatValidation valide_acces_membre(NoeudExpressionMembre *expression_membre);

    RésultatValidation valide_entete_fonction(NoeudDeclarationEnteteFonction *);
    RésultatValidation valide_entete_operateur(NoeudDeclarationEnteteFonction *);
    RésultatValidation valide_entete_operateur_pour(NoeudDeclarationOperateurPour *);
    void valide_parametres_constants_fonction(NoeudDeclarationEnteteFonction *);
    RésultatValidation valide_parametres_fonction(NoeudDeclarationEnteteFonction *);
    RésultatValidation valide_types_parametres_fonction(NoeudDeclarationEnteteFonction *);
    RésultatValidation valide_definition_unique_fonction(NoeudDeclarationEnteteFonction *);
    RésultatValidation valide_definition_unique_operateur(NoeudDeclarationEnteteFonction *);
    RésultatValidation valide_symbole_externe(NoeudDeclarationSymbole *, TypeSymbole type_symbole);
    RésultatValidation valide_fonction(NoeudDeclarationCorpsFonction *);
    RésultatValidation valide_operateur(NoeudDeclarationCorpsFonction *);

    template <int N>
    RésultatValidation valide_enum_impl(NoeudEnum *decl);
    RésultatValidation valide_enum(NoeudEnum *);

    RésultatValidation valide_structure(NoeudStruct *);
    RésultatValidation valide_union(NoeudUnion *);
    RésultatValidation valide_declaration_variable(NoeudDeclarationVariable *decl);
    RésultatValidation valide_déclaration_constante(NoeudDeclarationConstante *decl);
    RésultatValidation valide_assignation(NoeudAssignation *inst);
    RésultatValidation valide_arbre_aplatis(NoeudExpression *declaration);
    RésultatValidation valide_expression_retour(NoeudRetour *inst_retour);
    RésultatValidation valide_cuisine(NoeudDirectiveCuisine *directive);
    RésultatValidation valide_référence_déclaration(NoeudExpressionReference *expr,
                                                    NoeudBloc *bloc_recherche);
    RésultatValidation valide_type_opaque(NoeudDeclarationTypeOpaque *decl);

    template <typename TypeControleBoucle>
    CodeRetourValidation valide_controle_boucle(TypeControleBoucle *inst);

    RésultatValidation valide_operateur_binaire(NoeudExpressionBinaire *expr);
    RésultatValidation valide_operateur_binaire_chaine(NoeudExpressionBinaire *expr);
    RésultatValidation valide_operateur_binaire_type(NoeudExpressionBinaire *expr);
    RésultatValidation valide_operateur_binaire_generique(NoeudExpressionBinaire *expr);
    RésultatValidation valide_comparaison_enum_drapeau_bool(
        NoeudExpressionBinaire *expr,
        NoeudExpression *expr_acces_enum,
        NoeudExpressionLitteraleBool *expr_bool);

    RésultatValidation valide_expression_logique(NoeudExpressionLogique *logique);

    RésultatValidation valide_discrimination(NoeudDiscr *inst);
    RésultatValidation valide_discr_énum(NoeudDiscr *inst, Type *type);
    RésultatValidation valide_discr_union(NoeudDiscr *inst, Type *type);
    RésultatValidation valide_discr_union_anonyme(NoeudDiscr *inst, Type *type);
    RésultatValidation valide_discr_scalaire(NoeudDiscr *inst, Type *type);

    CodeRetourValidation resoud_type_final(NoeudExpression *expression_type, Type *&type_final);

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

    RésultatValidation crée_transtypage_implicite_si_possible(
        NoeudExpression *&expression, Type *type_cible, RaisonTranstypageImplicite const raison);

    NoeudExpression *racine_validation() const;

    MetaProgramme *crée_metaprogramme_corps_texte(NoeudBloc *bloc_corps_texte,
                                                  NoeudBloc *bloc_parent,
                                                  const Lexeme *lexème);

    MetaProgramme *crée_metaprogramme_pour_directive(NoeudDirectiveExecute *directive);

    RésultatValidation valide_instruction_pour(NoeudPour *inst);

    RésultatValidation valide_instruction_si(NoeudSi *inst);

    RésultatValidation valide_dépendance_bibliothèque(NoeudDirectiveDependanceBibliotheque *noeud);

    RésultatValidation valide_instruction_importe(NoeudInstructionImporte *inst);

    ArbreAplatis *donne_un_arbre_aplatis();

    RésultatValidation valide_expression_comme(NoeudComme *expr);

    RésultatValidation valide_expression_type_tableau_fixe(NoeudExpressionTypeTableauFixe *expr);

    RésultatValidation valide_expression_type_tableau_dynamique(
        NoeudExpressionTypeTableauDynamique *expr);

    RésultatValidation valide_expression_type_tranche(NoeudExpressionTypeTranche *expr);

    RésultatValidation valide_expression_type_fonction(NoeudExpressionTypeFonction *expr);
};
