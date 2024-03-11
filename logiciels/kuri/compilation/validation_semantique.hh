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
struct Lexème;
struct MetaProgramme;
struct NoeudAssignation;
struct NoeudAssignationMultiple;
struct NoeudBloc;
struct NoeudComme;
struct NoeudDéclarationConstante;
struct NoeudDéclarationCorpsFonction;
struct NoeudDéclarationEntêteFonction;
struct NoeudDéclarationOpérateurPour;
struct NoeudDéclarationSymbole;
struct NoeudDéclarationType;
struct NoeudDéclarationTypeComposé;
struct NoeudDéclarationTypeOpaque;
struct NoeudDéclarationTypeTableauFixe;
struct NoeudDéclarationVariable;
struct NoeudDéclarationVariableMultiple;
struct NoeudDirectiveCuisine;
struct NoeudDirectiveDépendanceBibliothèque;
struct NoeudDirectiveExécute;
struct NoeudDiscr;
struct NoeudEnum;
struct NoeudExpressionAssignationLogique;
struct NoeudExpressionBinaire;
struct NoeudExpressionConstructionTableauTypé;
struct NoeudExpressionLittéraleBool;
struct NoeudExpressionLogique;
struct NoeudExpressionMembre;
struct NoeudExpressionTypeFonction;
struct NoeudExpressionTypeTableauDynamique;
struct NoeudExpressionTypeTableauFixe;
struct NoeudExpressionTypeTranche;
struct NoeudInstructionImporte;
struct NoeudPour;
struct NoeudInstructionRetour;
struct NoeudInstructionRetourMultiple;
struct NoeudSi;
struct NoeudStruct;
struct NoeudUnion;
struct Tacheronne;
struct TransformationType;
struct UniteCompilation;

using Type = NoeudDéclarationType;
using TypeCompose = NoeudDéclarationTypeComposé;
using TypeTableauFixe = NoeudDéclarationTypeTableauFixe;

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
struct ContexteValidationDéclaration {
    struct DéclarationEtRéférence {
        NoeudExpression *ref_decl = nullptr;
        NoeudDéclarationVariable *decl = nullptr;
    };

    /* Les variables déclarées, entre les virgules, si quelqu'une. */
    kuri::tablet<NoeudExpression *, 6> feuilles_variables{};

    /* Les noeuds de déclarations des variables et les références pointant vers ceux-ci. */
    kuri::tablet<DéclarationEtRéférence, 6> decls_et_refs{};

    /* Les expressions pour les initialisations, entre les virgules, si quelqu'une. */
    kuri::tablet<NoeudExpression *, 6> feuilles_expressions{};

    /* Les variables à assigner, chaque expression le nombre de variables nécessaires pour recevoir
     * le résultat de son évaluation. */
    kuri::file_fixe<NoeudExpression *, 6> variables{};

    /* Les données finales pour les assignations, faisant correspondre les expressions aux
     * variables. */
    kuri::tablet<DonneesAssignations, 6> données_assignations{};

    /* Données temporaires pour la constructions des donnees_assignations. */
    DonneesAssignations données_temp{};
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

    ContexteValidationDéclaration m_contexte_validation_déclaration{};

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

    NoeudDéclarationEntêteFonction *fonction_courante() const;

    Type *union_ou_structure_courante() const;

    AssembleuseArbre *donne_assembleuse();

    StatistiquesTypage &donne_stats_typage();

    void rassemble_statistiques(Statistiques &stats);

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
    RésultatValidation valide_sémantique_noeud(NoeudExpression *);
    RésultatValidation valide_accès_membre(NoeudExpressionMembre *expression_membre);

    RésultatValidation valide_entête_fonction(NoeudDéclarationEntêteFonction *);
    RésultatValidation valide_entête_opérateur(NoeudDéclarationEntêteFonction *);
    RésultatValidation valide_entête_opérateur_pour(NoeudDéclarationOpérateurPour *);
    void valide_paramètres_constants_fonction(NoeudDéclarationEntêteFonction *);
    RésultatValidation valide_paramètres_fonction(NoeudDéclarationEntêteFonction *);
    RésultatValidation valide_types_paramètres_fonction(NoeudDéclarationEntêteFonction *);
    RésultatValidation valide_définition_unique_fonction(NoeudDéclarationEntêteFonction *);
    RésultatValidation valide_définition_unique_opérateur(NoeudDéclarationEntêteFonction *);
    RésultatValidation valide_symbole_externe(NoeudDéclarationSymbole *, TypeSymbole type_symbole);
    RésultatValidation valide_fonction(NoeudDéclarationCorpsFonction *);
    RésultatValidation valide_opérateur(NoeudDéclarationCorpsFonction *);

    template <int N>
    RésultatValidation valide_énum_impl(NoeudEnum *decl);
    RésultatValidation valide_énum(NoeudEnum *);

    RésultatValidation valide_structure(NoeudStruct *);
    RésultatValidation valide_union(NoeudUnion *);
    RésultatValidation valide_déclaration_variable(NoeudDéclarationVariable *decl);
    RésultatValidation valide_déclaration_variable_multiple(
        NoeudDéclarationVariableMultiple *decl);
    RésultatValidation valide_déclaration_constante(NoeudDéclarationConstante *decl);
    RésultatValidation valide_assignation(NoeudAssignation *inst);
    RésultatValidation valide_assignation_multiple(NoeudAssignationMultiple *inst);
    RésultatValidation valide_arbre_aplatis(NoeudExpression *declaration);
    RésultatValidation valide_expression_retour(NoeudInstructionRetour *inst_retour);
    RésultatValidation valide_instruction_retourne_multiple(
        NoeudInstructionRetourMultiple *inst_retour);
    RésultatValidation valide_cuisine(NoeudDirectiveCuisine *directive);
    RésultatValidation valide_référence_déclaration(NoeudExpressionRéférence *expr,
                                                    NoeudBloc *bloc_recherche);
    RésultatValidation valide_type_opaque(NoeudDéclarationTypeOpaque *decl);

    template <typename TypeControleBoucle>
    CodeRetourValidation valide_controle_boucle(TypeControleBoucle *inst);

    RésultatValidation valide_opérateur_binaire(NoeudExpressionBinaire *expr);
    RésultatValidation valide_opérateur_binaire_chaine(NoeudExpressionBinaire *expr);
    RésultatValidation valide_opérateur_binaire_type(NoeudExpressionBinaire *expr);
    RésultatValidation valide_opérateur_binaire_générique(NoeudExpressionBinaire *expr);
    RésultatValidation valide_comparaison_énum_drapeau_bool(
        NoeudExpressionBinaire *expr,
        NoeudExpression *expr_acces_enum,
        NoeudExpressionLittéraleBool *expr_bool);

    RésultatValidation valide_expression_logique(NoeudExpressionLogique *logique);
    RésultatValidation valide_assignation_logique(NoeudExpressionAssignationLogique *logique);

    RésultatValidation valide_discrimination(NoeudDiscr *inst);
    RésultatValidation valide_discr_énum(NoeudDiscr *inst, Type *type);
    RésultatValidation valide_discr_union(NoeudDiscr *inst, Type *type);
    RésultatValidation valide_discr_union_anonyme(NoeudDiscr *inst, Type *type);
    RésultatValidation valide_discr_scalaire(NoeudDiscr *inst, Type *type);

    CodeRetourValidation résoud_type_final(NoeudExpression *expression_type, Type *&type_final);

    void rapporte_erreur(const char *message, const NoeudExpression *noeud, erreur::Genre genre);
    void rapporte_erreur_redéfinition_symbole(NoeudExpression *decl, NoeudDéclaration *decl_prec);
    void rapporte_erreur_redéfinition_fonction(NoeudDéclarationEntêteFonction *decl,
                                               NoeudDéclaration *decl_prec);
    void rapporte_erreur_type_arguments(NoeudExpression *type_arg, NoeudExpression *type_enf);
    void rapporte_erreur_assignation_type_différents(const Type *type_gauche,
                                                     const Type *type_droite,
                                                     NoeudExpression *noeud);
    void rapporte_erreur_type_opération(const Type *type_gauche,
                                        const Type *type_droite,
                                        NoeudExpression *noeud);
    void rapporte_erreur_accès_hors_limites(NoeudExpression *b,
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

    MetaProgramme *crée_métaprogramme_corps_texte(NoeudBloc *bloc_corps_texte,
                                                  NoeudBloc *bloc_parent,
                                                  const Lexème *lexème);

    MetaProgramme *crée_métaprogramme_pour_directive(NoeudDirectiveExécute *directive);

    CodeRetourValidation valide_expression_pour_condition(NoeudExpression const *condition,
                                                          bool permet_déclaration);

    RésultatValidation valide_instruction_pour(NoeudPour *inst);

    RésultatValidation valide_instruction_si(NoeudSi *inst);

    RésultatValidation valide_dépendance_bibliothèque(NoeudDirectiveDépendanceBibliothèque *noeud);

    RésultatValidation valide_instruction_importe(NoeudInstructionImporte *inst);

    ArbreAplatis *donne_un_arbre_aplatis();

    RésultatValidation valide_expression_comme(NoeudComme *expr);

    RésultatValidation valide_expression_type_tableau_fixe(NoeudExpressionTypeTableauFixe *expr);

    RésultatValidation valide_expression_type_tableau_dynamique(
        NoeudExpressionTypeTableauDynamique *expr);

    RésultatValidation valide_expression_type_tranche(NoeudExpressionTypeTranche *expr);

    RésultatValidation valide_expression_type_fonction(NoeudExpressionTypeFonction *expr);

    RésultatValidation valide_construction_tableau_typé(
        NoeudExpressionConstructionTableauTypé *tableau);
};
