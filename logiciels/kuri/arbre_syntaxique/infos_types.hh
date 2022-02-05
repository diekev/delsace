/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/tableau_page.hh"

#include "structures/chaine_statique.hh"
#include "structures/tableau.hh"

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
};

struct InfoType {
    GenreInfoType genre{};
    uint taille_en_octet = 0;
    uint index_dans_table_des_types = 0;
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

// À FAIRE : déduplique avec AnnotationCode
struct AnnotationMembre {
    kuri::chaine_statique nom;
    kuri::chaine_statique valeur;
};

struct InfoTypeMembreStructure {
    // Les Drapeaux sont définis dans TypeCompose::Membre

    kuri::chaine_statique nom{};
    InfoType *info = nullptr;
    long decalage = 0;  // décalage en octets dans la structure
    int drapeaux = 0;
    kuri::tableau<AnnotationMembre *> annotations{};
};

struct InfoTypeStructure : public InfoType {
    kuri::chaine_statique nom{};
    kuri::tableau<InfoTypeMembreStructure *> membres{};
    kuri::tableau<InfoTypeStructure *> structs_employees{};
};

struct InfoTypeUnion : public InfoType {
    kuri::chaine_statique nom{};
    kuri::tableau<InfoTypeMembreStructure *> membres{};
    InfoType *type_le_plus_grand = nullptr;
    long decalage_index = 0;
    bool est_sure = false;
};

struct InfoTypeFonction : public InfoType {
    kuri::tableau<InfoType *> types_entrees{};
    kuri::tableau<InfoType *> types_sorties{};
    bool est_coroutine = false;
};

struct InfoTypeEnum : public InfoType {
    kuri::chaine_statique nom{};
    kuri::tableau<int> valeurs{};  // À FAIRE typage selon énum
    kuri::tableau<kuri::chaine_statique> noms{};
    bool est_drapeau = false;
};

struct InfoTypeOpaque : public InfoType {
    kuri::chaine_statique nom{};
    InfoType *type_opacifie = nullptr;
};

struct AllocatriceInfosType {
    tableau_page<InfoType> infos_types{};
    tableau_page<InfoTypeEntier> infos_types_entiers{};
    tableau_page<InfoTypeEnum> infos_types_enums{};
    tableau_page<InfoTypeFonction> infos_types_fonctions{};
    tableau_page<InfoTypeMembreStructure> infos_types_membres_structures{};
    tableau_page<InfoTypePointeur> infos_types_pointeurs{};
    tableau_page<InfoTypeStructure> infos_types_structures{};
    tableau_page<InfoTypeTableau> infos_types_tableaux{};
    tableau_page<InfoTypeUnion> infos_types_unions{};
    tableau_page<InfoTypeOpaque> infos_types_opaques{};

    long memoire_utilisee() const;
};
