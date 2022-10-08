/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <utility>

#include "biblinternes/outils/definitions.h"
#include "structures/ensemble.hh"

#include "compilation/operateurs.hh"
#include "compilation/typage.hh"

#include "code_binaire.hh"

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
struct Lexeme;
struct Type;

struct Atome {
    enum class Genre {
        CONSTANTE,
        FONCTION,
        INSTRUCTION,
        GLOBALE,
    };

    Type *type = nullptr;
    IdentifiantCode *ident = nullptr;

    Genre genre_atome{};
    // vrai si l'atome est celui d'une instruction chargeable
    bool est_chargeable = false;
    bool ri_generee = false;

    int nombre_utilisations = 0;

    // machine à état utilisée pour déterminer si un atome a été utilisé ou non
    int etat = 0;

    Atome() = default;

    COPIE_CONSTRUCT(Atome);

    inline Instruction *comme_instruction();
    inline Instruction const *comme_instruction() const;

    inline bool est_constante() const
    {
        return genre_atome == Genre::CONSTANTE;
    }
    inline bool est_fonction() const
    {
        return genre_atome == Genre::FONCTION;
    }
    inline bool est_globale() const
    {
        return genre_atome == Genre::GLOBALE;
    }
    inline bool est_instruction() const
    {
        return genre_atome == Genre::INSTRUCTION;
    }
};

struct AtomeConstante : public Atome {
    AtomeConstante()
    {
        genre_atome = Atome::Genre::CONSTANTE;
    }

    enum class Genre {
        GLOBALE,
        VALEUR,
        TRANSTYPE_CONSTANT,
        OP_BINAIRE_CONSTANTE,
        OP_UNAIRE_CONSTANTE,
        ACCES_INDEX_CONSTANT,
    };

    Genre genre{};
};

struct AtomeValeurConstante : public AtomeConstante {
    AtomeValeurConstante()
    {
        genre = Genre::VALEUR;
    }

    struct Valeur {
        union {
            unsigned long long valeur_entiere;
            double valeur_reelle;
            bool valeur_booleenne;
            struct {
                char *pointeur;
                long taille;
            } valeur_tdc;
            struct {
                AtomeConstante **pointeur;
                long taille;
                long capacite;
            } valeur_structure;
            struct {
                AtomeConstante **pointeur;
                long taille;
                long capacite;
            } valeur_tableau;
            Type *type;
        };

        enum class Genre {
            INDEFINIE,
            ENTIERE,
            REELLE,
            BOOLEENNE,
            NULLE,
            CARACTERE,
            STRUCTURE,
            TABLEAU_FIXE,
            TABLEAU_DONNEES_CONSTANTES,
            TYPE,
            TAILLE_DE,
        };

        Genre genre{};

        ~Valeur()
        {
            if (genre == Genre::STRUCTURE) {
                memoire::deloge_tableau(
                    "valeur_structure", valeur_structure.pointeur, valeur_structure.capacite);
            }

            if (genre == Genre::TABLEAU_FIXE) {
                memoire::deloge_tableau(
                    "valeur_tableau", valeur_tableau.pointeur, valeur_tableau.capacite);
            }
        }
    };

    Valeur valeur{};

    AtomeValeurConstante(Type *type_, unsigned long long valeur_) : AtomeValeurConstante()
    {
        this->type = type_;
        this->valeur.genre = Valeur::Genre::ENTIERE;
        this->valeur.valeur_entiere = valeur_;
    }

    AtomeValeurConstante(Type *type_, double valeur_) : AtomeValeurConstante()
    {
        this->type = type_;
        this->valeur.genre = Valeur::Genre::REELLE;
        this->valeur.valeur_reelle = valeur_;
    }

    AtomeValeurConstante(Type *type_, bool valeur_) : AtomeValeurConstante()
    {
        this->type = type_;
        this->valeur.genre = Valeur::Genre::BOOLEENNE;
        this->valeur.valeur_booleenne = valeur_;
    }

    AtomeValeurConstante(Type *type_) : AtomeValeurConstante()
    {
        this->type = type_;
        this->valeur.genre = Valeur::Genre::NULLE;
    }

    AtomeValeurConstante(Type *type_, Type *pointeur_type) : AtomeValeurConstante()
    {
        this->type = type_;
        this->valeur.genre = Valeur::Genre::TYPE;
        this->valeur.type = pointeur_type;
    }

    AtomeValeurConstante(Type *type_, kuri::tableau<char> &&donnees_constantes)
        : AtomeValeurConstante()
    {
        this->type = type_;
        this->valeur.genre = Valeur::Genre::TABLEAU_DONNEES_CONSTANTES;
        this->valeur.valeur_tdc.pointeur = nullptr;
        this->valeur.valeur_tdc.taille = 0;
        this->valeur.valeur_tdc.taille = 0;
        auto valeur_tdc = reinterpret_cast<kuri::tableau<char> *>(&this->valeur.valeur_tdc);
        valeur_tdc->permute(donnees_constantes);
    }

    AtomeValeurConstante(Type *type_, char *pointeur, long taille) : AtomeValeurConstante()
    {
        this->type = type_;
        this->valeur.genre = Valeur::Genre::TABLEAU_DONNEES_CONSTANTES;
        this->valeur.valeur_tdc.pointeur = pointeur;
        this->valeur.valeur_tdc.taille = taille;
    }

    AtomeValeurConstante(Type *type_, kuri::tableau<AtomeConstante *> &&valeurs)
        : AtomeValeurConstante()
    {
        this->type = type_;
        this->valeur.genre = Valeur::Genre::STRUCTURE;
        this->valeur.valeur_structure.pointeur = nullptr;
        this->valeur.valeur_structure.taille = 0;
        this->valeur.valeur_structure.taille = 0;
        auto valeur_structure = reinterpret_cast<kuri::tableau<AtomeConstante *> *>(
            &this->valeur.valeur_structure);
        valeur_structure->permute(valeurs);
    }
};

struct AtomeGlobale : public AtomeConstante {
    AtomeGlobale()
    {
        genre = Genre::GLOBALE;
        genre_atome = Atome::Genre::GLOBALE;
        est_chargeable = true;
    }

    AtomeConstante *initialisateur{};
    bool est_externe = false;
    bool est_constante = false;

    // index de la globale pour le code binaire
    int index = -1;

    /* Adresse spéciale pour les exécutions de métaprogrammes. Ceci est principalement pour les
     * infos-types où nous voulons une même adresse pour les NoeudCodes et le code binaire. Dans ce
     * cas l'adresse sera celle d'un InfoType créé par la compilatrice (créée notamment lors de la
     * conversion vers NoeudCode). */
    void *adresse_pour_execution = nullptr;

    COPIE_CONSTRUCT(AtomeGlobale);

    AtomeGlobale(Type *type_,
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
};

struct TranstypeConstant : public AtomeConstante {
    TranstypeConstant()
    {
        genre = Genre::TRANSTYPE_CONSTANT;
    }

    AtomeConstante *valeur = nullptr;

    COPIE_CONSTRUCT(TranstypeConstant);

    TranstypeConstant(Type *type_, AtomeConstante *valeur_) : TranstypeConstant()
    {
        this->type = type_;
        this->valeur = valeur_;
    }
};

struct OpBinaireConstant : public AtomeConstante {
    OpBinaireConstant()
    {
        genre = Genre::OP_BINAIRE_CONSTANTE;
    }

    OperateurBinaire::Genre op{};
    AtomeConstante *operande_gauche = nullptr;
    AtomeConstante *operande_droite = nullptr;

    COPIE_CONSTRUCT(OpBinaireConstant);

    OpBinaireConstant(Type *type_,
                      OperateurBinaire::Genre op_,
                      AtomeConstante *operande_gauche_,
                      AtomeConstante *operande_droite_)
        : OpBinaireConstant()
    {
        this->type = type_;
        this->op = op_;
        this->operande_gauche = operande_gauche_;
        this->operande_droite = operande_droite_;
    }
};

struct OpUnaireConstant : public AtomeConstante {
    OpUnaireConstant()
    {
        genre = Genre::OP_UNAIRE_CONSTANTE;
    }

    OperateurUnaire::Genre op{};
    AtomeConstante *operande = nullptr;

    COPIE_CONSTRUCT(OpUnaireConstant);

    OpUnaireConstant(Type *type_, OperateurUnaire::Genre op_, AtomeConstante *operande_)
        : OpUnaireConstant()
    {
        this->type = type_;
        this->op = op_;
        this->operande = operande_;
    }
};

struct AccedeIndexConstant : public AtomeConstante {
    AccedeIndexConstant()
    {
        genre = Genre::ACCES_INDEX_CONSTANT;
    }

    AtomeConstante *accede = nullptr;
    AtomeConstante *index = nullptr;

    COPIE_CONSTRUCT(AccedeIndexConstant);

    AccedeIndexConstant(Type *type_, AtomeConstante *accede_, AtomeConstante *index_)
        : AccedeIndexConstant()
    {
        this->type = type_;
        this->accede = accede_;
        this->index = index_;
    }
};

struct AtomeFonction : public Atome {
    kuri::chaine nom{};

    kuri::tableau<Atome *, int> params_entrees{};
    Atome *param_sortie = nullptr;

    kuri::tableau<Instruction *, int> instructions{};

    /* pour les traces d'appels */
    Lexeme const *lexeme = nullptr;

    bool sanstrace = false;
    bool est_externe = false;
    bool enligne = false;
    NoeudDeclarationEnteteFonction const *decl = nullptr;

    // Pour les exécutions
    Chunk chunk{};

    long decalage_appel_init_globale = 0;

    struct DonneesFonctionExterne {
        kuri::tablet<ffi_type *, 6> types_entrees{};
        ffi_cif cif{};
        void (*ptr_fonction)() = nullptr;
    };

    DonneesFonctionExterne donnees_externe{};

    AtomeFonction(Lexeme const *lexeme_, kuri::chaine const &nom_) : nom(nom_), lexeme(lexeme_)
    {
        genre_atome = Atome::Genre::FONCTION;
    }

    AtomeFonction(Lexeme const *lexeme_,
                  kuri::chaine const &nom_,
                  kuri::tableau<Atome *, int> &&params_)
        : AtomeFonction(lexeme_, nom_)
    {
        this->params_entrees = std::move(params_);
    }

    Instruction *derniere_instruction() const;

    COPIE_CONSTRUCT(AtomeFonction);
};

struct Instruction : public Atome {
    enum class Genre {
        INVALIDE,

        APPEL,
        ALLOCATION,
        OPERATION_BINAIRE,
        OPERATION_UNAIRE,
        CHARGE_MEMOIRE,
        STOCKE_MEMOIRE,
        LABEL,
        BRANCHE,
        BRANCHE_CONDITION,
        RETOUR,
        ACCEDE_MEMBRE,
        ACCEDE_INDEX,
        TRANSTYPE,
    };

    enum {
        SUPPRIME_INSTRUCTION = 1,
    };

    Genre genre = Genre::INVALIDE;
    int numero = 0;
    int drapeaux = 0;
    NoeudExpression *site = nullptr;

    Instruction()
    {
        genre_atome = Atome::Genre::INSTRUCTION;
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
        return genre == Genre::ACCEDE_INDEX;
    }
    inline bool est_acces_membre() const
    {
        return genre == Genre::ACCEDE_MEMBRE;
    }
    inline bool est_alloc() const
    {
        return genre == Genre::ALLOCATION;
    }
    inline bool est_appel() const
    {
        return genre == Genre::APPEL;
    }
    inline bool est_branche() const
    {
        return genre == Genre::BRANCHE;
    }
    inline bool est_branche_cond() const
    {
        return genre == Genre::BRANCHE_CONDITION;
    }
    inline bool est_charge() const
    {
        return genre == Genre::CHARGE_MEMOIRE;
    }
    inline bool est_label() const
    {
        return genre == Genre::LABEL;
    }
    inline bool est_op_binaire() const
    {
        return genre == Genre::OPERATION_BINAIRE;
    }
    inline bool est_op_unaire() const
    {
        return genre == Genre::OPERATION_UNAIRE;
    }
    inline bool est_retour() const
    {
        return genre == Genre::RETOUR;
    }
    inline bool est_stocke_mem() const
    {
        return genre == Genre::STOCKE_MEMOIRE;
    }
    inline bool est_transtype() const
    {
        return genre == Genre::TRANSTYPE;
    }

    inline bool est_branche_ou_retourne() const
    {
        return est_branche() || est_branche_cond() || est_retour();
    }

#undef COMME_INST
};

inline Instruction *Atome::comme_instruction()
{
    return static_cast<Instruction *>(this);
}

inline Instruction const *Atome::comme_instruction() const
{
    return static_cast<Instruction const *>(this);
}

struct InstructionAppel : public Instruction {
    explicit InstructionAppel(NoeudExpression *site_)
    {
        site = site_;
        genre = Instruction::Genre::APPEL;
    }

    Atome *appele = nullptr;
    kuri::tableau<Atome *, int> args{};
    InstructionAllocation *adresse_retour = nullptr;

    COPIE_CONSTRUCT(InstructionAppel);

    InstructionAppel(NoeudExpression *site_, Atome *appele_) : InstructionAppel(site_)
    {
        auto type_fonction = appele_->type->comme_fonction();
        this->type = type_fonction->type_sortie;

        this->appele = appele_;
    }

    InstructionAppel(NoeudExpression *site_, Atome *appele_, kuri::tableau<Atome *, int> &&args_)
        : InstructionAppel(site_, appele_)
    {
        this->args = std::move(args_);
    }
};

struct InstructionAllocation : public Instruction {
    explicit InstructionAllocation(NoeudExpression *site_)
    {
        site = site_;
        genre = Instruction::Genre::ALLOCATION;
        est_chargeable = true;
    }

    /* le nombre total de blocs utilisant cet allocation */
    int blocs_utilisants = 0;

    // pour la génération de code binaire, mise en place lors de la génération de celle-ci
    int index_locale = 0;

    // le décalage en octet où se trouve l'allocation sur la pile
    int decalage_pile = 0;

    InstructionAllocation(NoeudExpression *site_, Type *type_, IdentifiantCode *ident_)
        : InstructionAllocation(site_)
    {
        this->type = type_;
        this->ident = ident_;
    }
};

struct InstructionRetour : public Instruction {
    explicit InstructionRetour(NoeudExpression *site_)
    {
        site = site_;
        genre = Instruction::Genre::RETOUR;
    }

    Atome *valeur = nullptr;

    COPIE_CONSTRUCT(InstructionRetour);

    InstructionRetour(NoeudExpression *site_, Atome *valeur_) : InstructionRetour(site_)
    {
        this->valeur = valeur_;
    }
};

struct InstructionOpBinaire : public Instruction {
    explicit InstructionOpBinaire(NoeudExpression *site_)
    {
        site = site_;
        genre = Instruction::Genre::OPERATION_BINAIRE;
    }

    OperateurBinaire::Genre op{};
    Atome *valeur_gauche = nullptr;
    Atome *valeur_droite = nullptr;

    COPIE_CONSTRUCT(InstructionOpBinaire);

    InstructionOpBinaire(NoeudExpression *site_,
                         Type *type_,
                         OperateurBinaire::Genre op_,
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
    explicit InstructionOpUnaire(NoeudExpression *site_)
    {
        site = site_;
        genre = Instruction::Genre::OPERATION_UNAIRE;
    }

    OperateurUnaire::Genre op{};
    Atome *valeur = nullptr;

    COPIE_CONSTRUCT(InstructionOpUnaire);

    InstructionOpUnaire(NoeudExpression *site_,
                        Type *type_,
                        OperateurUnaire::Genre op_,
                        Atome *valeur_)
        : InstructionOpUnaire(site_)
    {
        this->type = type_;
        this->op = op_;
        this->valeur = valeur_;
    }
};

struct InstructionChargeMem : public Instruction {
    explicit InstructionChargeMem(NoeudExpression *site_)
    {
        site = site_;
        genre = Instruction::Genre::CHARGE_MEMOIRE;
        est_chargeable = true;
    }

    Atome *chargee = nullptr;

    COPIE_CONSTRUCT(InstructionChargeMem);

    InstructionChargeMem(NoeudExpression *site_, Type *type_, Atome *chargee_)
        : InstructionChargeMem(site_)
    {
        this->type = type_;
        this->chargee = chargee_;
        this->est_chargeable = type->genre == GenreType::POINTEUR;
    }
};

struct InstructionStockeMem : public Instruction {
    explicit InstructionStockeMem(NoeudExpression *site_)
    {
        site = site_;
        genre = Instruction::Genre::STOCKE_MEMOIRE;
    }

    Atome *ou = nullptr;
    Atome *valeur = nullptr;

    COPIE_CONSTRUCT(InstructionStockeMem);

    InstructionStockeMem(NoeudExpression *site_, Type *type_, Atome *ou_, Atome *valeur_)
        : InstructionStockeMem(site_)
    {
        this->type = type_;
        this->ou = ou_;
        this->valeur = valeur_;
    }
};

struct InstructionLabel : public Instruction {
    explicit InstructionLabel(NoeudExpression *site_)
    {
        site = site_;
        genre = Instruction::Genre::LABEL;
    }

    int id = 0;

    InstructionLabel(NoeudExpression *site_, int id_) : InstructionLabel(site_)
    {
        this->id = id_;
    }
};

struct InstructionBranche : public Instruction {
    explicit InstructionBranche(NoeudExpression *site_)
    {
        site = site_;
        genre = Instruction::Genre::BRANCHE;
    }

    InstructionLabel *label = nullptr;

    COPIE_CONSTRUCT(InstructionBranche);

    InstructionBranche(NoeudExpression *site_, InstructionLabel *label_)
        : InstructionBranche(site_)
    {
        this->label = label_;
    }
};

struct InstructionBrancheCondition : public Instruction {
    explicit InstructionBrancheCondition(NoeudExpression *site_)
    {
        site = site_;
        genre = Instruction::Genre::BRANCHE_CONDITION;
    }

    Atome *condition = nullptr;
    InstructionLabel *label_si_vrai = nullptr;
    InstructionLabel *label_si_faux = nullptr;

    COPIE_CONSTRUCT(InstructionBrancheCondition);

    InstructionBrancheCondition(NoeudExpression *site_,
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
    explicit InstructionAccedeMembre(NoeudExpression *site_)
    {
        site = site_;
        genre = Instruction::Genre::ACCEDE_MEMBRE;
        est_chargeable = true;
    }

    Atome *accede = nullptr;
    Atome *index = nullptr;

    COPIE_CONSTRUCT(InstructionAccedeMembre);

    InstructionAccedeMembre(NoeudExpression *site_, Type *type_, Atome *accede_, Atome *index_)
        : InstructionAccedeMembre(site_)
    {
        this->type = type_;
        this->accede = accede_;
        this->index = index_;
    }
};

struct InstructionAccedeIndex : public Instruction {
    explicit InstructionAccedeIndex(NoeudExpression *site_)
    {
        site = site_;
        genre = Instruction::Genre::ACCEDE_INDEX;
        est_chargeable = true;
    }

    Atome *accede = nullptr;
    Atome *index = nullptr;

    COPIE_CONSTRUCT(InstructionAccedeIndex);

    InstructionAccedeIndex(NoeudExpression *site_, Type *type_, Atome *accede_, Atome *index_)
        : InstructionAccedeIndex(site_)
    {
        this->type = type_;
        this->accede = accede_;
        this->index = index_;
    }
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
    explicit InstructionTranstype(NoeudExpression *site_)
    {
        site = site_;
        genre = Instruction::Genre::TRANSTYPE;
        est_chargeable = false;  // À FAIRE : uniquement si la valeur est un pointeur
    }

    Atome *valeur = nullptr;
    TypeTranstypage op{};

    COPIE_CONSTRUCT(InstructionTranstype);

    InstructionTranstype(NoeudExpression *site_, Type *type_, Atome *valeur_, TypeTranstypage op_)
        : InstructionTranstype(site_)
    {
        this->type = type_;
        this->valeur = valeur_;
        this->op = op_;
    }
};

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
void visite_operandes_instruction(Instruction *inst, std::function<void(Atome *)> rappel);
