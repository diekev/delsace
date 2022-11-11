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
#include "structures/tablet.hh"

#include "operateurs.hh"

struct AtomeConstante;
struct GrapheDependance;
struct IdentifiantCode;
struct InfoType;
struct Operateurs;
struct OperateurBinaire;
struct OperateurUnaire;
struct NoeudDeclarationVariable;
struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationTypeOpaque;
struct NoeudDependance;
struct NoeudEnum;
struct NoeudExpression;
struct NoeudStruct;
struct Statistiques;
struct Typeuse;
struct TypeCompose;
struct TypeEnum;
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

enum class TypeBase : char {
    N8,
    N16,
    N32,
    N64,
    Z8,
    Z16,
    Z32,
    Z64,
    R16,
    R32,
    R64,
    EINI,
    CHAINE,
    RIEN,
    BOOL,
    OCTET,
    ENTIER_CONSTANT,

    PTR_N8,
    PTR_N16,
    PTR_N32,
    PTR_N64,
    PTR_Z8,
    PTR_Z16,
    PTR_Z32,
    PTR_Z64,
    PTR_R16,
    PTR_R32,
    PTR_R64,
    PTR_EINI,
    PTR_CHAINE,
    PTR_RIEN,
    PTR_NUL,
    PTR_BOOL,
    PTR_OCTET,

    REF_N8,
    REF_N16,
    REF_N32,
    REF_N64,
    REF_Z8,
    REF_Z16,
    REF_Z32,
    REF_Z64,
    REF_R16,
    REF_R32,
    REF_R64,
    REF_EINI,
    REF_CHAINE,
    REF_RIEN,
    REF_BOOL,

    TABL_N8,
    TABL_N16,
    TABL_N32,
    TABL_N64,
    TABL_Z8,
    TABL_Z16,
    TABL_Z32,
    TABL_Z64,
    TABL_R16,
    TABL_R32,
    TABL_R64,
    TABL_EINI,
    TABL_CHAINE,
    TABL_BOOL,
    TABL_OCTET,

    TOTAL,
};

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

enum {
    /* DISPONIBLE = 1, */
    TYPE_EST_POLYMORPHIQUE = 2,
    TYPE_FUT_VALIDE = 4,
    INITIALISATION_TYPE_FUT_CREEE = 8,
    POSSEDE_TYPE_POINTEUR = 16,
    POSSEDE_TYPE_REFERENCE = 32,
    POSSEDE_TYPE_TABLEAU_FIXE = 64,
    POSSEDE_TYPE_TABLEAU_DYNAMIQUE = 128,
    POSSEDE_TYPE_TYPE_DE_DONNEES = 256,
    CODE_BINAIRE_TYPE_FUT_GENERE = 512,
    TYPE_EST_NORMALISE = 1024,
    /* DISPONIBLE = 2048, */
    UNITE_POUR_INITIALISATION_FUT_CREE = 4096,
};

struct Type {
    GenreType genre{};
    unsigned taille_octet = 0;
    unsigned alignement = 0;
    int drapeaux = 0;
    unsigned index_dans_table_types = 0;

    kuri::chaine nom_broye{};

    InfoType *info_type = nullptr;
    AtomeConstante *atome_info_type = nullptr;
    NoeudDependance *noeud_dependance = nullptr;

    NoeudDeclarationEnteteFonction *fonction_init = nullptr;

    TableOperateurs operateurs{};

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

    /* À FAIRE: déplace ceci dans une table? */
    TypePointeur *type_pointeur = nullptr;

    POINTEUR_NUL(Type)

    static Type *cree_entier(unsigned taille_octet, bool est_naturel);
    static Type *cree_entier_constant();
    static Type *cree_reel(unsigned taille_octet);
    static Type *cree_rien();
    static Type *cree_bool();
    static Type *cree_octet();

#define __DEFINIS_DISCRIMINATIONS(nom, Genre, TypeRafine)                                         \
    inline bool est_##nom() const                                                                 \
    {                                                                                             \
        return genre == GenreType::Genre;                                                         \
    }

    ENUMERE_TYPE(__DEFINIS_DISCRIMINATIONS)

#undef __DEFINIS_DISCRIMINATIONS

#define __DECLARE_COMME_TYPE(nom, Genre, TypeRafine)                                              \
    inline TypeRafine *comme_##nom();                                                             \
    inline const TypeRafine *comme_##nom() const;

    ENUMERE_TYPE(__DECLARE_COMME_TYPE)

#undef __DECLARE_COMME_TYPE

    inline TypeCompose *comme_compose();
    inline const TypeCompose *comme_compose() const;
};

struct TypePointeur : public Type {
    TypePointeur()
    {
        genre = GenreType::POINTEUR;
    }

    explicit TypePointeur(Type *type_pointe);

    COPIE_CONSTRUCT(TypePointeur);

    Type *type_pointe = nullptr;
};

struct TypeReference : public Type {
    TypeReference()
    {
        genre = GenreType::REFERENCE;
    }

    explicit TypeReference(Type *type_pointe);

    COPIE_CONSTRUCT(TypeReference);

    Type *type_pointe = nullptr;
};

struct TypeFonction : public Type {
    TypeFonction()
    {
        genre = GenreType::FONCTION;
    }

    TypeFonction(kuri::tablet<Type *, 6> const &entrees, Type *sortie);

    COPIE_CONSTRUCT(TypeFonction);

    kuri::tableau<Type *, int> types_entrees{};
    Type *type_sortie{};

    uint64_t tag_entrees = 0;
    uint64_t tag_sorties = 0;

    void marque_polymorphique();
};

/* Type de base pour tous les types ayant des membres (structures, énumérations, etc.).
 */
struct TypeCompose : public Type {
    struct Membre {
        enum {
            // si le membre est une constante (par exemple, la définition d'une énumération, ou une
            // simple valeur)
            EST_CONSTANT = (1 << 0),
            // si le membre est défini par la compilatrice (par exemple, « nombre_éléments » des
            // énumérations)
            EST_IMPLICITE = (1 << 1),
        };

        NoeudDeclarationVariable *decl = nullptr;
        Type *type = nullptr;
        IdentifiantCode *nom = nullptr;
        unsigned decalage = 0;
        int valeur = 0;                                       // pour les énumérations
        NoeudExpression *expression_valeur_defaut = nullptr;  // pour les membres des structures
        int drapeaux = 0;
    };

    kuri::tableau<Membre, int> membres{};

    /* Le nom tel que donné dans le script (p.e. Structure, pour Structure :: struct ...). */
    IdentifiantCode *nom = nullptr;

    /* Le nom final, contenant les informations de portée (p.e. ModuleStructure, pour Structure ::
     * struct dans le module Module). */
    kuri::chaine nom_portable_{};

    /* Le nom de la hierarchie, sans le nom du module. Chaque nom est séparé par des points.
     * Ceci est le nom qui sera utilisé dans les infos types.
     * À FAIRE : remplace ceci par l'utilisation d'un pointeur dans les infos-types contenant la
     * type parent. */
    kuri::chaine nom_hierarchique_ = "";

    static TypeCompose *cree_eini();

    static TypeCompose *cree_chaine();

    void marque_polymorphique();
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

    COPIE_CONSTRUCT(TypeStructure);

    kuri::tableau<TypeStructure *, int> types_employes{};

    NoeudStruct *decl = nullptr;

    TypeUnion *union_originelle = nullptr;

    bool est_anonyme = false;

    kuri::chaine const &nom_portable();
    kuri::chaine_statique nom_hierarchique();
};

struct TypeUnion final : public TypeCompose {
    TypeUnion()
    {
        genre = GenreType::UNION;
    }

    COPIE_CONSTRUCT(TypeUnion);

    Type *type_le_plus_grand = nullptr;
    TypeStructure *type_structure = nullptr;

    NoeudStruct *decl = nullptr;

    unsigned decalage_index = 0;
    bool est_nonsure = false;
    bool est_anonyme = false;

    void cree_type_structure(Typeuse &typeuse, unsigned alignement_membre_actif);

    kuri::chaine const &nom_portable();
    kuri::chaine_statique nom_hierarchique();
};

struct TypeEnum final : public TypeCompose {
    TypeEnum()
    {
        genre = GenreType::ENUM;
    }

    COPIE_CONSTRUCT(TypeEnum);

    Type *type_donnees{};

    NoeudEnum *decl = nullptr;
    bool est_drapeau = false;
    bool est_erreur = false;

    kuri::chaine const &nom_portable();
    kuri::chaine_statique nom_hierarchique();
};

struct TypeTableauFixe final : public TypeCompose {
    TypeTableauFixe()
    {
        genre = GenreType::TABLEAU_FIXE;
    }

    TypeTableauFixe(Type *type_pointe, int taille, kuri::tableau<Membre, int> &&membres);

    COPIE_CONSTRUCT(TypeTableauFixe);

    Type *type_pointe = nullptr;
    int taille = 0;
};

struct TypeTableauDynamique final : public TypeCompose {
    TypeTableauDynamique()
    {
        genre = GenreType::TABLEAU_DYNAMIQUE;
    }

    TypeTableauDynamique(Type *type_pointe, kuri::tableau<TypeCompose::Membre, int> &&membres);

    COPIE_CONSTRUCT(TypeTableauDynamique);

    Type *type_pointe = nullptr;
};

struct TypeVariadique final : public TypeCompose {
    TypeVariadique()
    {
        genre = GenreType::VARIADIQUE;
    }

    TypeVariadique(Type *type_pointe, kuri::tableau<TypeCompose::Membre, int> &&membres);

    COPIE_CONSTRUCT(TypeVariadique);

    Type *type_pointe = nullptr;
    /* Type tableau dynamique pour la génération de code, si le type est ...z32, le type
     * tableau dynamique sera []z32. */
    Type *type_tableau_dyn = nullptr;
};

struct TypeTypeDeDonnees : public Type {
    TypeTypeDeDonnees()
    {
        genre = GenreType::TYPE_DE_DONNEES;
    }

    explicit TypeTypeDeDonnees(Type *type_connu);

    COPIE_CONSTRUCT(TypeTypeDeDonnees);

    // Non-nul si le type est connu lors de la compilation.
    Type *type_connu = nullptr;
};

struct TypePolymorphique : public Type {
    TypePolymorphique()
    {
        genre = GenreType::POLYMORPHIQUE;
        drapeaux = TYPE_EST_POLYMORPHIQUE;
    }

    explicit TypePolymorphique(IdentifiantCode *ident);

    COPIE_CONSTRUCT(TypePolymorphique);

    IdentifiantCode *ident = nullptr;

    bool est_structure_poly = false;
    NoeudStruct *structure = nullptr;
    kuri::tableau<Type *, int> types_constants_structure{};
};

struct TypeOpaque : public Type {
    TypeOpaque()
    {
        genre = GenreType::OPAQUE;
    }

    TypeOpaque(NoeudDeclarationTypeOpaque *decl_, Type *opacifie);

    COPIE_CONSTRUCT(TypeOpaque);

    NoeudDeclarationTypeOpaque *decl = nullptr;
    IdentifiantCode *ident = nullptr;
    Type *type_opacifie = nullptr;
    kuri::chaine nom_portable_ = "";
    kuri::chaine nom_hierarchique_ = "";

    kuri::chaine const &nom_portable();
    kuri::chaine_statique nom_hierarchique();
};

/* Pour les sorties multiples des fonctions. */
struct TypeTuple : public TypeCompose {
    TypeTuple()
    {
        genre = GenreType::TUPLE;
    }

    void marque_polymorphique();
};

/* ************************************************************************** */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#define __DEFINIS_COMME_TYPE(nom, Genre, TypeRafine)                                              \
    inline TypeRafine *Type::comme_##nom()                                                        \
    {                                                                                             \
        assert_rappel(genre == GenreType::Genre,                                                  \
                      [this] { std::cerr << "Le type est " << genre << "\n"; });                  \
        return static_cast<TypeRafine *>(this);                                                   \
    }                                                                                             \
    inline const TypeRafine *Type::comme_##nom() const                                            \
    {                                                                                             \
        assert_rappel(genre == GenreType::Genre,                                                  \
                      [this] { std::cerr << "Le type est " << genre << "\n"; });                  \
        return static_cast<const TypeRafine *>(this);                                             \
    }

ENUMERE_TYPE(__DEFINIS_COMME_TYPE)

#undef __DEFINIS_COMME_TYPE
#pragma GCC diagnostic pop

inline TypeCompose *Type::comme_compose()
{
    assert(est_type_compose(this));
    return static_cast<TypeCompose *>(this);
}

inline const TypeCompose *Type::comme_compose() const
{
    assert(est_type_compose(this));
    return static_cast<const TypeCompose *>(this);
}

/* ************************************************************************** */

void rassemble_noms_type_polymorphique(Type *type, kuri::tableau<kuri::chaine_statique> &noms);

// À FAIRE(table type) : il peut y avoir une concurrence critique pour l'assignation d'index aux
// types
struct Typeuse {
    dls::outils::Synchrone<GrapheDependance> &graphe_;
    dls::outils::Synchrone<Operateurs> &operateurs_;

    template <typename T>
    using tableau_synchrone = dls::outils::Synchrone<kuri::tableau<T, int>>;

    template <typename T>
    using tableau_page_synchrone = dls::outils::Synchrone<tableau_page<T>>;

    // NOTE : nous synchronisons les tableaux individuellement et non la Typeuse
    // dans son entièreté afin que différents threads puissent accéder librement
    // à différents types de types.
    kuri::tableau<Type *> types_communs{};
    tableau_synchrone<Type *> types_simples{};
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
    Type *type_position_code_source = nullptr;
    Type *type_info_fonction_trace_appel = nullptr;
    Type *type_trace_appel = nullptr;
    Type *type_base_allocatrice = nullptr;
    Type *type_info_appel_trace_appel = nullptr;
    Type *type_stockage_temporaire = nullptr;
    Type *type_annotation = nullptr;
    // séparés car nous devons désalloué selon la bonne taille et ce sont plus des types « simples
    // »
    TypeCompose *type_eini = nullptr;
    TypeCompose *type_chaine = nullptr;

    // -------------------------

    Typeuse(dls::outils::Synchrone<GrapheDependance> &g, dls::outils::Synchrone<Operateurs> &o);

    Typeuse(Typeuse const &) = delete;
    Typeuse &operator=(Typeuse const &) = delete;

    ~Typeuse();

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

    TypeUnion *union_anonyme(const kuri::tablet<TypeCompose::Membre, 6> &membres);

    TypeEnum *reserve_type_erreur(NoeudEnum *decl);

    TypePolymorphique *cree_polymorphique(IdentifiantCode *ident);

    TypeOpaque *cree_opaque(NoeudDeclarationTypeOpaque *decl, Type *type_opacifie);

    TypeOpaque *monomorphe_opaque(NoeudDeclarationTypeOpaque *decl, Type *type_monomorphique);

    TypeTuple *cree_tuple(const kuri::tablet<TypeCompose::Membre, 6> &membres);

    inline Type *operator[](TypeBase type_base) const
    {
        return types_communs[static_cast<long>(type_base)];
    }

    void rassemble_statistiques(Statistiques &stats) const;

    NoeudDeclaration *decl_pour_info_type(const InfoType *info_type);
};

/* ************************************************************************** */

kuri::chaine chaine_type(Type const *type);

Type *type_dereference_pour(Type *type);

inline bool est_type_entier(Type const *type)
{
    return type->genre == GenreType::ENTIER_NATUREL || type->genre == GenreType::ENTIER_RELATIF;
}

bool est_type_booleen_implicite(Type *type);

Type *normalise_type(Typeuse &typeuse, Type *type);

void calcule_taille_type_compose(TypeCompose *type, bool compacte, uint32_t alignement_desire);

NoeudDeclaration *decl_pour_type(const Type *type);

bool est_type_polymorphique(Type *type);

std::optional<Attente> attente_sur_type_si_drapeau_manquant(
    kuri::ensemblon<Type *, 16> const &types_utilises, int drapeau);
