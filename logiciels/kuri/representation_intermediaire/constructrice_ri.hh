/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "instructions.hh"

#include "arbre_syntaxique/noeud_code.hh" /* Pour Annotation */

#include "structures/tablet.hh"

struct Compilatrice;
struct NoeudBloc;
struct NoeudDeclarationVariable;
struct NoeudExpression;
struct NoeudExpressionMembre;
struct NoeudInstructionTente;
struct TypeEnum;
struct TypeTableauFixe;

struct ConstructriceRI {
  private:
    tableau_page<AtomeValeurConstante> atomes_constante{};
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
    tableau_page<OpBinaireConstant> op_binaires_constants{};
    tableau_page<OpUnaireConstant> op_unaires_constants{};
    tableau_page<AccedeIndexConstant> accede_index_constants{};

    Compilatrice &m_compilatrice;

    int nombre_labels = 0;

    bool expression_gauche = true;

    EspaceDeTravail *m_espace = nullptr;

    /* cette pile est utilisée pour stocker les valeurs des noeuds, quand nous
     * appelons les genere_ri_*, il faut dépiler la valeur que nous désirons, si
     * nous en désirons une */
    kuri::tablet<Atome *, 8> m_pile{};

  public:
    AtomeFonction *fonction_courante = nullptr;

    double temps_generation = 0.0;

    explicit ConstructriceRI(Compilatrice &compilatrice);

    EMPECHE_COPIE(ConstructriceRI);

    ~ConstructriceRI();

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

    InstructionAllocation *crée_allocation(NoeudExpression *site_,
                                           Type const *type,
                                           IdentifiantCode *ident,
                                           bool crée_seulement = false);

    void rassemble_statistiques(Statistiques &stats);

    AtomeConstante *crée_constante_booleenne(bool valeur);
    AtomeConstante *crée_constante_caractere(Type const *type, uint64_t valeur);
    AtomeConstante *crée_constante_entiere(Type const *type, uint64_t valeur);
    AtomeConstante *crée_constante_type(Type const *pointeur_type);
    AtomeConstante *crée_constante_taille_de(Type const *pointeur_type);
    AtomeConstante *crée_z32(uint64_t valeur);
    AtomeConstante *crée_z64(uint64_t valeur);
    AtomeConstante *crée_constante_nulle(Type const *type);
    AtomeConstante *crée_constante_reelle(Type const *type, double valeur);
    AtomeConstante *crée_constante_structure(Type const *type,
                                             kuri::tableau<AtomeConstante *> &&valeurs);
    AtomeConstante *crée_constante_tableau_fixe(Type const *type,
                                                kuri::tableau<AtomeConstante *> &&valeurs);
    AtomeConstante *crée_constante_tableau_donnees_constantes(
        Type const *type, kuri::tableau<char> &&donnees_constantes);
    AtomeConstante *crée_constante_tableau_donnees_constantes(Type const *type,
                                                              char *pointeur,
                                                              int64_t taille);
    AtomeGlobale *crée_globale(Type const *type,
                               AtomeConstante *initialisateur,
                               bool est_externe,
                               bool est_constante);
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
    InstructionLabel *reserve_label(NoeudExpression *site_);
    void insere_label(InstructionLabel *label);
    void insere_label_si_utilise(InstructionLabel *label);
    InstructionRetour *crée_retour(NoeudExpression *site_, Atome *valeur);
    InstructionStockeMem *crée_stocke_mem(NoeudExpression *site_,
                                          Atome *ou,
                                          Atome *valeur,
                                          bool crée_seulement = false);
    InstructionChargeMem *crée_charge_mem(NoeudExpression *site_,
                                          Atome *ou,
                                          bool crée_seulement = false);
    InstructionAppel *crée_appel(NoeudExpression *site_, Atome *appele);
    InstructionAppel *crée_appel(NoeudExpression *site_,
                                 Atome *appele,
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

    InstructionAccedeIndex *crée_acces_index(NoeudExpression *site_, Atome *accede, Atome *index);
    InstructionAccedeMembre *crée_reference_membre(NoeudExpression *site_,
                                                   Atome *accede,
                                                   int index,
                                                   bool crée_seulement = false);
    Instruction *crée_reference_membre_et_charge(NoeudExpression *site_, Atome *accede, int index);

    InstructionTranstype *crée_transtype(NoeudExpression *site_,
                                         Type const *type,
                                         Atome *valeur,
                                         TypeTranstypage op);

    TranstypeConstant *crée_transtype_constant(Type const *type, AtomeConstante *valeur);
    OpUnaireConstant *crée_op_unaire_constant(Type const *type,
                                              OpérateurUnaire::Genre op,
                                              AtomeConstante *valeur);
    OpBinaireConstant *crée_op_binaire_constant(Type const *type,
                                                OpérateurBinaire::Genre op,
                                                AtomeConstante *valeur_gauche,
                                                AtomeConstante *valeur_droite);
    OpBinaireConstant *crée_op_comparaison_constant(OpérateurBinaire::Genre op,
                                                    AtomeConstante *valeur_gauche,
                                                    AtomeConstante *valeur_droite);
    AccedeIndexConstant *crée_acces_index_constant(AtomeConstante *accede, AtomeConstante *index);

    AtomeConstante *crée_info_type(Type const *type, NoeudExpression *site);
    AtomeConstante *transtype_base_info_type(AtomeConstante *info_type);

    void genere_ri_pour_initialisation_globales(EspaceDeTravail *espace,
                                                AtomeFonction *fonction_init,
                                                const kuri::tableau<AtomeGlobale *> &globales);

    void genere_ri_pour_initialisation_globales(AtomeFonction *fonction_init,
                                                const kuri::tableau<AtomeGlobale *> &globales);

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
    AtomeConstante *genere_initialisation_defaut_pour_type(Type const *type);
    void genere_ri_pour_condition(NoeudExpression *condition,
                                  InstructionLabel *label_si_vrai,
                                  InstructionLabel *label_si_faux);
    void genere_ri_pour_condition_implicite(NoeudExpression *condition,
                                            InstructionLabel *label_si_vrai,
                                            InstructionLabel *label_si_faux);
    void genere_ri_pour_expression_logique(NoeudExpression *noeud, Atome *place);
    void genere_ri_insts_differees(NoeudBloc *bloc, const NoeudBloc *bloc_final);
    void genere_ri_pour_position_code_source(NoeudExpression *noeud);
    void genere_ri_pour_declaration_variable(NoeudDeclarationVariable *decl);

    void transforme_valeur(NoeudExpression *noeud,
                           Atome *valeur,
                           const TransformationType &transformation,
                           Atome *place);

    AtomeConstante *crée_constante_info_type_pour_base(uint32_t index, Type const *pour_type);
    void remplis_membres_de_bases_info_type(kuri::tableau<AtomeConstante *> &valeurs,
                                            uint32_t index,
                                            Type const *pour_type);
    AtomeConstante *crée_info_type_defaut(unsigned index, Type const *pour_type);
    AtomeConstante *crée_info_type_entier(Type const *pour_type, bool est_relatif);
    AtomeConstante *crée_info_type_avec_transtype(Type const *type, NoeudExpression *site);
    AtomeConstante *crée_globale_info_type(Type const *type_info_type,
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

    /* pour pouvoir accéder aux tableaux d'instructions */
    friend struct CopieuseInstruction;
};
