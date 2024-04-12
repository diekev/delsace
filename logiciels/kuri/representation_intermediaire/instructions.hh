/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <utility>

#include "compilation/operateurs.hh"
#include "compilation/typage.hh"

#include "structures/chaine_statique.hh"
#include "structures/ensemble.hh"

#include "utilitaires/macros.hh"

struct DonnéesExécutionFonction;
struct IdentifiantCode;
struct NoeudDéclarationType;
using Type = NoeudDéclarationType;

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
    O(CONSTANTE_INDEX_TABLE_TYPE, AtomeIndexTableType, index_table_type)                          \
    O(CONSTANTE_TAILLE_DE, AtomeConstanteTailleDe, taille_de)                                     \
    O(INITIALISATION_TABLEAU, AtomeInitialisationTableau, initialisation_tableau)                 \
    O(NON_INITIALISATION, AtomeNonInitialisation, non_initialisation)                             \
    O(TRANSTYPE_CONSTANT, TranstypeConstant, transtype_constant)                                  \
    O(ACCÈS_INDEX_CONSTANT, AccèdeIndexConstant, accès_index_constant)                            \
    O(FONCTION, AtomeFonction, fonction)                                                          \
    O(INSTRUCTION, Instruction, instruction)                                                      \
    O(GLOBALE, AtomeGlobale, globale)

#define PREDECLARE_CLASSE_ATOME(genre_, nom_classe, ident) struct nom_classe;
ENUMERE_GENRE_ATOME(PREDECLARE_CLASSE_ATOME)
#undef PREDECLARE_CLASSE_ATOME

#define ENUMERE_GENRE_INSTRUCTION(O)                                                              \
    O(APPEL, InstructionAppel, appel)                                                             \
    O(ALLOCATION, InstructionAllocation, alloc)                                                   \
    O(OPERATION_BINAIRE, InstructionOpBinaire, op_binaire)                                        \
    O(OPERATION_UNAIRE, InstructionOpUnaire, op_unaire)                                           \
    O(CHARGE_MEMOIRE, InstructionChargeMem, charge)                                               \
    O(STOCKE_MEMOIRE, InstructionStockeMem, stocke_mem)                                           \
    O(LABEL, InstructionLabel, label)                                                             \
    O(BRANCHE, InstructionBranche, branche)                                                       \
    O(BRANCHE_CONDITION, InstructionBrancheCondition, branche_cond)                               \
    O(RETOUR, InstructionRetour, retour)                                                          \
    O(ACCEDE_MEMBRE, InstructionAccèdeMembre, acces_membre)                                       \
    O(ACCEDE_INDEX, InstructionAccèdeIndex, acces_index)                                          \
    O(TRANSTYPE, InstructionTranstype, transtype)                                                 \
    O(INATTEIGNABLE, InstructionInatteignable, inatteignable)

#define PREDECLARE_CLASSE_INSTRUCTION(genre_, nom_classe, ident) struct nom_classe;
ENUMERE_GENRE_INSTRUCTION(PREDECLARE_CLASSE_INSTRUCTION)
#undef PREDECLARE_CLASSE_INSTRUCTION

enum class DrapeauxAtome : uint8_t {
    ZÉRO = 0,
    EST_À_SUPPRIMER = (1 << 0),
    EST_PARAMÈTRE_FONCTION = (1 << 1),
    /* Vrai si l'atome est celui d'une instruction chargeable (qui possède une adresse en mémoire).
     */
    EST_CHARGEABLE = (1 << 2),
    /* Utilisé pour détecter si nous devons ou non compléter la RI, dans le cas ou une fonction ou
       une globale fut créée préemptivement. */
    RI_FUT_GÉNÉRÉE = (1 << 3),
    EST_UTILISÉ = (1 << 4),
};
DEFINIS_OPERATEURS_DRAPEAU(DrapeauxAtome)

struct Atome {
    enum class Genre {
#define ENUMERE_GENRE_ATOME_EX(__genre, __type, __ident) __genre,
        ENUMERE_GENRE_ATOME(ENUMERE_GENRE_ATOME_EX)
#undef ENUMERE_GENRE_ATOME_EX
    };

    Type const *type = nullptr;

    Genre genre_atome{};
    DrapeauxAtome drapeaux = DrapeauxAtome::ZÉRO;

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

    inline bool possède_drapeau(DrapeauxAtome d) const
    {
        return (drapeaux & d) != DrapeauxAtome::ZÉRO;
    }
};

std::ostream &operator<<(std::ostream &os, Atome::Genre genre_atome);

struct AtomeConstante : public Atome {
    /* Ce type n'existe que pour différencier les atomes constantes des instructions (pour la
     * sûreté de type). */
};

struct AtomeConstanteEntière : public AtomeConstante {
    const uint64_t valeur = 0;

    AtomeConstanteEntière(Type const *type_, uint64_t v) : valeur(v)
    {
        type = type_;
        genre_atome = Genre::CONSTANTE_ENTIÈRE;
    }
};

struct AtomeConstanteRéelle : public AtomeConstante {
    const double valeur = 0;

    AtomeConstanteRéelle(Type const *type_, double v) : valeur(v)
    {
        type = type_;
        genre_atome = Genre::CONSTANTE_RÉELLE;
    }
};

struct AtomeConstanteBooléenne : public AtomeConstante {
    const bool valeur = false;

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
    const uint64_t valeur = 0;

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
        int64_t capacité = 0;
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
        int64_t capacité = 0;
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

struct AtomeIndexTableType : public AtomeConstante {
    Type const *type_de_données = nullptr;

    AtomeIndexTableType(Type const *type_, Type const *type_de_données_)
        : type_de_données(type_de_données_)
    {
        type = type_;
        genre_atome = Genre::CONSTANTE_INDEX_TABLE_TYPE;
    }

    EMPECHE_COPIE(AtomeIndexTableType);
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
        drapeaux |= DrapeauxAtome::EST_CHARGEABLE;
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

    NoeudDéclarationVariable *decl = nullptr;

    EMPECHE_COPIE(AtomeGlobale);

    AtomeGlobale(IdentifiantCode *ident_,
                 Type const *type_,
                 AtomeConstante *initialisateur_,
                 bool est_externe_,
                 bool est_constante_)
        : AtomeGlobale()
    {
        this->ident = ident_;
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

struct AccèdeIndexConstant : public AtomeConstante {
    AccèdeIndexConstant()
    {
        genre_atome = Genre::ACCÈS_INDEX_CONSTANT;
    }

    AtomeConstante *accédé = nullptr;
    int64_t index = 0;

    EMPECHE_COPIE(AccèdeIndexConstant);

    AccèdeIndexConstant(Type const *type_, AtomeConstante *accédé_, int64_t index_)
        : AccèdeIndexConstant()
    {
        this->type = type_;
        this->accédé = accédé_;
        this->index = index_;
    }

    Type const *donne_type_accédé() const;
};

struct AtomeFonction : public AtomeConstante {
    kuri::chaine_statique nom{};

    kuri::tableau<InstructionAllocation *, int> params_entrée{};
    InstructionAllocation *param_sortie = nullptr;

    kuri::tableau<Instruction *, int> instructions{};

    bool sanstrace = false;
    bool est_externe = false;
    bool enligne = false;
    NoeudDéclarationEntêteFonction const *decl = nullptr;

    /* Pour les exécutions dans la machine virtuelle. */
    DonnéesExécutionFonction *données_exécution = nullptr;

    int64_t décalage_appel_init_globale = 0;

    AtomeGlobale *info_trace_appel = nullptr;

    AtomeFonction(NoeudDéclarationEntêteFonction const *decl_, kuri::chaine_statique nom_)
        : nom(nom_), decl(decl_)
    {
        genre_atome = Genre::FONCTION;
    }

    AtomeFonction(NoeudDéclarationEntêteFonction const *decl_,
                  kuri::chaine_statique nom_,
                  kuri::tableau<InstructionAllocation *, int> &&params_,
                  InstructionAllocation *param_sortie_)
        : AtomeFonction(decl_, nom_)
    {
        this->params_entrée = std::move(params_);
        this->param_sortie = param_sortie_;
    }

    ~AtomeFonction();

    Instruction *dernière_instruction() const;

    int nombre_d_instructions_avec_entrées_sorties() const;

    int32_t numérote_instructions() const;

    EMPECHE_COPIE(AtomeFonction);
};

#define DECLARE_FONCTIONS_DISCRIMINATION_INSTRUCTION(genre_, nom_classe, ident)                   \
    inline bool est_##ident() const                                                               \
    {                                                                                             \
        return this->genre == GenreInstruction::genre_;                                           \
    }                                                                                             \
    inline nom_classe *comme_##ident();                                                           \
    inline nom_classe const *comme_##ident() const;

#define DEFINIS_FONCTIONS_DISCRIMINATION_INSTRUCTION(genre_, nom_classe, ident)                   \
    inline nom_classe *Instruction::comme_##ident()                                               \
    {                                                                                             \
        assert(est_##ident());                                                                    \
        return static_cast<nom_classe *>(this);                                                   \
    }                                                                                             \
    inline nom_classe const *Instruction::comme_##ident() const                                   \
    {                                                                                             \
        assert(est_##ident());                                                                    \
        return static_cast<nom_classe const *>(this);                                             \
    }

enum class GenreInstruction : uint32_t {
#define ENUMERE_GENRE_INSTRUCTION_EX(Genre, nom_classe, ident) Genre,
    ENUMERE_GENRE_INSTRUCTION(ENUMERE_GENRE_INSTRUCTION_EX)
#undef ENUMERE_GENRE_INSTRUCTION_EX
};

std::ostream &operator<<(std::ostream &os, GenreInstruction genre);

struct Instruction : public Atome {
    GenreInstruction genre{};
    int numero = 0;
    NoeudExpression const *site = nullptr;

    Instruction()
    {
        genre_atome = Genre::INSTRUCTION;
    }

#define ENUMERE_GENRE_INSTRUCTION_EX(Genre, nom_classe, ident) Genre,
    ENUMERE_GENRE_INSTRUCTION(DECLARE_FONCTIONS_DISCRIMINATION_INSTRUCTION)
#undef ENUMERE_GENRE_INSTRUCTION_EX

    inline bool est_terminatrice() const
    {
        return est_branche() || est_branche_cond() || est_retour() || est_inatteignable();
    }
};

struct InstructionAppel : public Instruction {
    explicit InstructionAppel(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::APPEL;
    }

    Atome *appelé = nullptr;
    kuri::tableau<Atome *, int> args{};
    AtomeGlobale *info_trace_appel = nullptr;

    EMPECHE_COPIE(InstructionAppel);

    InstructionAppel(NoeudExpression const *site_, Atome *appele_);

    InstructionAppel(NoeudExpression const *site_,
                     Atome *appele_,
                     kuri::tableau<Atome *, int> &&args_);
};

struct InstructionAllocation : public Instruction {
    explicit InstructionAllocation(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::ALLOCATION;
        drapeaux |= DrapeauxAtome::EST_CHARGEABLE;
    }

    IdentifiantCode *ident = nullptr;

    InstructionAllocation(NoeudExpression const *site_,
                          Type const *type_,
                          IdentifiantCode *ident_);

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

    InstructionRetour(NoeudExpression const *site_, Atome *valeur_);
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
                         Atome *valeur_droite_);
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
                        Atome *valeur_);
};

struct InstructionChargeMem : public Instruction {
    explicit InstructionChargeMem(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::CHARGE_MEMOIRE;
    }

    Atome *chargée = nullptr;

    EMPECHE_COPIE(InstructionChargeMem);

    InstructionChargeMem(NoeudExpression const *site_, Type const *type_, Atome *chargee_);
};

struct InstructionStockeMem : public Instruction {
    explicit InstructionStockeMem(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::STOCKE_MEMOIRE;
    }

    Atome *destination = nullptr;
    Atome *source = nullptr;

    EMPECHE_COPIE(InstructionStockeMem);

    InstructionStockeMem(NoeudExpression const *site_, Atome *ou_, Atome *valeur_);
};

struct InstructionLabel : public Instruction {
    explicit InstructionLabel(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::LABEL;
    }

    int id = 0;

    InstructionLabel(NoeudExpression const *site_, int id_);
};

struct InstructionBranche : public Instruction {
    explicit InstructionBranche(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::BRANCHE;
    }

    InstructionLabel *label = nullptr;

    EMPECHE_COPIE(InstructionBranche);

    InstructionBranche(NoeudExpression const *site_, InstructionLabel *label_);
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
                                InstructionLabel *label_si_faux_);
};

struct InstructionAccèdeMembre : public Instruction {
    explicit InstructionAccèdeMembre(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::ACCEDE_MEMBRE;
        drapeaux |= DrapeauxAtome::EST_CHARGEABLE;
    }

    Atome *accédé = nullptr;
    /* Index du membre accédé dans le type structurel accédé. */
    int index = 0;

    EMPECHE_COPIE(InstructionAccèdeMembre);

    InstructionAccèdeMembre(NoeudExpression const *site_,
                            Type const *type_,
                            Atome *accede_,
                            int index_);

    const Type *donne_type_accédé() const;

    const MembreTypeComposé &donne_membre_accédé() const;
};

struct InstructionAccèdeIndex : public Instruction {
    explicit InstructionAccèdeIndex(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::ACCEDE_INDEX;
        drapeaux |= DrapeauxAtome::EST_CHARGEABLE;
    }

    Atome *accédé = nullptr;
    Atome *index = nullptr;

    EMPECHE_COPIE(InstructionAccèdeIndex);

    InstructionAccèdeIndex(NoeudExpression const *site_,
                           Type const *type_,
                           Atome *accede_,
                           Atome *index_);

    const Type *donne_type_accédé() const;
};

#define ENUMERE_TYPE_TRANSTYPAGE(O)                                                               \
    O(AUGMENTE_NATUREL, augmente_naturel)                                                         \
    O(AUGMENTE_RELATIF, augmente_relatif)                                                         \
    O(AUGMENTE_REEL, augmente_réel)                                                               \
    O(DIMINUE_NATUREL, diminue_naturel)                                                           \
    O(DIMINUE_RELATIF, diminue_relatif)                                                           \
    O(DIMINUE_REEL, diminue_réel)                                                                 \
    O(AUGMENTE_NATUREL_VERS_RELATIF, augmente_naturel_vers_relatif)                               \
    O(AUGMENTE_RELATIF_VERS_NATUREL, augmente_relatif_vers_naturel)                               \
    O(DIMINUE_NATUREL_VERS_RELATIF, diminue_naturel_vers_relatif)                                 \
    O(DIMINUE_RELATIF_VERS_NATUREL, diminue_relatif_vers_naturel)                                 \
    O(POINTEUR_VERS_ENTIER, pointeur_vers_entier)                                                 \
    O(ENTIER_VERS_POINTEUR, entier_vers_pointeur)                                                 \
    O(REEL_VERS_ENTIER_RELATIF, réel_vers_relatif)                                                \
    O(REEL_VERS_ENTIER_NATUREL, réel_vers_naturel)                                                \
    O(ENTIER_RELATIF_VERS_REEL, relatif_vers_réel)                                                \
    O(ENTIER_NATUREL_VERS_REEL, naturel_vers_réel)                                                \
    O(BITS, transtype_bits)

enum TypeTranstypage {
#define ENUMERE_TYPE_TRANSTYPAGE_EX(genre, ident) genre,
    ENUMERE_TYPE_TRANSTYPAGE(ENUMERE_TYPE_TRANSTYPAGE_EX)
#undef ENUMERE_TYPE_TRANSTYPAGE_EX
};

kuri::chaine_statique chaine_pour_type_transtypage(TypeTranstypage const type);

TypeTranstypage type_transtypage_depuis_ident(IdentifiantCode const *ident);

struct InstructionTranstype : public Instruction {
    explicit InstructionTranstype(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::TRANSTYPE;
    }

    Atome *valeur = nullptr;
    TypeTranstypage op{};

    EMPECHE_COPIE(InstructionTranstype);

    InstructionTranstype(NoeudExpression const *site_,
                         Type const *type_,
                         Atome *valeur_,
                         TypeTranstypage op_);
};

struct InstructionInatteignable : public Instruction {
    explicit InstructionInatteignable(NoeudExpression const *site_)
    {
        site = site_;
        genre = GenreInstruction::INATTEIGNABLE;
    }

    EMPECHE_COPIE(InstructionInatteignable);
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

/**
 * Retourne vrai si \a atome est une constante nul, en incluant les transtypes potentiels vers un
 * type pointeur.
 */
bool est_constante_pointeur_nul(Atome const *atome);

/**
 * Retourne vrai si l'instruction est la racine d'un arbre.
 * Utilisée par les coulisses ne générant pas le code linéairement.
 */
bool instruction_est_racine(Instruction const *inst);

/** Si l'instruction est de forme `x == 0` ou `x == nul`, retourne x.
 *  Sinon retourne nul. */
Atome const *est_comparaison_égal_zéro_ou_nul(Instruction const *inst);

/** Si l'instruction est de forme `x != 0` ou `x != nul`, retourne x.
 *  Sinon retourne nul. */
Atome const *est_comparaison_inégal_zéro_ou_nul(Instruction const *inst);

/** Retourne vrai si l'atome est une instruction binaire de comparaison.
 */
bool est_instruction_comparaison(Atome const *atome);

struct AccèsMembreFusionné {
    Atome *accédé = nullptr;
    uint32_t décalage = 0;
};

/* "Fusionne" les accès de membre consécutifs (x.y.z).
 * Retourne l'atome accédé à la fin de la chaine ainsi que le décalage total. */
AccèsMembreFusionné fusionne_accès_membres(InstructionAccèdeMembre const *accès_membre);

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

ENUMERE_GENRE_INSTRUCTION(DEFINIS_FONCTIONS_DISCRIMINATION_INSTRUCTION)

struct VisiteuseAtome {
    /* Les atomes peuvent avoir des dépendances cycliques, donc tenons trace de ceux qui ont été
     * visités. */
    kuri::ensemble<Atome *> visites{};

    void réinitialise();

    void visite_atome(Atome *racine, std::function<void(Atome *)> rappel);
};

/* Visite récursivement l'atome. */
void visite_atome(Atome *racine, std::function<void(Atome *)> rappel);

bool est_tableau_données_constantes(AtomeConstante const *constante);
bool est_globale_pour_tableau_données_constantes(AtomeGlobale const *globale);

/* ------------------------------------------------------------------------- */
/** \name Drapeaux pour déterminer où est utilisé un atome.
 *  Peut être utilisé par les générateurs de code.
 * \{ */

enum class UtilisationAtome : uint32_t {
    AUCUNE = 0,
    RACINE = (1u << 0),
    POUR_GLOBALE = (1u << 1),
    POUR_BRANCHE_CONDITION = (1u << 2),

    /* Opérandes de stocke. */
    POUR_SOURCE_ÉCRITURE = (1u << 3),
    POUR_DESTINATION_ÉCRITURE = (1u << 4),

    /* Opérande d'un opérateur binaire ou unaire. */
    POUR_OPÉRATEUR = (1u << 5),

    /* Opérande de charge. */
    POUR_LECTURE = (1u << 6),

    POUR_OPÉRANDE = (POUR_BRANCHE_CONDITION | POUR_SOURCE_ÉCRITURE | POUR_DESTINATION_ÉCRITURE),
};
DEFINIS_OPERATEURS_DRAPEAU(UtilisationAtome)

inline bool est_drapeau_actif(UtilisationAtome const utilisation, UtilisationAtome const drapeau)
{
    return (utilisation & drapeau) != UtilisationAtome::AUCUNE;
}

std::ostream &operator<<(std::ostream &os, UtilisationAtome const utilisation);

/** \} */
