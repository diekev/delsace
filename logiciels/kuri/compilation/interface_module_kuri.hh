/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

/* Interface avec le module « Kuri », pour mettre en cache des pointeurs vers des fonctions ou
 * types connus de la Compilatrice. */

#include "parsage/identifiant.hh"

struct NoeudDeclarationEnteteFonction;
struct Typeuse;
struct NoeudDeclarationType;
using Type = NoeudDeclarationType;

#define ENUMERE_TYPE_INTERFACE_MODULE_KURI(O)                                                     \
    O(type_info_type_, ID::InfoType)                                                              \
    O(type_info_type_enum, ID::InfoTypeEnum)                                                      \
    O(type_info_type_structure, ID::InfoTypeStructure)                                            \
    O(type_info_type_union, ID::InfoTypeUnion)                                                    \
    O(type_info_type_membre_structure, ID::InfoTypeMembreStructure)                               \
    O(type_info_type_entier, ID::InfoTypeEntier)                                                  \
    O(type_info_type_tableau, ID::InfoTypeTableau)                                                \
    O(type_info_type_tableau_fixe, ID::InfoTypeTableauFixe)                                       \
    O(type_info_type_tranche, ID::InfoTypeTranche)                                                \
    O(type_info_type_pointeur, ID::InfoTypePointeur)                                              \
    O(type_info_type_fonction, ID::InfoTypeFonction)                                              \
    O(type_position_code_source, ID::PositionCodeSource)                                          \
    O(type_info_fonction_trace_appel, ID::InfoFonctionTraceAppel)                                 \
    O(type_trace_appel, ID::TraceAppel)                                                           \
    O(type_base_allocatrice, ID::BaseAllocatrice)                                                 \
    O(type_info_appel_trace_appel, ID::InfoAppelTraceAppel)                                       \
    O(type_stockage_temporaire, ID::StockageTemporaire)                                           \
    O(type_info_type_opaque, ID::InfoTypeOpaque)                                                  \
    O(type_info_type_variadique, ID::InfoTypeVariadique)                                          \
    O(type_contexte, ID::ContexteProgramme)                                                       \
    O(type_annotation, ID::AnnotationCode)

#define ENUMERE_FONCTIONS_INTERFACE_MODULE_KURI(Op)                                               \
    Op(decl_panique, ID::panique) Op(decl_panique_memoire, ID::panique_hors_memoire)              \
        Op(decl_panique_tableau, ID::panique_depassement_limites_tableau)                         \
            Op(decl_panique_chaine, ID::panique_depassement_limites_chaine) Op(                   \
                decl_panique_membre_union,                                                        \
                ID::panique_membre_union) Op(decl_panique_erreur, ID::panique_erreur_non_geree)   \
                Op(decl_rappel_panique_defaut, ID::__rappel_panique_defaut) Op(                   \
                    decl_dls_vers_r32, ID::DLS_vers_r32) Op(decl_dls_vers_r64, ID::DLS_vers_r64)  \
                    Op(decl_dls_depuis_r32, ID::DLS_depuis_r32)                                   \
                        Op(decl_dls_depuis_r64, ID::DLS_depuis_r64)                               \
                            Op(decl_creation_contexte, ID::crée_contexte)                         \
                                Op(decl_init_execution_kuri, ID::init_execution_kuri)             \
                                    Op(decl_fini_execution_kuri, ID::fini_execution_kuri)         \
                                        Op(decl_init_globales_kuri, ID::init_globales_kuri)

struct InterfaceKuri {
#define DECLARATION_MEMBRE(nom_membre, id) NoeudDeclarationEnteteFonction *nom_membre = nullptr;

    ENUMERE_FONCTIONS_INTERFACE_MODULE_KURI(DECLARATION_MEMBRE)

#undef DECLARATION_MEMBRE

    NoeudDeclarationEnteteFonction *declaration_pour_ident(const IdentifiantCode *ident);

    void mute_membre(NoeudDeclarationEnteteFonction *noeud);
};

void renseigne_type_interface(Typeuse &typeuse, const IdentifiantCode *ident, Type *type);

bool ident_est_pour_fonction_interface(const IdentifiantCode *ident);

bool ident_est_pour_type_interface(const IdentifiantCode *ident);

/** Retourne vrai si le type correspondant à l'IdentifiantCode est non-nul. */
bool est_type_interface_disponible(Typeuse &typeuse, const IdentifiantCode *ident);
