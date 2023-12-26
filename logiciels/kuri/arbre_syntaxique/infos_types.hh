/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "biblinternes/structures/tableau_page.hh"

#include "structures/chaine_statique.hh"
#include "structures/tableau.hh"
#include "structures/tablet.hh"
#include "structures/tranche.hh"

/* À FAIRE : utilise AnnotationCode. */
struct Annotation;

/* Structures utilisées pour passer les informations des types au métaprogrammes.
 * Celles-ci sont les pendantes de celles dans le module Kuri et doivent être
 * synchronisées avec elles.
 */

enum class GenreInfoType : int {
    ENTIER,
    REEL,
    BOOLEEN,
    CHAINE,
    POINTEUR,
    STRUCTURE,
    FONCTION,
    TABLEAU,
    EINI,
    RIEN,
    ENUM,
    OCTET,
    TYPE_DE_DONNEES,
    UNION,
    OPAQUE,
    VARIADIQUE,
    TRANCHE,
};

struct InfoType {
    GenreInfoType genre{};
    uint32_t taille_en_octet = 0;
    uint32_t index_dans_table_des_types = 0;
};

struct InfoTypeEntier : public InfoType {
    bool est_signe = false;
};

struct InfoTypePointeur : public InfoType {
    InfoType *type_pointe = nullptr;
    bool est_reference = false;
};

struct InfoTypeTableau : public InfoType {
    InfoType *type_pointe = nullptr;
    bool est_tableau_fixe = false;
    int taille_fixe = 0;
};

struct InfoTypeTranche : public InfoType {
    InfoType *type_élément = nullptr;
};

struct InfoTypeMembreStructure {
    // Les Drapeaux sont définis dans MembreTypeComposé

    kuri::chaine_statique nom{};
    InfoType *info = nullptr;
    int64_t decalage = 0;  // décalage en octets dans la structure
    int drapeaux = 0;
    kuri::tranche<const Annotation *> annotations{};
};

struct InfoTypeStructure : public InfoType {
    kuri::chaine_statique nom{};
    kuri::tranche<InfoTypeMembreStructure *> membres{};
    kuri::tranche<InfoTypeStructure *> structs_employees{};
    kuri::tranche<const Annotation *> annotations{};
};

struct InfoTypeUnion : public InfoType {
    kuri::chaine_statique nom{};
    kuri::tranche<InfoTypeMembreStructure *> membres{};
    InfoType *type_le_plus_grand = nullptr;
    int64_t decalage_index = 0;
    bool est_sure = false;
    kuri::tranche<const Annotation *> annotations{};
};

struct InfoTypeFonction : public InfoType {
    kuri::tranche<InfoType *> types_entrees{};
    kuri::tranche<InfoType *> types_sorties{};
    bool est_coroutine = false;
};

struct InfoTypeEnum : public InfoType {
    kuri::chaine_statique nom{};
    kuri::tranche<int> valeurs{};  // À FAIRE typage selon énum
    kuri::tranche<kuri::chaine_statique> noms{};
    bool est_drapeau = false;
    InfoTypeEntier *type_sous_jacent = nullptr;
};

struct InfoTypeOpaque : public InfoType {
    kuri::chaine_statique nom{};
    InfoType *type_opacifie = nullptr;
};

struct InfoTypeVariadique : public InfoType {
    InfoType *type_élément = nullptr;
};

#define ENUME_TYPES_TRANCHES_INFO_TYPE(O)                                                         \
    O(Annotation const *, annotations)                                                            \
    O(int, valeurs_énums)                                                                         \
    O(kuri::chaine_statique, noms_énums)                                                          \
    O(InfoTypeMembreStructure *, membres)                                                         \
    O(InfoType *, tableau_info_type)                                                              \
    O(InfoTypeStructure *, structs_employées)

struct AllocatriceInfosType {
    tableau_page<InfoType> infos_types{};
    tableau_page<InfoTypeEntier> infos_types_entiers{};
    tableau_page<InfoTypeEnum> infos_types_enums{};
    tableau_page<InfoTypeFonction> infos_types_fonctions{};
    tableau_page<InfoTypeMembreStructure> infos_types_membres_structures{};
    tableau_page<InfoTypePointeur> infos_types_pointeurs{};
    tableau_page<InfoTypeStructure> infos_types_structures{};
    tableau_page<InfoTypeTableau> infos_types_tableaux{};
    tableau_page<InfoTypeTranche> infos_types_tranches{};
    tableau_page<InfoTypeUnion> infos_types_unions{};
    tableau_page<InfoTypeOpaque> infos_types_opaques{};
    tableau_page<InfoTypeVariadique> infos_types_variadiques{};

#define ENUME_TYPES_TRANCHES_INFO_TYPE_EX(type__, nom__)                                          \
    kuri::tableau<kuri::tranche<type__>> tranches_##nom__{};                                      \
    void stocke_tranche(kuri::tranche<type__> tranche)                                            \
    {                                                                                             \
        tranches_##nom__.ajoute(tranche);                                                         \
    }

    ENUME_TYPES_TRANCHES_INFO_TYPE(ENUME_TYPES_TRANCHES_INFO_TYPE_EX)

#undef ENUME_TYPES_TRANCHES_INFO_TYPE_EX

    int64_t memoire_utilisee() const;

    template <typename T>
    kuri::tranche<T> donne_tranche(kuri::tablet<T, 6> const &tableau);
};
