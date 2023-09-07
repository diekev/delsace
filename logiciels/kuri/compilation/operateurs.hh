/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/tableau_page.hh"

#include "transformation_type.hh"

#include "structures/tableau.hh"
#include "structures/tableau_compresse.hh"
#include "structures/tablet.hh"

#include <optional>

enum class GenreLexeme : uint32_t;
struct EspaceDeTravail;
struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationOperateurPour;
struct NoeudExpressionBinaire;
struct Statistiques;
struct Type;
struct Typeuse;
struct TypeEnum;
struct TypeFonction;
struct TypePointeur;

enum class IndiceTypeOp {
    ENTIER_NATUREL,
    ENTIER_RELATIF,
    REEL,
};

/* Non genre, chaine RI. */
#define ENUMERE_OPERATEURS_UNAIRE                                                                 \
    ENUMERE_GENRE_OPUNAIRE_EX(Invalide, invalide)                                                 \
    ENUMERE_GENRE_OPUNAIRE_EX(Positif, plus)                                                      \
    ENUMERE_GENRE_OPUNAIRE_EX(Complement, moins)                                                  \
    ENUMERE_GENRE_OPUNAIRE_EX(Non_Logique, nonl)                                                  \
    ENUMERE_GENRE_OPUNAIRE_EX(Non_Binaire, nonb)                                                  \
    ENUMERE_GENRE_OPUNAIRE_EX(Prise_Adresse, addr)

struct OperateurUnaire {
    enum class Genre : char {
#define ENUMERE_GENRE_OPUNAIRE_EX(genre, nom) genre,
        ENUMERE_OPERATEURS_UNAIRE
#undef ENUMERE_GENRE_OPUNAIRE_EX
    };

    Type *type_operande = nullptr;
    Type *type_resultat = nullptr;

    NoeudDeclarationEnteteFonction *decl = nullptr;

    Genre genre{};
    bool est_basique = true;
};

const char *chaine_pour_genre_op(OperateurUnaire::Genre genre);

/* Nom genre, chaine RI, code opération MV. */
#define ENUMERE_OPERATEURS_BINAIRE                                                                \
    ENUMERE_GENRE_OPBINAIRE_EX(Invalide, invalide, octet_t(-1))                                   \
    ENUMERE_GENRE_OPBINAIRE_EX(Addition, ajt, OP_AJOUTE)                                          \
    ENUMERE_GENRE_OPBINAIRE_EX(Addition_Reel, ajtr, OP_AJOUTE_REEL)                               \
    ENUMERE_GENRE_OPBINAIRE_EX(Soustraction, sst, OP_SOUSTRAIT)                                   \
    ENUMERE_GENRE_OPBINAIRE_EX(Soustraction_Reel, sstr, OP_SOUSTRAIT_REEL)                        \
    ENUMERE_GENRE_OPBINAIRE_EX(Multiplication, mul, OP_MULTIPLIE)                                 \
    ENUMERE_GENRE_OPBINAIRE_EX(Multiplication_Reel, mulr, OP_MULTIPLIE_REEL)                      \
    ENUMERE_GENRE_OPBINAIRE_EX(Division_Naturel, divn, OP_DIVISE)                                 \
    ENUMERE_GENRE_OPBINAIRE_EX(Division_Relatif, divz, OP_DIVISE_RELATIF)                         \
    ENUMERE_GENRE_OPBINAIRE_EX(Division_Reel, divr, OP_DIVISE_REEL)                               \
    ENUMERE_GENRE_OPBINAIRE_EX(Reste_Naturel, modn, OP_RESTE_NATUREL)                             \
    ENUMERE_GENRE_OPBINAIRE_EX(Reste_Relatif, modz, OP_RESTE_RELATIF)                             \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Egal, eg, OP_COMP_EGAL)                                       \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inegal, neg, OP_COMP_INEGAL)                                  \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf, inf, OP_COMP_INF)                                        \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf_Egal, infeg, OP_COMP_INF_EGAL)                            \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup, sup, OP_COMP_SUP)                                        \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup_Egal, supeg, OP_COMP_SUP_EGAL)                            \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf_Nat, infn, OP_COMP_INF_NATUREL)                           \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf_Egal_Nat, infegn, OP_COMP_INF_EGAL_NATUREL)               \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup_Nat, supn, OP_COMP_SUP_NATUREL)                           \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup_Egal_Nat, supegn, OP_COMP_SUP_EGAL_NATUREL)               \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Egal_Reel, egr, OP_COMP_EGAL_REEL)                            \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inegal_Reel, negr, OP_COMP_INEGAL_REEL)                       \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf_Reel, infr, OP_COMP_INF_REEL)                             \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Inf_Egal_Reel, infegr, OP_COMP_INF_EGAL_REEL)                 \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup_Reel, supr, OP_COMP_SUP_REEL)                             \
    ENUMERE_GENRE_OPBINAIRE_EX(Comp_Sup_Egal_Reel, supegr, OP_COMP_SUP_EGAL_REEL)                 \
    ENUMERE_GENRE_OPBINAIRE_EX(Et_Binaire, et, OP_ET_BINAIRE)                                     \
    ENUMERE_GENRE_OPBINAIRE_EX(Ou_Binaire, ou, OP_OU_BINAIRE)                                     \
    ENUMERE_GENRE_OPBINAIRE_EX(Ou_Exclusif, oux, OP_OU_EXCLUSIF)                                  \
    ENUMERE_GENRE_OPBINAIRE_EX(Dec_Gauche, decg, OP_DEC_GAUCHE)                                   \
    ENUMERE_GENRE_OPBINAIRE_EX(Dec_Droite_Arithm, decda, OP_DEC_DROITE_ARITHM)                    \
    ENUMERE_GENRE_OPBINAIRE_EX(Dec_Droite_Logique, decdl, OP_DEC_DROITE_LOGIQUE)                  \
    ENUMERE_GENRE_OPBINAIRE_EX(Indexage, idx, octet_t(-1))

struct OperateurBinaire {
    enum class Genre : char {
#define ENUMERE_GENRE_OPBINAIRE_EX(genre, id, op_code) genre,
        ENUMERE_OPERATEURS_BINAIRE
#undef ENUMERE_GENRE_OPBINAIRE_EX
    };

    Type *type1{};
    Type *type2{};
    Type *type_resultat{};

    NoeudDeclarationEnteteFonction *decl = nullptr;

    Genre genre{};

    /* vrai si l'on peut sainement inverser les paramètres,
     * vrai pour : +, *, !=, == */
    bool est_commutatif = false;

    /* faux pour les opérateurs définis par l'utilisateur */
    bool est_basique = true;

    /* vrai pour les opérateurs d'arithmétiques de pointeurs */
    bool est_arithmetique_pointeur = false;
};

const char *chaine_pour_genre_op(OperateurBinaire::Genre genre);

/* Structure stockant les opérateurs binaires pour un Type.
 * Le Type n'est pas stocké ici, mais chaque Type possède une telle table.
 * Une Table stocke les opérateurs binaires pour un Type si celui-ci est le type
 * de l'opérande à gauche. */
struct TableOperateurs {
    using type_conteneur = kuri::tableau_compresse<OperateurBinaire *, char>;

  private:
    kuri::tableau<type_conteneur, int> operateurs_{};

  public:
    /* À FAIRE : ces opérateurs ne sont que pour la simplification du code, nous devrions les
     * généraliser */
    OperateurBinaire *operateur_ajt = nullptr;
    OperateurBinaire *operateur_sst = nullptr;
    OperateurBinaire *operateur_sup = nullptr;
    OperateurBinaire *operateur_seg = nullptr;
    OperateurBinaire *operateur_inf = nullptr;
    OperateurBinaire *operateur_ieg = nullptr;
    OperateurBinaire *operateur_egt = nullptr;
    OperateurBinaire *operateur_oub = nullptr;
    OperateurBinaire *operateur_etb = nullptr;
    OperateurBinaire *operateur_dif = nullptr;
    OperateurBinaire *operateur_mul = nullptr;
    OperateurBinaire *operateur_div = nullptr;
    OperateurUnaire *operateur_non = nullptr;

    /* Opérateur 'pour'. */
    NoeudDeclarationOperateurPour *opérateur_pour = nullptr;

    void ajoute(GenreLexeme lexeme, OperateurBinaire *operateur);

    type_conteneur const &operateurs(GenreLexeme lexeme);

    int64_t memoire_utilisée() const;
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
    tableau_page<TableOperateurs> m_tables_opérateurs{};

  public:
    using type_conteneur_binaire = tableau_page<OperateurBinaire>;
    using type_conteneur_unaire = tableau_page<OperateurUnaire>;

    kuri::tableau<type_conteneur_binaire> operateurs_binaires{};
    kuri::tableau<type_conteneur_unaire> operateurs_unaires{};

    OperateurBinaire *op_comp_egal_types = nullptr;
    OperateurBinaire *op_comp_diff_types = nullptr;

    RegistreDesOpérateurs();
    ~RegistreDesOpérateurs();

    EMPECHE_COPIE(RegistreDesOpérateurs);

    /** Retourne la table d'opérateur du type, ou s'il n'en a pas, crées-en une et retourne-la. */
    TableOperateurs *donne_ou_crée_table_opérateurs(Type *type);

    type_conteneur_unaire const &trouve_unaire(GenreLexeme id) const;

    OperateurBinaire *ajoute_basique(GenreLexeme id,
                                     Type *type,
                                     Type *type_resultat,
                                     IndiceTypeOp indice_type);
    OperateurBinaire *ajoute_basique(
        GenreLexeme id, Type *type1, Type *type2, Type *type_resultat, IndiceTypeOp indice_type);

    OperateurUnaire *ajoute_basique_unaire(GenreLexeme id, Type *type, Type *type_resultat);

    void ajoute_perso(GenreLexeme id,
                      Type *type1,
                      Type *type2,
                      Type *type_resultat,
                      NoeudDeclarationEnteteFonction *decl);

    void ajoute_perso_unaire(GenreLexeme id,
                             Type *type,
                             Type *type_resultat,
                             NoeudDeclarationEnteteFonction *decl);

    void ajoute_operateur_basique_enum(TypeEnum *type);

    void ajoute_operateurs_basiques_pointeur(TypePointeur *type);

    void ajoute_operateurs_basiques_fonction(TypeFonction *type);

    void rassemble_statistiques(Statistiques &stats) const;

    void ajoute_opérateurs_comparaison(Type *pour_type, IndiceTypeOp indice);
    void ajoute_opérateurs_entiers_réel(Type *pour_type, IndiceTypeOp indice);
    void ajoute_opérateurs_entiers(Type *pour_type, IndiceTypeOp indice);
    void ajoute_opérateurs_entiers_unaires(Type *pour_type);
};

/** \} */

OperateurUnaire const *cherche_operateur_unaire(RegistreDesOpérateurs const &operateurs,
                                                Type *type1,
                                                GenreLexeme type_op);

void enregistre_operateurs_basiques(Typeuse &typeuse, RegistreDesOpérateurs &registre);

struct OperateurCandidat {
    OperateurBinaire const *op = nullptr;
    TransformationType transformation_type1{};
    TransformationType transformation_type2{};
    double poids = 0.0;
    bool permute_operandes = false;

    POINTEUR_NUL(OperateurCandidat)
};

std::optional<Attente> cherche_candidats_operateurs(
    EspaceDeTravail &espace,
    Type *type1,
    Type *type2,
    GenreLexeme type_op,
    kuri::tablet<OperateurCandidat, 10> &candidats);

using RésultatRechercheOpérateur = std::variant<Attente, OperateurCandidat, bool>;

RésultatRechercheOpérateur trouve_opérateur_pour_expression(EspaceDeTravail &espace,
                                                            NoeudExpressionBinaire *site,
                                                            Type *type1,
                                                            Type *type2,
                                                            GenreLexeme type_op);
