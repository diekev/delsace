/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "biblinternes/moultfilage/synchrone.hh"
#include "biblinternes/outils/assert.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/plage.hh"
#include "biblinternes/structures/tableau_page.hh"

#include "parsage/lexemes.hh"

#include "structures/chaine.hh"
#include "structures/ensemblon.hh"
#include "structures/table_hachage.hh"
#include "structures/tableaux_partage_synchrones.hh"
#include "structures/tablet.hh"

#include "operateurs.hh"

struct AtomeGlobale;
struct Compilatrice;
struct IdentifiantCode;
struct InfoType;
struct RegistreDesOpérateurs;
struct OpérateurBinaire;
struct OpérateurUnaire;
struct NoeudDeclarationVariable;
struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationTypeOpaque;
struct NoeudDeclarationOperateurPour;
struct NoeudDependance;
struct NoeudEnum;
struct NoeudExpression;
struct NoeudStruct;
struct Statistiques;
struct Typeuse;
struct TypeCompose;
struct TypeEnum;
struct TypeFonction;
struct TypeOpaque;
struct TypePointeur;
struct TypePolymorphique;
struct TypeReference;
struct TypeStructure;
struct TypeTableauDynamique;
struct TypeTableauFixe;
struct TypeTuple;
struct TypeTypeDeDonnees;
struct TypeUnion;
struct TypeVariadique;

/* ************************************************************************** */

namespace TypeBase {
#define ENUMERE_TYPE_FONDAMENTAL(O)                                                               \
    O(N8)                                                                                         \
    O(N16)                                                                                        \
    O(N32)                                                                                        \
    O(N64)                                                                                        \
    O(Z8)                                                                                         \
    O(Z16)                                                                                        \
    O(Z32)                                                                                        \
    O(Z64)                                                                                        \
    O(R16)                                                                                        \
    O(R32)                                                                                        \
    O(R64)                                                                                        \
    O(EINI)                                                                                       \
    O(CHAINE)                                                                                     \
    O(RIEN)                                                                                       \
    O(BOOL)                                                                                       \
    O(OCTET)                                                                                      \
    O(ENTIER_CONSTANT)                                                                            \
    O(PTR_N8)                                                                                     \
    O(PTR_N16)                                                                                    \
    O(PTR_N32)                                                                                    \
    O(PTR_N64)                                                                                    \
    O(PTR_Z8)                                                                                     \
    O(PTR_Z16)                                                                                    \
    O(PTR_Z32)                                                                                    \
    O(PTR_Z64)                                                                                    \
    O(PTR_R16)                                                                                    \
    O(PTR_R32)                                                                                    \
    O(PTR_R64)                                                                                    \
    O(PTR_EINI)                                                                                   \
    O(PTR_CHAINE)                                                                                 \
    O(PTR_RIEN)                                                                                   \
    O(PTR_NUL)                                                                                    \
    O(PTR_BOOL)                                                                                   \
    O(PTR_OCTET)                                                                                  \
    O(REF_N8)                                                                                     \
    O(REF_N16)                                                                                    \
    O(REF_N32)                                                                                    \
    O(REF_N64)                                                                                    \
    O(REF_Z8)                                                                                     \
    O(REF_Z16)                                                                                    \
    O(REF_Z32)                                                                                    \
    O(REF_Z64)                                                                                    \
    O(REF_R16)                                                                                    \
    O(REF_R32)                                                                                    \
    O(REF_R64)                                                                                    \
    O(REF_EINI)                                                                                   \
    O(REF_CHAINE)                                                                                 \
    O(REF_RIEN)                                                                                   \
    O(REF_BOOL)                                                                                   \
    O(TABL_N8)                                                                                    \
    O(TABL_N16)                                                                                   \
    O(TABL_N32)                                                                                   \
    O(TABL_N64)                                                                                   \
    O(TABL_Z8)                                                                                    \
    O(TABL_Z16)                                                                                   \
    O(TABL_Z32)                                                                                   \
    O(TABL_Z64)                                                                                   \
    O(TABL_R16)                                                                                   \
    O(TABL_R32)                                                                                   \
    O(TABL_R64)                                                                                   \
    O(TABL_EINI)                                                                                  \
    O(TABL_CHAINE)                                                                                \
    O(TABL_BOOL)                                                                                  \
    O(TABL_OCTET)

#define DECLARE_EXTERNE_TYPE(nom) extern Type *nom;
ENUMERE_TYPE_FONDAMENTAL(DECLARE_EXTERNE_TYPE)
#undef DECLARE_EXTERNE_TYPE
}  // namespace TypeBase

#define ENUMERE_TYPE(O)                                                                           \
    O(pointeur, POINTEUR, TypePointeur)                                                           \
    O(structure, STRUCTURE, TypeStructure)                                                        \
    O(reference, REFERENCE, TypeReference)                                                        \
    O(tableau_fixe, TABLEAU_FIXE, TypeTableauFixe)                                                \
    O(tableau_dynamique, TABLEAU_DYNAMIQUE, TypeTableauDynamique)                                 \
    O(union, UNION, TypeUnion)                                                                    \
    O(fonction, FONCTION, TypeFonction)                                                           \
    O(enum, ENUM, TypeEnum)                                                                       \
    O(erreur, ERREUR, TypeEnum)                                                                   \
    O(variadique, VARIADIQUE, TypeVariadique)                                                     \
    O(type_de_donnees, TYPE_DE_DONNEES, TypeTypeDeDonnees)                                        \
    O(polymorphique, POLYMORPHIQUE, TypePolymorphique)                                            \
    O(opaque, OPAQUE, TypeOpaque)                                                                 \
    O(tuple, TUPLE, TypeTuple)                                                                    \
    O(entier_naturel, ENTIER_NATUREL, Type)                                                       \
    O(entier_relatif, ENTIER_RELATIF, Type)                                                       \
    O(entier_constant, ENTIER_CONSTANT, Type)                                                     \
    O(reel, REEL, Type)                                                                           \
    O(eini, EINI, TypeCompose)                                                                    \
    O(chaine, CHAINE, TypeCompose)                                                                \
    O(rien, RIEN, Type)                                                                           \
    O(bool, BOOL, Type)                                                                           \
    O(octet, OCTET, Type)
/* O(eini_ereur, EINI, TypeCompose) */

enum class GenreType : int {
#define ENUMERE_GENRE_TYPE(nom, Genre, TypeRafine) Genre,
    ENUMERE_TYPE(ENUMERE_GENRE_TYPE)
#undef ENUMERE_GENRE_TYPE
};

const char *chaine_genre_type(GenreType genre);
std::ostream &operator<<(std::ostream &os, GenreType genre);

bool est_type_polymorphique(Type const *type);

/* ------------------------------------------------------------------------- */
/** \name Drapeaux pour les types.
 * \{ */

enum class DrapeauxTypes : uint32_t {
    AUCUN = 0,

    /* Pour les types variadiques externes, et les structures externes opaques (sans bloc). */
    TYPE_NE_REQUIERS_PAS_D_INITIALISATION = (1u << 0),
    TYPE_EST_POLYMORPHIQUE = (1u << 1),
    TYPE_FUT_VALIDE = (1u << 2),
    INITIALISATION_TYPE_FUT_CREEE = (1u << 3),
    POSSEDE_TYPE_POINTEUR = (1u << 4),
    POSSEDE_TYPE_REFERENCE = (1u << 5),
    POSSEDE_TYPE_TABLEAU_FIXE = (1u << 6),
    POSSEDE_TYPE_TABLEAU_DYNAMIQUE = (1u << 7),
    POSSEDE_TYPE_TYPE_DE_DONNEES = (1u << 8),
    CODE_BINAIRE_TYPE_FUT_GENERE = (1u << 9),
    TYPE_POSSEDE_OPERATEURS_DE_BASE = (1u << 10),
    UNITE_POUR_INITIALISATION_FUT_CREE = (1u << 11),
    INITIALISATION_TYPE_FUT_REQUISE = (1u << 12),
};
DEFINIS_OPERATEURS_DRAPEAU(DrapeauxTypes)

std::ostream &operator<<(std::ostream &os, DrapeauxTypes const drapeaux);

/** \} */

struct Type {
    GenreType genre{};
    unsigned taille_octet = 0;
    unsigned alignement = 0;
    DrapeauxTypes drapeaux_type = DrapeauxTypes::AUCUN;
    unsigned index_dans_table_types = 0;

    kuri::chaine_statique nom_broye{};

    mutable InfoType *info_type = nullptr;
    mutable AtomeGlobale *atome_info_type = nullptr;
    NoeudDependance *noeud_dependance = nullptr;

    NoeudDeclarationEnteteFonction *fonction_init = nullptr;

    TableOpérateurs *table_opérateurs = nullptr;

    /* À FAIRE: déplace ceci dans une table? */
    TypePointeur *type_pointeur = nullptr;

    POINTEUR_NUL(Type)

#define __DEFINIS_DISCRIMINATIONS(nom, Genre, TypeRafine)                                         \
    inline bool est_type_##nom() const                                                            \
    {                                                                                             \
        return genre == GenreType::Genre;                                                         \
    }

    ENUMERE_TYPE(__DEFINIS_DISCRIMINATIONS)

#undef __DEFINIS_DISCRIMINATIONS

#define __DECLARE_COMME_TYPE(nom, Genre, TypeRafine)                                              \
    inline TypeRafine *comme_type_##nom();                                                        \
    inline const TypeRafine *comme_type_##nom() const;

    ENUMERE_TYPE(__DECLARE_COMME_TYPE)

#undef __DECLARE_COMME_TYPE

    inline TypeCompose *comme_type_compose();
    inline const TypeCompose *comme_type_compose() const;

    inline bool possède_drapeau(DrapeauxTypes drapeaux_) const
    {
        return (drapeaux_type & drapeaux_) != DrapeauxTypes::AUCUN;
    }
};

struct TypePointeur : public Type {
    TypePointeur()
    {
        genre = GenreType::POINTEUR;
    }

    explicit TypePointeur(Type *type_pointe);

    EMPECHE_COPIE(TypePointeur);

    Type *type_pointe = nullptr;
};

struct TypeReference : public Type {
    TypeReference()
    {
        genre = GenreType::REFERENCE;
    }

    explicit TypeReference(Type *type_pointe);

    EMPECHE_COPIE(TypeReference);

    Type *type_pointe = nullptr;
};

struct TypeFonction : public Type {
    TypeFonction()
    {
        genre = GenreType::FONCTION;
    }

    TypeFonction(kuri::tablet<Type *, 6> const &entrees, Type *sortie);

    EMPECHE_COPIE(TypeFonction);

    kuri::tableau<Type *, int> types_entrees{};
    Type *type_sortie{};
};

struct MembreTypeComposé {
    enum {
        // si le membre est une constante (par exemple, la définition d'une énumération, ou une
        // simple valeur)
        EST_CONSTANT = (1 << 0),
        // si le membre est défini par la compilatrice (par exemple, « nombre_éléments » des
        // énumérations)
        EST_IMPLICITE = (1 << 1),
        // si le membre provient d'une instruction empl
        PROVIENT_D_UN_EMPOI = (1 << 2),
        // si le membre est employé
        EST_UN_EMPLOI = (1 << 3),
        // si l'expression du membre est sur-écrite dans la définition de la structure (x = y,
        // pour x déclaré en amont)
        POSSÈDE_EXPRESSION_SPÉCIALE = (1 << 4),

        MEMBRE_NE_DOIT_PAS_ÊTRE_DANS_CODE_MACHINE = (EST_CONSTANT | PROVIENT_D_UN_EMPOI),
    };

    NoeudDeclaration *decl = nullptr;
    Type *type = nullptr;
    IdentifiantCode *nom = nullptr;
    unsigned decalage = 0;
    int valeur = 0;                                       // pour les énumérations
    NoeudExpression *expression_valeur_defaut = nullptr;  // pour les membres des structures
    int drapeaux = 0;
    uint32_t rembourrage = 0;

    inline bool possède_drapeau(int drapeau) const
    {
        return (drapeaux & drapeau) != 0;
    }

    inline bool est_implicite() const
    {
        return possède_drapeau(EST_IMPLICITE);
    }

    inline bool est_constant() const
    {
        return possède_drapeau(EST_CONSTANT);
    }

    inline bool est_utilisable_pour_discrimination() const
    {
        return !est_implicite() && !est_constant();
    }

    inline bool ne_doit_pas_être_dans_code_machine() const
    {
        return possède_drapeau(MEMBRE_NE_DOIT_PAS_ÊTRE_DANS_CODE_MACHINE);
    }

    inline bool expression_initialisation_est_spéciale() const
    {
        return possède_drapeau(POSSÈDE_EXPRESSION_SPÉCIALE);
    }

    inline bool est_un_emploi() const
    {
        return possède_drapeau(EST_UN_EMPLOI);
    }
};

struct InformationMembreTypeCompose {
    MembreTypeComposé membre{};
    int index_membre = -1;
};

/* Type de base pour tous les types ayant des membres (structures, énumérations, etc.).
 */
struct TypeCompose : public Type {
    kuri::tableau<MembreTypeComposé, int> membres{};

    /* Le nom tel que donné dans le script (p.e. Structure, pour Structure :: struct ...). */
    IdentifiantCode *ident = nullptr;

    /* Le nom final, contenant les informations de portée (p.e. ModuleStructure, pour Structure ::
     * struct dans le module Module). */
    kuri::chaine nom_portable_{};

    /* Le nom de la hiérarchie, sans le nom du module. Chaque nom est séparé par des points.
     * Ceci est le nom qui sera utilisé dans les infos types.
     * À FAIRE : remplace ceci par l'utilisation d'un pointeur dans les infos-types contenant la
     * type parent. */
    kuri::chaine nom_hiérarchique_ = "";

    /* Nombre de membres qui seront dans le code machine.
     * Afin de simplifier la génération de code, les membres « réels » sont placés au début des
     * membres du type. Ainsi, la représentation de constante de type dans la RI aura une
     * disposition similaire à celle des membres réels et nous n'aurons pas gérer des indexages
     * différents (c-à-d que l'index de la valeur du membre correspond à l'index du membre, sans
     * mappage entre les deux dû à l'absence des membres constants ou provenant d'un emploi dans la
     * RI). */
    int nombre_de_membres_réels = 0;

    kuri::tableau_statique<const MembreTypeComposé> donne_membres_pour_code_machine() const
    {
        return {membres.begin(), nombre_de_membres_réels};
    }
};

inline bool est_type_compose(const Type *type)
{
    return dls::outils::est_element(type->genre,
                                    GenreType::CHAINE,
                                    GenreType::EINI,
                                    GenreType::ENUM,
                                    GenreType::ERREUR,
                                    GenreType::STRUCTURE,
                                    GenreType::TABLEAU_DYNAMIQUE,
                                    GenreType::TABLEAU_FIXE,
                                    GenreType::TUPLE,
                                    GenreType::UNION,
                                    GenreType::VARIADIQUE);
}

struct TypeStructure final : public TypeCompose {
    TypeStructure()
    {
        genre = GenreType::STRUCTURE;
    }

    EMPECHE_COPIE(TypeStructure);

    /* Stocke les membres pour avoir accès à leurs décalages. */
    kuri::tableau<MembreTypeComposé const *, int> types_employés{};

    NoeudStruct *decl = nullptr;

    TypeUnion *union_originelle = nullptr;

    bool est_anonyme = false;
};

struct TypeUnion final : public TypeCompose {
    TypeUnion()
    {
        genre = GenreType::UNION;
    }

    EMPECHE_COPIE(TypeUnion);

    Type *type_le_plus_grand = nullptr;
    TypeStructure *type_structure = nullptr;

    NoeudStruct *decl = nullptr;

    unsigned decalage_index = 0;
    bool est_nonsure = false;
    bool est_anonyme = false;
};

struct TypeEnum final : public TypeCompose {
    TypeEnum()
    {
        genre = GenreType::ENUM;
    }

    EMPECHE_COPIE(TypeEnum);

    Type *type_sous_jacent{};

    NoeudEnum *decl = nullptr;
    bool est_drapeau = false;
    bool est_erreur = false;
};

struct TypeTableauFixe final : public TypeCompose {
    TypeTableauFixe()
    {
        genre = GenreType::TABLEAU_FIXE;
    }

    TypeTableauFixe(Type *type_pointe,
                    int taille,
                    kuri::tableau<MembreTypeComposé, int> &&membres);

    EMPECHE_COPIE(TypeTableauFixe);

    Type *type_pointe = nullptr;
    int taille = 0;
};

struct TypeTableauDynamique final : public TypeCompose {
    TypeTableauDynamique()
    {
        genre = GenreType::TABLEAU_DYNAMIQUE;
    }

    TypeTableauDynamique(Type *type_pointe, kuri::tableau<MembreTypeComposé, int> &&membres);

    EMPECHE_COPIE(TypeTableauDynamique);

    Type *type_pointe = nullptr;
};

struct TypeVariadique final : public TypeCompose {
    TypeVariadique()
    {
        genre = GenreType::VARIADIQUE;
    }

    TypeVariadique(Type *type_pointe, kuri::tableau<MembreTypeComposé, int> &&membres);

    EMPECHE_COPIE(TypeVariadique);

    Type *type_pointe = nullptr;
    /* Type tableau dynamique pour la génération de code, si le type est ...z32, le type
     * tableau dynamique sera []z32. */
    Type *type_tableau_dynamique = nullptr;
};

struct TypeTypeDeDonnees : public Type {
    TypeTypeDeDonnees()
    {
        genre = GenreType::TYPE_DE_DONNEES;
    }

    explicit TypeTypeDeDonnees(Type *type_connu);

    EMPECHE_COPIE(TypeTypeDeDonnees);

    // Non-nul si le type est connu lors de la compilation.
    Type *type_connu = nullptr;
};

struct TypePolymorphique : public Type {
    TypePolymorphique()
    {
        genre = GenreType::POLYMORPHIQUE;
        drapeaux_type = DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
    }

    explicit TypePolymorphique(IdentifiantCode *ident);

    EMPECHE_COPIE(TypePolymorphique);

    IdentifiantCode *ident = nullptr;

    bool est_structure_poly = false;
    NoeudStruct const *structure = nullptr;
};

struct TypeOpaque : public Type {
    TypeOpaque()
    {
        genre = GenreType::OPAQUE;
    }

    TypeOpaque(NoeudDeclarationTypeOpaque *decl_, Type *opacifie);

    EMPECHE_COPIE(TypeOpaque);

    NoeudDeclarationTypeOpaque *decl = nullptr;
    IdentifiantCode *ident = nullptr;
    Type *type_opacifie = nullptr;
    kuri::chaine nom_portable_ = "";
    kuri::chaine nom_hiérarchique_ = "";
};

/* Pour les sorties multiples des fonctions. */
struct TypeTuple : public TypeCompose {
    TypeTuple()
    {
        genre = GenreType::TUPLE;
    }
};

/* ************************************************************************** */

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
void imprime_genre_type_pour_assert(GenreType genre);
#define __DEFINIS_COMME_TYPE(nom, Genre, TypeRafine)                                              \
    inline TypeRafine *Type::comme_type_##nom()                                                   \
    {                                                                                             \
        assert_rappel(genre == GenreType::Genre,                                                  \
                      [this] { imprime_genre_type_pour_assert(genre); });                         \
        return static_cast<TypeRafine *>(this);                                                   \
    }                                                                                             \
    inline const TypeRafine *Type::comme_type_##nom() const                                       \
    {                                                                                             \
        assert_rappel(genre == GenreType::Genre,                                                  \
                      [this] { imprime_genre_type_pour_assert(genre); });                         \
        return static_cast<const TypeRafine *>(this);                                             \
    }

ENUMERE_TYPE(__DEFINIS_COMME_TYPE)

#undef __DEFINIS_COMME_TYPE
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

inline TypeCompose *Type::comme_type_compose()
{
    assert(est_type_compose(this));
    return static_cast<TypeCompose *>(this);
}

inline const TypeCompose *Type::comme_type_compose() const
{
    assert(est_type_compose(this));
    return static_cast<const TypeCompose *>(this);
}

/* ************************************************************************** */

/**
 * Arbre Trie pour stocker les types de fonctions selon les types d'entrée et de sortie.
 *
 * Chaque type d'entrée et de sortie est stocké dans un noeud de l'arbre selon sa position
 * dans les paramètres de la fonction. Les noeuds possèdent ensuite un pointeur vers les
 * types suivants possible. Un noeud final est ajouté après le noeud du type de sortie afin
 * de stocker le type fonction final.
 *
 * Les noeuds enfants peuvent être stockés dans l'une de deux listes : une pour les entrées,
 * une pour les sorties.
 *
 * https://fr.wikipedia.org/wiki/Trie_(informatique)
 */
struct Trie {
    static constexpr auto TAILLE_MAX_ENFANTS_TABLET = 16;
    struct Noeud;

    /**
     * Structure pour abstraire les listes d'enfants des noeuds.
     *
     * Par défaut nous utilisons un tablet, mais si nous avons trop d'enfants (déterminer
     * selon TAILLE_MAX_ENFANTS_TABLET), nous les stockons dans une table de hachage afin
     * d'accélérer les requêtes.
     */
    struct StockageEnfants {
        kuri::tablet<Noeud *, TAILLE_MAX_ENFANTS_TABLET> enfants{};
        kuri::table_hachage<Type const *, Noeud *> table{"Noeud Trie"};

        Noeud *trouve_noeud_pour_type(Type const *type);

        int64_t taille() const;

        void ajoute(Noeud *noeud);
    };

    /**
     * Noeud stockant un type, et les pointeurs vers les types entrée et sortie possibles après
     * celui-ci.
     */
    struct Noeud {
        Type const *type = nullptr;
        StockageEnfants enfants{};
        StockageEnfants enfants_sortie{};

        Noeud *trouve_noeud_pour_type(Type const *type);

        Noeud *trouve_noeud_sortie_pour_type(Type const *type);
    };

    Noeud *racine = nullptr;
    tableau_page<Noeud> noeuds{};

    using TypeResultat = std::variant<Noeud *, TypeFonction *>;

    /**
     * Retourne soit un TypeFonction existant, soit un Noeud pour insérer un nouveau
     * TypeFonction selon les types entrée et sortie donnés.
     */
    TypeResultat trouve_type_ou_noeud_insertion(kuri::tablet<Type *, 6> const &entrees,
                                                Type *type_sortie);

  private:
    Noeud *ajoute_enfant(Noeud *parent, Type const *type, bool est_sortie);
};

// À FAIRE(table type) : il peut y avoir une concurrence critique pour l'assignation d'index aux
// types
struct Typeuse {
    dls::outils::Synchrone<RegistreDesOpérateurs> &operateurs_;

    // NOTE : nous synchronisons les tableaux individuellement et non la Typeuse
    // dans son entièreté afin que différents threads puissent accéder librement
    // à différents types de types.
    kuri::tableau_synchrone<Type *> types_simples{};
    tableau_page_synchrone<TypePointeur> types_pointeurs{};
    tableau_page_synchrone<TypeReference> types_references{};
    tableau_page_synchrone<TypeStructure> types_structures{};
    tableau_page_synchrone<TypeEnum> types_enums{};
    tableau_page_synchrone<TypeTableauFixe> types_tableaux_fixes{};
    tableau_page_synchrone<TypeTableauDynamique> types_tableaux_dynamiques{};
    tableau_page_synchrone<TypeFonction> types_fonctions{};
    tableau_page_synchrone<TypeVariadique> types_variadiques{};
    tableau_page_synchrone<TypeUnion> types_unions{};
    tableau_page_synchrone<TypeTypeDeDonnees> types_type_de_donnees{};
    tableau_page_synchrone<TypePolymorphique> types_polymorphiques{};
    tableau_page_synchrone<TypeOpaque> types_opaques{};
    tableau_page_synchrone<TypeTuple> types_tuples{};

    // mise en cache de plusieurs types pour mieux les trouver
    TypeTypeDeDonnees *type_type_de_donnees_ = nullptr;
    Type *type_contexte = nullptr;
    Type *type_info_type_ = nullptr;
    Type *type_info_type_structure = nullptr;
    Type *type_info_type_union = nullptr;
    Type *type_info_type_membre_structure = nullptr;
    Type *type_info_type_entier = nullptr;
    Type *type_info_type_tableau = nullptr;
    Type *type_info_type_pointeur = nullptr;
    Type *type_info_type_enum = nullptr;
    Type *type_info_type_fonction = nullptr;
    Type *type_info_type_opaque = nullptr;
    Type *type_info_type_variadique = nullptr;
    Type *type_position_code_source = nullptr;
    Type *type_info_fonction_trace_appel = nullptr;
    Type *type_trace_appel = nullptr;
    Type *type_base_allocatrice = nullptr;
    Type *type_info_appel_trace_appel = nullptr;
    Type *type_stockage_temporaire = nullptr;
    Type *type_annotation = nullptr;

    /* Trie pour les types fonctions. */
    Trie trie{};

    kuri::table_hachage<Type *, TypeTypeDeDonnees *> table_types_de_donnees{""};

    /* Sauvegarde des fonctions d'initialisation des types pour les partager entre types.
     * Ces fonctions sont créées avant que tout autre travail de compilation soit effectué, et
     * donc nous n'avons pas besoin de synchroniser la lecture ou l'écriture de ces données. */
    NoeudDeclarationEnteteFonction *init_type_n8 = nullptr;
    NoeudDeclarationEnteteFonction *init_type_n16 = nullptr;
    NoeudDeclarationEnteteFonction *init_type_n32 = nullptr;
    NoeudDeclarationEnteteFonction *init_type_n64 = nullptr;
    NoeudDeclarationEnteteFonction *init_type_z8 = nullptr;
    NoeudDeclarationEnteteFonction *init_type_z16 = nullptr;
    NoeudDeclarationEnteteFonction *init_type_z32 = nullptr;
    NoeudDeclarationEnteteFonction *init_type_z64 = nullptr;
    NoeudDeclarationEnteteFonction *init_type_pointeur = nullptr;

    struct DonnéesInsertionTypeGraphe {
        Type *type_parent = nullptr;
        Type *type_enfant = nullptr;
    };
    kuri::tableaux_partage_synchrones<DonnéesInsertionTypeGraphe> types_à_insérer_dans_graphe{};

    // -------------------------

    Typeuse(dls::outils::Synchrone<RegistreDesOpérateurs> &o);

    Typeuse(Typeuse const &) = delete;
    Typeuse &operator=(Typeuse const &) = delete;

    ~Typeuse();

    /* Ajoute des tâches avant le début de la compilation afin de préparer des données pour
     * celle-ci. */
    static void crée_tâches_précompilation(Compilatrice &compilatrice);

    Type *type_pour_lexeme(GenreLexeme lexeme);

    TypePointeur *type_pointeur_pour(Type *type,
                                     bool ajoute_operateurs = true,
                                     bool insere_dans_graphe = true);

    TypeReference *type_reference_pour(Type *type);

    TypeTableauFixe *type_tableau_fixe(Type *type_pointe,
                                       int taille,
                                       bool insere_dans_graphe = true);

    TypeTableauDynamique *type_tableau_dynamique(Type *type_pointe,
                                                 bool insere_dans_graphe = true);

    TypeVariadique *type_variadique(Type *type_pointe);

    TypeFonction *discr_type_fonction(TypeFonction *it, kuri::tablet<Type *, 6> const &entrees);

    TypeFonction *type_fonction(kuri::tablet<Type *, 6> const &entrees,
                                Type *type_sortie,
                                bool ajoute_operateurs = true);

    TypeTypeDeDonnees *type_type_de_donnees(Type *type_connu);

    TypeStructure *reserve_type_structure(NoeudStruct *decl);

    TypeEnum *reserve_type_enum(NoeudEnum *decl);

    TypeUnion *reserve_type_union(NoeudStruct *decl);

    TypeUnion *union_anonyme(const kuri::tablet<MembreTypeComposé, 6> &membres);

    TypeEnum *reserve_type_erreur(NoeudEnum *decl);

    TypePolymorphique *crée_polymorphique(IdentifiantCode *ident);

    TypeOpaque *crée_opaque(NoeudDeclarationTypeOpaque *decl, Type *type_opacifie);

    TypeOpaque *monomorphe_opaque(NoeudDeclarationTypeOpaque *decl, Type *type_monomorphique);

    TypeTuple *crée_tuple(const kuri::tablet<MembreTypeComposé, 6> &membres);

    void rassemble_statistiques(Statistiques &stats) const;

    NoeudDeclaration *decl_pour_info_type(const InfoType *info_type);
};

/* ------------------------------------------------------------------------- */
/** \name Fonctions diverses pour les types.
 * \{ */

void assigne_fonction_init(Type *type, NoeudDeclarationEnteteFonction *fonction);

/* Retourne vrai si le type à besoin d'une fonction d'initialisation que celle-ci soit partagée
 * ou non.
 */
bool requiers_fonction_initialisation(Type const *type);

/* Retourne vrai si une fonction d'initialisation doit être créée pour ce type, s'il en besoin
 * et qu'elle n'a pas encore été créée.
 */
bool requiers_création_fonction_initialisation(Type const *type);

/* Retourne le type entier sous-jacent pour les types pouvant être représentés par un nombre entier
 * (p.e. les énumérations). Retourne nul sinon. */
Type const *type_entier_sous_jacent(Type const *type);

/** Si \a type_base_potentiel est un type employé par \a type_dérivé, ou employé par un type
 * employé par \a type_dérivé, retourne le décalage absolu en octet dans la structure de \a
 * type_dérivé du \a type_base_potentiel. Ceci prend en compte le décalage des emplois
 * intermédiaire pour arriver à \a type_base_potentiel.
 * S'il n'y a aucune filliation entre les types, ne retourne rien. */
std::optional<uint32_t> est_type_de_base(TypeStructure const *type_dérivé,
                                         TypeStructure const *type_base_potentiel);

std::optional<uint32_t> est_type_de_base(Type const *type_dérivé, Type const *type_base_potentiel);

bool est_type_pointeur_nul(Type const *type);

/* Calcule la « profondeur » du type : à savoir, le nombre de déréférencement du type (jusqu'à
 * arriver à un type racine) + 1.
 * Par exemple, *z32 a une profondeur de 2 (1 déréférencement de pointeur + 1), alors que []*z32 en
 * a une de 3. */
int donne_profondeur_type(Type const *type);

/* Retourne vrai la variable est d'un type pouvant être le membre d'une structure. */
bool est_type_valide_pour_membre(Type const *membre_type);

bool peut_construire_union_via_rien(TypeUnion const *type_union);

/* Décide si le type peut être utilisé pour les expressions d'indexages basiques du langage.
 * NOTE : les entiers relatifs ne sont pas considérées ici car nous utilisons cette décision pour
 * transtyper automatiquement vers le type cible (z64), et nous les gérons séparément. */
bool est_type_implicitement_utilisable_pour_indexage(Type const *type);

bool peut_etre_type_constante(Type const *type);

/**
 * Retourne vrai si type_dest opacifie type_source.
 */
bool est_type_opacifié(Type const *type_dest, Type const *type_source);

/**
 * Retourne vrai si le type est un type fondamental (p.e. z32, r32, bool, etc.).
 */
bool est_type_fondamental(Type const *type);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Accès aux membres des types composés.
 * \{ */

std::optional<InformationMembreTypeCompose> donne_membre_pour_type(TypeCompose const *type_composé,
                                                                   Type const *type);

std::optional<InformationMembreTypeCompose> donne_membre_pour_nom(
    TypeCompose const *type_composé, IdentifiantCode const *nom_membre);

template <typename T, int tag>
struct ValeurOpaqueTaguee {
    T valeur;
};

enum {
    INDEX_MEMBRE = 0,
    AUCUN_TROUVE = 1,
    PLUSIEURS_TROUVES = 2,
};

using IndexMembre = ValeurOpaqueTaguee<int, INDEX_MEMBRE>;
using PlusieursMembres = ValeurOpaqueTaguee<int, PLUSIEURS_TROUVES>;
using AucunMembre = ValeurOpaqueTaguee<int, AUCUN_TROUVE>;

using ResultatRechercheMembre = std::variant<IndexMembre, PlusieursMembres, AucunMembre>;

ResultatRechercheMembre trouve_index_membre_unique_type_compatible(TypeCompose const *type,
                                                                   Type const *type_a_tester);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Accès aux noms hiérarchiques des types.
 * \{ */

kuri::chaine_statique donne_nom_hiérarchique(TypeUnion *type);
kuri::chaine_statique donne_nom_hiérarchique(TypeEnum *type);
kuri::chaine_statique donne_nom_hiérarchique(TypeOpaque *type);
kuri::chaine_statique donne_nom_hiérarchique(TypeStructure *type);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Accès aux noms portables des types.
 * \{ */

kuri::chaine const &donne_nom_portable(TypeUnion *type);
kuri::chaine const &donne_nom_portable(TypeEnum *type);
kuri::chaine const &donne_nom_portable(TypeOpaque *type);
kuri::chaine const &donne_nom_portable(TypeStructure *type);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Marquage des types comme étant polymorphiques.
 * \{ */

void marque_polymorphique(TypeFonction *type);
void marque_polymorphique(TypeCompose *type);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Fonctions pour les unions.
 * \{ */

void crée_type_structure(Typeuse &typeuse, TypeUnion *type, unsigned alignement_membre_actif);

/** \} */

/* ************************************************************************** */

/* Retourne une chaine correspondant au nom du type.
 *
 * Si ajoute ajoute_nom_paramètres_polymorphiques est vrai, alors pour les types provenant d'une
 * monomorphisation, ceci ajoute les noms des paramètres avant les valeurs. C'est-à-dire, pour une
 * monomorphisation « Vec3(r32) » retourne « Vec3(T: r32) ».
 */
kuri::chaine chaine_type(Type const *type, bool ajoute_nom_paramètres_polymorphiques = true);

Type *type_dereference_pour(Type const *type);

inline bool est_type_entier(Type const *type)
{
    return type->est_type_entier_naturel() || type->est_type_entier_relatif();
}

bool est_type_booleen_implicite(Type *type);

bool est_type_tableau_fixe(Type const *type);

bool est_pointeur_vers_tableau_fixe(Type const *type);

/* Retourne vrai si le type possède un info type qui est seulement une instance de InfoType et non
 * un type dérivé. */
bool est_structure_info_type_défaut(GenreType genre);

void calcule_taille_type_compose(TypeCompose *type, bool compacte, uint32_t alignement_desire);

NoeudDeclaration *decl_pour_type(const Type *type);

/* Retourne le type à la racine d'une chaine potentielle de types opaques ou le type opacifié s'il
 * n'est pas lui-même un type opaque. */
Type const *donne_type_opacifié_racine(TypeOpaque const *type_opaque);

void attentes_sur_types_si_drapeau_manquant(kuri::ensemblon<Type *, 16> const &types,
                                            DrapeauxTypes drapeau,
                                            kuri::tablet<Attente, 16> &attentes);

std::optional<Attente> attente_sur_type_si_drapeau_manquant(
    kuri::ensemblon<Type *, 16> const &types, DrapeauxTypes drapeau);
