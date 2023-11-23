/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "instructions.hh"

#include "structures/tablet.hh"

struct Annotation;
struct Broyeuse;
struct Compilatrice;
struct ConstructriceRI;
struct NoeudBloc;
struct NoeudDeclarationVariable;
struct NoeudExpression;
struct NoeudExpressionConstructionTableau;
struct NoeudExpressionMembre;
struct NoeudInstructionTente;
struct TypeEnum;
struct TypeTableauFixe;

/* ------------------------------------------------------------------------- */
/** \name RegistreSymboliqueRI
 * Le registre symbolique crée et stocke les atomes de toutes les fonctions et
 * toutes les globales.
 * \{ */

struct RegistreSymboliqueRI {
  private:
    tableau_page<AtomeFonction> fonctions{};
    tableau_page<AtomeGlobale> globales{};

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
    AtomeFonction *trouve_ou_insère_fonction(NoeudDeclarationEnteteFonction *decl);

    AtomeGlobale *crée_globale(Type const *type,
                               AtomeConstante *initialisateur,
                               bool est_externe,
                               bool est_constante);

    AtomeGlobale *trouve_globale(NoeudDeclaration *decl);

    AtomeGlobale *trouve_ou_insère_globale(NoeudDeclaration *decl);

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
    tableau_page<AtomeConstanteEntière> constantes_entières{};
    tableau_page<AtomeConstanteRéelle> constantes_réelles{};
    tableau_page<AtomeConstanteBooléenne> constantes_booléennes{};
    tableau_page<AtomeConstanteNulle> constantes_nulles{};
    tableau_page<AtomeConstanteCaractère> constantes_caractères{};
    tableau_page<AtomeConstanteStructure> constantes_structures{};
    tableau_page<AtomeConstanteTableauFixe> constantes_tableaux{};
    tableau_page<AtomeConstanteDonnéesConstantes> constantes_données_constantes{};
    tableau_page<AtomeConstanteType> constantes_types{};
    tableau_page<AtomeConstanteTailleDe> constantes_taille_de{};
    tableau_page<AtomeInitialisationTableau> initialisations_tableau{};
    tableau_page<AtomeNonInitialisation> non_initialisations{};
    tableau_page<InstructionAllocation> insts_allocation{};
    tableau_page<InstructionAppel> insts_appel{};
    tableau_page<InstructionBranche> insts_branche{};
    tableau_page<InstructionBrancheCondition> insts_branche_condition{};
    tableau_page<InstructionChargeMem> insts_charge_memoire{};
    tableau_page<InstructionLabel> insts_label{};
    tableau_page<InstructionOpBinaire> insts_opbinaire{};
    tableau_page<InstructionOpUnaire> insts_opunaire{};
    tableau_page<InstructionRetour> insts_retour{};
    tableau_page<InstructionStockeMem> insts_stocke_memoire{};
    tableau_page<InstructionAccedeIndex> insts_accede_index{};
    tableau_page<InstructionAccedeMembre> insts_accede_membre{};
    tableau_page<InstructionTranstype> insts_transtype{};
    tableau_page<TranstypeConstant> transtypes_constants{};
    tableau_page<AccedeIndexConstant> accede_index_constants{};

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

    AtomeFonction *trouve_ou_insère_fonction(NoeudDeclarationEnteteFonction *decl);

    AtomeGlobale *crée_globale(Type const *type,
                               AtomeConstante *initialisateur,
                               bool est_externe,
                               bool est_constante);

    AtomeGlobale *trouve_globale(NoeudDeclaration *decl);

    AtomeGlobale *trouve_ou_insère_globale(NoeudDeclaration *decl);

    InstructionAllocation *crée_allocation(NoeudExpression *site_,
                                           Type const *type,
                                           IdentifiantCode *ident,
                                           bool crée_seulement = false);

    AtomeConstanteBooléenne *crée_constante_booléenne(bool valeur);
    AtomeConstanteCaractère *crée_constante_caractère(Type const *type, uint64_t valeur);
    AtomeConstanteEntière *crée_constante_nombre_entier(Type const *type, uint64_t valeur);
    AtomeConstanteType *crée_constante_type(Type const *pointeur_type);
    AtomeConstanteTailleDe *crée_constante_taille_de(Type const *pointeur_type);
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
    AtomeConstante *crée_tableau_global(Type const *type,
                                        kuri::tableau<AtomeConstante *> &&valeurs);
    AtomeConstante *crée_tableau_global(AtomeConstante *tableau_fixe);
    AtomeConstante *crée_initialisation_tableau_global(AtomeGlobale *globale_tableau_fixe,
                                                       TypeTableauFixe const *type_tableau_fixe);

    InstructionBranche *crée_branche(NoeudExpression *site_,
                                     InstructionLabel *label,
                                     bool crée_seulement = false);
    InstructionBrancheCondition *crée_branche_condition(NoeudExpression *site_,
                                                        Atome *valeur,
                                                        InstructionLabel *label_si_vrai,
                                                        InstructionLabel *label_si_faux);
    InstructionLabel *crée_label(NoeudExpression *site_);
    InstructionLabel *réserve_label(NoeudExpression *site_);
    void insère_label(InstructionLabel *label);
    void insère_label_si_utilisé(InstructionLabel *label);
    InstructionRetour *crée_retour(NoeudExpression *site_, Atome *valeur);
    InstructionStockeMem *crée_stocke_mem(NoeudExpression *site_,
                                          Atome *ou,
                                          Atome *valeur,
                                          bool crée_seulement = false);
    InstructionChargeMem *crée_charge_mem(NoeudExpression *site_,
                                          Atome *ou,
                                          bool crée_seulement = false);
    InstructionAppel *crée_appel(NoeudExpression *site_, Atome *appelé);
    InstructionAppel *crée_appel(NoeudExpression *site_,
                                 Atome *appelé,
                                 kuri::tableau<Atome *, int> &&args);

    InstructionOpUnaire *crée_op_unaire(NoeudExpression *site_,
                                        Type const *type,
                                        OpérateurUnaire::Genre op,
                                        Atome *valeur);
    InstructionOpBinaire *crée_op_binaire(NoeudExpression *site_,
                                          Type const *type,
                                          OpérateurBinaire::Genre op,
                                          Atome *valeur_gauche,
                                          Atome *valeur_droite);
    InstructionOpBinaire *crée_op_comparaison(NoeudExpression *site_,
                                              OpérateurBinaire::Genre op,
                                              Atome *valeur_gauche,
                                              Atome *valeur_droite);

    InstructionAccedeIndex *crée_accès_index(NoeudExpression *site_, Atome *accédé, Atome *index);
    InstructionAccedeMembre *crée_référence_membre(NoeudExpression *site_,
                                                   Type const *type,
                                                   Atome *accédé,
                                                   int index,
                                                   bool crée_seulement = false);
    InstructionAccedeMembre *crée_référence_membre(NoeudExpression *site_,
                                                   Atome *accédé,
                                                   int index,
                                                   bool crée_seulement = false);
    Instruction *crée_reference_membre_et_charge(NoeudExpression *site_, Atome *accédé, int index);

    InstructionTranstype *crée_transtype(NoeudExpression *site_,
                                         Type const *type,
                                         Atome *valeur,
                                         TypeTranstypage op);

    TranstypeConstant *crée_transtype_constant(Type const *type, AtomeConstante *valeur);
    AccedeIndexConstant *crée_accès_index_constant(AtomeConstante *accédé, int64_t index);

    AtomeConstante *crée_initialisation_défaut_pour_type(Type const *type);

  private:
    kuri::chaine imprime_site(NoeudExpression *site) const;
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
     * appelons les genere_ri_*, il faut dépiler la valeur que nous désirons, si
     * nous en désirons une */
    kuri::tablet<Atome *, 8> m_pile{};

    AtomeFonction *m_fonction_courante = nullptr;

    /* Globale pour les annotations vides des membres des infos-type.
     * Nous n'en créons qu'une seule dans ce cas afin d'économiser de la mémoire.
     */
    AtomeConstante *m_globale_annotations_vides = nullptr;

    RegistreAnnotations m_registre_annotations{};

  public:
    double temps_generation = 0.0;

    explicit CompilatriceRI(Compilatrice &compilatrice);

    EMPECHE_COPIE(CompilatriceRI);

    ~CompilatriceRI();

    void genere_ri_pour_noeud(EspaceDeTravail *espace, NoeudExpression *noeud);
    void genere_ri_pour_fonction_metaprogramme(EspaceDeTravail *espace,
                                               NoeudDeclarationEnteteFonction *fonction);
    AtomeFonction *genere_fonction_init_globales_et_appel(
        EspaceDeTravail *espace,
        const kuri::tableau<AtomeGlobale *> &globales,
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

    Atome *crée_charge_mem_si_chargeable(NoeudExpression *site_, Atome *source);
    Atome *crée_temporaire_si_non_chargeable(NoeudExpression *site_, Atome *source);
    InstructionAllocation *crée_temporaire(NoeudExpression *site_, Atome *source);

    AtomeGlobale *crée_info_type(Type const *type, NoeudExpression *site);
    AtomeConstante *transtype_base_info_type(AtomeConstante *info_type);

    AtomeConstante *crée_tableau_global(Type const *type,
                                        kuri::tableau<AtomeConstante *> &&valeurs);

    void genere_ri_pour_initialisation_globales(EspaceDeTravail *espace,
                                                AtomeFonction *fonction_init,
                                                const kuri::tableau<AtomeGlobale *> &globales);

    void genere_ri_pour_initialisation_globales(AtomeFonction *fonction_init,
                                                const kuri::tableau<AtomeGlobale *> &globales);

    AtomeGlobale *crée_info_fonction_pour_trace_appel(AtomeFonction *pour_fonction);
    AtomeGlobale *crée_info_appel_pour_trace_appel(InstructionAppel *pour_appel);
    void crée_trace_appel(AtomeFonction *fonction);

  private:
    void crée_appel_fonction_init_type(NoeudExpression *site_, Type const *type, Atome *argument);

    AtomeFonction *genere_fonction_init_globales_et_appel(
        const kuri::tableau<AtomeGlobale *> &globales, AtomeFonction *fonction_pour);

    void genere_ri_pour_noeud(NoeudExpression *noeud);
    void genere_ri_pour_fonction(NoeudDeclarationEnteteFonction *decl);
    void genere_ri_pour_fonction_metaprogramme(NoeudDeclarationEnteteFonction *fonction);
    void genere_ri_pour_expression_droite(NoeudExpression *noeud, Atome *place);
    void genere_ri_transformee_pour_noeud(NoeudExpression *noeud,
                                          Atome *place,
                                          TransformationType const &transformation);
    void genere_ri_pour_tente(NoeudInstructionTente *noeud);
    void genere_ri_pour_acces_membre(NoeudExpressionMembre *noeud);
    void genere_ri_pour_acces_membre_union(NoeudExpressionMembre *noeud);
    void genere_ri_pour_condition(NoeudExpression *condition,
                                  InstructionLabel *label_si_vrai,
                                  InstructionLabel *label_si_faux);
    void genere_ri_pour_condition_implicite(NoeudExpression *condition,
                                            InstructionLabel *label_si_vrai,
                                            InstructionLabel *label_si_faux);
    void genere_ri_pour_expression_logique(NoeudExpression *noeud, Atome *place);
    void genere_ri_insts_differees(NoeudBloc *bloc, const NoeudBloc *bloc_final);
    void genere_ri_pour_position_code_source(NoeudExpression *noeud);
    void génère_ri_pour_déclaration_variable(NoeudDeclarationVariable *decl);
    void génère_ri_pour_variable_globale(NoeudDeclarationVariable *decl);
    void génère_ri_pour_variable_locale(NoeudDeclarationVariable *decl);
    void génère_ri_pour_construction_tableau(NoeudExpressionConstructionTableau *expr);

    void transforme_valeur(NoeudExpression *noeud,
                           Atome *valeur,
                           const TransformationType &transformation,
                           Atome *place);

    AtomeConstante *crée_constante_info_type_pour_base(uint32_t index, Type const *pour_type);
    void remplis_membres_de_bases_info_type(kuri::tableau<AtomeConstante *> &valeurs,
                                            uint32_t index,
                                            Type const *pour_type);
    AtomeGlobale *crée_info_type_defaut(unsigned index, Type const *pour_type);
    AtomeGlobale *crée_info_type_entier(Type const *pour_type, bool est_relatif);
    AtomeConstante *crée_info_type_avec_transtype(Type const *type, NoeudExpression *site);
    AtomeGlobale *crée_globale_info_type(Type const *type_info_type,
                                         kuri::tableau<AtomeConstante *> &&valeurs);

    Atome *converti_vers_tableau_dyn(NoeudExpression *noeud,
                                     Atome *pointeur_tableau_fixe,
                                     TypeTableauFixe *type_tableau_fixe,
                                     Atome *place);

    AtomeConstante *crée_chaine(kuri::chaine_statique chaine);

    Atome *valeur_enum(TypeEnum *type_enum, IdentifiantCode *ident);

    void empile_valeur(Atome *valeur);
    Atome *depile_valeur();

    AtomeConstante *crée_tableau_annotations_pour_info_membre(
        const kuri::tableau<Annotation, int> &annotations);

    Atome *crée_transtype_entre_base_et_dérivé(NoeudExpression *noeud,
                                               Atome *valeur,
                                               const TransformationType &transformation,
                                               OpérateurBinaire::Genre op);

    void définis_fonction_courante(AtomeFonction *fonction_courante);
};

/** \} */
