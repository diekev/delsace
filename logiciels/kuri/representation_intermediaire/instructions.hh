/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <utility>

#include "compilation/operateurs.hh"
#include "compilation/typage.hh"

#include "structures/chaine_statique.hh"
#include "structures/ensemble.hh"

#include "utilitaires/macros.hh"

struct AccedeIndexConstant;
struct AtomeConstanteBooléenne;
struct AtomeConstanteCaractère;
struct AtomeConstanteDonnéesConstantes;
struct AtomeConstanteEntière;
struct AtomeConstanteNulle;
struct AtomeConstanteRéelle;
struct AtomeConstanteStructure;
struct AtomeConstanteTableauFixe;
struct AtomeConstanteTailleDe;
struct AtomeConstanteType;
struct AtomeFonction;
struct AtomeInitialisationTableau;
struct AtomeNonInitialisation;
struct TranstypeConstant;
struct DonnéesExécutionFonction;
struct IdentifiantCode;
struct Instruction;
struct InstructionAccedeIndex;
struct InstructionAccedeMembre;
struct InstructionAllocation;
struct InstructionAppel;
struct InstructionBranche;
struct InstructionBrancheCondition;
struct InstructionChargeMem;
struct InstructionLabel;
struct InstructionOpBinaire;
struct InstructionOpUnaire;
struct InstructionRetour;
struct InstructionStockeMem;
struct InstructionTranstype;
struct Type;

enum class VisibilitéSymbole : uint8_t;

#define ENUMERE_GENRE_ATOME(O)                                                                    \
    O(CONSTANTE_ENTIÈRE, AtomeConstanteEntière, constante_entière)                                \
    O(CONSTANTE_RÉELLE, AtomeConstanteRéelle, constante_réelle)                                   \
    O(CONSTANTE_BOOLÉENNE, AtomeConstanteBooléenne, constante_booléenne)                          \
    O(CONSTANTE_NULLE, AtomeConstanteNulle, constante_nulle)                                      \
    O(CONSTANTE_CARACTÈRE, AtomeConstanteCaractère, constante_caractère)                          \
    O(CONSTANTE_STRUCTURE, AtomeConstanteStructure, constante_structure)                          \
    O(CONSTANTE_TABLEAU_FIXE, AtomeConstanteTableauFixe, constante_tableau)                       \
    O(CONSTANTE_DONNÉES_CONSTANTES, AtomeConstanteDonnéesConstantes, données_constantes)          \
    O(CONSTANTE_TYPE, AtomeConstanteType, constante_type)                                         \
    O(CONSTANTE_TAILLE_DE, AtomeConstanteTailleDe, taille_de)                                     \
    O(INITIALISATION_TABLEAU, AtomeInitialisationTableau, initialisation_tableau)                 \
    O(NON_INITIALISATION, AtomeNonInitialisation, non_initialisation)                             \
    O(TRANSTYPE_CONSTANT, TranstypeConstant, transtype_constant)                                  \
    O(ACCÈS_INDEX_CONSTANT, AccedeIndexConstant, accès_index_constant)                            \
    O(FONCTION, AtomeFonction, fonction)                                                          \
    O(INSTRUCTION, Instruction, instruction)                                                      \
    O(GLOBALE, AtomeGlobale, globale)

struct Atome {
    enum class Genre {
#define ENUMERE_GENRE_ATOME_EX(__genre, __type, __ident) __genre,
        ENUMERE_GENRE_ATOME(ENUMERE_GENRE_ATOME_EX)
#undef ENUMERE_GENRE_ATOME_EX
    };

    Type const *type = nullptr;

    Genre genre_atome{};
    // vrai si l'atome est celui d'une instruction chargeable
    bool est_chargeable = false;
    bool ri_generee = false;

    int nombre_utilisations = 0;

    // machine à état utilisée pour déterminer si un atome a été utilisé ou non
    int etat = 0;

#define ENUMERE_GENRE_ATOME_EX(__genre, __type, __ident)                                          \
    inline __type *comme_##__ident();                                                             \
    inline __type const *comme_##__ident() const;                                                 \
    inline bool est_##__ident() const;
    ENUMERE_GENRE_ATOME(ENUMERE_GENRE_ATOME_EX)
#undef ENUMERE_GENRE_ATOME_EX

    inline bool est_constante() const
    {
        return genre_atome >= Genre::CONSTANTE_ENTIÈRE &&
               genre_atome <= Genre::CONSTANTE_TAILLE_DE;
    }
};

std::ostream &operator<<(std::ostream &os, Atome::Genre genre_atome);

struct AtomeConstante : public Atome {
    /* Ce type n'existe que pour différencier les atomes constantes des instructions (pour la
     * sûreté de type). */
};

struct AtomeConstanteEntière : public AtomeConstante {
    uint64_t valeur = 0;

    AtomeConstanteEntière(Type const *type_, uint64_t v) : valeur(v)
    {
        type = type_;
        genre_atome = Genre::CONSTANTE_ENTIÈRE;
    }
};

struct AtomeConstanteRéelle : public AtomeConstante {
    double valeur = 0;

    AtomeConstanteRéelle(Type const *type_, double v) : valeur(v)
    {
        type = type_;
        genre_atome = Genre::CONSTANTE_RÉELLE;
    }
};

struct AtomeConstanteBooléenne : public AtomeConstante {
    bool valeur = false;

    AtomeConstanteBooléenne(Type const *type_, bool v) : valeur(v)
    {
        type = type_;
        genre_atome = Genre::CONSTANTE_BOOLÉENNE;
    }
};

struct AtomeConstanteNulle : public AtomeConstante {
    AtomeConstanteNulle(Type const *type_)
    {
        type = type_;
        genre_atome = Genre::CONSTANTE_NULLE;
    }
};

struct AtomeConstanteCaractère : public AtomeConstante {
    uint64_t valeur = 0;

    AtomeConstanteCaractère(Type const *type_, uint64_t v) : valeur(v)
    {
        type = type_;
        genre_atome = Genre::CONSTANTE_CARACTÈRE;
    }
};

struct AtomeConstanteStructure : public AtomeConstante {
  private:
    struct Données {
        AtomeConstante **pointeur = nullptr;
        int64_t taille = 0;
        int64_t capacite = 0;
    };

    Données données{};

  public:
    AtomeConstanteStructure(Type const *type_, kuri::tableau<AtomeConstante *> &&valeurs)
    {
        type = type_;
        genre_atome = Genre::CONSTANTE_STRUCTURE;
        auto tableau = reinterpret_cast<kuri::tableau<AtomeConstante *> *>(&this->données);
        tableau->permute(valeurs);
    }

    ~AtomeConstanteStructure();

    kuri::tableau_statique<AtomeConstante *> donne_atomes_membres() const
    {
        return {données.pointeur, données.taille};
    }
};

struct AtomeConstanteTableauFixe : public AtomeConstante {
  private:
    struct Données {
        AtomeConstante **pointeur = nullptr;
        int64_t taille = 0;
        int64_t capacite = 0;
    };

    Données données{};

  public:
    AtomeConstanteTableauFixe(Type const *type_, kuri::tableau<AtomeConstante *> &&valeurs)
    {
        type = type_;
        genre_atome = Genre::CONSTANTE_TABLEAU_FIXE;
        auto tableau = reinterpret_cast<kuri::tableau<AtomeConstante *> *>(&this->données);
        tableau->permute(valeurs);
    }

    ~AtomeConstanteTableauFixe();

    kuri::tableau_statique<AtomeConstante *> donne_atomes_éléments() const
    {
        return {données.pointeur, données.taille};
    }
};

struct AtomeConstanteDonnéesConstantes : public AtomeConstante {
  private:
    struct Données {
        char *pointeur = nullptr;
        int64_t taille = 0;
        int64_t capacité = 0;
    };

    Données données{};

    AtomeConstanteDonnéesConstantes(Type const *type_)
    {
        type = type_;
        genre_atome = Genre::CONSTANTE_DONNÉES_CONSTANTES;
    }

  public:
    AtomeConstanteDonnéesConstantes(Type const *type_, char *pointeur, int64_t taille)
        : AtomeConstanteDonnéesConstantes(type_)
    {
        données.pointeur = pointeur;
        données.taille = taille;
    }

    AtomeConstanteDonnéesConstantes(Type const *type_, kuri::tableau<char> &&donnees_constantes)
        : AtomeConstanteDonnéesConstantes(type_)
    {
        auto valeur_tdc = reinterpret_cast<kuri::tableau<char> *>(&this->données);
        valeur_tdc->permute(donnees_constantes);
    }

    kuri::tableau_statique<char> donne_données() const
    {
        return {données.pointeur, données.taille};
    }
};

struct AtomeConstanteType : public AtomeConstante {
    Type const *type_de_données = nullptr;

    AtomeConstanteType(Type const *type_, Type const *type_de_données_)
        : type_de_données(type_de_données_)
    {
        type = type_;
        genre_atome = Genre::CONSTANTE_TYPE;
    }

    EMPECHE_COPIE(AtomeConstanteType);
};

struct AtomeConstanteTailleDe : public AtomeConstante {
    Type const *type_de_données = nullptr;

    AtomeConstanteTailleDe(Type const *type_, Type const *type_de_données_)
        : type_de_données(type_de_données_)
    {
        type = type_;
        genre_atome = Genre::CONSTANTE_TAILLE_DE;
    }

    EMPECHE_COPIE(AtomeConstanteTailleDe);
};

/* Pour initialiser les tableaux constants avec une même valeur. */
struct AtomeInitialisationTableau : public AtomeConstante {
    /* La valeur à répéter (le compte est celui de la taille du tableau). */
    AtomeConstante const *valeur = nullptr;

    AtomeInitialisationTableau(Type const *type_, AtomeConstante const *valeur_) : valeur(valeur_)
    {
        type = type_;
        genre_atome = Genre::INITIALISATION_TABLEAU;
    }
};

struct AtomeNonInitialisation : public AtomeConstante {
    AtomeNonInitialisation()
    {
        genre_atome = Genre::NON_INITIALISATION;
    }
};

struct AtomeGlobale : public AtomeConstante {
    AtomeGlobale()
    {
        genre_atome = Genre::GLOBALE;
        est_chargeable = true;
    }

    IdentifiantCode *ident = nullptr;
    AtomeConstante *initialisateur{};
    bool est_externe = false;
    bool est_constante = false;

    // index de la globale pour le code binaire
    int index = -1;

    /* Non nul si la globale est une info type. Dans l'exécution des métaprogrammes, les globales
     * infos types ne sont pas converties en code binaire : nous les substituons par un objet
     * InfoType créé par la compilatrice. */
    const Type *est_info_type_de = nullptr;

    NoeudDeclarationVariable *decl = nullptr;

    EMPECHE_COPIE(AtomeGlobale);

    AtomeGlobale(Type const *type_,
                 AtomeConstante *initialisateur_,
                 bool est_externe_,
                 bool est_constante_)
        : AtomeGlobale()
    {
        this->type = type_;
        this->initialisateur = initialisateur_;
        this->est_externe = est_externe_;
        this->est_constante = est_constante_;
    }

    Type const *donne_type_alloué() const;

    VisibilitéSymbole donne_visibilité_symbole() const;
};

struct TranstypeConstant : public AtomeConstante {
    TranstypeConstant()
    {
        genre_atome = Genre::TRANSTYPE_CONSTANT;
    }

    AtomeConstante *valeur = nullptr;

    EMPECHE_COPIE(TranstypeConstant);

    TranstypeConstant(Type const *type_, AtomeConstante *valeur_) : TranstypeConstant()
    {
        this->type = type_;
        this->valeur = valeur_;
    }
};

struct AccedeIndexConstant : public AtomeConstante {
    AccedeIndexConstant()
    {
        genre_atome = Genre::ACCÈS_INDEX_CONSTANT;
    }

    AtomeConstante *accede = nullptr;
    int64_t index = 0;

    EMPECHE_COPIE(AccedeIndexConstant);

    AccedeIndexConstant(Type const *type_, AtomeConstante *accede_, int64_t index_)
        : AccedeIndexConstant()
    {
        this->type = type_;
        this->accede = accede_;
        this->index = index_;
    }

    Type const *donne_type_accédé() const;
};

struct AtomeFonction : public AtomeConstante {
    kuri::chaine_statique nom{};

    kuri::tableau<InstructionAllocation *, int> params_entrees{};
    InstructionAllocation *param_sortie = nullptr;

    kuri::tableau<Instruction *, int> instructions{};

    bool sanstrace = false;
    bool est_externe = false;
    bool enligne = false;
    NoeudDeclarationEnteteFonction const *decl = nullptr;

    /* Pour les exécutions dans la machine virtuelle. */
    DonnéesExécutionFonction *données_exécution = nullptr;

    int64_t decalage_appel_init_globale = 0;

    AtomeGlobale *info_trace_appel = nullptr;

    AtomeFonction(NoeudDeclarationEnteteFonction const *decl_, kuri::chaine_statique nom_)
        : nom(nom_), decl(decl_)
    {
        genre_atome = Genre::FONCTION;
    }

    AtomeFonction(NoeudDeclarationEnteteFonction const *decl_,
                  kuri::chaine_statique nom_,
                  kuri::tableau<InstructionAllocation *, int> &&params_,
                  InstructionAllocation *param_sortie_)
        : AtomeFonction(decl_, nom_)
    {
        this->params_entrees = std::move(params_);
        this->param_sortie = param_sortie_;
    }

    ~AtomeFonction();

    Instruction *derniere_instruction() const;

    int nombre_d_instructions_avec_entrées_sorties() const;

    EMPECHE_COPIE(AtomeFonction);
};

#define ENUMERE_GENRE_INSTRUCTION(O)                                                              \
    O(INVALIDE)                                                                                   \
    O(APPEL)                                                                                      \
    O(ALLOCATION)                                                                                 \
    O(OPERATION_BINAIRE)                                                                          \
    O(OPERATION_UNAIRE)                                                                           \
    O(CHARGE_MEMOIRE)                                                                             \
    O(STOCKE_MEMOIRE)                                                                             \
    O(LABEL)                                                                                      \
    O(BRANCHE)                                                                                    \
    O(BRANCHE_CONDITION)                                                                          \
    O(RETOUR)                                                                                     \
    O(ACCEDE_MEMBRE)                                                                              \
    O(ACCEDE_INDEX)                                                                               \
    O(TRANSTYPE)

enum class GenreInstruction : uint32_t {
#define ENUMERE_GENRE_INSTRUCTION_EX(Genre) Genre,
    ENUMERE_GENRE_INSTRUCTION(ENUMERE_GENRE_INSTRUCTION_EX)
#undef ENUMERE_GENRE_INSTRUCTION_EX
};

std::ostream &operator<<(std::ostream &os, GenreInstruction genre);

struct Instruction : public Atome {
    GenreInstruction genre = GenreInstruction::INVALIDE;
    int numero = 0;
    NoeudExpression const *site = nullptr;

    Instruction()
    {
        genre_atome = Genre::INSTRUCTION;
    }

#define COMME_INST(Type, Genre)                                                                   \
    inline Type *comme_##Genre();                                                                 \
    inline Type const *comme_##Genre() const

    COMME_INST(InstructionAccedeIndex, acces_index);
    COMME_INST(InstructionAccedeMembre, acces_membre);
    COMME_INST(InstructionAllocation, alloc);
    COMME_INST(InstructionAppel, appel);
    COMME_INST(InstructionBranche, branche);
    COMME_INST(InstructionBrancheCondition, branche_cond);
    COMME_INST(InstructionChargeMem, charge);
    COMME_INST(InstructionLabel, label);
    COMME_INST(InstructionOpBinaire, op_binaire);
    COMME_INST(InstructionOpUnaire, op_unaire);
    COMME_INST(InstructionRetour, retour);
    COMME_INST(InstructionStockeMem, stocke_mem);
    COMME_INST(InstructionTranstype, transtype);

    inline bool est_acces_index() const
    {
        return genre == GenreInstruction::ACCEDE_INDEX;
    }
    inline bool est_acces_membre() const
    {
        return genre == GenreInstruction::ACCEDE_MEMBRE;
    }
    inline bool est_alloc() const
    {
        return genre == GenreInstruction::ALLOCATION;
    }
    inline bool est_appel() const
    {
        return genre == GenreInstruction::APPEL;
    }
    inline bool est_branche() const
    {
        return genre == GenreInstruction::BRANCHE;
    }
    inline bool est_branche_cond() const
    {
        return genre == GenreInstruction::BRANCHE_CONDITION;
    }
    inline bool est_charge() const
    {
        return genre == GenreInstruction::CHARGE_MEMOIRE;
    }
    inline bool est_label() const
    {
        return genre == GenreInstruction::LABEL;
    }
    inline bool est_op_binaire() const
    {
        return genre == GenreInstruction::OPERATION_BINAIRE;
    }
    inline bool est_op_unaire() const
    {
        return genre == GenreInstruction::OPERATION_UNAIRE;
    }
    inline bool est_retour() const
    {
        return genre == GenreInstruction::RETOUR;
    }
    inline bool est_stocke_mem() const
    {
        return genre == GenreInstruction::STOCKE_MEMOIRE;
    }
    inline bool est_transtype() const
    {
        return genre == GenreInstruction::TRANSTYPE;
    }

    inline bool est_branche_ou_retourne() const
    {
        return est_branche() || est_branche_cond() || est_retour();
    }

#undef COMME_INST
};

struct InstructionAppel : public Instruction {
    explicit InstructionAppel(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::APPEL;
    }

    Atome *appele = nullptr;
    kuri::tableau<Atome *, int> args{};
    InstructionAllocation *adresse_retour = nullptr;
    AtomeGlobale *info_trace_appel = nullptr;

    EMPECHE_COPIE(InstructionAppel);

    InstructionAppel(NoeudExpression const *site_, Atome *appele_) : InstructionAppel(site_)
    {
        auto type_fonction = appele_->type->comme_type_fonction();
        this->type = type_fonction->type_sortie;

        this->appele = appele_;
    }

    InstructionAppel(NoeudExpression const *site_,
                     Atome *appele_,
                     kuri::tableau<Atome *, int> &&args_)
        : InstructionAppel(site_, appele_)
    {
        this->args = std::move(args_);
    }
};

struct InstructionAllocation : public Instruction {
    explicit InstructionAllocation(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::ALLOCATION;
        est_chargeable = true;
    }

    IdentifiantCode *ident = nullptr;

    InstructionAllocation(NoeudExpression const *site_, Type const *type_, IdentifiantCode *ident_)
        : InstructionAllocation(site_)
    {
        this->type = type_;
        this->ident = ident_;
    }

    const Type *donne_type_alloué() const;
};

struct InstructionRetour : public Instruction {
    explicit InstructionRetour(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::RETOUR;
    }

    Atome *valeur = nullptr;

    EMPECHE_COPIE(InstructionRetour);

    InstructionRetour(NoeudExpression const *site_, Atome *valeur_) : InstructionRetour(site_)
    {
        this->valeur = valeur_;
    }
};

struct InstructionOpBinaire : public Instruction {
    explicit InstructionOpBinaire(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::OPERATION_BINAIRE;
    }

    OpérateurBinaire::Genre op{};
    Atome *valeur_gauche = nullptr;
    Atome *valeur_droite = nullptr;

    EMPECHE_COPIE(InstructionOpBinaire);

    InstructionOpBinaire(NoeudExpression const *site_,
                         Type const *type_,
                         OpérateurBinaire::Genre op_,
                         Atome *valeur_gauche_,
                         Atome *valeur_droite_)
        : InstructionOpBinaire(site_)
    {
        this->type = type_;
        this->op = op_;
        this->valeur_gauche = valeur_gauche_;
        this->valeur_droite = valeur_droite_;
    }
};

struct InstructionOpUnaire : public Instruction {
    explicit InstructionOpUnaire(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::OPERATION_UNAIRE;
    }

    OpérateurUnaire::Genre op{};
    Atome *valeur = nullptr;

    EMPECHE_COPIE(InstructionOpUnaire);

    InstructionOpUnaire(NoeudExpression const *site_,
                        Type const *type_,
                        OpérateurUnaire::Genre op_,
                        Atome *valeur_)
        : InstructionOpUnaire(site_)
    {
        this->type = type_;
        this->op = op_;
        this->valeur = valeur_;
    }
};

struct InstructionChargeMem : public Instruction {
    explicit InstructionChargeMem(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::CHARGE_MEMOIRE;
        est_chargeable = true;
    }

    Atome *chargee = nullptr;

    EMPECHE_COPIE(InstructionChargeMem);

    InstructionChargeMem(NoeudExpression const *site_, Type const *type_, Atome *chargee_)
        : InstructionChargeMem(site_)
    {
        this->type = type_;
        this->chargee = chargee_;
        this->est_chargeable = type->est_type_pointeur();
    }
};

struct InstructionStockeMem : public Instruction {
    explicit InstructionStockeMem(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::STOCKE_MEMOIRE;
    }

    Atome *ou = nullptr;
    Atome *valeur = nullptr;

    EMPECHE_COPIE(InstructionStockeMem);

    InstructionStockeMem(NoeudExpression const *site_,
                         Type const *type_,
                         Atome *ou_,
                         Atome *valeur_)
        : InstructionStockeMem(site_)
    {
        this->type = type_;
        this->ou = ou_;
        this->valeur = valeur_;
    }
};

struct InstructionLabel : public Instruction {
    explicit InstructionLabel(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::LABEL;
    }

    int id = 0;

    InstructionLabel(NoeudExpression const *site_, int id_) : InstructionLabel(site_)
    {
        this->id = id_;
    }
};

struct InstructionBranche : public Instruction {
    explicit InstructionBranche(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::BRANCHE;
    }

    InstructionLabel *label = nullptr;

    EMPECHE_COPIE(InstructionBranche);

    InstructionBranche(NoeudExpression const *site_, InstructionLabel *label_)
        : InstructionBranche(site_)
    {
        this->label = label_;
    }
};

struct InstructionBrancheCondition : public Instruction {
    explicit InstructionBrancheCondition(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::BRANCHE_CONDITION;
    }

    Atome *condition = nullptr;
    InstructionLabel *label_si_vrai = nullptr;
    InstructionLabel *label_si_faux = nullptr;

    EMPECHE_COPIE(InstructionBrancheCondition);

    InstructionBrancheCondition(NoeudExpression const *site_,
                                Atome *condition_,
                                InstructionLabel *label_si_vrai_,
                                InstructionLabel *label_si_faux_)
        : InstructionBrancheCondition(site_)
    {
        this->condition = condition_;
        this->label_si_vrai = label_si_vrai_;
        this->label_si_faux = label_si_faux_;
    }
};

struct InstructionAccedeMembre : public Instruction {
    explicit InstructionAccedeMembre(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::ACCEDE_MEMBRE;
        est_chargeable = true;
    }

    Atome *accede = nullptr;
    /* Index du membre accédé dans le type structurel accédé. */
    int index = 0;

    EMPECHE_COPIE(InstructionAccedeMembre);

    InstructionAccedeMembre(NoeudExpression const *site_,
                            Type const *type_,
                            Atome *accede_,
                            int index_)
        : InstructionAccedeMembre(site_)
    {
        this->type = type_;
        this->accede = accede_;
        this->index = index_;
    }

    const Type *donne_type_accédé() const;
};

struct InstructionAccedeIndex : public Instruction {
    explicit InstructionAccedeIndex(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::ACCEDE_INDEX;
        est_chargeable = true;
    }

    Atome *accede = nullptr;
    Atome *index = nullptr;

    EMPECHE_COPIE(InstructionAccedeIndex);

    InstructionAccedeIndex(NoeudExpression const *site_,
                           Type const *type_,
                           Atome *accede_,
                           Atome *index_)
        : InstructionAccedeIndex(site_)
    {
        this->type = type_;
        this->accede = accede_;
        this->index = index_;
    }

    const Type *donne_type_accédé() const;
};

enum TypeTranstypage {
    AUGMENTE_NATUREL,
    AUGMENTE_RELATIF,
    AUGMENTE_REEL,
    DIMINUE_NATUREL,
    DIMINUE_RELATIF,
    DIMINUE_REEL,
    POINTEUR_VERS_ENTIER,
    ENTIER_VERS_POINTEUR,
    REEL_VERS_ENTIER,
    ENTIER_VERS_REEL,
    BITS,
    DEFAUT,  // À SUPPRIMER
};

struct InstructionTranstype : public Instruction {
    explicit InstructionTranstype(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::TRANSTYPE;
        est_chargeable = false;  // À FAIRE : uniquement si la valeur est un pointeur
    }

    Atome *valeur = nullptr;
    TypeTranstypage op{};

    EMPECHE_COPIE(InstructionTranstype);

    InstructionTranstype(NoeudExpression const *site_,
                         Type const *type_,
                         Atome *valeur_,
                         TypeTranstypage op_)
        : InstructionTranstype(site_)
    {
        this->type = type_;
        this->valeur = valeur_;
        this->op = op_;
    }
};

bool est_valeur_constante(Atome const *atome);
bool est_constante_entière_zéro(Atome const *atome);
bool est_constante_entière_un(Atome const *atome);
bool est_allocation(Atome const *atome);
bool est_locale_ou_globale(Atome const *atome);

/**
 * Retourne vrai si \a inst0 est un stockage vers \a inst1.
 */
bool est_stockage_vers(Instruction const *inst0, Instruction const *inst1);

/**
 * Retourne vrai si \a inst0 est un transtypage de \a inst1.
 */
bool est_transtypage_de(Instruction const *inst0, Instruction const *inst1);

/**
 * Retourne vrai si \a inst0 est un chargement de \a inst1.
 */
bool est_chargement_de(Instruction const *inst0, Instruction const *inst1);

/**
 * Retourne vrai si \a inst correspond à `x = x + 1`.
 */
bool est_stocke_alloc_incrémente(InstructionStockeMem const *inst);

/**
 * Retourne vrai si \a inst est un opérateur binaire dont les opérandes sont des constantes.
 */
bool est_opérateur_binaire_constant(Instruction const *inst);

InstructionAllocation const *est_stocke_alloc_depuis_charge_alloc(
    InstructionStockeMem const *inst);

#define ENUMERE_GENRE_ATOME_EX(__genre, __type, __ident)                                          \
    inline __type *Atome::comme_##__ident()                                                       \
    {                                                                                             \
        assert(est_##__ident());                                                                  \
        return static_cast<__type *>(this);                                                       \
    }                                                                                             \
    inline __type const *Atome::comme_##__ident() const                                           \
    {                                                                                             \
        assert(est_##__ident());                                                                  \
        return static_cast<__type const *>(this);                                                 \
    }                                                                                             \
    inline bool Atome::est_##__ident() const                                                      \
    {                                                                                             \
        return genre_atome == Genre::__genre;                                                     \
    }
ENUMERE_GENRE_ATOME(ENUMERE_GENRE_ATOME_EX)
#undef ENUMERE_GENRE_ATOME_EX

#define COMME_INST(Type, Genre)                                                                   \
    inline Type *Instruction::comme_##Genre()                                                     \
    {                                                                                             \
        assert(est_##Genre());                                                                    \
        return static_cast<Type *>(this);                                                         \
    }                                                                                             \
    inline Type const *Instruction::comme_##Genre() const                                         \
    {                                                                                             \
        assert(est_##Genre());                                                                    \
        return static_cast<Type const *>(this);                                                   \
    }

COMME_INST(InstructionAccedeIndex, acces_index)
COMME_INST(InstructionAccedeMembre, acces_membre)
COMME_INST(InstructionAllocation, alloc)
COMME_INST(InstructionAppel, appel)
COMME_INST(InstructionBranche, branche)
COMME_INST(InstructionBrancheCondition, branche_cond)
COMME_INST(InstructionChargeMem, charge)
COMME_INST(InstructionLabel, label)
COMME_INST(InstructionOpBinaire, op_binaire)
COMME_INST(InstructionOpUnaire, op_unaire)
COMME_INST(InstructionRetour, retour)
COMME_INST(InstructionStockeMem, stocke_mem)
COMME_INST(InstructionTranstype, transtype)

#undef COMME_INST

struct VisiteuseAtome {
    /* Les atomes peuvent avoir des dépendances cycliques, donc tenons trace de ceux qui ont été
     * visités. */
    kuri::ensemble<Atome *> visites{};

    void reinitialise();

    void visite_atome(Atome *racine, std::function<void(Atome *)> rappel);
};

/* Visite récursivement l'atome. */
void visite_atome(Atome *racine, std::function<void(Atome *)> rappel);

/* Visite uniquement les opérandes de l'instruction, s'il y en a. */
void visite_opérandes_instruction(Instruction *inst, std::function<void(Atome *)> rappel);

bool est_tableau_données_constantes(AtomeConstante const *constante);
bool est_globale_pour_tableau_données_constantes(AtomeGlobale const *globale);
