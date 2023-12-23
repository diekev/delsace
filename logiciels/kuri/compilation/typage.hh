/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "biblinternes/moultfilage/synchrone.hh"
#include "biblinternes/outils/assert.hh"
#include "biblinternes/structures/plage.hh"
#include "biblinternes/structures/tableau_page.hh"

#include "parsage/lexemes.hh"

#include "structures/chaine.hh"
#include "structures/ensemblon.hh"
#include "structures/table_hachage.hh"
#include "structures/tablet.hh"

#include "operateurs.hh"

struct AllocatriceNoeud;
struct AtomeGlobale;
struct Compilatrice;
struct GrapheDependance;
struct IdentifiantCode;
struct InfoType;
struct RegistreDesOpérateurs;
struct OpérateurBinaire;
struct OpérateurUnaire;
struct MembreTypeComposé;
struct NoeudBloc;
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

struct NoeudDeclarationType;
struct NoeudDeclarationTypeCompose;
struct NoeudDeclarationTypeFonction;
struct NoeudDeclarationTypePointeur;
struct NoeudDeclarationTypePolymorphique;
struct NoeudDeclarationTypeReference;
struct NoeudDeclarationTypeTableauDynamique;
struct NoeudDeclarationTypeTableauFixe;
struct NoeudDeclarationTypeTuple;
struct NoeudDeclarationTypeTypeDeDonnees;
struct NoeudDeclarationTypeVariadique;
struct NoeudUnion;

using Type = NoeudDeclarationType;
using TypeStructure = NoeudStruct;
using TypeEnum = NoeudEnum;
using TypeFonction = NoeudDeclarationTypeFonction;
using TypeOpaque = NoeudDeclarationTypeOpaque;
using TypePointeur = NoeudDeclarationTypePointeur;
using TypePolymorphique = NoeudDeclarationTypePolymorphique;
using TypeReference = NoeudDeclarationTypeReference;
using TypeTableauDynamique = NoeudDeclarationTypeTableauDynamique;
using TypeTableauFixe = NoeudDeclarationTypeTableauFixe;
using TypeTuple = NoeudDeclarationTypeTuple;
using TypeTypeDeDonnees = NoeudDeclarationTypeTypeDeDonnees;
using TypeUnion = NoeudUnion;
using TypeVariadique = NoeudDeclarationTypeVariadique;
using TypeCompose = NoeudDeclarationTypeCompose;

enum class GenreNoeud : uint8_t;
enum class DrapeauxNoeud : uint32_t;
enum class DrapeauxTypes : uint32_t;

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

bool est_type_polymorphique(Type const *type);

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
    dls::outils::Synchrone<GrapheDependance> &graphe_;
    dls::outils::Synchrone<RegistreDesOpérateurs> &operateurs_;

    // NOTE : nous synchronisons les tableaux individuellement et non la Typeuse
    // dans son entièreté afin que différents threads puissent accéder librement
    // à différents types de types.
    kuri::tableau_synchrone<Type *> types_simples{};
    AllocatriceNoeud *alloc = nullptr;
    std::mutex mutex_types_pointeurs{};
    std::mutex mutex_types_references{};
    std::mutex mutex_types_structures{};
    std::mutex mutex_types_enums{};
    std::mutex mutex_types_tableaux_fixes{};
    std::mutex mutex_types_tableaux_dynamiques{};
    std::mutex mutex_types_fonctions{};
    std::mutex mutex_types_variadiques{};
    std::mutex mutex_types_unions{};
    std::mutex mutex_types_type_de_donnees{};
    std::mutex mutex_types_polymorphiques{};
    std::mutex mutex_types_opaques{};
    std::mutex mutex_types_tuples{};

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

  private:
    std::mutex mutex_infos_types_vers_types{};
    kuri::table_hachage<InfoType const *, Type const *> m_infos_types_vers_types{""};

  public:
    // -------------------------

    Typeuse(dls::outils::Synchrone<GrapheDependance> &g,
            dls::outils::Synchrone<RegistreDesOpérateurs> &o);

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

    TypeStructure *reserve_type_structure();

    TypeUnion *union_anonyme(Lexeme const *lexeme,
                             NoeudBloc *bloc_parent,
                             const kuri::tablet<MembreTypeComposé, 6> &membres);

    TypePolymorphique *crée_polymorphique(IdentifiantCode *ident);

    TypeOpaque *monomorphe_opaque(NoeudDeclarationTypeOpaque const *decl,
                                  Type *type_monomorphique);

    TypeTuple *crée_tuple(const kuri::tablet<MembreTypeComposé, 6> &membres);

    void rassemble_statistiques(Statistiques &stats) const;

    void définis_info_type_pour_type(const InfoType *info_type, const Type *type);

    NoeudDeclaration const *decl_pour_info_type(const InfoType *info_type);
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

struct InformationMembreTypeCompose;

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

enum class OptionsImpressionType : uint32_t {
    AUCUNE = 0,
    AJOUTE_PARAMÈTRES_POLYMORPHIQUE = (1u << 0),
    NORMALISE_PARENTHÈSE_PARAMÈTRE = (1u << 1),
    EXCLUS_TYPE_SOUS_JACENT = (1u << 2),
    NORMALISE_SÉPARATEUR_HIÉRARCHIE = (1u << 3),
    NORMALISE_PARENTHÈSE_FONCTION = (1u << 4),
    NORMALISE_SPÉCIFIANT_TYPE = (1u << 5),
    INCLUS_HIÉRARCHIE = (1u << 6),

    /* Options pour le nom des fonctions d'initialisation. */
    POUR_FONCTION_INITIALISATION = (INCLUS_HIÉRARCHIE | NORMALISE_PARENTHÈSE_PARAMÈTRE |
                                    NORMALISE_SÉPARATEUR_HIÉRARCHIE |
                                    NORMALISE_PARENTHÈSE_FONCTION | NORMALISE_SPÉCIFIANT_TYPE),
};
DEFINIS_OPERATEURS_DRAPEAU(OptionsImpressionType)

kuri::chaine chaine_type(Type const *type, OptionsImpressionType options);

Type *type_dereference_pour(Type const *type);

bool est_type_entier(Type const *type);

bool est_type_booleen_implicite(Type *type);

bool est_type_tableau_fixe(Type const *type);

bool est_pointeur_vers_tableau_fixe(Type const *type);

/* Retourne vrai si le type possède un info type qui est seulement une instance de InfoType et non
 * un type dérivé. */
bool est_structure_info_type_défaut(GenreNoeud genre);

void calcule_taille_type_compose(TypeCompose *type, bool compacte, uint32_t alignement_desire);

/* Retourne le type à la racine d'une chaine potentielle de types opaques ou le type opacifié s'il
 * n'est pas lui-même un type opaque. */
Type const *donne_type_opacifié_racine(TypeOpaque const *type_opaque);

void attentes_sur_types_si_drapeau_manquant(kuri::ensemblon<Type *, 16> const &types,
                                            DrapeauxTypes drapeau,
                                            kuri::tablet<Attente, 16> &attentes);

std::optional<Attente> attente_sur_type_si_drapeau_manquant(
    kuri::ensemblon<Type *, 16> const &types, DrapeauxNoeud drapeau);

std::optional<Attente> attente_sur_type_si_drapeau_manquant(
    kuri::ensemblon<Type *, 16> const &types, DrapeauxTypes drapeau);
