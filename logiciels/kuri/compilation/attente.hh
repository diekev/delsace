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

struct IdentifiantCode;
struct MetaProgramme;
struct NoeudDeclaration;
struct NoeudExpression;
struct NoeudExpressionReference;
struct Type;

struct Attente {
    Type *attend_sur_type = nullptr;
    const char *attend_sur_interface_kuri = nullptr;
    MetaProgramme *attend_sur_metaprogramme = nullptr;
    NoeudDeclaration *attend_sur_declaration = nullptr;
    NoeudExpressionReference *attend_sur_symbole = nullptr;
    NoeudExpression *attend_sur_operateur = nullptr;
    /* ATTENTION ! Ne pas ajouter autre chose que des pointeurs, ou changer est_valide() ! */

    static Attente sur_type(Type *type)
    {
        auto attente = Attente{};
        attente.attend_sur_type = type;
        return attente;
    }

    static Attente sur_interface_kuri(const char *nom_fonction)
    {
        auto attente = Attente{};
        attente.attend_sur_interface_kuri = nom_fonction;
        return attente;
    }

    static Attente sur_metaprogramme(MetaProgramme *metaprogramme)
    {
        auto attente = Attente{};
        attente.attend_sur_metaprogramme = metaprogramme;
        return attente;
    }

    static Attente sur_declaration(NoeudDeclaration *declaration)
    {
        auto attente = Attente{};
        attente.attend_sur_declaration = declaration;
        return attente;
    }

    static Attente sur_symbole(NoeudExpressionReference *ident)
    {
        auto attente = Attente{};
        attente.attend_sur_symbole = ident;
        return attente;
    }

    static Attente sur_operateur(NoeudExpression *operateur)
    {
        auto attente = Attente{};
        attente.attend_sur_operateur = operateur;
        return attente;
    }

    /* Retourne vrai si l'attente est valide, c'est-à-dire qu'elle contient quelque chose sur quoi
     * attendre. */
    bool est_valide() const
    {
        void *const *pointeurs = reinterpret_cast<void *const *>(&this->attend_sur_type);

        for (auto i = 0u; i < sizeof(Attente) / sizeof(void *); ++i) {
            if (pointeurs[i] != nullptr) {
                return true;
            }
        }

        return false;
    }
};
