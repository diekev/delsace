/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "instructions.hh"

#include "biblinternes/moultfilage/synchrone.hh"

#include "arbre_syntaxique/noeud_code.hh" /* Pour Annotation */

#include "structures/tablet.hh"

struct Compilatrice;
struct NoeudBloc;
struct NoeudDeclarationVariable;
struct NoeudDirectiveExecute;
struct NoeudDiscr;
struct NoeudExpression;
struct NoeudExpressionAppel;
struct NoeudExpressionMembre;
struct NoeudPour;
struct NoeudStruct;
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

    /* La taille allouée nous sert à ternir trace des allocations dans les blocs
     * afin de pouvoir réutiliser la mémoire des variables quand nous sortons d'un
     * bloc pour l'exécution du code dans la MachineVirtuelle.
     */
    int taille_allouee = 0;

    NoeudExpressionAppel *m_noeud_pour_appel = nullptr;

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

    COPIE_CONSTRUCT(ConstructriceRI);

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

    InstructionAllocation *cree_allocation(NoeudExpression *site_,
                                           Type const *type,
                                           IdentifiantCode *ident,
                                           bool cree_seulement = false);

    void rassemble_statistiques(Statistiques &stats);

    AtomeConstante *cree_constante_booleenne(bool valeur);
    AtomeConstante *cree_constante_caractere(Type const *type, uint64_t valeur);
    AtomeConstante *cree_constante_entiere(Type const *type, uint64_t valeur);
    AtomeConstante *cree_constante_type(Type const *pointeur_type);
    AtomeConstante *cree_constante_taille_de(Type const *pointeur_type);
    AtomeConstante *cree_z32(uint64_t valeur);
    AtomeConstante *cree_z64(uint64_t valeur);
    AtomeConstante *cree_constante_nulle(Type const *type);
    AtomeConstante *cree_constante_reelle(Type const *type, double valeur);
    AtomeConstante *cree_constante_structure(Type const *type,
                                             kuri::tableau<AtomeConstante *> &&valeurs);
    AtomeConstante *cree_constante_tableau_fixe(Type const *type,
                                                kuri::tableau<AtomeConstante *> &&valeurs);
    AtomeConstante *cree_constante_tableau_donnees_constantes(
        Type const *type, kuri::tableau<char> &&donnees_constantes);
    AtomeConstante *cree_constante_tableau_donnees_constantes(Type const *type,
                                                              char *pointeur,
                                                              long taille);
    AtomeGlobale *cree_globale(Type const *type,
                               AtomeConstante *initialisateur,
                               bool est_externe,
                               bool est_constante);
    AtomeConstante *cree_tableau_global(Type const *type,
                                        kuri::tableau<AtomeConstante *> &&valeurs);
    AtomeConstante *cree_tableau_global(AtomeConstante *tableau_fixe);
    AtomeConstante *cree_initialisation_tableau_global(AtomeGlobale *globale_tableau_fixe,
                                                       TypeTableauFixe const *type_tableau_fixe);

    InstructionBranche *cree_branche(NoeudExpression *site_,
                                     InstructionLabel *label,
                                     bool cree_seulement = false);
    InstructionBrancheCondition *cree_branche_condition(NoeudExpression *site_,
                                                        Atome *valeur,
                                                        InstructionLabel *label_si_vrai,
                                                        InstructionLabel *label_si_faux);
    InstructionLabel *cree_label(NoeudExpression *site_);
    InstructionLabel *reserve_label(NoeudExpression *site_);
    void insere_label(InstructionLabel *label);
    void insere_label_si_utilise(InstructionLabel *label);
    InstructionRetour *cree_retour(NoeudExpression *site_, Atome *valeur);
    InstructionStockeMem *cree_stocke_mem(NoeudExpression *site_,
                                          Atome *ou,
                                          Atome *valeur,
                                          bool cree_seulement = false);
    InstructionChargeMem *cree_charge_mem(NoeudExpression *site_,
                                          Atome *ou,
                                          bool cree_seulement = false);
    InstructionAppel *cree_appel(NoeudExpression *site_, Atome *appele);
    InstructionAppel *cree_appel(NoeudExpression *site_,
                                 Atome *appele,
                                 kuri::tableau<Atome *, int> &&args);

    InstructionOpUnaire *cree_op_unaire(NoeudExpression *site_,
                                        Type const *type,
                                        OperateurUnaire::Genre op,
                                        Atome *valeur);
    InstructionOpBinaire *cree_op_binaire(NoeudExpression *site_,
                                          Type const *type,
                                          OperateurBinaire::Genre op,
                                          Atome *valeur_gauche,
                                          Atome *valeur_droite);
    InstructionOpBinaire *cree_op_comparaison(NoeudExpression *site_,
                                              OperateurBinaire::Genre op,
                                              Atome *valeur_gauche,
                                              Atome *valeur_droite);

    InstructionAccedeIndex *cree_acces_index(NoeudExpression *site_, Atome *accede, Atome *index);
    InstructionAccedeMembre *cree_reference_membre(NoeudExpression *site_,
                                                   Atome *accede,
                                                   int index,
                                                   bool cree_seulement = false);
    Instruction *cree_reference_membre_et_charge(NoeudExpression *site_, Atome *accede, int index);

    InstructionTranstype *cree_transtype(NoeudExpression *site_,
                                         Type const *type,
                                         Atome *valeur,
                                         TypeTranstypage op);

    TranstypeConstant *cree_transtype_constant(Type const *type, AtomeConstante *valeur);
    OpUnaireConstant *cree_op_unaire_constant(Type const *type,
                                              OperateurUnaire::Genre op,
                                              AtomeConstante *valeur);
    OpBinaireConstant *cree_op_binaire_constant(Type const *type,
                                                OperateurBinaire::Genre op,
                                                AtomeConstante *valeur_gauche,
                                                AtomeConstante *valeur_droite);
    OpBinaireConstant *cree_op_comparaison_constant(OperateurBinaire::Genre op,
                                                    AtomeConstante *valeur_gauche,
                                                    AtomeConstante *valeur_droite);
    AccedeIndexConstant *cree_acces_index_constant(AtomeConstante *accede, AtomeConstante *index);

    AtomeConstante *cree_info_type(Type const *type, NoeudExpression *site);
    AtomeConstante *transtype_base_info_type(AtomeConstante *info_type);

    void genere_ri_pour_initialisation_globales(EspaceDeTravail *espace,
                                                AtomeFonction *fonction_init,
                                                const kuri::tableau<AtomeGlobale *> &globales);

    void genere_ri_pour_initialisation_globales(AtomeFonction *fonction_init,
                                                const kuri::tableau<AtomeGlobale *> &globales);

  private:
    void cree_appel_fonction_init_type(NoeudExpression *site_, Type const *type, Atome *argument);

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

    void remplis_membres_de_bases_info_type(kuri::tableau<AtomeConstante *> &valeurs,
                                            unsigned int index,
                                            Type const *pour_type);
    AtomeConstante *cree_info_type_defaut(unsigned index, Type const *pour_type);
    AtomeConstante *cree_info_type_entier(Type const *pour_type, bool est_relatif);
    AtomeConstante *cree_info_type_avec_transtype(Type const *type, NoeudExpression *site);
    AtomeConstante *cree_globale_info_type(Type const *type_info_type,
                                           kuri::tableau<AtomeConstante *> &&valeurs);

    Atome *converti_vers_tableau_dyn(NoeudExpression *noeud,
                                     Atome *pointeur_tableau_fixe,
                                     TypeTableauFixe *type_tableau_fixe,
                                     Atome *place);

    AtomeConstante *cree_chaine(kuri::chaine_statique chaine);

    Atome *valeur_enum(TypeEnum *type_enum, IdentifiantCode *ident);

    void empile_valeur(Atome *valeur);
    Atome *depile_valeur();

    AtomeConstante *cree_tableau_annotations_pour_info_membre(
        const kuri::tableau<Annotation, int> &annotations);

    /* pour pouvoir accéder aux tableaux d'instructions */
    friend struct CopieuseInstruction;
};
