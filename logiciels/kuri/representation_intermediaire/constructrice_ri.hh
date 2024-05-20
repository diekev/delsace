/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "instructions.hh"

#include "arbre_syntaxique/prodeclaration.hh"

#include "structures/tableau_page.hh"
#include "structures/tablet.hh"
#include "structures/trie.hh"

class Broyeuse;
struct Annotation;
struct Compilatrice;
struct ConstructriceRI;
struct DonneesAssignations;

using TypeEnum = NoeudEnum;
using TypeTableauFixe = NoeudDéclarationTypeTableauFixe;

enum class GenreInfoType : int32_t;

/* ------------------------------------------------------------------------- */
/** \name RegistreSymboliqueRI
 * Le registre symbolique crée et stocke les atomes de toutes les fonctions et
 * toutes les globales.
 * \{ */

struct RegistreSymboliqueRI {
  private:
    kuri::tableau_page<AtomeFonction> fonctions{};
    kuri::tableau_page<AtomeGlobale> globales{};

    std::mutex mutex_atomes_fonctions{};
    std::mutex mutex_atomes_globales{};

    Broyeuse *broyeuse = nullptr;

    Typeuse &m_typeuse;

    ConstructriceRI *m_constructrice = nullptr;

  public:
    RegistreSymboliqueRI(Typeuse &typeuse);

    EMPECHE_COPIE(RegistreSymboliqueRI);

    ~RegistreSymboliqueRI();

    AtomeFonction *crée_fonction(kuri::chaine_statique nom_fonction);

    /* Il existe des dépendances cycliques entre les fonctions qui nous empêche de
     * générer le code linéairement. Cette fonction nous sers soit à trouver le
     * pointeur vers l'atome d'une fonction si nous l'avons déjà généré, soit de le
     * créer en préparation de la génération de la RI de son corps.
     */
    AtomeFonction *trouve_ou_insère_fonction(NoeudDéclarationEntêteFonction *decl);

    AtomeGlobale *crée_globale(IdentifiantCode &ident,
                               Type const *type,
                               AtomeConstante *initialisateur,
                               bool est_externe,
                               bool est_constante);

    AtomeGlobale *trouve_globale(NoeudDéclaration *decl);

    AtomeGlobale *trouve_ou_insère_globale(NoeudDéclaration *decl);

    void rassemble_statistiques(Statistiques &stats) const;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ConstructriceRI
 * La constructrice RI est responsable de créer les atomes et instructions pour
 * les fonctions et les initialisations des globales.
 * \{ */

struct ConstructriceRI {
  private:
#define ENUMERE_GENRE_ATOME_EX(genre, classe, ident) kuri::tableau_page<classe> m_##ident{};
    ENUMERE_GENRE_ATOME(ENUMERE_GENRE_ATOME_EX)
    ENUMERE_GENRE_INSTRUCTION(ENUMERE_GENRE_ATOME_EX)
#undef ENUMERE_GENRE_ATOME_EX

    /* Utilisé pour assigner des identifiants aux labels. */
    int m_nombre_labels = 0;

    Typeuse &m_typeuse;
    RegistreSymboliqueRI &m_registre;

    AtomeFonction *m_fonction_courante = nullptr;

  public:
    explicit ConstructriceRI(Typeuse &typeuse, RegistreSymboliqueRI &registre)
        : m_typeuse(typeuse), m_registre(registre)
    {
    }

    EMPECHE_COPIE(ConstructriceRI);

    void rassemble_statistiques(Statistiques &stats);

    void définis_fonction_courante(AtomeFonction *fonction_courante);

    AtomeFonction *crée_fonction(kuri::chaine_statique nom_fonction);

    AtomeFonction *trouve_ou_insère_fonction(NoeudDéclarationEntêteFonction *decl);

    AtomeGlobale *crée_globale(IdentifiantCode &ident,
                               Type const *type,
                               AtomeConstante *initialisateur,
                               bool est_externe,
                               bool est_constante);

    AtomeGlobale *trouve_globale(NoeudDéclaration *decl);

    AtomeGlobale *trouve_ou_insère_globale(NoeudDéclaration *decl);

    InstructionAllocation *crée_allocation(NoeudExpression const *site_,
                                           Type const *type,
                                           IdentifiantCode *ident,
                                           bool crée_seulement = false);

    AtomeConstanteBooléenne *crée_constante_booléenne(bool valeur);
    AtomeConstanteCaractère *crée_constante_caractère(Type const *type, uint64_t valeur);
    AtomeConstanteEntière *crée_constante_nombre_entier(Type const *type, uint64_t valeur);
    AtomeConstanteType *crée_constante_type(Type const *pointeur_type);
    AtomeConstanteTailleDe *crée_constante_taille_de(Type const *pointeur_type);
    AtomeIndexTableType *crée_index_table_type(Type const *pointeur_type);
    AtomeConstante *crée_z32(uint64_t valeur);
    AtomeConstante *crée_z64(uint64_t valeur);
    AtomeConstanteNulle *crée_constante_nulle(Type const *type);
    AtomeConstanteRéelle *crée_constante_nombre_réel(Type const *type, double valeur);
    AtomeConstanteStructure *crée_constante_structure(Type const *type,
                                                      kuri::tableau<AtomeConstante *> &&valeurs);
    AtomeConstanteTableauFixe *crée_constante_tableau_fixe(
        Type const *type, kuri::tableau<AtomeConstante *> &&valeurs);
    AtomeConstanteDonnéesConstantes *crée_constante_tableau_données_constantes(
        Type const *type, kuri::tableau<char> &&données_constantes);
    AtomeConstanteDonnéesConstantes *crée_constante_tableau_données_constantes(Type const *type,
                                                                               char *pointeur,
                                                                               int64_t taille);
    AtomeInitialisationTableau *crée_initialisation_tableau(Type const *type,
                                                            AtomeConstante const *valeur);
    AtomeNonInitialisation *crée_non_initialisation();
    AtomeConstante *crée_tranche_globale(IdentifiantCode &ident,
                                         Type const *type,
                                         kuri::tableau<AtomeConstante *> &&valeurs);
    AtomeConstante *crée_tranche_globale(IdentifiantCode &ident, AtomeConstante *tableau_fixe);
    AtomeConstante *crée_initialisation_tableau_global(AtomeGlobale *globale_tableau_fixe,
                                                       TypeTableauFixe const *type_tableau_fixe);

    InstructionBranche *crée_branche(NoeudExpression const *site_,
                                     InstructionLabel *label,
                                     bool crée_seulement = false);
    InstructionBrancheCondition *crée_branche_condition(NoeudExpression const *site_,
                                                        Atome *valeur,
                                                        InstructionLabel *label_si_vrai,
                                                        InstructionLabel *label_si_faux);
    InstructionLabel *crée_label(NoeudExpression const *site_);
    InstructionLabel *réserve_label(NoeudExpression const *site_);
    void insère_label(InstructionLabel *label);
    void insère_label_si_utilisé(InstructionLabel *label);
    InstructionRetour *crée_retour(NoeudExpression const *site_, Atome *valeur);
    InstructionStockeMem *crée_stocke_mem(NoeudExpression const *site_,
                                          Atome *ou,
                                          Atome *valeur,
                                          bool crée_seulement = false);
    InstructionChargeMem *crée_charge_mem(NoeudExpression const *site_,
                                          Atome *ou,
                                          bool crée_seulement = false);
    InstructionAppel *crée_appel(NoeudExpression const *site_, Atome *appelé);
    InstructionAppel *crée_appel(NoeudExpression const *site_,
                                 Atome *appelé,
                                 kuri::tableau<Atome *, int> &&args);

    Atome *crée_op_unaire(NoeudExpression const *site_,
                          Type const *type,
                          OpérateurUnaire::Genre op,
                          Atome *valeur);
    Atome *crée_op_binaire(NoeudExpression const *site_,
                           Type const *type,
                           OpérateurBinaire::Genre op,
                           Atome *valeur_gauche,
                           Atome *valeur_droite);
    Atome *crée_op_comparaison(NoeudExpression const *site_,
                               OpérateurBinaire::Genre op,
                               Atome *valeur_gauche,
                               Atome *valeur_droite);

    InstructionAccèdeIndex *crée_accès_index(NoeudExpression const *site_,
                                             Atome *accédé,
                                             Atome *index);
    InstructionAccèdeMembre *crée_référence_membre(NoeudExpression const *site_,
                                                   Type const *type,
                                                   Atome *accédé,
                                                   int index,
                                                   bool crée_seulement = false);
    InstructionAccèdeMembre *crée_référence_membre(NoeudExpression const *site_,
                                                   Atome *accédé,
                                                   int index,
                                                   bool crée_seulement = false);
    Instruction *crée_reference_membre_et_charge(NoeudExpression const *site_,
                                                 Atome *accédé,
                                                 int index);

    Atome *crée_transtype(NoeudExpression const *site_,
                          Type const *type,
                          Atome *valeur,
                          TypeTranstypage op);

    TranstypeConstant *crée_transtype_constant(Type const *type, AtomeConstante *valeur);
    AccèdeIndexConstant *crée_accès_index_constant(AtomeConstante *accédé, int64_t index);

    AtomeConstante *crée_initialisation_défaut_pour_type(Type const *type);

    InstructionInatteignable *crée_inatteignable(NoeudExpression const *site,
                                                 bool crée_seulement = false);
    InstructionSélection *crée_sélection(NoeudExpression const *site, bool crée_seulement);

  private:
    void insère(Instruction *inst);

    kuri::chaine imprime_site(NoeudExpression const *site) const;

    /* Déduplication des instructions de chargement. Nous ne créons des chargements que la première
     * fois qu'une valeur est chargée et après chaque stockage vers la valeur chargée. */
    kuri::tableau<InstructionChargeMem *, int> m_charges{};
    InstructionChargeMem *donne_charge(Atome *source);
    /* Appelé lors des stockages pour invalider le cache. */
    void invalide_charge(Atome *source);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name RegistreAnnotations
 * Stucture contenant les atomes globaux pour les Annotation, afin de ne créer
 * qu'une seule globale pour les annotations de même nom et de même valeur.
 * \{ */

struct RegistreAnnotations {
  private:
    struct PaireValeurGlobale {
        kuri::chaine_statique valeur{};
        AtomeGlobale *globale = nullptr;
    };

    kuri::table_hachage<kuri::chaine_statique, kuri::tableau<PaireValeurGlobale, int>> m_table{
        "table_annotations"};

  public:
    AtomeGlobale *trouve_globale_pour_annotation(Annotation const &annotation) const;

    void ajoute_annotation(Annotation const &annotation, AtomeGlobale *globale);

    int64_t mémoire_utilisée() const;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name CompilatriceRI
 * La compilatrice RI convertis en RI les noeuds syntaxiques des fonctions et
 * des globales.
 * \{ */

struct CompilatriceRI {
  private:
    Compilatrice &m_compilatrice;
    ConstructriceRI m_constructrice;

    bool expression_gauche = true;

    EspaceDeTravail *m_espace = nullptr;

    /* cette pile est utilisée pour stocker les valeurs des noeuds, quand nous
     * appelons les génère_ri_*, il faut dépiler la valeur que nous désirons, si
     * nous en désirons une */
    kuri::tablet<Atome *, 8> m_pile{};

    bool m_est_dans_diffère = false;
    kuri::tableau<NoeudExpression *> m_instructions_diffères{};

    AtomeFonction *m_fonction_courante = nullptr;

    /* Globale pour les annotations vides des membres des infos-type.
     * Nous n'en créons qu'une seule dans ce cas afin d'économiser de la mémoire.
     */
    AtomeConstante *m_globale_annotations_vides = nullptr;

    RegistreAnnotations m_registre_annotations{};

    /* Il est possible qu'une boucle retourne tout le temps ou soit infinie,
     * ou qu'un arbre de décision retourne tout le temps.
     * Dans ces cas, le label postérieur à ces controles ne sont jamais
     * insèrer (car nous utilisons #ConstructriceRI.insère_label_si_utilisé,
     * et ces labeles ne sont pas utilisés).
     * Toutefois, il est possible qu'il existe du code après ces instructions.
     * Nous devons donc tout de même insérer ces labels. Pour ce faire, nous
     * les mettons en cache, et les insérons avant la génération de code
     * suivant. */
    InstructionLabel *m_label_après_controle = nullptr;

    /* Un seul tableau pour toutes les structures n'ayant pas d'employées. */
    AtomeConstante *m_tableau_structs_employées_vide = nullptr;
    kuri::trie<AtomeConstante *, AtomeConstante *> m_trie_structs_employées{};

    /* Trie pour les entrées et sorties des fonctions. Nous partageons les tableaux pour les
     * entrées et sorties. */
    kuri::trie<Type *, AtomeConstante *> m_trie_types_entrée_sortie{};

    /* Un seul tableau pour toutes les fonctions n'ayant pas d'entrées. */
    AtomeConstante *m_tableau_types_entrées_vide = nullptr;

    /* Un seul tableau pour toutes les fonctions ne retournant « rien ». */
    AtomeConstante *m_tableau_types_sorties_rien = nullptr;

  public:
    double temps_generation = 0.0;

    explicit CompilatriceRI(Compilatrice &compilatrice);

    EMPECHE_COPIE(CompilatriceRI);

    ~CompilatriceRI();

    void génère_ri_pour_noeud(EspaceDeTravail *espace, NoeudExpression *noeud);
    void génère_ri_pour_fonction_métaprogramme(EspaceDeTravail *espace,
                                               NoeudDéclarationEntêteFonction *fonction);
    AtomeFonction *genere_fonction_init_globales_et_appel(
        EspaceDeTravail *espace,
        kuri::tableau_statique<AtomeGlobale *> globales,
        AtomeFonction *fonction_pour);

    Compilatrice &compilatrice() const
    {
        return m_compilatrice;
    }

    EspaceDeTravail *espace() const
    {
        return m_espace;
    }

    ConstructriceRI &donne_constructrice()
    {
        return m_constructrice;
    }

    void rassemble_statistiques(Statistiques &stats);

    AtomeGlobale *crée_info_type(Type const *type, NoeudExpression const *site);
    AtomeConstante *transtype_base_info_type(AtomeConstante *info_type);

    AtomeConstante *crée_tranche_globale(IdentifiantCode &ident,
                                         Type const *type,
                                         kuri::tableau<AtomeConstante *> &&valeurs);

    void génère_ri_pour_initialisation_globales(EspaceDeTravail *espace,
                                                AtomeFonction *fonction_init,
                                                kuri::tableau_statique<AtomeGlobale *> globales);

    void génère_ri_pour_initialisation_globales(AtomeFonction *fonction_init,
                                                kuri::tableau_statique<AtomeGlobale *> globales);

    AtomeGlobale *crée_info_fonction_pour_trace_appel(AtomeFonction *pour_fonction);
    AtomeGlobale *crée_info_appel_pour_trace_appel(InstructionAppel *pour_appel);
    void crée_trace_appel(AtomeFonction *fonction);

  private:
    Atome *crée_charge_mem_si_chargeable(NoeudExpression const *site_, Atome *source);
    Atome *crée_temporaire_si_non_chargeable(NoeudExpression const *site_, Atome *source);
    InstructionAllocation *crée_temporaire(NoeudExpression const *site_, Atome *source);
    /** Crée une temporaire pour \a source si \a place est nul et empile la valeur. Si \a place est
     * non-nulle, stocke la valeur directement dans celle-ci. */
    void crée_temporaire_ou_mets_dans_place(NoeudExpression const *site_,
                                            Atome *source,
                                            Atome *place);

    void crée_appel_fonction_init_type(NoeudExpression const *site_,
                                       Type const *type,
                                       Atome *argument);

    AtomeFonction *genere_fonction_init_globales_et_appel(
        kuri::tableau_statique<AtomeGlobale *> globales, AtomeFonction *fonction_pour);

    void génère_ri_pour_noeud(NoeudExpression *noeud, Atome *place = nullptr);
    void génère_ri_pour_fonction(NoeudDéclarationEntêteFonction *decl);
    void génère_ri_pour_fonction_métaprogramme(NoeudDéclarationEntêteFonction *fonction);
    void génère_ri_pour_expression_droite(NoeudExpression const *noeud, Atome *place);
    void génère_ri_transformee_pour_noeud(NoeudExpression const *noeud,
                                          Atome *place,
                                          TransformationType const &transformation);
    void génère_ri_pour_accès_membre(NoeudExpressionMembre const *noeud);
    void génère_ri_pour_accès_membre_union(NoeudExpressionMembre const *noeud);
    void génère_ri_pour_condition(NoeudExpression const *condition,
                                  InstructionLabel *label_si_vrai,
                                  InstructionLabel *label_si_faux);
    void génère_ri_pour_condition_implicite(NoeudExpression const *condition,
                                            InstructionLabel *label_si_vrai,
                                            InstructionLabel *label_si_faux);
    void génère_ri_pour_expression_logique(NoeudExpressionLogique const *noeud, Atome *place);
    void génère_ri_insts_différées(NoeudBloc const *bloc_final);
    void génère_ri_pour_déclaration_variable(NoeudDéclarationVariable *decl);
    void génère_ri_pour_variable_globale(NoeudDéclarationVariable *decl);
    void génère_ri_pour_variable_locale(NoeudDéclarationVariable *decl);
    void compile_déclaration_variable_multiple(NoeudDéclarationVariableMultiple *decl);
    void compile_déclaration_globale_multiple(NoeudDéclarationVariableMultiple *decl);
    void compile_déclaration_locale_multiple(NoeudDéclarationVariableMultiple *decl);
    void compile_globale(NoeudDéclaration *decl,
                         NoeudExpression *expression,
                         TransformationType const &transformation);
    void compile_locale(NoeudExpression *variable,
                        NoeudExpression *expression,
                        TransformationType const &transformation);

    Atome *donne_atome_pour_locale(NoeudExpression *expression);

    void génère_ri_pour_construction_tableau(NoeudExpressionConstructionTableau const *expr,
                                             Atome *place);
    void génère_ri_pour_assignation_variable(
        kuri::tableau_compresse<DonneesAssignations, int> const &données_exprs);

    void transforme_valeur(NoeudExpression const *noeud,
                           Atome *valeur,
                           const TransformationType &transformation,
                           Atome *place);

    AtomeConstante *crée_constante_info_type_pour_base(GenreInfoType index, Type const *pour_type);
    void remplis_membres_de_bases_info_type(kuri::tableau<AtomeConstante *> &valeurs,
                                            GenreInfoType index,
                                            Type const *pour_type);
    AtomeGlobale *crée_info_type_défaut(GenreInfoType index, Type const *pour_type);
    AtomeGlobale *crée_info_type_entier(Type const *pour_type, bool est_relatif);
    AtomeConstante *crée_info_type_avec_transtype(Type const *type, NoeudExpression const *site);
    AtomeGlobale *crée_globale_info_type(Type const *type_info_type,
                                         kuri::tableau<AtomeConstante *> &&valeurs);
    AtomeGlobale *crée_info_type_membre_structure(MembreTypeComposé const &membre,
                                                  NoeudExpression const *site);

    AtomeConstante *donne_tableau_pour_structs_employées(TypeStructure const *type_structure,
                                                         NoeudExpression const *site);

    AtomeConstante *donne_tableau_pour_types_entrées(TypeFonction const *type_fonction,
                                                     NoeudExpression const *site);

    AtomeConstante *donne_tableau_pour_type_sortie(TypeFonction const *type_fonction,
                                                   NoeudExpression const *site);

    Atome *convertis_vers_tranche(NoeudExpression const *noeud,
                                  Atome *pointeur_tableau_fixe,
                                  TypeTableauFixe const *type_tableau_fixe,
                                  Atome *place);

    Atome *convertis_vers_tranche(NoeudExpression const *noeud,
                                  Atome *pointeur_tableau,
                                  TypeTableauDynamique const *type_tableau_fixe,
                                  Atome *place);

    AtomeConstante *crée_chaine(kuri::chaine_statique chaine);

    void empile_valeur(Atome *valeur);
    Atome *depile_valeur();

    AtomeConstante *crée_tableau_annotations_pour_info_membre(
        const kuri::tableau<Annotation, int> &annotations);

    Atome *crée_transtype_entre_base_et_dérivé(NoeudExpression const *noeud,
                                               Atome *valeur,
                                               const TransformationType &transformation,
                                               OpérateurBinaire::Genre op);

    void définis_fonction_courante(AtomeFonction *fonction_courante);
};

/** \} */
