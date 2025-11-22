/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "arbre_syntaxique/prodeclaration.hh"

#include "transformation_type.hh"

#include "structures/tableau.hh"
#include "structures/tableau_compresse.hh"
#include "structures/tableau_page.hh"
#include "structures/tablet.hh"

#include "utilitaires/macros.hh"

#include <optional>

enum class GenreLexème : uint32_t;
struct Contexte;
struct EspaceDeTravail;
struct ItemMonomorphisation;
struct Sémanticienne;
struct Statistiques;
struct Typeuse;

using Type = NoeudDéclarationType;
using TypeEnum = NoeudEnum;
using TypeFonction = NoeudDéclarationTypeFonction;
using TypePointeur = NoeudDéclarationTypePointeur;

namespace kuri {
struct chaine_statique;
}

enum class IndiceTypeOp {
    ENTIER_NATUREL,
    ENTIER_RELATIF,
    RÉEL,
};

/* Non genre, chaine RI. */
#define ENUMERE_OPERATEURS_UNAIRE                                                                 \
    ENUMERE_GENRE_OPUNAIRE_EX(Invalide, invalide)                                                 \
    ENUMERE_GENRE_OPUNAIRE_EX(Positif, plus)                                                      \
    ENUMERE_GENRE_OPUNAIRE_EX(Négation, moins)                                                    \
    ENUMERE_GENRE_OPUNAIRE_EX(Négation_Binaire, nonb)

struct OpérateurUnaire {
    enum class Genre : char {
#define ENUMERE_GENRE_OPUNAIRE_EX(genre, nom) genre,
        ENUMERE_OPERATEURS_UNAIRE
#undef ENUMERE_GENRE_OPUNAIRE_EX
    };

    Type *type_opérande = nullptr;
    Type *type_résultat = nullptr;

    NoeudDéclarationEntêteFonction *déclaration = nullptr;

    Genre genre{};
    bool est_basique = true;
};

const char *chaine_pour_genre_op(OpérateurUnaire::Genre genre);

std::ostream &operator<<(std::ostream &os, OpérateurUnaire::Genre genre);

/* Nom genre, chaine RI, code opération MV. */
#define ENUMERE_OPERATEURS_BINAIRE                                                                \
    ENUMERE_GENRE_OPBINAIRE_EX(Invalide, invalide, octet_t(-1))                                   \
    ENUMERE_GENRE_OPBINAIRE_EX(Addition, ajt, OP_AJOUTE)                                          \
    ENUMERE_GENRE_OPBINAIRE_EX(Addition_Réel, ajtr, OP_AJOUTE_RÉEL)                               \
    ENUMERE_GENRE_OPBINAIRE_EX(Soustraction, sst, OP_SOUSTRAIT)                                   \
    ENUMERE_GENRE_OPBINAIRE_EX(Soustraction_Réel, sstr, OP_SOUSTRAIT_RÉEL)                        \
    ENUMERE_GENRE_OPBINAIRE_EX(Multiplication, mul, OP_MULTIPLIE)                                 \
    ENUMERE_GENRE_OPBINAIRE_EX(Multiplication_Réel, mulr, OP_MULTIPLIE_RÉEL)                      \
    ENUMERE_GENRE_OPBINAIRE_EX(Division_Naturel, divn, OP_DIVISE)                                 \
    ENUMERE_GENRE_OPBINAIRE_EX(Division_Relatif, divz, OP_DIVISE_RELATIF)                         \
    ENUMERE_GENRE_OPBINAIRE_EX(Division_Réel, divr, OP_DIVISE_RÉEL)                               \
    ENUMERE_GENRE_OPBINAIRE_EX(Reste_Naturel, modn, OP_RESTE_NATUREL)                             \
    ENUMERE_GENRE_OPBINAIRE_EX(Reste_Relatif, modz, OP_RESTE_RELATIF)                             \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Égal, eg, OP_COMP_ÉGAL)                                       \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inégal, neg, OP_COMP_INÉGAL)                                  \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf, inf, OP_COMP_INF)                                        \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf_Égal, infeg, OP_COMP_INF_ÉGAL)                            \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup, sup, OP_COMP_SUP)                                        \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup_Égal, supeg, OP_COMP_SUP_ÉGAL)                            \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf_Nat, infn, OP_COMP_INF_NATUREL)                           \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf_Égal_Nat, infegn, OP_COMP_INF_ÉGAL_NATUREL)               \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup_Nat, supn, OP_COMP_SUP_NATUREL)                           \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup_Égal_Nat, supegn, OP_COMP_SUP_ÉGAL_NATUREL)               \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Égal_Réel, egr, OP_COMP_ÉGAL_RÉEL)                            \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inégal_Réel, negr, OP_COMP_INÉGAL_RÉEL)                       \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf_Réel, infr, OP_COMP_INF_RÉEL)                             \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf_Égal_Réel, infegr, OP_COMP_INF_ÉGAL_RÉEL)                 \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup_Réel, supr, OP_COMP_SUP_RÉEL)                             \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup_Égal_Réel, supegr, OP_COMP_SUP_ÉGAL_RÉEL)                 \
    ENUMERE_GENRE_OPBINAIRE_EX(Et_Binaire, et, OP_ET_BINAIRE)                                     \
    ENUMERE_GENRE_OPBINAIRE_EX(Ou_Binaire, ou, OP_OU_BINAIRE)                                     \
    ENUMERE_GENRE_OPBINAIRE_EX(Ou_Exclusif, oux, OP_OU_EXCLUSIF)                                  \
    ENUMERE_GENRE_OPBINAIRE_EX(Dec_Gauche, decg, OP_DEC_GAUCHE)                                   \
    ENUMERE_GENRE_OPBINAIRE_EX(Dec_Droite_Arithm, decda, OP_DEC_DROITE_ARITHM)                    \
    ENUMERE_GENRE_OPBINAIRE_EX(Dec_Droite_Logique, decdl, OP_DEC_DROITE_LOGIQUE)                  \
    ENUMERE_GENRE_OPBINAIRE_EX(Indexage, idx, octet_t(-1))

struct OpérateurBinaire {
    enum class Genre : char {
#define ENUMERE_GENRE_OPBINAIRE_EX(genre, id, op_code) genre,
        ENUMERE_OPERATEURS_BINAIRE
#undef ENUMERE_GENRE_OPBINAIRE_EX
    };

    Type *type1{};
    Type *type2{};
    Type *type_résultat{};

    NoeudDéclarationEntêteFonction *decl = nullptr;

    OpérateurBinaire *doit_être_synthétisé_depuis = nullptr;

    Genre genre{};

    /* vrai si l'on peut sainement inverser les paramètres,
     * vrai pour : +, *, !=, == */
    bool est_commutatif = false;

    /* faux pour les opérateurs définis par l'utilisateur */
    bool est_basique = true;

    /* vrai pour les opérateurs d'arithmétiques de pointeurs */
    bool est_arithmétique_pointeur = false;

    bool est_assignation_composée = false;
};

const char *chaine_pour_genre_op(OpérateurBinaire::Genre genre);

std::ostream &operator<<(std::ostream &os, OpérateurBinaire::Genre genre);

/* Structure stockant les opérateurs binaires pour un Type.
 * Le Type n'est pas stocké ici, mais chaque Type possède une telle table.
 * Une Table stocke les opérateurs binaires pour un Type si celui-ci est le type
 * de l'opérande à gauche. */
struct TableOpérateurs {
    using type_conteneur = kuri::tableau_compresse<OpérateurBinaire *, char>;

  private:
    kuri::tableau<type_conteneur, int> opérateurs_{};

  public:
    /* À FAIRE : ces opérateurs ne sont que pour la simplification du code, nous devrions les
     * généraliser */
    OpérateurBinaire *opérateur_ajt = nullptr;
    OpérateurBinaire *opérateur_sst = nullptr;
    OpérateurBinaire *opérateur_sup = nullptr;
    OpérateurBinaire *opérateur_seg = nullptr;
    OpérateurBinaire *opérateur_inf = nullptr;
    OpérateurBinaire *opérateur_ieg = nullptr;
    OpérateurBinaire *opérateur_egt = nullptr;
    OpérateurBinaire *opérateur_oub = nullptr;
    OpérateurBinaire *opérateur_etb = nullptr;
    OpérateurBinaire *opérateur_dif = nullptr;
    OpérateurBinaire *opérateur_mul = nullptr;
    OpérateurBinaire *opérateur_div = nullptr;
    OpérateurBinaire *opérateur_indexage = nullptr;
    OpérateurUnaire *opérateur_non = nullptr;

    /* Opérateur 'pour'. */
    NoeudDéclarationOpérateurPour *opérateur_pour = nullptr;

    void ajoute(GenreLexème lexème, OpérateurBinaire *operateur);

    OpérateurBinaire *donne_opérateur(GenreLexème genre_lexème, Type *type_opérande_droite) const;

    type_conteneur const &opérateurs(GenreLexème lexème) const;

    int64_t mémoire_utilisée() const;
};

/* ------------------------------------------------------------------------- */
/** \name Registre des opérateurs.
 * Cette structure posède et alloue dynamiquement tous les opérateurs de tous
 * les types, ainsi que les TableOpérateurs de ces types.
 *
 * À FAIRE(opérateurs) : considère ne synchroniser que les conteneurs des
 * opérateurs au lieu du registre. Il faudra sans doute revoir l'interface afin
 * de ne pas avoir à trop prendre de verrous.
 * \{ */

struct RegistreDesOpérateurs {
  private:
    kuri::tableau_page<TableOpérateurs> m_tables_opérateurs{};

  public:
    using type_conteneur_binaire = kuri::tableau_page<OpérateurBinaire>;
    using type_conteneur_unaire = kuri::tableau_page<OpérateurUnaire>;

    kuri::tableau<type_conteneur_binaire> opérateurs_binaires{};
    kuri::tableau<type_conteneur_unaire> opérateurs_unaires{};

    OpérateurBinaire *op_comp_égal_types = nullptr;
    OpérateurBinaire *op_comp_diff_types = nullptr;

    RegistreDesOpérateurs();
    ~RegistreDesOpérateurs();

    EMPECHE_COPIE(RegistreDesOpérateurs);

    /** Retourne la table d'opérateur du type, ou s'il n'en a pas, crées-en une et retourne-la. */
    TableOpérateurs *donne_ou_crée_table_opérateurs(Type *type);

    type_conteneur_unaire const &trouve_unaire(GenreLexème id) const;

    OpérateurBinaire *ajoute_basique(GenreLexème id,
                                     Type *type,
                                     Type *type_résultat,
                                     IndiceTypeOp indice_type);
    OpérateurBinaire *ajoute_basique(
        GenreLexème id, Type *type1, Type *type2, Type *type_résultat, IndiceTypeOp indice_type);

    OpérateurUnaire *ajoute_basique_unaire(GenreLexème id, Type *type, Type *type_résultat);

    OpérateurBinaire *crée_opérateur_binaire(GenreLexème id,
                                             Type *type1,
                                             Type *type2,
                                             Type *type_résultat,
                                             NoeudDéclarationEntêteFonction *decl);

    void crée_opérateur_symétrique(TableOpérateurs *table,
                                   OpérateurBinaire *opérateur_source,
                                   GenreLexème id,
                                   Type *type1,
                                   Type *type2,
                                   Type *type_résultat);

    void ajoute_perso(GenreLexème id,
                      Type *type1,
                      Type *type2,
                      Type *type_résultat,
                      NoeudDéclarationEntêteFonction *decl);

    void ajoute_perso_unaire(GenreLexème id,
                             Type *type,
                             Type *type_résultat,
                             NoeudDéclarationEntêteFonction *decl);

    void ajoute_opérateur_basique_enum(Typeuse &typeuse, TypeEnum *type);

    void ajoute_opérateurs_basiques_pointeur(Typeuse &typeuse, TypePointeur *type);

    void ajoute_opérateurs_basiques_fonction(Typeuse &typeuse, TypeFonction *type);

    void rassemble_statistiques(Statistiques &stats) const;

    void ajoute_opérateurs_comparaison(Typeuse &typeuse, Type *pour_type, IndiceTypeOp indice);
    void ajoute_opérateurs_entiers_réel(Type *pour_type, IndiceTypeOp indice);
    void ajoute_opérateurs_entiers(Type *pour_type, IndiceTypeOp indice);
    void ajoute_opérateurs_entiers_unaires(Type *pour_type);

    void ajoute_opérateurs_basiques_au_besoin(Typeuse &typeuse, Type *type);
};

/** \} */

OpérateurUnaire const *cherche_opérateur_unaire(RegistreDesOpérateurs const &operateurs,
                                                Type *type1,
                                                GenreLexème type_op);

void enregistre_opérateurs_basiques(Typeuse &typeuse, RegistreDesOpérateurs &registre);

struct OpérateurCandidat {
    OpérateurBinaire const *op = nullptr;
    TransformationType transformation_type1{};
    TransformationType transformation_type2{};
    double poids = 0.0;
    bool permute_opérandes = false;

    kuri::tableau<ItemMonomorphisation, int> items_monomorphisation{};

    POINTEUR_NUL(OpérateurCandidat)
};

std::optional<Attente> cherche_candidats_opérateurs(
    EspaceDeTravail &espace,
    NoeudExpression *expression_binaire,
    Type *type1,
    Type *type2,
    GenreLexème type_op,
    kuri::tablet<OpérateurCandidat, 10> &candidats);

using RésultatRechercheOpérateur = std::variant<Attente, OpérateurCandidat, bool>;

RésultatRechercheOpérateur trouve_opérateur_pour_expression(
    Contexte *contexte, NoeudExpression *site, Type *type1, Type *type2, GenreLexème type_op);

kuri::chaine_statique donne_chaine_lexème_pour_op_binaire(OpérateurBinaire::Genre op);

bool est_opérateur_comparaison(OpérateurBinaire::Genre const genre);

bool est_opérateur_comparaison_ordre(OpérateurBinaire::Genre const genre);

bool peut_permuter_opérandes(OpérateurBinaire::Genre const genre);

OpérateurBinaire::Genre donne_opérateur_pour_permutation_opérandes(
    OpérateurBinaire::Genre const genre);

GenreLexème donne_genre_lexème_pour_opérateur_symétrique(GenreLexème genre);

kuri::chaine_statique donne_chaine_lexème_pour_op_binaire(GenreLexème genre);
