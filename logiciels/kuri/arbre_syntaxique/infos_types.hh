/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "structures/chaine_statique.hh"
#include "structures/tableau.hh"
#include "structures/tableau_page.hh"
#include "structures/tablet.hh"
#include "structures/tranche.hh"

/* À FAIRE : utilise AnnotationCode. */
struct Annotation;

/* À tenir synchronisé avec info_type.kuri.
 * Structures utilisées pour passer les informations des types au métaprogrammes.
 */

enum class GenreInfoType : int32_t {
    ENTIER = 0,
    RÉEL = 1,
    BOOLÉEN = 2,
    CHAINE = 3,
    POINTEUR = 4,
    STRUCTURE = 5,
    FONCTION = 6,
    TABLEAU = 7,
    EINI = 8,
    RIEN = 9,
    ÉNUM = 10,
    OCTET = 11,
    TYPE_DE_DONNÉES = 12,
    UNION = 13,
    OPAQUE = 14,
    VARIADIQUE = 15,
    TRANCHE = 16,
    TABLEAU_FIXE = 17,
    ADRESSE_FONCTION = 18,
};

struct InfoType {
    GenreInfoType genre{};
    uint32_t taille_en_octet = 0;
    uint32_t index_dans_table_des_types = 0;
};

struct InfoTypeEntier : public InfoType {
    bool est_signé = false;
};

struct InfoTypePointeur : public InfoType {
    InfoType *type_pointé = nullptr;
    bool est_référence = false;
};

struct InfoTypeTableau : public InfoType {
    InfoType *type_élément = nullptr;
};

struct InfoTypeTableauFixe : public InfoType {
    InfoType *type_élément = nullptr;
    uint32_t nombre_éléments = 0;
};

struct InfoTypeTranche : public InfoType {
    InfoType *type_élément = nullptr;
};

struct InfoTypeMembreStructure {
    // Les Drapeaux sont définis dans MembreTypeComposé

    kuri::chaine_statique nom{};
    InfoType *info = nullptr;
    int64_t décalage = 0;  // décalage en octets dans la structure
    int drapeaux = 0;
    kuri::tranche<const Annotation *> annotations{};
};

struct InfoTypeStructure : public InfoType {
    kuri::chaine_statique nom{};
    kuri::tranche<InfoTypeMembreStructure *> membres{};
    kuri::tranche<InfoTypeStructure *> structs_employées{};
    kuri::tranche<const Annotation *> annotations{};
    bool est_polymorphique = false;
    InfoTypeStructure *polymorphe_de_base = nullptr;
};

struct InfoTypeUnion : public InfoType {
    kuri::chaine_statique nom{};
    kuri::tranche<InfoTypeMembreStructure *> membres{};
    InfoType *type_le_plus_grand = nullptr;
    int64_t décalage_index = 0;
    bool est_sûre = false;
    bool est_polymorphique = false;
    kuri::tranche<const Annotation *> annotations{};
    InfoTypeUnion *polymorphe_de_base = nullptr;
};

struct InfoTypeFonction : public InfoType {
    kuri::tranche<InfoType *> types_entrée{};
    kuri::tranche<InfoType *> types_sortie{};
    bool est_coroutine = false;
};

struct InfoTypeÉnum : public InfoType {
    kuri::chaine_statique nom{};
    kuri::tranche<char> valeurs{};
    kuri::tranche<kuri::chaine_statique> noms{};
    bool est_drapeau = false;
    InfoTypeEntier *type_sous_jacent = nullptr;
};

struct InfoTypeOpaque : public InfoType {
    kuri::chaine_statique nom{};
    InfoType *type_opacifié = nullptr;
};

struct InfoTypeVariadique : public InfoType {
    InfoType *type_élément = nullptr;
};

#define ENUMERE_TYPES_INFO_TYPE(O)                                                                \
    O(InfoType, infos_types)                                                                      \
    O(InfoTypeEntier, infos_types_entiers)                                                        \
    O(InfoTypeÉnum, infos_types_énums)                                                            \
    O(InfoTypeFonction, infos_types_fonctions)                                                    \
    O(InfoTypeMembreStructure, infos_types_membres_structures)                                    \
    O(InfoTypeStructure, infos_types_structures)                                                  \
    O(InfoTypePointeur, infos_types_pointeurs)                                                    \
    O(InfoTypeTableau, infos_types_tableaux)                                                      \
    O(InfoTypeTableauFixe, infos_types_tableaux_fixes)                                            \
    O(InfoTypeTranche, infos_types_tranches)                                                      \
    O(InfoTypeUnion, infos_types_unions)                                                          \
    O(InfoTypeOpaque, infos_types_opaques)                                                        \
    O(InfoTypeVariadique, infos_types_variadiques)

#define ENUME_TYPES_TRANCHES_INFO_TYPE(O)                                                         \
    O(Annotation const *, annotations)                                                            \
    O(char, valeurs_énums)                                                                        \
    O(kuri::chaine_statique, noms_énums)                                                          \
    O(InfoTypeMembreStructure *, membres)                                                         \
    O(InfoType *, tableau_info_type)                                                              \
    O(InfoTypeStructure *, structs_employées)

struct AllocatriceInfosType {
#define ENUMERE_TYPES_INFO_TYPE_EX(type__, nom__) kuri::tableau_page<type__> nom__{};
    ENUMERE_TYPES_INFO_TYPE(ENUMERE_TYPES_INFO_TYPE_EX)
#undef ENUMERE_TYPES_INFO_TYPE_EX

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

    template <typename T>
    kuri::tranche<T> donne_tranche(kuri::tableau<T> const &tableau);
};
