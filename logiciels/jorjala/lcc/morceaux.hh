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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

/* Ce fichier est généré automatiquement. NE PAS ÉDITER ! */

#pragma once

#include "biblinternes/structures/vue_chaine.hh"

enum class id_morceau : unsigned int {
    EXCLAMATION,
    GUILLEMET,
    DIESE,
    DOLLAR,
    POURCENT,
    ESPERLUETTE,
    APOSTROPHE,
    PARENTHESE_OUVRANTE,
    PARENTHESE_FERMANTE,
    FOIS,
    PLUS,
    VIRGULE,
    MOINS,
    POINT,
    DIVISE,
    DOUBLE_POINTS,
    POINT_VIRGULE,
    INFERIEUR,
    EGAL,
    SUPERIEUR,
    AROBASE,
    CROCHET_OUVRANT,
    CROCHET_FERMANT,
    CHAPEAU,
    ACCOLADE_OUVRANTE,
    BARRE,
    ACCOLADE_FERMANTE,
    TILDE,
    DIFFERENCE,
    DIRECTIVE,
    ESP_ESP,
    FOIS_EGAL,
    PLUS_EGAL,
    MOINS_EGAL,
    DIVISE_EGAL,
    DECALAGE_GAUCHE,
    INFERIEUR_EGAL,
    EGALITE,
    SUPERIEUR_EGAL,
    DECALAGE_DROITE,
    BARRE_BARRE,
    ARRETE,
    BOOL,
    BOUCLE,
    CONTINUE,
    DANS,
    DEC,
    ENT,
    FAUX,
    MAT3,
    MAT4,
    NUL,
    POUR,
    RETOURNE,
    SI,
    SINON,
    VEC2,
    VEC3,
    VEC4,
    VRAI,
    NOMBRE_REEL,
    NOMBRE_ENTIER,
    NOMBRE_HEXADECIMAL,
    NOMBRE_OCTAL,
    NOMBRE_BINAIRE,
    PLUS_UNAIRE,
    MOINS_UNAIRE,
    TROIS_POINTS,
    CHAINE_CARACTERE,
    CHAINE_LITTERALE,
    CARACTERE,
    TABLEAU,
    INCONNU,
};

inline id_morceau operator&(id_morceau id1, int id2)
{
    return static_cast<id_morceau>(static_cast<int>(id1) & id2);
}

inline id_morceau operator|(id_morceau id1, int id2)
{
    return static_cast<id_morceau>(static_cast<int>(id1) | id2);
}

inline id_morceau operator|(id_morceau id1, id_morceau id2)
{
    return static_cast<id_morceau>(static_cast<int>(id1) | static_cast<int>(id2));
}

inline id_morceau operator<<(id_morceau id1, int id2)
{
    return static_cast<id_morceau>(static_cast<int>(id1) << id2);
}

inline id_morceau operator>>(id_morceau id1, int id2)
{
    return static_cast<id_morceau>(static_cast<int>(id1) >> id2);
}

struct DonneesMorceaux {
    using type = id_morceau;
    static constexpr type INCONNU = id_morceau::INCONNU;

    dls::vue_chaine chaine;
    unsigned long ligne_pos;
    id_morceau genre;
    int module = 0;
};

const char *chaine_identifiant(id_morceau id);

void construit_tables_caractere_speciaux();

bool est_caractere_special(char c, id_morceau &i);

id_morceau id_caractere_double(const dls::vue_chaine &chaine);

id_morceau id_chaine(const dls::vue_chaine &chaine);
