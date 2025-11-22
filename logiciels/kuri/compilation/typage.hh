/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "arbre_syntaxique/prodeclaration.hh"

#include "parsage/lexemes.hh"

#include "structures/chaine.hh"
#include "structures/ensemblon.hh"
#include "structures/table_hachage.hh"
#include "structures/tableau_page.hh"
#include "structures/tableaux_partage_synchrones.hh"
#include "structures/tablet.hh"

#include "utilitaires/synchrone.hh"

#include "operateurs.hh"

#include "plateforme/windows.h"

struct AllocatriceNoeud;
struct Compilatrice;
struct IdentifiantCode;
struct InfoType;
struct RegistreDesOpérateurs;
struct RubriqueTypeComposé;
struct Statistiques;

using Type = NoeudDéclarationType;
using TypeStructure = NoeudStruct;
using TypeEnum = NoeudEnum;
using TypeFonction = NoeudDéclarationTypeFonction;
using TypeOpaque = NoeudDéclarationTypeOpaque;
using TypePointeur = NoeudDéclarationTypePointeur;
using TypePolymorphique = NoeudDéclarationTypePolymorphique;
using TypeReference = NoeudDéclarationTypeRéférence;
using TypeTableauDynamique = NoeudDéclarationTypeTableauDynamique;
using TypeTableauFixe = NoeudDéclarationTypeTableauFixe;
using TypeTuple = NoeudDéclarationTypeTuple;
using TypeTypeDeDonnees = NoeudDéclarationTypeTypeDeDonnées;
using TypeUnion = NoeudUnion;
using TypeVariadique = NoeudDéclarationTypeVariadique;
using TypeCompose = NoeudDéclarationTypeComposé;

enum class GenreNoeud : uint8_t;
enum class DrapeauxNoeud : uint32_t;
enum class DrapeauxTypes : uint32_t;

/* ************************************************************************** */

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
    kuri::tableau_page<Noeud> noeuds{};

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

struct Typeuse {
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
    std::mutex mutex_types_tranches{};
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
    Type *type_info_type_rubrique_structure = nullptr;
    Type *type_info_type_entier = nullptr;
    Type *type_info_type_tableau = nullptr;
    Type *type_info_type_tableau_fixe = nullptr;
    Type *type_info_type_tranche = nullptr;
    Type *type_info_type_pointeur = nullptr;
    Type *type_info_type_polymorphique = nullptr;
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
    NoeudDéclarationEntêteFonction *init_type_n8 = nullptr;
    NoeudDéclarationEntêteFonction *init_type_n16 = nullptr;
    NoeudDéclarationEntêteFonction *init_type_n32 = nullptr;
    NoeudDéclarationEntêteFonction *init_type_n64 = nullptr;
    NoeudDéclarationEntêteFonction *init_type_z8 = nullptr;
    NoeudDéclarationEntêteFonction *init_type_z16 = nullptr;
    NoeudDéclarationEntêteFonction *init_type_z32 = nullptr;
    NoeudDéclarationEntêteFonction *init_type_z64 = nullptr;
    NoeudDéclarationEntêteFonction *init_type_pointeur = nullptr;

    NoeudDéclarationType *type_n8 = nullptr;
    NoeudDéclarationType *type_n16 = nullptr;
    NoeudDéclarationType *type_n32 = nullptr;
    NoeudDéclarationType *type_n64 = nullptr;
    NoeudDéclarationType *type_z8 = nullptr;
    NoeudDéclarationType *type_z16 = nullptr;
    NoeudDéclarationType *type_z32 = nullptr;
    NoeudDéclarationType *type_z64 = nullptr;
    NoeudDéclarationType *type_r16 = nullptr;
    NoeudDéclarationType *type_r32 = nullptr;
    NoeudDéclarationType *type_r64 = nullptr;
    NoeudDéclarationType *type_rien = nullptr;
    NoeudDéclarationType *type_bool = nullptr;
    NoeudDéclarationType *type_adresse_fonction = nullptr;

    NoeudDéclarationTypePointeur *type_ptr_nul = nullptr;
    NoeudDéclarationTypePointeur *type_ptr_n8 = nullptr;
    NoeudDéclarationTypePointeur *type_ptr_z8 = nullptr;
    NoeudDéclarationTypePointeur *type_ptr_octet = nullptr;
    NoeudDéclarationTypePointeur *type_ptr_rien = nullptr;

    NoeudDéclarationTypeRéférence *type_ref_n8 = nullptr;
    NoeudDéclarationTypeRéférence *type_ref_n64 = nullptr;
    NoeudDéclarationTypeRéférence *type_ref_z8 = nullptr;

    NoeudDéclarationTypeTableauDynamique *type_tabl_n8 = nullptr;

    NoeudDéclarationTypeOpaque *type_octet = nullptr;
    NoeudDéclarationTypeOpaque *type_entier_constant = nullptr;

    NoeudDéclarationTypeOpaque *type_dff_adr = nullptr;
    NoeudDéclarationTypeOpaque *type_adr_plt_nat = nullptr;
    NoeudDéclarationTypeOpaque *type_adr_plt_rel = nullptr;
    NoeudDéclarationTypeOpaque *type_taille_nat = nullptr;
    NoeudDéclarationTypeOpaque *type_taille_rel = nullptr;
    NoeudDéclarationTypeOpaque *type_nbr_nat = nullptr;
    NoeudDéclarationTypeOpaque *type_nbr_rel = nullptr;
    NoeudDéclarationTypeOpaque *type_nbf_flt = nullptr;

    NoeudDéclarationTypeComposé *type_eini = nullptr;
    NoeudDéclarationTypeComposé *type_chaine = nullptr;

    NoeudDéclarationType *type_tranche_octet = nullptr;

    NoeudDéclarationType *type_entier_vers_pointeur_nat = nullptr;
    NoeudDéclarationType *type_entier_vers_pointeur = nullptr;
    NoeudDéclarationType *type_taille_tableau = nullptr;
    NoeudDéclarationType *type_pointeur_vers_entier = nullptr;
    NoeudDéclarationType *type_indexage = nullptr;

    struct DonnéesInsertionTypeGraphe {
        Type *type_parent = nullptr;
        Type *type_enfant = nullptr;
    };
    kuri::tableaux_partage_synchrones<DonnéesInsertionTypeGraphe> types_à_insérer_dans_graphe{};

  private:
    std::mutex mutex_infos_types_vers_types{};
    kuri::table_hachage<InfoType const *, Type const *> m_infos_types_vers_types{""};

  public:
    // -------------------------

    Typeuse();

    Typeuse(Typeuse const &) = delete;
    Typeuse &operator=(Typeuse const &) = delete;

    ~Typeuse();

    /* Ajoute des tâches avant le début de la compilation afin de préparer des données pour
     * celle-ci. */
    static void crée_tâches_précompilation(Compilatrice &compilatrice, EspaceDeTravail *espace);

    Type *type_pour_lexeme(GenreLexème lexeme);

    TypePointeur *type_pointeur_pour(Type *type, bool insere_dans_graphe = true);

    TypeReference *type_reference_pour(Type *type);

    TypeTableauFixe *type_tableau_fixe(Type *type_pointe,
                                       int taille,
                                       bool insere_dans_graphe = true);

    TypeTableauFixe *type_tableau_fixe(NoeudExpression const *expression_taille,
                                       Type *type_élément);

    TypeTableauDynamique *type_tableau_dynamique(Type *type_pointe,
                                                 bool insere_dans_graphe = true);

    NoeudDéclarationTypeTranche *crée_type_tranche(Type *type_élément,
                                                   bool insère_dans_graphe = true);

    TypeVariadique *type_variadique(Type *type_pointe);

    TypeFonction *discr_type_fonction(TypeFonction *it, kuri::tablet<Type *, 6> const &entrees);

    TypeFonction *type_fonction(kuri::tablet<Type *, 6> const &entrees, Type *type_sortie);

    TypeTypeDeDonnees *type_type_de_donnees(Type *type_connu);

    TypeStructure *réserve_type_structure();

    TypeUnion *union_anonyme(Lexème const *lexeme,
                             NoeudBloc *bloc_parent,
                             const kuri::tablet<RubriqueTypeComposé, 6> &rubriques);

    TypePolymorphique *crée_polymorphique(IdentifiantCode *ident);

    TypeOpaque *monomorphe_opaque(NoeudDéclarationTypeOpaque const *decl,
                                  Type *type_monomorphique);

    TypeTuple *crée_tuple(const kuri::tablet<RubriqueTypeComposé, 6> &rubriques);

    void rassemble_statistiques(Statistiques &stats) const;

    void définis_info_type_pour_type(const InfoType *info_type, const Type *type);

    NoeudDéclaration const *decl_pour_info_type(const InfoType *info_type);

  private:
    NoeudDéclarationTypeOpaque *crée_opaque_défaut(Type *type_opacifié, IdentifiantCode *ident);
};

/* ------------------------------------------------------------------------- */
/** \name Fonctions diverses pour les types.
 * \{ */

void assigne_fonction_init(Type *type, NoeudDéclarationEntêteFonction *fonction);

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

/* Retourne le type primitif du type donné. Par exemple, pour un type énumération, son type
 * sous-jacent. */
Type const *donne_type_primitif(Type const *type);

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
 * Par exemple, *z32 a une profondeur de 2 (1 déréférencement de pointeur + 1), alors que [..]*z32
 * en a une de 3. */
int donne_profondeur_type(Type const *type);

/* Retourne vrai la variable est d'un type pouvant être la rubrique d'une structure. */
bool est_type_valide_pour_rubrique(Type const *rubrique_type);

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
/** \name Accès aux rubriques des types composés.
 * \{ */

struct InformationRubriqueTypeCompose;

std::optional<InformationRubriqueTypeCompose> donne_rubrique_pour_type(
    TypeCompose const *type_composé, Type const *type);

std::optional<InformationRubriqueTypeCompose> donne_rubrique_pour_nom(
    TypeCompose const *type_composé, IdentifiantCode const *nom_rubrique);

template <typename T, int tag>
struct ValeurOpaqueTaguee {
    T valeur;
};

enum {
    INDICE_RUBRIQUE = 0,
    AUCUN_TROUVE = 1,
    PLUSIEURS_TROUVES = 2,
};

using IndexRubrique = ValeurOpaqueTaguee<int, INDICE_RUBRIQUE>;
using PlusieursRubriques = ValeurOpaqueTaguee<int, PLUSIEURS_TROUVES>;
using AucunRubrique = ValeurOpaqueTaguee<int, AUCUN_TROUVE>;

using ResultatRechercheRubrique = std::variant<IndexRubrique, PlusieursRubriques, AucunRubrique>;

ResultatRechercheRubrique trouve_indice_rubrique_unique_type_compatible(TypeCompose const *type,
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

kuri::chaine_statique donne_nom_portable(TypeUnion *type);
kuri::chaine_statique donne_nom_portable(TypeEnum *type);
kuri::chaine_statique donne_nom_portable(TypeOpaque *type);
kuri::chaine_statique donne_nom_portable(TypeStructure *type);

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

void crée_type_structure(Typeuse &typeuse, TypeUnion *type, unsigned alignement_rubrique_active);

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
    NORMALISE_SÉPARATEUR_RUBRIQUES_ANONYMES = (1u << 7),

    /* Options pour le nom des fonctions d'initialisation. */
    POUR_FONCTION_INITIALISATION = (INCLUS_HIÉRARCHIE | NORMALISE_PARENTHÈSE_PARAMÈTRE |
                                    NORMALISE_SÉPARATEUR_HIÉRARCHIE |
                                    NORMALISE_PARENTHÈSE_FONCTION | NORMALISE_SPÉCIFIANT_TYPE |
                                    NORMALISE_SÉPARATEUR_RUBRIQUES_ANONYMES),
};
DEFINIS_OPERATEURS_DRAPEAU(OptionsImpressionType)

kuri::chaine chaine_type(Type const *type, OptionsImpressionType options);

Type *type_déréférencé_pour(Type const *type);

bool est_type_entier(Type const *type);

bool est_type_ptr_rien(const Type *type);

bool est_type_ptr_octet(const Type *type);

bool est_type_booléen_implicite(Type *type);

bool est_type_tableau_fixe(Type const *type);

bool est_pointeur_vers_tableau_fixe(Type const *type);

bool est_type_sse2(Type const *type);

bool stockage_type_doit_utiliser_memcpy(Type const *type);

/* Retourne vrai si le type possède un info type qui est seulement une instance de InfoType et non
 * un type dérivé. */
bool est_structure_info_type_défaut(GenreNoeud genre);

void calcule_taille_type_composé(TypeCompose *type, bool compacte, uint32_t alignement_desire);

/* Retourne le type à la racine d'une chaine potentielle de types opaques ou le type opacifié s'il
 * n'est pas lui-même un type opaque. */
Type const *donne_type_opacifié_racine(TypeOpaque const *type_opaque);

void attentes_sur_types_si_drapeau_manquant(kuri::ensemblon<Type *, 16> const &types,
                                            DrapeauxTypes drapeau,
                                            kuri::tablet<Attente, 16> &attentes);

void attentes_sur_types_si_drapeau_manquant(kuri::ensemblon<Type *, 16> const &types,
                                            DrapeauxNoeud drapeau,
                                            kuri::tablet<Attente, 16> &attentes);

/* ------------------------------------------------------------------------- */
/** \name VisiteuseType.
 * \{ */

struct VisiteuseType {
    kuri::ensemble<Type *> visites{};
    bool visite_types_fonctions_init = true;

    void visite_type(Type *type, std::function<void(Type *)> rappel);
};

/** \} */

NoeudDéclarationClasse const *donne_polymorphe_de_base(Type const *type);
